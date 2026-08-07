#pragma once

#include <string>

namespace rdp_bridge {

enum class RenderOutputOwner {
    Gdi = 0,
    Avc444Gpu = 1,
    Avc420Gpu = 2,
};

enum class RenderOutputOwnerTransitionReason {
    SessionReset,
    SurfaceDestroyed,
    Avc444Reset,
    Avc444Takeover,
    Avc420ConfigureReset,
    Avc420FreeRdpPolicyRelease,
    Avc420FatalFallback,
    Avc420BootstrapFallback,
    Avc420Takeover,
};

struct RenderOutputOwnerTransition {
    RenderOutputOwner previous = RenderOutputOwner::Gdi;
    RenderOutputOwnerTransitionReason reason = RenderOutputOwnerTransitionReason::SessionReset;
};

RenderOutputOwner CurrentRenderOutputOwner();
RenderOutputOwnerTransition TransitionRenderOutputOwner(
    RenderOutputOwner owner, RenderOutputOwnerTransitionReason reason);
bool IsAvc444GpuRenderOutputOwner();
bool IsAvc420GpuRenderOutputOwner();
std::string RenderOutputOwnerName(RenderOutputOwner owner);
std::string RenderOutputOwnerTransitionReasonName(RenderOutputOwnerTransitionReason reason);
std::string CurrentRenderOutputOwnerName();

} // namespace rdp_bridge
