#pragma once

#include <string>

namespace rdp_bridge {

enum class RenderOutputOwner {
    Gdi = 0,
    Avc444Gpu = 1,
    Avc420Gpu = 2,
};

RenderOutputOwner CurrentRenderOutputOwner();
RenderOutputOwner ExchangeRenderOutputOwner(RenderOutputOwner owner);
bool IsAvc444GpuRenderOutputOwner();
bool IsAvc420GpuRenderOutputOwner();
std::string RenderOutputOwnerName(RenderOutputOwner owner);
std::string CurrentRenderOutputOwnerName();

} // namespace rdp_bridge
