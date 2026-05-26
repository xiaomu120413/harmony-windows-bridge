#include "channels/rdpgfx_pipeline.h"

#include "channels/rdpgfx_diagnostics.h"
#include "common/string_utils.h"
#include "freerdp/freerdp_runtime.h"
#include "surface/avc444_gpu_compositor.h"
#include "surface/render_output_owner.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>

#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

namespace rdp_bridge {
namespace {

std::atomic_bool g_avc420SurfaceRouteArmed{false};
std::atomic_bool g_avc444GpuCompositorConfigured{false};
std::atomic<uint64_t> g_avc444EndFrameCallbackCount{0};
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

std::mutex g_ohosRdpgfxBridgeMutex;
freerdpOhosRdpgfxBridge *g_ohosRdpgfxBridge = nullptr;
std::mutex g_ohosAvc420RouteMutex;
freerdpOhosAvc420Route *g_ohosAvc420Route = nullptr;

RdpgfxPipelineCallbacks SnapshotCallbacks() {
  std::lock_guard<std::mutex> lock(g_callbacksMutex);
  return g_callbacks;
}

void LogThroughCallbacks(const std::string &line) {
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.log != nullptr) {
    callbacks.log(line);
  }
}

void ResetAvc444GpuOutputOwner(const std::string &reason) {
  const RenderOutputOwner previous =
      ExchangeRenderOutputOwner(RenderOutputOwner::Gdi);
  if (previous == RenderOutputOwner::Avc444Gpu) {
    LogThroughCallbacks("render output owner reset after " + reason +
                        ": avc444-gpu -> gdi");
  }
}

freerdpOhosRdpgfxBridge *EnsureOhosRdpgfxBridge(FreerdpRuntimeApi &api,
                                                std::string &error) {
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

freerdpOhosRdpgfxBridge *CurrentOhosRdpgfxBridge() {
  std::lock_guard<std::mutex> lock(g_ohosRdpgfxBridgeMutex);
  return g_ohosRdpgfxBridge;
}

void UpdateOhosRdpgfxSurfaceTarget(FreerdpRuntimeApi &api, uint32_t width,
                                   uint32_t height) {
  freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
  if (bridge != nullptr && api.ohosRdpgfxBridgeSetSurfaceTarget != nullptr) {
    api.ohosRdpgfxBridgeSetSurfaceTarget(bridge, width, height);
  }
}

void OhosRdpgfxLogCallback(const char *message, void *) {
  if (message != nullptr && message[0] != '\0') {
    LogThroughCallbacks(message);
  }
}

void OhosAvc420RouteLogCallback(const char *message, void *) {
  if (message != nullptr && message[0] != '\0') {
    LogThroughCallbacks(message);
  }
}

void PrepareNativeWindowForAvcDecoder(OHNativeWindow *window,
                                      const std::string &reason);
void RestoreNativeWindowToRgba(OHNativeWindow *window,
                               const std::string &reason);

BOOL OhosAvc420RouteGetOutputTarget(
    FREERDP_OHOS_AVC420_ROUTE_OUTPUT_TARGET *target, void *) {
  if (target == nullptr) {
    return FALSE;
  }
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.decoderSurfaceTarget == nullptr) {
    return FALSE;
  }
  const DecoderSurfaceTarget snapshot = callbacks.decoderSurfaceTarget();
  target->window = snapshot.window;
  target->width = snapshot.width;
  target->height = snapshot.height;
  return snapshot.window != nullptr && snapshot.width > 0 && snapshot.height > 0
             ? TRUE
             : FALSE;
}

void OhosAvc420RoutePrepareOutputTarget(void *window, const char *reason,
                                        void *) {
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.stopRenderPipeline != nullptr) {
    callbacks.stopRenderPipeline();
  }
  if (callbacks.releaseRenderTarget != nullptr) {
    callbacks.releaseRenderTarget("before AVC420 AVCodec surface bind after " +
                                  SafeCString(reason));
  }
  PrepareNativeWindowForAvcDecoder(static_cast<OHNativeWindow *>(window),
                                   SafeCString(reason));
}

