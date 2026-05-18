#include "channels/rdpgfx_pipeline.h"

#include "channels/rdpgfx_diagnostics.h"
#include "freerdp_runtime.h"
#include "string_utils.h"

#include <atomic>
#include <array>
#include <cstddef>
#include <cstdint>
#include <mutex>

namespace rdp_bridge {
namespace {

std::atomic_bool g_avc420SurfaceOutputConfigured{false};
std::atomic_bool g_avc420SurfaceOutputActive{false};
std::atomic_bool g_avc444GpuSurfaceOutputActive{false};
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::mutex g_ohosRdpgfxBridgeMutex;
freerdpOhosRdpgfxBridge* g_ohosRdpgfxBridge = nullptr;
std::mutex g_ohosCompositorMutex;
freerdpOhosCompositor* g_ohosCompositor = nullptr;
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

void OhosCompositorLogCallback(const char* message, void*)
{
    if (message != nullptr && message[0] != '\0') {
        LogThroughCallbacks(message);
    }
}

freerdpOhosCompositor* EnsureOhosCompositor(FreerdpRuntimeApi& api, std::string& error)
{
    std::lock_guard<std::mutex> lock(g_ohosCompositorMutex);
    if (g_ohosCompositor != nullptr) {
        return g_ohosCompositor;
    }
    if (api.ohosCompositorNew == nullptr) {
        error = "FreeRDP OHOS compositor symbols are not loaded";
        return nullptr;
    }
    g_ohosCompositor = api.ohosCompositorNew();
    if (g_ohosCompositor == nullptr) {
        error = "create FreeRDP OHOS compositor failed";
        return nullptr;
    }
    if (api.ohosCompositorConfigure != nullptr) {
        FREERDP_OHOS_COMPOSITOR_CONFIG config {};
        config.log = OhosCompositorLogCallback;
        std::array<char, 256> message {};
        if (!api.ohosCompositorConfigure(
                g_ohosCompositor, &config, message.data(), message.size())) {
            error = message[0] == '\0' ? "configure FreeRDP OHOS compositor failed" : message.data();
            api.ohosCompositorFree(g_ohosCompositor);
            g_ohosCompositor = nullptr;
            return nullptr;
        }
    }
    return g_ohosCompositor;
}

freerdpOhosCompositor* CurrentOhosCompositor()
{
    std::lock_guard<std::mutex> lock(g_ohosCompositorMutex);
    return g_ohosCompositor;
}

bool UpdateOhosCompositorOutputTarget(
    FreerdpRuntimeApi& api, const DecoderSurfaceTarget& target, const std::string& reason)
{
    if (api.ohosCompositorSetOutputTarget == nullptr) {
        return true;
    }
    std::string error;
    freerdpOhosCompositor* compositor = EnsureOhosCompositor(api, error);
    if (compositor == nullptr) {
        LogThroughCallbacks("OHOS compositor target update skipped after " + reason + ": " + error);
        return true;
    }
    FREERDP_OHOS_COMPOSITOR_OUTPUT_TARGET output {};
    output.window = target.window;
    output.width = target.width;
    output.height = target.height;
    std::array<char, 256> message {};
    if (!api.ohosCompositorSetOutputTarget(
            compositor, &output, message.data(), message.size())) {
        LogThroughCallbacks("OHOS compositor target update failed after " + reason + ": " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }
    LogThroughCallbacks(message.data());
    return true;
}

bool BeginOhosCompositorAvc420Route(FreerdpRuntimeApi& api, const std::string& reason)
{
    if (api.ohosCompositorBeginAvc420Surface == nullptr) {
        return true;
    }
    std::string error;
    freerdpOhosCompositor* compositor = EnsureOhosCompositor(api, error);
    if (compositor == nullptr) {
        LogThroughCallbacks("OHOS compositor AVC420 route skipped after " + reason + ": " + error);
        return true;
    }
    std::array<char, 256> message {};
    if (!api.ohosCompositorBeginAvc420Surface(compositor, message.data(), message.size())) {
        LogThroughCallbacks("OHOS compositor AVC420 route rejected after " + reason + ": " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }
    LogThroughCallbacks(message.data());
    return true;
}

bool BindAvcSurfaceOutput(
    const std::string& reason, const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO* command)
{
    if (!g_avc420SurfaceOutputConfigured.load()) {
        return false;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return false;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": decoder surface callback is not configured");
        return false;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": XComponent surface unavailable");
        return false;
    }
    if (!UpdateOhosCompositorOutputTarget(api, target, reason)) {
        return false;
    }
    if (!BeginOhosCompositorAvc420Route(api, reason)) {
        return false;
    }
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);

    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("before AVCodec surface bind after " + reason);
    }

