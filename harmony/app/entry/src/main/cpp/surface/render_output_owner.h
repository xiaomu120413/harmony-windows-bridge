#pragma once

#include <string>

namespace rdp_bridge {

enum class RenderOutputOwner {
    Gdi = 0,
    Avc444Gpu = 1,
};

RenderOutputOwner CurrentRenderOutputOwner();
RenderOutputOwner ExchangeRenderOutputOwner(RenderOutputOwner owner);
bool IsAvc444GpuRenderOutputOwner();
std::string RenderOutputOwnerName(RenderOutputOwner owner);
std::string CurrentRenderOutputOwnerName();

} // namespace rdp_bridge
