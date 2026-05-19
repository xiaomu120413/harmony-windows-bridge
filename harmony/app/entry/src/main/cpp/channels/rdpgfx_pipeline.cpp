#include "channels/rdpgfx_pipeline.h"

#include "channels/rdpgfx_diagnostics.h"
#include "freerdp_runtime.h"
#include "string_utils.h"

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

namespace rdp_bridge {
namespace {

std::atomic_bool g_avc420SurfaceOutputConfigured{false};
std::atomic_bool g_avc420SurfaceOutputActive{false};
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::mutex g_ohosRdpgfxBridgeMutex;
freerdpOhosRdpgfxBridge* g_ohosRdpgfxBridge = nullptr;
std::mutex g_ohosAvc420RouteMutex;
freerdpOhosAvc420Route* g_ohosAvc420Route = nullptr;
#endif

RdpgfxPipelineCallbacks SnapshotCallbacks()
{
    std::lock_guard<std::mutex> lock(g_callbacksMutex);
    return g_callbacks;
}

void LogThroughCallbacks(const std::string& line)
{
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.log != nullptr) {
        callbacks.log(line);
    }
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
freerdpOhosRdpgfxBridge* EnsureOhosRdpgfxBridge(FreerdpRuntimeApi& api, std::string& error)
{
    std::lock_guard<std::mutex> lock(g_ohosRdpgfxBridgeMutex);
    if (g_ohosRdpgfxBridge != nullptr) {
        return g_ohosRdpgfxBridge;
    }
    if (api.ohosRdpgfxBridgeNew == nullptr) {
        error = "FreeRDP OHOS rdpgfx bridge symbols are not loaded";
        return nullptr;
    }
    g_ohosRdpgfxBridge = api.ohosRdpgfxBridgeNew();
    if (g_ohosRdpgfxBridge == nullptr) {
        error = "create FreeRDP OHOS rdpgfx bridge failed";
        return nullptr;
    }
    return g_ohosRdpgfxBridge;
}

freerdpOhosRdpgfxBridge* CurrentOhosRdpgfxBridge()
{
    std::lock_guard<std::mutex> lock(g_ohosRdpgfxBridgeMutex);
    return g_ohosRdpgfxBridge;
}

void UpdateOhosRdpgfxSurfaceTarget(FreerdpRuntimeApi& api, uint32_t width, uint32_t height)
{
    freerdpOhosRdpgfxBridge* bridge = CurrentOhosRdpgfxBridge();
    if (bridge != nullptr && api.ohosRdpgfxBridgeSetSurfaceTarget != nullptr) {
        api.ohosRdpgfxBridgeSetSurfaceTarget(bridge, width, height);
    }
}

void OhosRdpgfxLogCallback(const char* message, void*)
{
    if (message != nullptr && message[0] != '\0') {
        LogThroughCallbacks(message);
    }
}

void OhosAvc420RouteLogCallback(const char* message, void*)
{
    if (message != nullptr && message[0] != '\0') {
        LogThroughCallbacks(message);
    }
}

freerdpOhosAvc420Route* EnsureOhosAvc420Route(FreerdpRuntimeApi& api, std::string& error)
{
    std::lock_guard<std::mutex> lock(g_ohosAvc420RouteMutex);
    if (g_ohosAvc420Route != nullptr) {
        return g_ohosAvc420Route;
    }
    if (api.ohosAvc420RouteNew == nullptr) {
        error = "FreeRDP OHOS AVC420 route symbols are not loaded";
        return nullptr;
    }
    g_ohosAvc420Route = api.ohosAvc420RouteNew();
    if (g_ohosAvc420Route == nullptr) {
        error = "create FreeRDP OHOS AVC420 route failed";
        return nullptr;
    }
    if (api.ohosAvc420RouteConfigure != nullptr) {
        FREERDP_OHOS_AVC420_ROUTE_CONFIG config {};
        config.log = OhosAvc420RouteLogCallback;
        std::array<char, 256> message {};
        if (!api.ohosAvc420RouteConfigure(
                g_ohosAvc420Route, &config, message.data(), message.size())) {
            error = message[0] == '\0' ? "configure FreeRDP OHOS AVC420 route failed" : message.data();
            api.ohosAvc420RouteFree(g_ohosAvc420Route);
            g_ohosAvc420Route = nullptr;
            return nullptr;
        }
    }
    return g_ohosAvc420Route;
}

freerdpOhosAvc420Route* CurrentOhosAvc420Route()
{
    std::lock_guard<std::mutex> lock(g_ohosAvc420RouteMutex);
    return g_ohosAvc420Route;
}

bool UpdateOhosAvc420RouteOutputTarget(
    FreerdpRuntimeApi& api, const DecoderSurfaceTarget& target, const std::string& reason)
{
    if (api.ohosAvc420RouteSetOutputTarget == nullptr) {
        return true;
    }
    std::string error;
    freerdpOhosAvc420Route* route = EnsureOhosAvc420Route(api, error);
    if (route == nullptr) {
        LogThroughCallbacks("OHOS AVC420 route target update skipped after " + reason + ": " + error);
        return true;
    }
    FREERDP_OHOS_AVC420_ROUTE_OUTPUT_TARGET output {};
    output.window = target.window;
    output.width = target.width;
    output.height = target.height;
    std::array<char, 256> message {};
    if (!api.ohosAvc420RouteSetOutputTarget(
            route, &output, message.data(), message.size())) {
        LogThroughCallbacks("OHOS AVC420 route target update failed after " + reason + ": " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }
    LogThroughCallbacks(message.data());
    return true;
}

bool BeginOhosAvc420Route(FreerdpRuntimeApi& api, const std::string& reason)
{
    if (api.ohosAvc420RouteBeginSurface == nullptr) {
        return true;
    }
    std::string error;
    freerdpOhosAvc420Route* route = EnsureOhosAvc420Route(api, error);
    if (route == nullptr) {
        LogThroughCallbacks("OHOS AVC420 route skipped after " + reason + ": " + error);
        return true;
    }
    std::array<char, 256> message {};
    if (!api.ohosAvc420RouteBeginSurface(route, message.data(), message.size())) {
        LogThroughCallbacks("OHOS AVC420 route rejected after " + reason + ": " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }
    LogThroughCallbacks(message.data());
    return true;
}

void PrepareNativeWindowForAvcDecoder(OHNativeWindow* window, const std::string& reason)
{
    if (window == nullptr) {
        return;
    }
    constexpr int32_t nv12Format = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP);
    const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, nv12Format);
    if (rc != 0) {
        LogThroughCallbacks("AVC420 surface output: SET_FORMAT(NV12) warning after " + reason +
            ": " + std::to_string(rc));
        return;
    }
    LogThroughCallbacks(
        "AVC420 surface output: NativeWindow format switched to NV12 before AVCodec bind (" +
        reason + ")");
}

void RestoreNativeWindowToRgba(OHNativeWindow* window, const std::string& reason)
{
    if (window == nullptr) {
        return;
    }
    constexpr int32_t rgbaFormat = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
    const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, rgbaFormat);
    if (rc != 0) {
        LogThroughCallbacks("AVC420 surface output: SET_FORMAT(RGBA_8888) warning after " + reason +
            ": " + std::to_string(rc));
        return;
    }
    LogThroughCallbacks(
        "AVC420 surface output: NativeWindow format restored to RGBA_8888 after " + reason);
}

bool BindAvc420SurfaceOutput(
    const std::string& reason, const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO* command)
{
    if (!g_avc420SurfaceOutputConfigured.load()) {
        return false;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        LogThroughCallbacks("AVC420 surface output activation skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return false;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        LogThroughCallbacks("AVC420 surface output activation skipped after " + reason +
            ": decoder surface callback is not configured");
        return false;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        LogThroughCallbacks("AVC420 surface output activation skipped after " + reason +
            ": XComponent surface unavailable");
        return false;
    }
    if (!UpdateOhosAvc420RouteOutputTarget(api, target, reason)) {
        return false;
    }
    if (!BeginOhosAvc420Route(api, reason)) {
        return false;
    }
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);

    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("before AVC420 AVCodec surface bind after " + reason);
    }
    PrepareNativeWindowForAvcDecoder(target.window, reason);

    if (!api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE)) {
        LogThroughCallbacks("AVC420 surface output activation failed after " + reason +
            ": OHOS AVCodec surface setup failed");
        if (callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        return false;
    }

    if (!g_avc420SurfaceOutputActive.exchange(true)) {
        std::string commandText;
        if (command != nullptr) {
            commandText = " surface=" + std::to_string(command->surfaceId) +
                " size=" + std::to_string(command->width) + "x" +
                std::to_string(command->height);
        }
        LogThroughCallbacks("AVC420 surface output activated after " + reason +
            ": target=" + std::to_string(target.width) + "x" + std::to_string(target.height) +
            commandText);
    } else {
        LogThroughCallbacks("AVC420 surface output updated after " + reason + ": target=" +
            std::to_string(target.width) + "x" + std::to_string(target.height));
    }
    return true;
}

