#pragma once

#include "common/bridge_types.h"

#include <cstdint>
#include <string>

namespace rdp_bridge {

std::string DescribeDirtyStats(const DirtyFrameStats& dirty);
DirtyFrameStats MergeDirtyStats(const DirtyFrameStats& first, const DirtyFrameStats& second,
    uint32_t frameWidth, uint32_t frameHeight);

} // namespace rdp_bridge