void OhosAvc420RouteRestoreOutputTarget(void *window, const char *reason,
                                        void *) {
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  RestoreNativeWindowToRgba(static_cast<OHNativeWindow *>(window),
                            SafeCString(reason));
  if (callbacks.releaseRenderTarget != nullptr) {
    callbacks.releaseRenderTarget("after AVCodec surface release: " +
                                  SafeCString(reason));
  }
  if (callbacks.startRenderPipeline != nullptr) {
    callbacks.startRenderPipeline();
  }
}

freerdpOhosAvc420Route *EnsureOhosAvc420Route(FreerdpRuntimeApi &api,
                                              std::string &error) {
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
    FREERDP_OHOS_AVC420_ROUTE_CONFIG config{};
    config.log = OhosAvc420RouteLogCallback;
    config.getOutputTarget = OhosAvc420RouteGetOutputTarget;
    config.prepareOutputTarget = OhosAvc420RoutePrepareOutputTarget;
    config.restoreOutputTarget = OhosAvc420RouteRestoreOutputTarget;
    std::array<char, 256> message{};
    if (!api.ohosAvc420RouteConfigure(g_ohosAvc420Route, &config,
                                      message.data(), message.size())) {
      error = message[0] == '\0' ? "configure FreeRDP OHOS AVC420 route failed"
                                 : message.data();
      api.ohosAvc420RouteFree(g_ohosAvc420Route);
      g_ohosAvc420Route = nullptr;
      return nullptr;
    }
  }
  return g_ohosAvc420Route;
}

freerdpOhosAvc420Route *CurrentOhosAvc420Route() {
  std::lock_guard<std::mutex> lock(g_ohosAvc420RouteMutex);
  return g_ohosAvc420Route;
}

void PrepareNativeWindowForAvcDecoder(OHNativeWindow *window,
                                      const std::string &reason) {
  if (window == nullptr) {
    return;
  }
  constexpr int32_t nv12Format =
      static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP);
  const int32_t rc =
      OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, nv12Format);
  if (rc != 0) {
    LogThroughCallbacks(
        "AVC420 surface output: SET_FORMAT(NV12) warning after " + reason +
        ": " + std::to_string(rc));
    return;
  }
  LogThroughCallbacks("AVC420 surface output: NativeWindow format switched to "
                      "NV12 before AVCodec bind (" +
                      reason + ")");
}

void RestoreNativeWindowToRgba(OHNativeWindow *window,
                               const std::string &reason) {
  if (window == nullptr) {
    return;
  }
  constexpr int32_t rgbaFormat =
      static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
  const int32_t rc =
      OH_NativeWindow_NativeWindowHandleOpt(window, SET_FORMAT, rgbaFormat);
  if (rc != 0) {
    LogThroughCallbacks(
        "AVC420 surface output: SET_FORMAT(RGBA_8888) warning after " + reason +
        ": " + std::to_string(rc));
    return;
  }
  LogThroughCallbacks("AVC420 surface output: NativeWindow format restored to "
                      "RGBA_8888 after " +
                      reason);
}

bool BindAvc420SurfaceOutput(
    const std::string &reason,
    const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO *command) {
  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  if (api.ohosAvc420RouteBeginSurface == nullptr) {
    return false;
  }
  std::string error;
  freerdpOhosAvc420Route *route = EnsureOhosAvc420Route(api, error);
  if (route == nullptr) {
    LogThroughCallbacks("OHOS AVC420 route skipped after " + reason + ": " +
                        error);
    return false;
  }

  std::array<char, 256> message{};
  if (!api.ohosAvc420RouteBeginSurface(route, message.data(), message.size())) {
    LogThroughCallbacks(
        "OHOS AVC420 route rejected after " + reason + ": " +
        std::string(message[0] == '\0' ? "unknown error" : message.data()));
    return false;
  }
  if (message[0] != '\0') {
    LogThroughCallbacks(message.data());
  }
  (void)command;
  return true;
}

BOOL OhosRdpgfxAvc420SurfaceCommandCallback(
    const FREERDP_OHOS_RDPGFX_SURFACE_COMMAND_INFO *command, void *) {
  if (command != nullptr) {
    return BindAvc420SurfaceOutput("RDPGFX AVC420 surface command", command)
               ? TRUE
               : FALSE;
  }
  return FALSE;
}

