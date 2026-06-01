#include "channels/rdpgfx_pipeline.h"

#include "channels/rdpgfx_diagnostics.h"
#include "common/string_utils.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/freerdp_runtime.h"
#include "surface/avc444_gpu_compositor.h"
#include "surface/render_output_owner.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>

namespace rdp_bridge {
namespace {

std::atomic_bool g_avc444GpuCompositorConfigured{false};
std::atomic<uint64_t> g_avc444EndFrameCallbackCount{0};
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

std::mutex g_ohosRdpgfxBridgeMutex;
freerdpOhosRdpgfxBridge *g_ohosRdpgfxBridge = nullptr;

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

void PrewarmAvc444GpuRendererIfReady(const std::string &reason,
                                     uint32_t fallbackWidth,
                                     uint32_t fallbackHeight) {
  if (!g_avc444GpuCompositorConfigured.load()) {
    return;
  }

  uint32_t width = RdpDesktopWidth();
  uint32_t height = RdpDesktopHeight();
  if (width == 0 || height == 0) {
    width = fallbackWidth;
    height = fallbackHeight;
  }
  if (width == 0 || height == 0) {
    return;
  }
  SharedAvc444GpuCompositor().Prewarm(width, height);
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

BOOL OhosRdpgfxAvc444SurfaceCommandCallback(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO *command, void *) {
  return SharedAvc444GpuCompositor().OnSurfaceCommand(command) ? TRUE : FALSE;
}

BOOL OhosRdpgfxAvc444EndFrameCallback(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO *frame, void *) {
  ++g_avc444EndFrameCallbackCount;
  const bool handled = SharedAvc444GpuCompositor().OnEndFrame(frame);
  return handled ? TRUE : FALSE;
}

} // namespace

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks) {
  std::lock_guard<std::mutex> lock(g_callbacksMutex);
  g_callbacks = std::move(callbacks);
}

bool IsAvc420SurfaceOutputEnabled() {
  return false;
}

void UpdateAvc420SurfaceOutputIfActive(const std::string &reason) {
  (void)reason;
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
  PrewarmAvc444GpuRendererIfReady("surface target update", target.width,
                                  target.height);
}

void ResetAvcSurfaceOutput(FreerdpRuntimeApi &api) {
  (void)api;
  g_avc444GpuCompositorConfigured.store(false);
  ResetAvc444GpuOutputOwner("AVC surface output reset");
  SharedAvc444GpuCompositor().Reset();
}

bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi &api,
                                  const GraphicsPipelineConfig &graphicsConfig,
                                  const FreerdpLogFn &log, std::string &error) {
  (void)error;
  if (!graphicsConfig.enabled || !graphicsConfig.h264) {
    ResetAvcSurfaceOutput(api);
    return true;
  }

  ResetAvcSurfaceOutput(api);
  if (log != nullptr) {
    log("OHOS AVC420 direct surface output disabled; using AVCodec buffer mode");
  }
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
  avc444Callbacks.stopRenderPipeline = callbacks.stopRenderPipeline;
  avc444Callbacks.startRenderPipeline = callbacks.startRenderPipeline;
  avc444Callbacks.releaseRenderTarget = callbacks.releaseRenderTarget;
  SharedAvc444GpuCompositor().Configure(g_avc444GpuCompositorConfigured.load(),
                                        log, std::move(avc444Callbacks));
  ResetRdpgfxDiagnosticsStats();

  if (!graphicsConfig.enabled) {
    freerdpOhosRdpgfxBridge *bridge = CurrentOhosRdpgfxBridge();
    if (bridge != nullptr && api.ohosRdpgfxBridgeReset != nullptr) {
      api.ohosRdpgfxBridgeReset(bridge, FALSE, FALSE);
    }
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
  (void)api;
  return "";
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
  config.avc444GpuCompositor =
      g_avc444GpuCompositorConfigured.load() ? TRUE : FALSE;
  RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
  if (callbacks.decoderSurfaceTarget != nullptr) {
    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    config.surfaceTargetWidth = target.width;
    config.surfaceTargetHeight = target.height;
  }
  config.log = OhosRdpgfxLogCallback;
  config.avc444SurfaceCommand = OhosRdpgfxAvc444SurfaceCommandCallback;
  config.avc444EndFrame = OhosRdpgfxAvc444EndFrameCallback;

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
  PrewarmAvc444GpuRendererIfReady("rdpgfx bridge attach",
                                  config.surfaceTargetWidth,
                                  config.surfaceTargetHeight);
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
