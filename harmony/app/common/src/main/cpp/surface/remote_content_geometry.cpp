#include "surface/remote_content_geometry.h"

#include <algorithm>

namespace rdp_bridge {
namespace {

uint32_t DimensionDelta(uint32_t left, uint32_t right)
{
    return left > right ? left - right : right - left;
}

bool IsPublishedRectValid(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t remoteWidth, uint32_t remoteHeight, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height, uint32_t publishedRemoteWidth,
    uint32_t publishedRemoteHeight)
{
    if (width == 0 || height == 0 || publishedRemoteWidth != remoteWidth ||
        publishedRemoteHeight != remoteHeight || x >= targetWidth || y >= targetHeight) {
        return false;
    }
    return static_cast<uint64_t>(x) + width <= targetWidth &&
        static_cast<uint64_t>(y) + height <= targetHeight;
}

} // namespace

RemoteContentGeometry FitRemoteContentIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t remoteWidth, uint32_t remoteHeight)
{
    RemoteContentGeometry geometry;
    geometry.targetWidth = targetWidth;
    geometry.targetHeight = targetHeight;
    geometry.remoteWidth = remoteWidth;
    geometry.remoteHeight = remoteHeight;
    if (targetWidth == 0 || targetHeight == 0 || remoteWidth == 0 || remoteHeight == 0) {
        return geometry;
    }

    if (remoteWidth <= targetWidth && remoteHeight <= targetHeight &&
        DimensionDelta(targetWidth, remoteWidth) <= kRemoteContentOneToOneTolerancePx &&
        DimensionDelta(targetHeight, remoteHeight) <= kRemoteContentOneToOneTolerancePx) {
        geometry.contentWidth = remoteWidth;
        geometry.contentHeight = remoteHeight;
    } else {
        const uint64_t targetByRemoteHeight = static_cast<uint64_t>(targetWidth) * remoteHeight;
        const uint64_t targetHeightByRemoteWidth = static_cast<uint64_t>(targetHeight) * remoteWidth;
        if (targetByRemoteHeight <= targetHeightByRemoteWidth) {
            geometry.contentWidth = targetWidth;
            geometry.contentHeight = static_cast<uint32_t>(
                std::max<uint64_t>(1, targetByRemoteHeight / remoteWidth));
        } else {
            geometry.contentHeight = targetHeight;
            geometry.contentWidth = static_cast<uint32_t>(
                std::max<uint64_t>(1, targetHeightByRemoteWidth / remoteHeight));
        }
    }
    geometry.contentWidth = std::min(geometry.contentWidth, targetWidth);
    geometry.contentHeight = std::min(geometry.contentHeight, targetHeight);
    geometry.contentX = (targetWidth - geometry.contentWidth) / 2U;
    geometry.contentY = (targetHeight - geometry.contentHeight) / 2U;
    geometry.valid = geometry.contentWidth != 0 && geometry.contentHeight != 0;
    return geometry;
}

RemoteContentGeometry ResolveRemoteContentGeometry(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t remoteWidth, uint32_t remoteHeight, uint32_t publishedX, uint32_t publishedY,
    uint32_t publishedWidth, uint32_t publishedHeight, uint32_t publishedRemoteWidth,
    uint32_t publishedRemoteHeight)
{
    RemoteContentGeometry geometry = FitRemoteContentIntoTarget(
        targetWidth, targetHeight, remoteWidth, remoteHeight);
    if (!geometry.valid || !IsPublishedRectValid(targetWidth, targetHeight,
        remoteWidth, remoteHeight, publishedX, publishedY, publishedWidth, publishedHeight,
        publishedRemoteWidth, publishedRemoteHeight)) {
        return geometry;
    }
    geometry.published = true;
    geometry.contentX = publishedX;
    geometry.contentY = publishedY;
    geometry.contentWidth = publishedWidth;
    geometry.contentHeight = publishedHeight;
    return geometry;
}

bool IsPointInsideRemoteContent(const RemoteContentGeometry& geometry, uint32_t x, uint32_t y)
{
    if (!geometry.valid) {
        return false;
    }
    return x >= geometry.contentX && y >= geometry.contentY &&
        static_cast<uint64_t>(x) < static_cast<uint64_t>(geometry.contentX) + geometry.contentWidth &&
        static_cast<uint64_t>(y) < static_cast<uint64_t>(geometry.contentY) + geometry.contentHeight;
}

} // namespace rdp_bridge
