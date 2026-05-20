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

RenderOutputOwner ExchangeRenderOutputOwner(RenderOutputOwner owner)
{
    return static_cast<RenderOutputOwner>(
        g_renderOutputOwner.exchange(static_cast<int>(owner)));
}

bool IsAvc444GpuRenderOutputOwner()
{
    return CurrentRenderOutputOwner() == RenderOutputOwner::Avc444Gpu;
}

std::string RenderOutputOwnerName(RenderOutputOwner owner)
{
    switch (owner) {
        case RenderOutputOwner::Avc444Gpu:
            return "avc444-gpu";
        case RenderOutputOwner::Gdi:
        default:
            return "gdi";
    }
}

std::string CurrentRenderOutputOwnerName()
{
    return RenderOutputOwnerName(CurrentRenderOutputOwner());
}

} // namespace rdp_bridge