void SwitchAvc420SurfaceToSoftwareFallback(const std::string& reason)
{
    if (!g_avc420SurfaceOutputConfigured.exchange(false)) {
        return;
    }
    g_avc420SurfaceOutputActive.store(false);

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    freerdpOhosAvc420Route* route = CurrentOhosAvc420Route();
    if (route != nullptr && api.ohosAvc420RouteEndSurface != nullptr) {
        api.ohosAvc420RouteEndSurface(route);
    }
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(nullptr, nullptr);
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget != nullptr) {
        RestoreNativeWindowToRgba(callbacks.decoderSurfaceTarget().window, reason);
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("after AVCodec surface release: " + reason);
    }
    if (callbacks.startRenderPipeline != nullptr) {
        callbacks.startRenderPipeline();
    }
    LogThroughCallbacks("AVC420 surface output disabled; using FreeRDP software/GDI rendering: " + reason);
}

void OnOhosAvcodecFallback(const char* reason, void*)
{
    SwitchAvc420SurfaceToSoftwareFallback(
        std::string("OHOS AVCodec runtime fallback: ") + SafeCString(reason));
}

BOOL OhosRdpgfxAvc420SurfaceCommandCallback(
    const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO* command, void*)
{
    if (command != nullptr) {
        return BindAvc420SurfaceOutput("RDPGFX AVC420 surface command", command) ? TRUE : FALSE;
    }
    return FALSE;
}

