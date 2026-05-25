#include "session/freerdp_session_runner.h"

#include "common/bridge_log.h"
#include "freerdp/certificate_policy.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/graphics_config.h"
#include "napi/microphone_permission_bridge.h"
#include "common/string_utils.h"

#include <array>
#include <chrono>
#include <cstdio>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client.h>
#include <freerdp/error.h>
#include <freerdp/settings.h>
#include <winpr/synch.h>
#endif

namespace rdp_bridge {
namespace {

struct UserParts {
    std::string domain;
    std::string username;
};

UserParts SplitDomainUsername(const std::string& value)
{
    size_t separator = value.find('\\');
    size_t separatorLength = 1;
    const size_t ideographicSeparator = value.find("\xE3\x80\x81");
    if (separator == std::string::npos ||
        (ideographicSeparator != std::string::npos && ideographicSeparator < separator)) {
        separator = ideographicSeparator;
        separatorLength = 3;
    }

    if (separator == std::string::npos || separator == 0 || separator + separatorLength >= value.size()) {
        return {"", TrimAscii(value)};
    }

    std::string domain = TrimAscii(value.substr(0, separator));
    std::string username = TrimAscii(value.substr(separator + separatorLength));
    if (domain.empty() || username.empty()) {
        return {"", TrimAscii(value)};
    }
    return {domain, username};
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
uint32_t AlignDownToMultiple(uint32_t value, uint32_t alignment, uint32_t minimum)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (EnsureFreerdpRuntimeLoaded(api, error) &&
        api.ohosGraphicsAlignDownToMultiple != nullptr) {
        return api.ohosGraphicsAlignDownToMultiple(value, alignment, minimum);
    }

    value -= value % alignment;
    if (value >= minimum) {
        return value;
    }
    return minimum + ((alignment - (minimum % alignment)) % alignment);
}

void StopRenderPipeline(const RdpSessionCallbacks& callbacks)
{
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
}

uint32_t ToNativeCertificatePolicy(CertificatePolicy policy)
{
    switch (policy) {
        case CertificatePolicy::Strict:
            return FREERDP_OHOS_CERTIFICATE_POLICY_STRICT;
        case CertificatePolicy::Ignore:
            return FREERDP_OHOS_CERTIFICATE_POLICY_IGNORE;
        case CertificatePolicy::Tofu:
        default:
            return FREERDP_OHOS_CERTIFICATE_POLICY_TOFU;
    }
}

void CopyCallbackMessage(char* message, size_t messageSize, const std::string& value)
{
    if (message == nullptr || messageSize == 0) {
        return;
    }
    std::snprintf(message, messageSize, "%s", value.c_str());
}

void AlignH264DesktopSize(const GraphicsPipelineConfig& graphicsConfig, uint32_t& width,
    uint32_t& height, const std::function<void(const std::string&)>& log)
{
    if (!graphicsConfig.enabled || !graphicsConfig.h264) {
        return;
    }

    const uint32_t requestedWidth = width;
    const uint32_t requestedHeight = height;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (EnsureFreerdpRuntimeLoaded(api, error) &&
        api.ohosGraphicsAlignH264DesktopSize != nullptr) {
        FREERDP_OHOS_GRAPHICS_CONFIG nativeConfig = {};
        nativeConfig.enabled = graphicsConfig.enabled;
        nativeConfig.h264 = graphicsConfig.h264;
        api.ohosGraphicsAlignH264DesktopSize(&nativeConfig, &width, &height);
    } else {
        constexpr uint32_t kH264DesktopAlignment = 16;
        width = AlignDownToMultiple(width, kH264DesktopAlignment, 320U);
        height = AlignDownToMultiple(height, kH264DesktopAlignment, 240U);
    }

    if ((width != requestedWidth || height != requestedHeight) && log) {
        log("FreeRDP H264 desktop size aligned to 16px dimensions: " +
            std::to_string(requestedWidth) + "x" + std::to_string(requestedHeight) +
            " -> " + std::to_string(width) + "x" + std::to_string(height));
    }
}

struct OhosSessionAdapter {
    FreerdpRuntimeApi& api;
    const ConnectParams& params;
    const GraphicsPipelineConfig& graphicsConfig;
    CertificatePolicy certificatePolicy;
    const RdpSessionCallbacks& callbacks;
    const FreerdpSetActiveFn& setActive;
    const FreerdpClearActiveFn& clearActive;
    const std::function<void(const std::string&)>& log;
    const FreerdpConnectedFn& onConnected;
    const FreerdpInputPumpFn& pumpInput;
    std::atomic_bool& running;
    HarmonyClipboardBridge clipboardBridge;
    bool activeSet = false;
    bool certificateRegistered = false;
    bool connected = false;
    std::chrono::steady_clock::time_point nextAudioDiagnosticsLog =
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

        setActive(&api, instance, context);
        activeSet = true;

