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
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::mutex g_ohosRdpgfxBridgeMutex;
freerdpOhosRdpgfxBridge* g_ohosRdpgfxBridge = nullptr;
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
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);

    if (!api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE)) {
        LogThroughCallbacks("AVC surface output activation failed after " + reason +
            ": OHOS AVCodec surface setup failed");
        return false;
    }

    if (!g_avc420SurfaceOutputActive.exchange(true)) {
        if (callbacks.stopRenderPipeline != nullptr) {
            callbacks.stopRenderPipeline();
        }
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

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
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
    if (!g_avc420SurfaceOutputConfigured.load() || !g_avc420SurfaceOutputActive.load()) {
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
    api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE);
    LogThroughCallbacks("AVC surface output updated after " + reason + ": " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " avc444=primary-avc420-surface");
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
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(OnOhosAvcodecFallback, nullptr);
    }
    if (callbacks.registerAvc444DecodeSurfaces != nullptr) {
        const bool avc444SurfacesReady =
            callbacks.registerAvc444DecodeSurfaces(api, target.width, target.height, log);
        log(std::string("OHOS AVC444 NativeImage surface route preparation: ") +
            (avc444SurfacesReady ? "ready" : "not-ready") +
            " route=disabled-until-gpu-compositor");
    }

    g_avc420SurfaceOutputConfigured.store(true);
    g_avc420SurfaceOutputActive.store(false);
    log("OHOS AVCodec output surface armed: XComponent NativeWindow " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " mode=deferred-until-avc-surface-command avc444=primary-avc420-surface gdi=active-before-h264");
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
