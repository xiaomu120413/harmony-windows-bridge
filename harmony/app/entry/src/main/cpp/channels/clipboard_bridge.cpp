#include "clipboard_bridge.h"

#include "bridge_log.h"
#include "clipboard_format.h"
#include "clipboard_pasteboard.h"
#include "string_utils.h"

#include <memory>
#include <mutex>
#include <string>
#include <cstring>
#include <unordered_map>
#include <vector>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client/cliprdr.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/event.h>
#include <winpr/clipboard.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
struct HarmonyClipboardBridge::Impl {
public:
    ~Impl()
    {
        Uninitialize();
    }

    bool Initialize(rdpContext* context, FreerdpRuntimeApi& api, const LogFn& log,
        std::string& error)
    {
        if (context == nullptr || context->pubSub == nullptr) {
            error = "FreeRDP pubSub is unavailable for clipboard";
            return false;
        }
        if (api.pubSubSubscribe == nullptr || api.pubSubUnsubscribe == nullptr) {
            error = "WinPR PubSub symbols are not loaded for clipboard";
            return false;
        }

        context_ = context;
        api_ = &api;
        log_ = log;

        {
            std::lock_guard<std::mutex> lock(RegistryMutex());
            Registry()[context_] = this;
        }

        int rc = api_->pubSubSubscribe(context_->pubSub, "ChannelConnected", OnChannelConnected);
        if (rc < 0) {
            RemoveFromRegistry();
            error = "subscribe ChannelConnected for clipboard failed: " + std::to_string(rc);
            return false;
        }
        subscribedConnected_ = true;

        rc = api_->pubSubSubscribe(context_->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        if (rc < 0) {
            Uninitialize();
            error = "subscribe ChannelDisconnected for clipboard failed: " + std::to_string(rc);
            return false;
        }
        subscribedDisconnected_ = true;
        Log("cliprdr bridge subscribed to FreeRDP channel events");

        pasteboard_.Initialize(log_, [this]() {
            (void)SendLocalFormatList("pasteboard changed");
        });
        return true;
    }

    void Uninitialize()
    {
        if (cliprdr_ != nullptr) {
            DetachCliprdr(cliprdr_);
        }

        pasteboard_.Uninitialize();

        if (api_ != nullptr && context_ != nullptr && context_->pubSub != nullptr) {
            if (subscribedConnected_) {
                (void)api_->pubSubUnsubscribe(context_->pubSub, "ChannelConnected", OnChannelConnected);
                subscribedConnected_ = false;
            }
            if (subscribedDisconnected_) {
                (void)api_->pubSubUnsubscribe(context_->pubSub, "ChannelDisconnected", OnChannelDisconnected);
                subscribedDisconnected_ = false;
            }
        }

        RemoveFromRegistry();
        context_ = nullptr;
        api_ = nullptr;
        log_ = nullptr;
    }

private:
    static std::mutex& RegistryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<rdpContext*, Impl*>& Registry()
    {
        static std::unordered_map<rdpContext*, Impl*> registry;
        return registry;
    }

    static Impl* FromContext(void* context)
    {
        auto* rdpCtx = static_cast<::rdpContext*>(context);
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(rdpCtx);
        return iter == Registry().end() ? nullptr : iter->second;
    }

    static Impl* FromCliprdr(CliprdrClientContext* cliprdr)
    {
        return cliprdr == nullptr ? nullptr : static_cast<Impl*>(cliprdr->custom);
    }

    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event)
    {
        Impl* bridge = FromContext(context);
        if (bridge == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }
        bridge->Log("FreeRDP channel connected: " + std::string(event->name));
        if (std::strcmp(event->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
            bridge->AttachCliprdr(static_cast<CliprdrClientContext*>(event->pInterface));
        }
    }

    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
    {
        Impl* bridge = FromContext(context);
        if (bridge == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }
        bridge->Log("FreeRDP channel disconnected: " + std::string(event->name));
        if (std::strcmp(event->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
            bridge->DetachCliprdr(static_cast<CliprdrClientContext*>(event->pInterface));
        }
    }

    static UINT CliprdrMonitorReady(CliprdrClientContext* cliprdr,
        const CLIPRDR_MONITOR_READY* monitorReady)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || monitorReady == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        UINT rc = bridge->SendClientCapabilities();
        if (rc != CHANNEL_RC_OK) {
            return rc;
        }
        bridge->Log("cliprdr monitor ready");
        return bridge->SendLocalFormatList("monitor ready");
    }

    static UINT CliprdrServerCapabilities(CliprdrClientContext* cliprdr,
        const CLIPRDR_CAPABILITIES* capabilities)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || capabilities == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        bridge->Log("cliprdr server capabilities received");
        return CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFormatList(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_LIST* formatList)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || formatList == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatList(*formatList);
    }

