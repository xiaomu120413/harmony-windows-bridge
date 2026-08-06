#include "surface/render_output_owner.h"

#include <atomic>

namespace rdp_bridge {
namespace {

std::atomic<int> g_renderOutputOwner{static_cast<int>(RenderOutputOwner::Gdi)};

} // namespace

RenderOutputOwner CurrentRenderOutputOwner()
{
    return static_cast<RenderOutputOwner>(g_renderOutputOwner.load());
}

RenderOutputOwnerTransition TransitionRenderOutputOwner(
    RenderOutputOwner owner, RenderOutputOwnerTransitionReason reason)
{
    const RenderOutputOwner previous = static_cast<RenderOutputOwner>(
        g_renderOutputOwner.exchange(static_cast<int>(owner)));
    return {previous, reason};
}

bool IsAvc444GpuRenderOutputOwner()
{
    return CurrentRenderOutputOwner() == RenderOutputOwner::Avc444Gpu;
}

bool IsAvc420GpuRenderOutputOwner()
{
    return CurrentRenderOutputOwner() == RenderOutputOwner::Avc420Gpu;
}

std::string RenderOutputOwnerName(RenderOutputOwner owner)
{
    switch (owner) {
        case RenderOutputOwner::Avc444Gpu:
            return "avc444-gpu";
        case RenderOutputOwner::Avc420Gpu:
            return "avc420-gpu";
        case RenderOutputOwner::Gdi:
        default:
            return "gdi";
    }
}

std::string RenderOutputOwnerTransitionReasonName(RenderOutputOwnerTransitionReason reason)
{
    switch (reason) {
        case RenderOutputOwnerTransitionReason::SurfaceDestroyed:
            return "surface destroyed";
        case RenderOutputOwnerTransitionReason::Avc444Reset:
            return "AVC444 reset";
        case RenderOutputOwnerTransitionReason::Avc444Takeover:
            return "AVC444 takeover";
        case RenderOutputOwnerTransitionReason::Avc420ConfigureReset:
            return "AVC420 configure reset";
        case RenderOutputOwnerTransitionReason::Avc420FreeRdpPolicyRelease:
            return "AVC420 FreeRDP policy release";
        case RenderOutputOwnerTransitionReason::Avc420FatalFallback:
            return "AVC420 fatal fallback";
        case RenderOutputOwnerTransitionReason::Avc420BootstrapFallback:
            return "AVC420 bootstrap fallback";
        case RenderOutputOwnerTransitionReason::Avc420Takeover:
            return "AVC420 takeover";
        case RenderOutputOwnerTransitionReason::SessionReset:
        default:
            return "session reset";
    }
}

std::string CurrentRenderOutputOwnerName()
{
    return RenderOutputOwnerName(CurrentRenderOutputOwner());
}

} // namespace rdp_bridge