    if (!api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE)) {
        LogThroughCallbacks("AVC surface output activation failed after " + reason +
            ": OHOS AVCodec surface setup failed");
        if (callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        return false;
    }

    g_avc444GpuSurfaceOutputActive.store(false);
    if (!g_avc420SurfaceOutputActive.exchange(true)) {
        std::string commandText;
        if (command != nullptr) {
            commandText = " surface=" + std::to_string(command->surfaceId) +
                " size=" + std::to_string(command->width) + "x" +
                std::to_string(command->height);
        }
        LogThroughCallbacks("AVC surface output activated after " + reason +
            ": target=" + std::to_string(target.width) + "x" + std::to_string(target.height) +
            commandText);
    } else {
        LogThroughCallbacks("AVC surface output updated after " + reason + ": target=" +
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
    g_avc444GpuSurfaceOutputActive.store(false);

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    freerdpOhosCompositor* compositor = CurrentOhosCompositor();
    if (compositor != nullptr && api.ohosCompositorEndAvc420Surface != nullptr) {
        api.ohosCompositorEndAvc420Surface(compositor);
    }
    if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
    }
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(nullptr, nullptr);
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.startRenderPipeline != nullptr) {
        callbacks.startRenderPipeline();
    }
    LogThroughCallbacks("AVC420 surface output disabled; using FreeRDP buffer/GLES fallback: " + reason);
}

void OnOhosAvcodecFallback(const char* reason, void*)
{
    SwitchAvc420SurfaceToSoftwareFallback(
        std::string("OHOS AVCodec runtime fallback: ") + SafeCString(reason));
}

BOOL OhosRdpgfxH264SurfaceCommandCallback(
    const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO* command, void*)
{
    if (command != nullptr) {
        return BindAvcSurfaceOutput("RDPGFX H264 surface command", command) ? TRUE : FALSE;
    }
    return FALSE;
}

void ActivateAvc444GpuCompositorOutput(FreerdpRuntimeApi& api)
{
    if (g_avc444GpuSurfaceOutputActive.exchange(true)) {
        return;
    }

    g_avc420SurfaceOutputActive.store(false);
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    freerdpOhosCompositor* compositor = CurrentOhosCompositor();
    if (compositor != nullptr && api.ohosCompositorEndAvc420Surface != nullptr) {
        api.ohosCompositorEndAvc420Surface(compositor);
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("before AVC444 GPU compositor output");
    }
    LogThroughCallbacks("AVC444 GPU compositor output activated; GDI renderer suppressed");
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
    return g_avc420SurfaceOutputActive.load() || g_avc444GpuSurfaceOutputActive.load();
}

void UpdateAvc420SurfaceOutputIfActive(const std::string& reason)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    const bool avc420Active = g_avc420SurfaceOutputActive.load();
    const bool avc444Active = g_avc444GpuSurfaceOutputActive.load();
    if (!g_avc420SurfaceOutputConfigured.load() || (!avc420Active && !avc444Active)) {
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
        g_avc444GpuSurfaceOutputActive.store(false);
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
            api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        if (callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        LogThroughCallbacks("AVC420 surface output disabled after " + reason +
            ": XComponent surface unavailable");
        return;
    }

    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
    if (!UpdateOhosCompositorOutputTarget(api, target, reason)) {
        return;
    }
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
    if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget("before AVCodec surface update after " + reason);
    }
    if (avc444Active) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        if (callbacks.registerAvc444DecodeSurfaces != nullptr) {
            callbacks.registerAvc444DecodeSurfaces(api, target.width, target.height, LogThroughCallbacks);
        }
        LogThroughCallbacks("AVC444 GPU compositor output updated after " + reason + ": " +
            std::to_string(target.width) + "x" + std::to_string(target.height));
        return;
    }
    if (!BeginOhosCompositorAvc420Route(api, reason)) {
        return;
    }
    api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE);
    LogThroughCallbacks("AVC420 surface output updated after " + reason + ": " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " avc444=gpu-compositor-prepared");
