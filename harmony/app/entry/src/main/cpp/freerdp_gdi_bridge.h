#pragma once

#include "bridge_types.h"

#include <functional>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/freerdp.h>
#include <freerdp/update.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
struct GdiBridgeCallbacks {
    std::function<bool()> isAvc420SurfaceOutputEnabled;
    std::function<bool(const RgbaFrame&, std::string&, bool)> queueFrame;
    std::function<void()> startRenderPipeline;
    std::function<void()> stopRenderPipeline;
    std::function<void(const std::string&)> log;
};

void SetGdiBridgeCallbacks(GdiBridgeCallbacks callbacks);
void SetRdpDesktopSize(uint32_t width, uint32_t height);
void ClearRdpDesktopSize();
uint32_t RdpDesktopWidth();
uint32_t RdpDesktopHeight();
bool RdpPrimaryFrameReady();

BOOL HarmonyBeginPaint(rdpContext* context);
BOOL HarmonyEndPaint(rdpContext* context);
BOOL HarmonyDesktopResize(rdpContext* context);
BOOL HarmonyPostConnect(freerdp* instance);
void HarmonyPostDisconnect(freerdp* instance);
#endif

} // namespace rdp_bridge