#endif

} // namespace

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks)
{
    std::lock_guard<std::mutex> lock(g_callbacksMutex);
    g_callbacks = std::move(callbacks);
}

bool IsAvc420SurfaceOutputEnabled()
{
    return g_avc420SurfaceOutputActive.load();
}

void UpdateAvc420SurfaceOutputIfActive(const std::string& reason)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    const bool avc420Active = g_avc420SurfaceOutputActive.load();
    if (!g_avc420SurfaceOutputConfigured.load() || !avc420Active) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        LogThroughCallbacks("AVC420 surface update skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        LogThroughCallbacks("AVC420 surface update skipped after " + reason +
            ": decoder surface callback is not configured");
        return;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        g_avc420SurfaceOutputActive.store(false);
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        if (callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        LogThroughCallbacks("AVC420 surface output disabled after " + reason +
            ": XComponent surface unavailable");
        return;
    }

    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
    if (!UpdateOhosAvc420RouteOutputTarget(api, target, reason)) {
        return;
    }
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("before AVCodec surface update after " + reason);
    }
    if (!BeginOhosAvc420Route(api, reason)) {
        return;
    }
    api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE);
    LogThroughCallbacks("AVC420 surface output updated after " + reason + ": " +
        std::to_string(target.width) + "x" + std::to_string(target.height));
#else
    (void)reason;
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void ResetAvcSurfaceOutput(FreerdpRuntimeApi& api)
{
    g_avc420SurfaceOutputConfigured.store(false);
    g_avc420SurfaceOutputActive.store(false);
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    freerdpOhosAvc420Route* route = CurrentOhosAvc420Route();
    if (route != nullptr && api.ohosAvc420RouteEndSurface != nullptr) {
        api.ohosAvc420RouteEndSurface(route);
    }
    if (route != nullptr && api.ohosAvc420RouteClearOutputTarget != nullptr) {
        std::array<char, 128> message {};
        api.ohosAvc420RouteClearOutputTarget(route, message.data(), message.size());
    }
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(nullptr, nullptr);
    }
}

bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const FreerdpLogFn& log, std::string& error)
{
    if (!graphicsConfig.enabled || !graphicsConfig.h264) {
        ResetAvcSurfaceOutput(api);
        return true;
    }

    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        error = "OHOS AVCodec surface output symbol is not loaded";
        return false;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        error = "AVC420 surface output requires a decoder surface callback";
        return false;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        error = "AVC420 surface output requires a ready XComponent NativeWindow";
        return false;
    }

    api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    if (!UpdateOhosAvc420RouteOutputTarget(api, target, "configure avc420 output")) {
        error = "OHOS AVC420 route output target setup failed";
        return false;
    }
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(OnOhosAvcodecFallback, nullptr);
    }
    g_avc420SurfaceOutputConfigured.store(true);
    g_avc420SurfaceOutputActive.store(false);
    log("OHOS AVCodec output surface armed: XComponent NativeWindow " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " mode=deferred-until-avc420-surface-command avc444=freerdp-native-gdi gdi=active");
    return true;
}

bool ConfigureGraphicsPipelineChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const FreerdpLogFn& log, std::string& error)
{
    (void)settings;
    SetRdpgfxRuntimeRequest(graphicsConfig.enabled, graphicsConfig.enabled && graphicsConfig.h264);
    SetRdpgfxBridgeAttached(false);
    ResetRdpgfxDiagnosticsStats();

    if (!graphicsConfig.enabled) {
        freerdpOhosRdpgfxBridge* bridge = CurrentOhosRdpgfxBridge();
        if (bridge != nullptr && api.ohosRdpgfxBridgeReset != nullptr) {
            api.ohosRdpgfxBridgeReset(bridge, FALSE, FALSE);
        }
        log("FreeRDP rdpgfx dynamic channel not requested: graphicsMode=gdi");
        log(BuildGraphicsPipelineStatsLog());
        return true;
    }

    if (api.gdiGraphicsPipelineInit == nullptr || api.gdiGraphicsPipelineUninit == nullptr) {
        error = "FreeRDP GDI graphics pipeline symbols are not loaded";
        return false;
    }

    freerdpOhosRdpgfxBridge* bridge = EnsureOhosRdpgfxBridge(api, error);
    if (bridge == nullptr) {
        return false;
    }
    if (api.ohosRdpgfxBridgeReset != nullptr) {
        api.ohosRdpgfxBridgeReset(
            bridge, TRUE, (graphicsConfig.h264 ? TRUE : FALSE));
    }

    log("FreeRDP rdpgfx requested: dynamic channel owned by OHOS session helper + GDI graphics pipeline bridge");
    log(BuildGraphicsPipelineStatsLog());
    return true;
}

std::string OhosRdpgfxBridgeDiagnostics(FreerdpRuntimeApi& api)
{
    freerdpOhosRdpgfxBridge* bridge = CurrentOhosRdpgfxBridge();
    if (bridge == nullptr || api.ohosRdpgfxBridgeGetDiagnostics == nullptr) {
        return "";
    }
    return SafeCString(api.ohosRdpgfxBridgeGetDiagnostics(bridge));
}

std::string OhosAvc420RouteDiagnostics(FreerdpRuntimeApi& api)
{
    freerdpOhosAvc420Route* route = CurrentOhosAvc420Route();
    if (route == nullptr || api.ohosAvc420RouteGetDiagnostics == nullptr) {
        return "";
    }
    return SafeCString(api.ohosAvc420RouteGetDiagnostics(route));
}

void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    freerdpOhosRdpgfxBridge* bridge = EnsureOhosRdpgfxBridge(api, error);
    if (bridge == nullptr) {
        LogThroughCallbacks("OHOS rdpgfx bridge attach skipped: " + error);
        return;
    }

    FREERDP_OHOS_RDPGFX_BRIDGE_CONFIG config = {};
    config.avc420SurfaceMode = g_avc420SurfaceOutputConfigured.load() ? TRUE : FALSE;
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget != nullptr) {
        const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
        config.surfaceTargetWidth = target.width;
        config.surfaceTargetHeight = target.height;
    }
    config.log = OhosRdpgfxLogCallback;
    config.avc420SurfaceCommand = OhosRdpgfxAvc420SurfaceCommandCallback;

    std::array<char, 256> message {};
    if (api.ohosRdpgfxBridgeAttach == nullptr ||
        !api.ohosRdpgfxBridgeAttach(bridge, gfx, &config, message.data(), message.size())) {
        LogThroughCallbacks("OHOS rdpgfx bridge attach failed: " +
            std::string(message[0] == '\0' ? "symbol unavailable" : message.data()));
        return;
    }
    if (api.ohosRdpgfxBridgeSetGdiAttached != nullptr) {
        api.ohosRdpgfxBridgeSetGdiAttached(bridge, TRUE);
    }
}

void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    freerdpOhosRdpgfxBridge* bridge = CurrentOhosRdpgfxBridge();
    if (bridge != nullptr && api.ohosRdpgfxBridgeDetach != nullptr) {
        api.ohosRdpgfxBridgeDetach(bridge, gfx);
    } else if (bridge != nullptr && api.ohosRdpgfxBridgeSetGdiAttached != nullptr) {
        api.ohosRdpgfxBridgeSetGdiAttached(bridge, FALSE);
    }
}
#endif

} // namespace rdp_bridge