#else
    (void)reason;
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void ResetAvcSurfaceOutput(FreerdpRuntimeApi& api)
{
    g_avc420SurfaceOutputConfigured.store(false);
    g_avc420SurfaceOutputActive.store(false);
    g_avc444GpuSurfaceOutputActive.store(false);
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    freerdpOhosCompositor* compositor = CurrentOhosCompositor();
    if (compositor != nullptr && api.ohosCompositorEndAvc420Surface != nullptr) {
        api.ohosCompositorEndAvc420Surface(compositor);
    }
    if (compositor != nullptr && api.ohosCompositorClearOutputTarget != nullptr) {
        std::array<char, 128> message {};
        api.ohosCompositorClearOutputTarget(compositor, message.data(), message.size());
    }
    if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
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
    if (!UpdateOhosCompositorOutputTarget(api, target, "configure avc420 output")) {
        error = "OHOS compositor output target setup failed";
        return false;
    }
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(OnOhosAvcodecFallback, nullptr);
    }
    if (callbacks.registerAvc444DecodeSurfaces != nullptr) {
        const bool avc444SurfacesReady =
            callbacks.registerAvc444DecodeSurfaces(api, target.width, target.height, log);
        log(std::string("OHOS AVC444 NativeImage surface route preparation: ") +
            (avc444SurfacesReady ? "ready" : "not-ready") +
            " route=enabled-gpu-compositor");
    }

    g_avc420SurfaceOutputConfigured.store(true);
    g_avc420SurfaceOutputActive.store(false);
    g_avc444GpuSurfaceOutputActive.store(false);
    log("OHOS AVCodec output surface armed: XComponent NativeWindow " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " mode=deferred-until-avc-surface-command avc444=gpu-compositor gdi=active-before-h264");
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

std::string OhosCompositorDiagnostics(FreerdpRuntimeApi& api)
{
    freerdpOhosCompositor* compositor = CurrentOhosCompositor();
    if (compositor == nullptr || api.ohosCompositorGetDiagnostics == nullptr) {
        return "";
    }
    return SafeCString(api.ohosCompositorGetDiagnostics(compositor));
}

bool RegisterOhosCompositorAvc444DecodeSurfaces(FreerdpRuntimeApi& api,
    const FREERDP_OHOS_AVC444_SURFACE_TARGETS& targets, const FreerdpLogFn& log)
{
    if (api.ohosCompositorSetAvc444DecodeSurfaces == nullptr) {
        return true;
    }
    std::string error;
    freerdpOhosCompositor* compositor = EnsureOhosCompositor(api, error);
    if (compositor == nullptr) {
        log("OHOS compositor AVC444 surface registration skipped: " + error);
        return true;
    }
    std::array<char, 256> message {};
    if (!api.ohosCompositorSetAvc444DecodeSurfaces(
            compositor, &targets, TRUE, message.data(), message.size())) {
        log("OHOS compositor AVC444 surface registration failed: " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }
    log(message.data());
    return true;
}

void NotifyOhosCompositorAvc444Frame(FreerdpRuntimeApi& api, uint32_t surfaceId,
    uint32_t width, uint32_t height, uint32_t op, uint32_t codecId)
{
    freerdpOhosCompositor* compositor = CurrentOhosCompositor();
    if (compositor != nullptr && api.ohosCompositorNotifyAvc444Frame != nullptr) {
        ActivateAvc444GpuCompositorOutput(api);
        api.ohosCompositorNotifyAvc444Frame(compositor, surfaceId, width, height, op, codecId);
    }
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
    config.h264SurfaceMode = g_avc420SurfaceOutputConfigured.load() ? TRUE : FALSE;
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget != nullptr) {
        const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
        config.surfaceTargetWidth = target.width;
        config.surfaceTargetHeight = target.height;
    }
    config.log = OhosRdpgfxLogCallback;
    config.h264SurfaceCommand = OhosRdpgfxH264SurfaceCommandCallback;

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
