#include "session/freerdp_session_runner.h"

#include "common/bridge_log.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_diagnostics.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/graphics_config.h"
#include "napi/location_bridge.h"
#include "napi/microphone_permission_bridge.h"
#include "common/string_utils.h"

#include <array>
#include <chrono>
#include <cstdio>

#include <freerdp/client.h>
#include <freerdp/error.h>
#include <freerdp/settings.h>
#include <winpr/synch.h>

namespace rdp_bridge {
namespace {

void StopRenderPipeline(const RdpSessionCallbacks& callbacks)
{
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
}

GraphicsPipelineConfig ToGraphicsPipelineConfig(const FREERDP_OHOS_GRAPHICS_CONFIG& nativeConfig)
{
    GraphicsPipelineConfig config;
    config.valid = nativeConfig.mode != FREERDP_OHOS_GRAPHICS_MODE_INVALID;
    config.enabled = nativeConfig.enabled;
    config.h264 = nativeConfig.h264;
    config.avc444GpuCompositor = nativeConfig.enabled && nativeConfig.h264;
    config.mode = nativeConfig.modeName == nullptr ? "gdi" : nativeConfig.modeName;
    return config;
}

void CopyCallbackMessage(char* message, size_t messageSize, const std::string& value)
{
    if (message == nullptr || messageSize == 0) {
        return;
    }
    std::snprintf(message, messageSize, "%s", value.c_str());
}

struct OhosSessionAdapter {
    FreerdpRuntimeApi& api;
    freerdpOhosSession* session = nullptr;
    const GraphicsPipelineConfig& graphicsConfig;
    const RdpSessionCallbacks& callbacks;
    const FreerdpSetActiveFn& setActive;
    const FreerdpClearActiveFn& clearActive;
    const std::function<void(const std::string&)>& log;
    const FreerdpConnectedFn& onConnected;
    const FreerdpInputPumpFn& pumpInput;
    std::atomic_bool& running;
    HarmonyClipboardBridge clipboardBridge;
    bool activeSet = false;
    bool connected = false;
    std::chrono::steady_clock::time_point nextDiagnosticsLog =
        std::chrono::steady_clock::now() + std::chrono::seconds(10);

    void EmitLog(const std::string& line)
    {
        if (log != nullptr && !line.empty()) {
            log(line);
        }
    }

    bool Configure(freerdp* instance, rdpContext* context, char* message, size_t messageSize)
    {
        if (instance == nullptr || context == nullptr || context->settings == nullptr) {
            CopyCallbackMessage(message, messageSize, "FreeRDP session configure context is unavailable");
            return false;
        }

        setActive(&api, instance, context, session);
        activeSet = true;

        std::string error;
        if (!ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error) ||
            !ConfigureGraphicsPipelineChannel(api, context->settings, graphicsConfig, log, error)) {
            CopyCallbackMessage(message, messageSize, error);
            return false;
        }

        if (!clipboardBridge.Initialize(context, api, log, error)) {
            CopyCallbackMessage(message, messageSize, error);
            return false;
        }

        instance->PostConnect = HarmonyPostConnect;
        instance->PostDisconnect = HarmonyPostDisconnect;
        SetGdiBridgeCallbacks({
            IsAvc420SurfaceOutputEnabled,
            callbacks.queueSurfaceRgbaFrame,
            callbacks.startRenderPipeline,
            callbacks.stopRenderPipeline,
            log,
        });
        ClearRdpDesktopSize();

        EmitLog("FreeRDP target configured through OHOS session API");
        EmitLog("FreeRDP mode=OhosSessionApi");
        EmitLog("FreeRDP GDI renderer configured");
        return true;
    }

    void Connected(freerdp*, rdpContext*)
    {
        connected = true;
        onConnected();
    }

    bool Pump(freerdp*, rdpContext* context)
    {
        pumpInput(&api, context);
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextDiagnosticsLog) {
            BridgeLogger::DebugPublic("FreeRDP health: connected=" +
                std::string(connected ? "yes" : "no") + " mode=" + graphicsConfig.mode +
                " | " + BuildGraphicsPipelineStatsLog());
            BridgeLogger::DebugPublic("FreeRDP render health: " + BuildRenderStatsLog());
            BridgeLogger::DebugPublic("FreeRDP audio health: " + BuildOHAudioStatsLog());
            nextDiagnosticsLog = now + std::chrono::seconds(10);
        }
        return true;
    }

    std::string BuildRenderStatsLog() const
    {
        if (callbacks.renderStats == nullptr) {
            return "render stats unavailable";
        }
        const std::string stats = callbacks.renderStats();
        return stats.empty() ? "render stats unavailable" : stats;
    }

    bool ShouldContinue() const
    {
        return running.load();
    }

    void Teardown(freerdp* instance, rdpContext*)
    {
        ResetAvcSurfaceOutput(api);
        clipboardBridge.Uninitialize();
        ClearRdpDesktopSize();
        StopRenderPipeline(callbacks);
        if (activeSet) {
            clearActive(instance);
            activeSet = false;
        }
    }

    static void LogCallback(const char* message, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        if (adapter != nullptr && message != nullptr && message[0] != '\0') {
            adapter->EmitLog(message);
        }
    }

    static void ErrorCallback(const char* message, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        if (adapter != nullptr && message != nullptr && message[0] != '\0') {
            adapter->EmitLog(message);
        }
    }

    static void StateCallback(const char* state, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        if (adapter != nullptr && state != nullptr && state[0] != '\0') {
            adapter->EmitLog(std::string("OHOS session state=") + state);
        }
    }

