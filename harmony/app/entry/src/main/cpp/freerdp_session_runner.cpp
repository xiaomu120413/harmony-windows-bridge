#include "freerdp_session_runner.h"

#include "bridge_log.h"
#include "certificate_policy.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp_gdi_bridge.h"
#include "graphics_config.h"
#include "microphone_permission_bridge.h"
#include "rdp_channel_config.h"
#include "string_utils.h"

#include <array>
#include <chrono>

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
        log("OPENSSL_MODULES=" + modulesPath);
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

    freerdp* instance = api.freerdpNew();
    if (instance == nullptr) {
        result.message = "freerdp_new failed";
        result.failed = true;
        return result;
    }

    bool contextCreated = false;
    HarmonyClipboardBridge clipboardBridge;
    auto cleanup = [&]() {
        ResetAvcSurfaceOutput(api);
        clipboardBridge.Uninitialize();
        ClearRdpDesktopSize();
        UnregisterCertificatePolicy(instance);
        ClearCertificatePolicyLogSink();
        if (contextCreated && instance->context != nullptr) {
            api.abortConnectContext(instance->context);
            api.disconnect(instance);
            StopRenderPipeline(callbacks);
            clearActive(instance);
            api.contextFree(instance);
        } else {
            clearActive(instance);
        }
        api.freerdpFree(instance);
    };

    if (!api.contextNew(instance)) {
        result.message = "freerdp_context_new failed";
        result.failed = true;
        cleanup();
        return result;
    }
    contextCreated = true;
    if (!EnableFreerdpClientChannels(api, instance, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }
    setActive(&api, instance, instance->context);

    rdpSettings* settings = instance->context == nullptr ? nullptr : instance->context->settings;
    if (settings == nullptr) {
        result.message = "FreeRDP settings unavailable";
        result.failed = true;
        cleanup();
        return result;
    }
    SetCertificatePolicyLogSink(log);

    UserParts user = SplitDomainUsername(params.username);
    const CertificatePolicy certificatePolicy = ParseCertificatePolicy(params.certPolicy);
    const bool ignoreCertificate = certificatePolicy == CertificatePolicy::Ignore;

    if (api.ohosSessionApplyConnectionSettings == nullptr) {
        result.message = "FreeRDP OHOS connection settings helper is not loaded";
        result.failed = true;
        cleanup();
        return result;
    }
    FREERDP_OHOS_CONNECTION_CONFIG connectionConfig = {};
    connectionConfig.serverHostname = params.host.c_str();
    connectionConfig.serverPort = port;
    connectionConfig.username = user.username.c_str();
    connectionConfig.password = params.password.c_str();
    connectionConfig.domain = user.domain.empty() ? nullptr : user.domain.c_str();
    connectionConfig.desktopWidth = width;
    connectionConfig.desktopHeight = height;
    connectionConfig.colorDepth = 32;
    connectionConfig.tcpConnectTimeoutMs = 5000;
    connectionConfig.ignoreCertificate = ignoreCertificate ? TRUE : FALSE;

    std::array<char, 256> connectionDetail {};
    if (!api.ohosSessionApplyConnectionSettings(
            settings, &connectionConfig, connectionDetail.data(), connectionDetail.size())) {
        result.message = connectionDetail[0] == '\0'
            ? "FreeRDP OHOS connection settings helper failed"
            : connectionDetail.data();
        result.failed = true;
        cleanup();
        return result;
    }
    log(connectionDetail[0] == '\0'
            ? "OHOS FreeRDP connection settings applied"
            : connectionDetail.data());

    if (!ConfigureFreerdpStoragePaths(api, settings, params, log, error) ||
        !ConfigureEnhancedRdpSettings(api, settings, graphicsConfig, log, error) ||
        !ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error) ||
        !ConfigureGraphicsPipelineChannel(api, settings, graphicsConfig, log, error) ||
        !ConfigureOhosStandardChannels(api, settings, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!clipboardBridge.Initialize(instance->context, api, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
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
    ClearRdpDesktopSize();

    log("FreeRDP target configured");
    log("FreeRDP mode=PersistentSession");
    log("FreeRDP GDI renderer configured");
    log(std::string("FreeRDP certificate policy=") + CertificatePolicyName(certificatePolicy));
    if (!user.domain.empty()) {
        log("FreeRDP domain parsed from username");
    }

    BOOL rc = api.connect(instance);
    uint32_t lastError = instance->context == nullptr ? UINT32_MAX : api.getLastError(instance->context);
    log(std::string("freerdp_connect returned ") + (rc ? "true" : "false"));

    if (!running.load()) {
        result.cancelled = true;
        result.message = "FreeRDP connect cancelled";
        cleanup();
        return result;
    }

    if (!rc) {
        result.failed = true;
        result.message = "FreeRDP connect failed: " + LastErrorMessage(api, lastError);
        cleanup();
        return result;
    }

    result.connected = true;
    result.message = "FreeRDP session connected";
    onConnected();
    log("FreeRDP event loop started");
    auto nextAudioDiagnosticsLog = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (running.load() && !api.shallDisconnectContext(instance->context)) {
        pumpInput(&api, instance->context);
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextAudioDiagnosticsLog) {
            EmitHilogInfo("FreeRDP audio diagnostics: " + BuildOHAudioStatsLog());
            nextAudioDiagnosticsLog = now + std::chrono::seconds(10);
        }

        HANDLE handles[MAXIMUM_WAIT_OBJECTS] = {};
        DWORD count = api.getEventHandles(instance->context, handles, MAXIMUM_WAIT_OBJECTS);
        if (count == 0) {
            uint32_t errorCode = api.getLastError(instance->context);
            result.failed = true;
            result.message = "freerdp_get_event_handles failed: " + LastErrorMessage(api, errorCode);
            break;
        }

        DWORD waitStatus = api.waitForMultipleObjects(count, handles, FALSE, 5);
        if (!running.load()) {
            result.cancelled = true;
            result.message = "FreeRDP session cancelled";
            break;
        }

        if (waitStatus == WAIT_TIMEOUT) {
            continue;
        }

        if (waitStatus == WAIT_FAILED) {
            result.failed = true;
            result.message = "WaitForMultipleObjects failed: " + Hex32(static_cast<uint32_t>(waitStatus));
            break;
        }

        if (!api.checkEventHandles(instance->context)) {
            uint32_t errorCode = api.getLastError(instance->context);
            if (errorCode == FREERDP_ERROR_SUCCESS) {
                result.message = "FreeRDP event loop stopped without error";
            } else if (graphicsConfig.h264 && errorCode == ERROR_NOT_SUPPORTED) {
                result.failed = true;
                result.message =
                    "FreeRDP graphics negotiation failed: server did not confirm requested RDPGFX AVC420 mode while AVC444 is disabled";
            } else {
                result.failed = true;
                result.message = "FreeRDP event loop failed: " + LastErrorMessage(api, errorCode);
            }
            break;
        }

        pumpInput(&api, instance->context);
    }

    if (!result.cancelled && !result.failed && result.message == "FreeRDP session connected") {
        result.message = "FreeRDP session ended";
    }

    cleanup();
    return result;
}
#else
RdpSessionRunResult RunFreerdpSessionUnavailable()
{
    RdpSessionRunResult result;
    result.available = false;
    result.message = "FreeRDP headers not found at build time";
    result.failed = true;
    return result;
}
#endif

} // namespace rdp_bridge