BOOL OhosRdpgfxAvc444SurfaceCommandCallback(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO *command, void *) {
  return SharedAvc444GpuCompositor().OnSurfaceCommand(command) ? TRUE : FALSE;
}

BOOL OhosRdpgfxAvc444EndFrameCallback(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO *frame, void *) {
  const uint64_t count = ++g_avc444EndFrameCallbackCount;
  const bool shouldLog = count <= 20U || (count % 60U) == 0U;
  if (shouldLog) {
    LogThroughCallbacks(
        "AVC444 GPU bridge EndFrame callback enter: frame=" +
        std::to_string(frame == nullptr ? 0U : frame->frameId) + " active=" +
        std::to_string(frame == nullptr ? 0U : frame->activeFrameId) +
        " matched=" +
        std::string((frame != nullptr && frame->matchedFrame) ? "yes" : "no") +
        " count=" + std::to_string(count));
  }
  const bool handled = SharedAvc444GpuCompositor().OnEndFrame(frame);
  if (shouldLog || !handled) {
    LogThroughCallbacks("AVC444 GPU bridge EndFrame callback leave: frame=" +
                        std::to_string(frame == nullptr ? 0U : frame->frameId) +
                        " handled=" + std::string(handled ? "yes" : "no") +
                        " count=" + std::to_string(count) + " diagnostics=" +
                        SharedAvc444GpuCompositor().Diagnostics());
  }
  return handled ? TRUE : FALSE;
}

void OhosRdpgfxAvc444OutputStateCallback(BOOL active, const char *reason,
                                         void *) {
  SharedAvc444GpuCompositor().SetOutputActive(active != FALSE,
                                              SafeCString(reason));
  if (active) {
    const RenderOutputOwner previous =
        ExchangeRenderOutputOwner(RenderOutputOwner::Avc444Gpu);
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (previous != RenderOutputOwner::Avc444Gpu) {
      if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
      }
      if (callbacks.releaseRenderTarget != nullptr) {
        callbacks.releaseRenderTarget(
            "before AVC444 GPU output takeover after " + SafeCString(reason));
      }
    }
    LogThroughCallbacks(
        "AVC444 GPU output owner activated by FreeRDP policy: previous=" +
        RenderOutputOwnerName(previous) + " reason=" + SafeCString(reason));
    return;
  }

  const RenderOutputOwner previous =
      ExchangeRenderOutputOwner(RenderOutputOwner::Gdi);
  if (previous == RenderOutputOwner::Avc444Gpu) {
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.startRenderPipeline != nullptr) {
      callbacks.startRenderPipeline();
    }
  }
  LogThroughCallbacks(
      "AVC444 GPU output owner released by FreeRDP policy: previous=" +
      RenderOutputOwnerName(previous) + " reason=" + SafeCString(reason));
}


} // namespace

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks) {
  std::lock_guard<std::mutex> lock(g_callbacksMutex);
  g_callbacks = std::move(callbacks);
}

bool IsAvc420SurfaceOutputEnabled() {
  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  freerdpOhosAvc420Route *route = CurrentOhosAvc420Route();
  return route != nullptr && api.ohosAvc420RouteIsSurfaceActive != nullptr &&
         api.ohosAvc420RouteIsSurfaceActive(route) != FALSE;
}

void UpdateAvc420SurfaceOutputIfActive(const std::string &reason) {
  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  freerdpOhosAvc420Route *route = CurrentOhosAvc420Route();
  if (route == nullptr || api.ohosAvc420RouteIsSurfaceActive == nullptr ||
      api.ohosAvc420RouteRefreshOutputTarget == nullptr ||
      api.ohosAvc420RouteIsSurfaceActive(route) == FALSE) {
    return;
  }

  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.decoderSurfaceTarget != nullptr) {
    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
  }
  std::array<char, 256> message{};
  if (!api.ohosAvc420RouteRefreshOutputTarget(route, reason.c_str(),
                                              message.data(), message.size())) {
    LogThroughCallbacks(
        "OHOS AVC420 route refresh failed after " + reason + ": " +
        std::string(message[0] == '\0' ? "unknown error" : message.data()));
  } else if (message[0] != '\0') {
    LogThroughCallbacks(message.data());
  }
}

