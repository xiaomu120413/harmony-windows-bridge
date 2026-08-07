#pragma once

#include <cstdint>

namespace rdp_bridge {

constexpr uint32_t kRemoteContentOneToOneTolerancePx = 16U;

struct RemoteContentGeometry {
    bool valid = false;
    bool published = false;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
    uint32_t contentX = 0;
    uint32_t contentY = 0;
    uint32_t contentWidth = 0;
    uint32_t contentHeight = 0;
    uint32_t remoteWidth = 0;
    uint32_t remoteHeight = 0;
};

RemoteContentGeometry FitRemoteContentIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t remoteWidth, uint32_t remoteHeight);

RemoteContentGeometry ResolveRemoteContentGeometry(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t remoteWidth, uint32_t remoteHeight, uint32_t publishedX, uint32_t publishedY,
    uint32_t publishedWidth, uint32_t publishedHeight, uint32_t publishedRemoteWidth,
    uint32_t publishedRemoteHeight);

bool IsPointInsideRemoteContent(const RemoteContentGeometry& geometry, uint32_t x, uint32_t y);

} // namespace rdp_bridge