    static UINT CliprdrServerFormatListResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_LIST_RESPONSE* response)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || response == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        bridge->Log("cliprdr server accepted local format list");
        return CHANNEL_RC_OK;
    }

    static UINT CliprdrServerLockClipboardData(CliprdrClientContext* cliprdr,
        const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
    {
        return (cliprdr == nullptr || lockClipboardData == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerUnlockClipboardData(CliprdrClientContext* cliprdr,
        const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
    {
        return (cliprdr == nullptr || unlockClipboardData == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFormatDataRequest(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_DATA_REQUEST* request)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || request == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatDataRequest(*request);
    }

    static UINT CliprdrServerFormatDataResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_DATA_RESPONSE* response)
    {
        Impl* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || response == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatDataResponse(*response);
    }

    static UINT CliprdrServerFileContentsRequest(CliprdrClientContext* cliprdr,
        const CLIPRDR_FILE_CONTENTS_REQUEST* request)
    {
        return (cliprdr == nullptr || request == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFileContentsResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FILE_CONTENTS_RESPONSE* response)
    {
        return (cliprdr == nullptr || response == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    void AttachCliprdr(CliprdrClientContext* cliprdr)
    {
        if (cliprdr == nullptr) {
            Log("cliprdr connected without client context");
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        cliprdr_ = cliprdr;
        cliprdr_->custom = this;
        cliprdr_->MonitorReady = CliprdrMonitorReady;
        cliprdr_->ServerCapabilities = CliprdrServerCapabilities;
        cliprdr_->ServerFormatList = CliprdrServerFormatList;
        cliprdr_->ServerFormatListResponse = CliprdrServerFormatListResponse;
        cliprdr_->ServerLockClipboardData = CliprdrServerLockClipboardData;
        cliprdr_->ServerUnlockClipboardData = CliprdrServerUnlockClipboardData;
        cliprdr_->ServerFormatDataRequest = CliprdrServerFormatDataRequest;
        cliprdr_->ServerFormatDataResponse = CliprdrServerFormatDataResponse;
        cliprdr_->ServerFileContentsRequest = CliprdrServerFileContentsRequest;
        cliprdr_->ServerFileContentsResponse = CliprdrServerFileContentsResponse;
        Log("cliprdr connected to HarmonyOS Pasteboard text bridge");
    }

    void DetachCliprdr(CliprdrClientContext* cliprdr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cliprdr_ == nullptr || cliprdr_ != cliprdr) {
            return;
        }

        cliprdr_->custom = nullptr;
        cliprdr_ = nullptr;
        serverFormats_.clear();
        requestedFormatId_ = 0;
        Log("cliprdr disconnected from HarmonyOS Pasteboard text bridge");
    }

    UINT SendClientCapabilities()
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientCapabilities == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_CAPABILITIES capabilities = {};
        CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet = {};
        capabilities.cCapabilitiesSets = 1;
        capabilities.capabilitySets = reinterpret_cast<CLIPRDR_CAPABILITY_SET*>(&generalCapabilitySet);
        generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
        generalCapabilitySet.capabilitySetLength = 12;
        generalCapabilitySet.version = CB_CAPS_VERSION_2;
        generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;
        return cliprdr->ClientCapabilities(cliprdr, &capabilities);
    }

    UINT SendClientFormatListResponse(bool accepted)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatListResponse == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_FORMAT_LIST_RESPONSE response = {};
        response.common.msgType = CB_FORMAT_LIST_RESPONSE;
        response.common.msgFlags = accepted ? CB_RESPONSE_OK : CB_RESPONSE_FAIL;
        response.common.dataLen = 0;
        const UINT rc = cliprdr->ClientFormatListResponse(cliprdr, &response);
        if (rc == CHANNEL_RC_OK) {
            Log(std::string("cliprdr server format list ") + (accepted ? "accepted" : "rejected"));
        }
        return rc;
    }

    UINT SendLocalFormatList(const char* reason)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatList == nullptr) {
            return CHANNEL_RC_OK;
        }

        std::string text;
        std::string error;
        const bool hasText = pasteboard_.ReadPlainText(text, error);
        if (!hasText && !error.empty()) {
            Log("HarmonyOS Pasteboard read warning: " + error);
        }

        CLIPRDR_FORMAT format = {};
        format.formatId = CF_UNICODETEXT;
        CLIPRDR_FORMAT_LIST formatList = {};
        formatList.common.msgType = CB_FORMAT_LIST;
        formatList.common.msgFlags = 0;
        formatList.numFormats = hasText ? 1U : 0U;
        formatList.formats = hasText ? &format : nullptr;

        UINT rc = cliprdr->ClientFormatList(cliprdr, &formatList);
        if (rc == CHANNEL_RC_OK) {
            Log(std::string("cliprdr local format list sent: ") +
                (hasText ? "CF_UNICODETEXT" : "empty") + " reason=" + SafeCString(reason));
        }
        return rc;
    }

    UINT HandleServerFormatList(const CLIPRDR_FORMAT_LIST& formatList)
    {
        UINT32 requested = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            serverFormats_.clear();
            for (UINT32 index = 0; index < formatList.numFormats; ++index) {
                serverFormats_.push_back(formatList.formats[index].formatId);
                if (formatList.formats[index].formatId == CF_UNICODETEXT) {
                    requested = CF_UNICODETEXT;
                } else if (requested == 0 && formatList.formats[index].formatId == CF_TEXT) {
                    requested = CF_TEXT;
                } else if (requested == 0 && formatList.formats[index].formatId == CF_OEMTEXT) {
                    requested = CF_OEMTEXT;
                }
            }
            requestedFormatId_ = requested;
        }

        Log("cliprdr server format list received: " + std::to_string(formatList.numFormats));
        UINT rc = SendClientFormatListResponse(true);
        if (rc != CHANNEL_RC_OK) {
            Log("cliprdr server format list response failed: " + std::to_string(rc));
            return rc;
        }
        if (requested == 0) {
            Log("cliprdr server format list has no supported text format");
            return CHANNEL_RC_OK;
        }

        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatDataRequest == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_FORMAT_DATA_REQUEST request = {};
        request.common.msgType = CB_FORMAT_DATA_REQUEST;
        request.requestedFormatId = requested;
        Log("cliprdr remote text request sent: format=" + std::to_string(requested));
        return cliprdr->ClientFormatDataRequest(cliprdr, &request);
    }

    UINT HandleServerFormatDataRequest(const CLIPRDR_FORMAT_DATA_REQUEST& request)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatDataResponse == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        std::string text;
        std::string error;
        const bool ok = pasteboard_.ReadPlainText(text, error);
        std::vector<BYTE> data;
        if (ok && request.requestedFormatId == CF_UNICODETEXT) {
            data = Utf8ToUtf16LeClipboard(text);
        } else if (ok && (request.requestedFormatId == CF_TEXT || request.requestedFormatId == CF_OEMTEXT)) {
            data.assign(text.begin(), text.end());
            data.push_back(0);
        }

        CLIPRDR_FORMAT_DATA_RESPONSE response = {};
        response.common.msgType = CB_FORMAT_DATA_RESPONSE;
        response.common.msgFlags = data.empty() ? CB_RESPONSE_FAIL : CB_RESPONSE_OK;
        response.common.dataLen = static_cast<UINT32>(data.size());
        response.requestedFormatData = data.empty() ? nullptr : data.data();

        if (data.empty()) {
            Log("cliprdr local text request failed: " + error);
        } else {
            Log("cliprdr local text response sent: " + std::to_string(data.size()) + " bytes");
        }
        return cliprdr->ClientFormatDataResponse(cliprdr, &response);
    }

    UINT HandleServerFormatDataResponse(const CLIPRDR_FORMAT_DATA_RESPONSE& response)
    {
        if ((response.common.msgFlags & CB_RESPONSE_FAIL) != 0 || response.requestedFormatData == nullptr) {
            Log("cliprdr remote text response failed: flags=" + std::to_string(response.common.msgFlags) +
                " length=" + std::to_string(response.common.dataLen));
            return CHANNEL_RC_OK;
        }

        UINT32 requested = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            requested = requestedFormatId_;
        }

        std::string text;
        if (requested == CF_UNICODETEXT) {
            text = Utf16LeClipboardToUtf8(response.requestedFormatData, response.common.dataLen);
        } else if (requested == CF_TEXT || requested == CF_OEMTEXT) {
            const auto* bytes = reinterpret_cast<const char*>(response.requestedFormatData);
            const size_t length = strnlen(bytes, response.common.dataLen);
            text.assign(bytes, length);
        }

        if (text.empty()) {
            Log("cliprdr remote text response was empty");
            return CHANNEL_RC_OK;
        }

        std::string error;
        if (!pasteboard_.WritePlainText(text, error)) {
            Log("HarmonyOS Pasteboard write failed: " + error);
            return ERROR_INTERNAL_ERROR;
        }

        Log("cliprdr remote text copied to HarmonyOS Pasteboard: " +
            std::to_string(text.size()) + " bytes utf8");
        return CHANNEL_RC_OK;
    }

    void RemoveFromRegistry()
    {
        if (context_ == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(context_);
        if (iter != Registry().end() && iter->second == this) {
            Registry().erase(iter);
        }
    }

    void Log(const std::string& line)
    {
        if (log_) {
            log_(line);
        } else {
            EmitHilogInfo(line);
        }
    }

    std::mutex mutex_;
    rdpContext* context_ = nullptr;
    FreerdpRuntimeApi* api_ = nullptr;
    LogFn log_;
    CliprdrClientContext* cliprdr_ = nullptr;
    ClipboardPasteboard pasteboard_;
    bool subscribedConnected_ = false;
    bool subscribedDisconnected_ = false;
    std::vector<UINT32> serverFormats_;
    UINT32 requestedFormatId_ = 0;
};


HarmonyClipboardBridge::HarmonyClipboardBridge() : impl_(std::make_unique<Impl>()) {}

HarmonyClipboardBridge::~HarmonyClipboardBridge() = default;

bool HarmonyClipboardBridge::Initialize(rdpContext* context, FreerdpRuntimeApi& api, const LogFn& log,
    std::string& error)
{
    return impl_->Initialize(context, api, log, error);
}

void HarmonyClipboardBridge::Uninitialize()
{
    impl_->Uninitialize();
}
#endif

} // namespace rdp_bridge