void UpdateRdpgfxSurfaceTargetIfReady(const std::string &reason) {
  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
  if (bridge == nullptr || api.ohosRdpgfxBridgeSetSurfaceTarget == nullptr) {
    return;
  }

  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.decoderSurfaceTarget == nullptr) {
    LogThroughCallbacks("RDPGFX surface target update skipped after " + reason +
                        ": decoder surface callback is not configured");
    return;
  }

  const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
  UpdateOhosRdpgfxSurfaceTarget(api, target.width, target.height);
  LogThroughCallbacks("RDPGFX surface target updated after " + reason + ": " +
                      std::to_string(target.width) + "x" +
                      std::to_string(target.height));
}

void ResetAvcSurfaceOutput(FreerdpRuntimeApi &api) {
  g_avc420SurfaceRouteArmed.store(false);
  g_avc444GpuCompositorConfigured.store(false);
  ResetAvc444GpuOutputOwner("AVC surface output reset");
  SharedAvc444GpuCompositor().Reset();
  freerdpOhosAvc420Route *route = CurrentOhosAvc420Route();
  if (route != nullptr && api.ohosAvc420RouteSetArmed != nullptr) {
    std::array<char, 128> message{};
    api.ohosAvc420RouteSetArmed(route, FALSE, message.data(), message.size());
  }
}

bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi &api,
                                  const GraphicsPipelineConfig &graphicsConfig,
                                  const FreerdpLogFn &log, std::string &error) {
  if (!graphicsConfig.enabled || !graphicsConfig.h264) {
    ResetAvcSurfaceOutput(api);
    return true;
  }

  if (api.ohosAvc420RouteSetArmed == nullptr) {
    error = "FreeRDP OHOS AVC420 route arm symbol is not loaded";
    return false;
  }

  freerdpOhosAvc420Route *route = EnsureOhosAvc420Route(api, error);
  if (route == nullptr) {
    return false;
  }

  std::array<char, 256> message{};
  if (!api.ohosAvc420RouteSetArmed(route, TRUE, message.data(),
                                   message.size())) {
    error =
        message[0] == '\0' ? "OHOS AVC420 route arm failed" : message.data();
    return false;
  }
  g_avc420SurfaceRouteArmed.store(true);
  if (message[0] != '\0') {
    log(message.data());
  }
  log("OHOS AVCodec output surface route armed: "
      "mode=deferred-until-avc420-surface-command "
      "fallback=FreeRDP-route-owned avc444=gpu-auto-with-gdi-fallback "
      "gdi=active");
  return true;
}

bool ConfigureGraphicsPipelineChannel(
    FreerdpRuntimeApi &api, rdpSettings *settings,
    const GraphicsPipelineConfig &graphicsConfig, const FreerdpLogFn &log,
    std::string &error) {
  (void)settings;
  SetRdpgfxRuntimeRequest(graphicsConfig.enabled,
                          graphicsConfig.enabled && graphicsConfig.h264);
  SetRdpgfxBridgeAttached(false);
  g_avc444GpuCompositorConfigured.store(graphicsConfig.enabled &&
                                        graphicsConfig.h264 &&
                                        graphicsConfig.avc444GpuCompositor);
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  Avc444GpuCompositorCallbacks avc444Callbacks;
  avc444Callbacks.decoderSurfaceTarget = callbacks.decoderSurfaceTarget;
  SharedAvc444GpuCompositor().Configure(g_avc444GpuCompositorConfigured.load(),
                                        log, std::move(avc444Callbacks));
  ResetRdpgfxDiagnosticsStats();

  if (!graphicsConfig.enabled) {
    freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
    if (bridge != nullptr && api.ohosRdpgfxBridgeReset != nullptr) {
      api.ohosRdpgfxBridgeReset(bridge, FALSE, FALSE);
    }
    log("FreeRDP rdpgfx dynamic channel not requested: graphicsMode=gdi");
    log(BuildGraphicsPipelineStatsLog());
    return true;
  }

  if (api.gdiGraphicsPipelineInit == nullptr ||
      api.gdiGraphicsPipelineUninit == nullptr) {
    error = "FreeRDP GDI graphics pipeline symbols are not loaded";
    return false;
  }

  freerdpOhosRdpgfxBridge *bridge = EnsureOhosRdpgfxBridge(api, error);
  if (bridge == nullptr) {
    return false;
  }
  if (api.ohosRdpgfxBridgeReset != nullptr) {
    api.ohosRdpgfxBridgeReset(bridge, TRUE,
                              (graphicsConfig.h264 ? TRUE : FALSE));
  }

  log("FreeRDP rdpgfx requested: dynamic channel owned by OHOS session helper "
      "+ GDI graphics pipeline bridge");
  log("OHOS AVC444 GPU compositor " +
      std::string(g_avc444GpuCompositorConfigured.load()
                      ? "enabled by default; GDI suppression is per-command "
                        "after GPU handling succeeds"
                      : "off; AVC444 stays on FreeRDP native GDI"));
  log(BuildGraphicsPipelineStatsLog());
  return true;
}

