#pragma once

#include "bridge_types.h"

#include <functional>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include "freerdp_runtime.h"

#include <freerdp/client/rdpgfx.h>
#include <freerdp/settings.h>
#endif

namespace rdp_bridge {

using FreerdpLogFn = std::function<void(const std::string&)>;

struct RdpgfxPipelineCallbacks {
    std::function<DecoderSurfaceTarget()> decoderSurfaceTarget;
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::function<bool(FreerdpRuntimeApi&, uint32_t, uint32_t, const FreerdpLogFn&)>
        registerAvc444DecodeSurfaces;
#endif
    std::function<void()> startRenderPipeline;
    std::function<void()> stopRenderPipeline;
    std::function<void(const std::string&)> releaseRenderTarget;
    std::function<void(const std::string&)> log;
};

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks);
bool IsAvc420SurfaceOutputEnabled();
void UpdateAvc420SurfaceOutputIfActive(const std::string& reason);

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void ResetAvcSurfaceOutput(FreerdpRuntimeApi& api);
bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const FreerdpLogFn& log, std::string& error);
bool ConfigureGraphicsPipelineChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const FreerdpLogFn& log, std::string& error);
std::string OhosRdpgfxBridgeDiagnostics(FreerdpRuntimeApi& api);
void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx);
void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx);
#endif

} // namespace rdp_bridge