        std::string error;
        SetCertificatePolicyLogSink(log);
        if (!ConfigureFreerdpStoragePaths(api, context->settings, params, log, error) ||
            !ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error) ||
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
        instance->VerifyCertificateEx = HarmonyVerifyCertificateEx;
        instance->VerifyChangedCertificateEx = HarmonyVerifyChangedCertificateEx;
        SetGdiBridgeCallbacks({
            IsAvc420SurfaceOutputEnabled,
            callbacks.queueSurfaceRgbaFrame,
            callbacks.startRenderPipeline,
            callbacks.stopRenderPipeline,
            log,
        });
        RegisterCertificatePolicy(instance, certificatePolicy);
        certificateRegistered = true;
        ClearRdpDesktopSize();

        EmitLog("FreeRDP target configured through OHOS session API");
        EmitLog("FreeRDP mode=OhosSessionApi");
        EmitLog("FreeRDP GDI renderer configured");
        EmitLog(std::string("FreeRDP certificate policy=") + CertificatePolicyName(certificatePolicy));
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
        if (now >= nextAudioDiagnosticsLog) {
            EmitHilogInfo("FreeRDP audio diagnostics: " + BuildOHAudioStatsLog());
            nextAudioDiagnosticsLog = now + std::chrono::seconds(10);
        }
        return true;
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
        if (certificateRegistered) {
            UnregisterCertificatePolicy(instance);
            certificateRegistered = false;
        }
        ClearCertificatePolicyLogSink();
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
#endif

} // namespace

#if defined(HARMONY_HAS_FREERDP_HEADERS)
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

    uint32_t port = 0;
    if (!ParsePort(params.port, port)) {
        result.message = "invalid RDP port: " + params.port;
        result.failed = true;
        return result;
    }

    uint32_t width = 0;
    uint32_t height = 0;
    if (!ParseResolution(params.resolution, width, height)) {
        result.message = "invalid RDP desktop resolution: " + params.resolution;
        result.failed = true;
        return result;
    }
    const GraphicsPipelineConfig graphicsConfig = ParseGraphicsPipelineConfig(params);
    AlignH264DesktopSize(graphicsConfig, width, height, log);

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

    if (api.ohosSessionNew == nullptr || api.ohosSessionFree == nullptr ||
        api.ohosSessionConnect == nullptr || api.ohosSessionGetDiagnostics == nullptr) {
        result.message = "FreeRDP OHOS session API symbols are not loaded";
        result.failed = true;
        return result;
    }
    if (api.ohosSessionConfigDefault == nullptr) {
        result.message = "FreeRDP OHOS session config symbol is not loaded";
        result.failed = true;
        return result;
    }

    UserParts user = SplitDomainUsername(params.username);
    const CertificatePolicy certificatePolicy = ParseCertificatePolicy(params.certPolicy);
    const bool ignoreCertificate = certificatePolicy == CertificatePolicy::Ignore;

    if (!user.domain.empty()) {
        log("FreeRDP domain parsed from username");
    }

    FREERDP_OHOS_SESSION_CONFIG sessionConfig = api.ohosSessionConfigDefault();
    sessionConfig.graphicsPipeline = graphicsConfig.enabled ? TRUE : FALSE;
    sessionConfig.h264 = graphicsConfig.enabled && graphicsConfig.h264 ? TRUE : FALSE;

    FREERDP_OHOS_SESSION_OPTIONS options = {};
    options.connection.serverHostname = params.host.c_str();
    options.connection.serverPort = port;
    options.connection.username = user.username.c_str();
    options.connection.password = params.password.c_str();
    options.connection.domain = user.domain.empty() ? nullptr : user.domain.c_str();
    options.connection.desktopWidth = width;
    options.connection.desktopHeight = height;
    options.connection.colorDepth = 32;
    options.connection.tcpConnectTimeoutMs = 5000;
    options.connection.ignoreCertificate = ignoreCertificate ? TRUE : FALSE;
    options.session = sessionConfig;
    options.appDataDir = params.appFilesDir.empty() ? nullptr : params.appFilesDir.c_str();
    options.certificatePolicy = ToNativeCertificatePolicy(certificatePolicy);

    freerdpOhosSession* session = api.ohosSessionNew();
    if (session == nullptr) {
        result.message = "freerdp_ohos_session_new failed";
        result.failed = true;
        return result;
    }

    OhosSessionAdapter adapter {
        api,
        params,
        graphicsConfig,
        certificatePolicy,
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

    std::array<char, 512> detail {};
    const BOOL ok = api.ohosSessionConnect(
        session, &options, &sessionCallbacks, detail.data(), detail.size());
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
#else
RdpSessionRunResult RunFreerdpSessionUnavailable()
{
    RdpSessionRunResult result;
    result.available = false;
    result.message = "explicit FreeRDP demo build has no headers";
    result.failed = true;
    return result;
}
#endif

} // namespace rdp_bridge