    static BOOL ConfigureCallback(freerdp* instance, rdpContext* context,
        const FREERDP_OHOS_SESSION_OPTIONS*, char* message, size_t messageSize, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        return adapter != nullptr && adapter->Configure(instance, context, message, messageSize)
            ? TRUE
            : FALSE;
    }

    static void ConnectedCallback(freerdp* instance, rdpContext* context, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        if (adapter != nullptr) {
            adapter->Connected(instance, context);
        }
    }

    static BOOL PumpCallback(freerdp* instance, rdpContext* context, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        return adapter != nullptr && adapter->Pump(instance, context) ? TRUE : FALSE;
    }

    static BOOL ShouldContinueCallback(void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        return adapter != nullptr && adapter->ShouldContinue() ? TRUE : FALSE;
    }

    static void TeardownCallback(freerdp* instance, rdpContext* context, void* userData)
    {
        auto* adapter = static_cast<OhosSessionAdapter*>(userData);
        if (adapter != nullptr) {
            adapter->Teardown(instance, context);
        }
    }
};

} // namespace

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const RdpSessionCallbacks& callbacks, const FreerdpSetActiveFn& setActive,
    const FreerdpClearActiveFn& clearActive, const std::function<void(const std::string&)>& log,
    const FreerdpConnectedFn& onConnected, const FreerdpInputPumpFn& pumpInput)
{
    RdpSessionRunResult result;
    result.available = true;

    std::string modulesPath = EnsureOpenSslModulesPath();
    if (!modulesPath.empty()) {
        log("OPENSSL_MODULES configured");
    }

    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        result.available = false;
        result.message = error;
        result.failed = true;
        return result;
    }
    log("FreeRDP runtime symbols loaded");
    RegisterMicrophonePermissionBridge(api, log);
    RegisterLocationBridge(api, log);

    if (api.ohosSessionPrepareOptions == nullptr || api.ohosSessionNew == nullptr ||
        api.ohosSessionFree == nullptr || api.ohosSessionConnect == nullptr ||
        api.ohosSessionGetDiagnostics == nullptr) {
        result.message = "FreeRDP OHOS session API symbols are not loaded";
        result.failed = true;
        return result;
    }
    if (api.ohosSessionConfigDefault == nullptr) {
        result.message = "FreeRDP OHOS session config symbol is not loaded";
        result.failed = true;
        return result;
    }

    FREERDP_OHOS_SESSION_INPUT input = {};
    input.serverHostname = params.host.c_str();
    input.serverPort = params.port.c_str();
    input.username = params.username.c_str();
    input.password = params.password.c_str();
    input.desktopSize = params.resolution.c_str();
    input.graphicsMode = params.graphicsMode.c_str();
    input.appDataDir = params.appFilesDir.c_str();
    input.certificatePolicy = params.certPolicy.c_str();
    input.colorDepth = 32;
    input.tcpConnectTimeoutMs = 5000;

    FREERDP_OHOS_SESSION_PREPARED_OPTIONS prepared = {};
    std::array<char, 512> detail {};
    if (!api.ohosSessionPrepareOptions(&input, &prepared, detail.data(), detail.size())) {
        result.message = detail[0] == '\0' ? "FreeRDP OHOS session option preparation failed" : detail.data();
        result.failed = true;
        return result;
    }
    if (detail[0] != '\0') {
        log(detail.data());
    }
    const GraphicsPipelineConfig graphicsConfig = ToGraphicsPipelineConfig(prepared.graphics);
    freerdpOhosSession* session = api.ohosSessionNew();
    if (session == nullptr) {
        result.message = "freerdp_ohos_session_new failed";
        result.failed = true;
        return result;
    }

    OhosSessionAdapter adapter {
        api,
        session,
        graphicsConfig,
        callbacks,
        setActive,
        clearActive,
        log,
        onConnected,
        pumpInput,
        running,
    };
    FREERDP_OHOS_SESSION_CALLBACKS sessionCallbacks = {};
    sessionCallbacks.StateChanged = OhosSessionAdapter::StateCallback;
    sessionCallbacks.Log = OhosSessionAdapter::LogCallback;
    sessionCallbacks.Error = OhosSessionAdapter::ErrorCallback;
    sessionCallbacks.Configure = OhosSessionAdapter::ConfigureCallback;
    sessionCallbacks.Connected = OhosSessionAdapter::ConnectedCallback;
    sessionCallbacks.Pump = OhosSessionAdapter::PumpCallback;
    sessionCallbacks.ShouldContinue = OhosSessionAdapter::ShouldContinueCallback;
    sessionCallbacks.Teardown = OhosSessionAdapter::TeardownCallback;
    sessionCallbacks.userData = &adapter;

    detail.fill('\0');
    const BOOL ok = api.ohosSessionConnect(
        session, &prepared.options, &sessionCallbacks, detail.data(), detail.size());
    result.connected = adapter.connected;
    result.message = detail[0] == '\0' ? SafeCString(api.ohosSessionGetDiagnostics(session)) : detail.data();
    api.ohosSessionFree(session);

    if (!running.load()) {
        result.cancelled = true;
        if (result.message.empty()) {
            result.message = "FreeRDP session cancelled";
        }
        return result;
    }

    if (!ok) {
        result.failed = true;
        if (result.message.empty()) {
            result.message = "FreeRDP OHOS session API failed";
        }
    } else if (result.message.empty()) {
        result.message = "FreeRDP session ended";
    }
    return result;
}

} // namespace rdp_bridge