std::string OhosRdpgfxBridgeDiagnostics(FreerdpRuntimeApi &api) {
  freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
  if (bridge == nullptr || api.ohosRdpgfxBridgeGetDiagnostics == nullptr) {
    return "";
  }
  return SafeCString(api.ohosRdpgfxBridgeGetDiagnostics(bridge));
}

std::string OhosAvc420RouteDiagnostics(FreerdpRuntimeApi &api) {
  freerdpOhosAvc420Route *route = CurrentOhosAvc420Route();
  if (route == nullptr || api.ohosAvc420RouteGetDiagnostics == nullptr) {
    return "";
  }
  return SafeCString(api.ohosAvc420RouteGetDiagnostics(route));
}

void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext *gfx) {
  if (gfx == nullptr) {
    return;
  }

  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  std::string error;
  freerdpOhosRdpgfxBridge *bridge = EnsureOhosRdpgfxBridge(api, error);
  if (bridge == nullptr) {
    LogThroughCallbacks("OHOS rdpgfx bridge attach skipped: " + error);
    return;
  }

  FREERDP_OHOS_RDPGFX_BRIDGE_CONFIG config = {};
  config.avc420SurfaceMode = g_avc420SurfaceRouteArmed.load() ? TRUE : FALSE;
  config.avc444GpuCompositor =
      g_avc444GpuCompositorConfigured.load() ? TRUE : FALSE;
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.decoderSurfaceTarget != nullptr) {
    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    config.surfaceTargetWidth = target.width;
    config.surfaceTargetHeight = target.height;
  }
  config.log = OhosRdpgfxLogCallback;
  config.avc420SurfaceCommand = OhosRdpgfxAvc420SurfaceCommandCallback;
  config.avc444SurfaceCommand = OhosRdpgfxAvc444SurfaceCommandCallback;
  config.avc444EndFrame = OhosRdpgfxAvc444EndFrameCallback;
  config.avc444OutputState = OhosRdpgfxAvc444OutputStateCallback;

  std::array<char, 256> message{};
  if (api.ohosRdpgfxBridgeAttach == nullptr ||
      !api.ohosRdpgfxBridgeAttach(bridge, gfx, &config, message.data(),
                                  message.size())) {
    LogThroughCallbacks("OHOS rdpgfx bridge attach failed: " +
                        std::string(message[0] == '\0' ? "symbol unavailable"
                                                       : message.data()));
    return;
  }
  if (api.ohosRdpgfxBridgeSetGdiAttached != nullptr) {
    api.ohosRdpgfxBridgeSetGdiAttached(bridge, TRUE);
  }
}

void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext *gfx) {
  if (gfx == nullptr) {
    return;
  }

  ResetAvc444GpuOutputOwner("rdpgfx diagnostics hook restore");

  FreerdpRuntimeApi &api = SharedFreerdpRuntimeApi();
  freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
  if (bridge != nullptr && api.ohosRdpgfxBridgeDetach != nullptr) {
    api.ohosRdpgfxBridgeDetach(bridge, gfx);
  } else if (bridge != nullptr &&
             api.ohosRdpgfxBridgeSetGdiAttached != nullptr) {
    api.ohosRdpgfxBridgeSetGdiAttached(bridge, FALSE);
  }
}

} // namespace rdp_bridge
