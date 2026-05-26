#pragma once

#include "common/bridge_types.h"

#include <functional>
#include <string>

#include <freerdp/freerdp.h>
#include <freerdp/update.h>

namespace rdp_bridge {

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

} // namespace rdp_bridge
