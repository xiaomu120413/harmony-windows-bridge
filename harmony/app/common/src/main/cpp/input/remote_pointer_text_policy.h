#pragma once

#include <cstddef>
#include <cstdint>

namespace rdp_bridge {

struct RemotePointerTextMetrics {
    uint32_t boundsWidth = 0;
    uint32_t boundsHeight = 0;
    uint32_t opaquePixels = 0;
    uint32_t verticalCoverage = 0;
    uint32_t topArmWidth = 0;
    uint32_t bottomArmWidth = 0;
};

bool IsRemoteTextPointerCandidate(const uint8_t* rgba, uint32_t width, uint32_t height,
    uint32_t stride, uint32_t hotspotX, uint32_t hotspotY,
    RemotePointerTextMetrics* metrics = nullptr);

} // namespace rdp_bridge
