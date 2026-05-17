#include "clipboard_bridge.h"

#include "bridge_log.h"

#include <memory>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
#include <client/OHOS/ohos_clipboard.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
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

        log_ = log;
        clipboard_ = freerdp_ohos_clipboard_new();
        if (clipboard_ == nullptr) {
            error = "create FreeRDP OHOS clipboard backend failed";
            return false;
        }

        FREERDP_OHOS_CLIPBOARD_CONFIG config = {};
        config.PubSubSubscribe = api.pubSubSubscribe;
        config.PubSubUnsubscribe = api.pubSubUnsubscribe;
        config.Log = LogThunk;
        config.logUserData = this;

        char errorBuffer[256] = {};
        if (!freerdp_ohos_clipboard_register(
                clipboard_, context, &config, errorBuffer, sizeof(errorBuffer))) {
            error = errorBuffer[0] == '\0' ? "register FreeRDP OHOS clipboard backend failed" :
                errorBuffer;
            freerdp_ohos_clipboard_free(clipboard_);
            clipboard_ = nullptr;
            return false;
        }
        return true;
    }

    void Uninitialize()
    {
        if (clipboard_ != nullptr) {
            const char* diagnostics = freerdp_ohos_clipboard_get_diagnostics(clipboard_);
            if (diagnostics != nullptr) {
                Log(diagnostics);
            }
            freerdp_ohos_clipboard_free(clipboard_);
            clipboard_ = nullptr;
        }
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
        if (log_) {
            log_(line);
        } else {
            EmitHilogInfo(line);
        }
    }

    LogFn log_;
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
#endif

} // namespace rdp_bridge
