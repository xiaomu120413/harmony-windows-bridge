#include "channels/clipboard_bridge.h"

#include "common/bridge_log.h"
#include "napi/clipboard_permission_bridge.h"

#include <memory>
#include <string>

#include <client/OHOS/ohos_clipboard.h>

namespace rdp_bridge {
namespace {

bool IsClipboardLog(const std::string& line)
{
    return line.find("clipboard") != std::string::npos ||
        line.find("cliprdr") != std::string::npos ||
        line.find("Pasteboard") != std::string::npos;
}

bool HasClipboardProblemKeyword(const std::string& line)
{
    if (line.find("format probe") != std::string::npos ||
        line.find("stats:") != std::string::npos) {
        return false;
    }
    return line.find("failed") != std::string::npos ||
        line.find("denied") != std::string::npos ||
        line.find("error") != std::string::npos ||
        line.find("invalid") != std::string::npos ||
        line.find("unavailable") != std::string::npos ||
        line.find("unsupported") != std::string::npos ||
        line.find("too large") != std::string::npos;
}

bool ShouldForwardClipboardLog(const std::string& line)
{
    return !IsClipboardLog(line) || HasClipboardProblemKeyword(line);
}

} // namespace

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
        if (api.pubSubSubscribe == nullptr || api.pubSubUnsubscribe == nullptr ||
            api.ohosClipboardNew == nullptr || api.ohosClipboardRegister == nullptr ||
            api.ohosClipboardFree == nullptr || api.ohosClipboardGetDiagnostics == nullptr) {
            error = "FreeRDP OHOS clipboard symbols are not loaded";
            return false;
        }

        log_ = log;
        api_ = &api;
        clipboard_ = api_->ohosClipboardNew();
        if (clipboard_ == nullptr) {
            error = "create FreeRDP OHOS clipboard backend failed";
            api_ = nullptr;
            return false;
        }

        FREERDP_OHOS_CLIPBOARD_CONFIG config = {};
        config.PubSubSubscribe = api.pubSubSubscribe;
        config.PubSubUnsubscribe = api.pubSubUnsubscribe;
        config.Log = LogThunk;
        config.logUserData = this;
        config.RequestReadPermission = RequestClipboardPermissionForPasteboard;
        config.permissionUserData = nullptr;

        char errorBuffer[256] = {};
        if (!api_->ohosClipboardRegister(
                clipboard_, context, &config, errorBuffer, sizeof(errorBuffer))) {
            error = errorBuffer[0] == '\0' ? "register FreeRDP OHOS clipboard backend failed" :
                errorBuffer;
            api_->ohosClipboardFree(clipboard_);
            clipboard_ = nullptr;
            api_ = nullptr;
            return false;
        }
        return true;
    }

    void Uninitialize()
    {
        if (clipboard_ != nullptr) {
            const char* diagnostics = api_ == nullptr ? nullptr :
                api_->ohosClipboardGetDiagnostics(clipboard_);
            if (diagnostics != nullptr) {
                Log(diagnostics);
            }
            if (api_ != nullptr) {
                api_->ohosClipboardFree(clipboard_);
            }
            clipboard_ = nullptr;
        }
        api_ = nullptr;
        log_ = nullptr;
    }

private:
    static void LogThunk(void* userData, const char* message)
    {
        auto* self = static_cast<Impl*>(userData);
        if (self == nullptr || message == nullptr) {
            return;
        }
        self->Log(message);
    }

    void Log(const std::string& line)
    {
        if (!ShouldForwardClipboardLog(line)) {
            return;
        }
        if (log_) {
            log_(line);
        } else {
            BridgeLogger::Debug(line);
        }
    }

    LogFn log_;
    FreerdpRuntimeApi* api_ = nullptr;
    freerdpOhosClipboard* clipboard_ = nullptr;
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

} // namespace rdp_bridge
