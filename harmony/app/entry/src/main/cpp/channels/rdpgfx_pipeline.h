#pragma once

#include "common/bridge_types.h"

#include <functional>
#include <string>

#include "freerdp/freerdp_runtime.h"

#include <freerdp/client/rdpgfx.h>
#include <freerdp/settings.h>

namespace rdp_bridge {

using FreerdpLogFn = std::function<void(const std::string&)>;

struct RdpgfxPipelineCallbacks {
    std::function<DecoderSurfaceTarget()> decoderSurfaceTarget;
    std::function<void()> startRenderPipeline;
    std::function<void()> stopRenderPipeline;
    std::function<void(const std::string&)> releaseRenderTarget;
    std::function<void(const std::string&)> log;
};

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks);
bool IsAvc420SurfaceOutputEnabled();
bool UpdateAvc420CompositeWithGdiFrame(const RgbaFrame& frame);
void UpdateAvc420SurfaceOutputIfActive(const std::string& reason);
void UpdateRdpgfxSurfaceTargetIfReady(const std::string& reason);

void ResetAvcSurfaceOutput(FreerdpRuntimeApi& api);
bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const FreerdpLogFn& log, std::string& error);
bool ConfigureGraphicsPipelineChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const FreerdpLogFn& log, std::string& error);
std::string OhosRdpgfxBridgeDiagnostics(FreerdpRuntimeApi& api);
std::string OhosAvc420RouteDiagnostics(FreerdpRuntimeApi& api);
void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx);
void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx);

} // namespace rdp_bridge
