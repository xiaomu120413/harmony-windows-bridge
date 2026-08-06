#include "input/remote_pointer_text_policy.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace rdp_bridge {

bool IsRemoteTextPointerCandidate(const uint8_t* rgba, uint32_t width, uint32_t height,
    uint32_t stride, uint32_t hotspotX, uint32_t hotspotY, RemotePointerTextMetrics* metrics)
{
    RemotePointerTextMetrics result;
    if (rgba == nullptr || width == 0 || height == 0 || stride < width * 4 ||
        width > 256 || height > 256) {
        if (metrics != nullptr) {
            *metrics = result;
        }
        return false;
    }

    uint32_t minX = width;
    uint32_t minY = height;
    uint32_t maxX = 0;
    uint32_t maxY = 0;
    for (uint32_t y = 0; y < height; ++y) {
        const uint8_t* row = rgba + static_cast<size_t>(y) * stride;
        for (uint32_t x = 0; x < width; ++x) {
            if (row[x * 4 + 3] == 0) {
                continue;
            }
            minX = std::min(minX, x);
            minY = std::min(minY, y);
            maxX = std::max(maxX, x);
            maxY = std::max(maxY, y);
            ++result.opaquePixels;
        }
    }
    if (result.opaquePixels == 0) {
        if (metrics != nullptr) {
            *metrics = result;
        }
        return false;
    }

    result.boundsWidth = maxX - minX + 1;
    result.boundsHeight = maxY - minY + 1;
    std::vector<uint32_t> rows(result.boundsHeight, 0);
    std::vector<uint32_t> columns(result.boundsWidth, 0);
    for (uint32_t y = minY; y <= maxY; ++y) {
        const uint8_t* row = rgba + static_cast<size_t>(y) * stride;
        for (uint32_t x = minX; x <= maxX; ++x) {
            if (row[x * 4 + 3] == 0) {
                continue;
            }
            ++rows[y - minY];
            ++columns[x - minX];
        }
    }

    const auto mainColumn = std::max_element(columns.begin(), columns.end());
    result.verticalCoverage = mainColumn == columns.end() ? 0 : *mainColumn;
    const uint32_t bandHeight = std::max(2U, result.boundsHeight / 5);
    for (uint32_t row = 0; row < bandHeight && row < rows.size(); ++row) {
        result.topArmWidth = std::max(result.topArmWidth, rows[row]);
        result.bottomArmWidth = std::max(result.bottomArmWidth, rows[rows.size() - 1 - row]);
    }

    const uint32_t mainColumnX = minX + static_cast<uint32_t>(mainColumn - columns.begin());
    const uint32_t centerY = minY + result.boundsHeight / 2;
    const uint64_t area = static_cast<uint64_t>(result.boundsWidth) * result.boundsHeight;
    const uint32_t minimumArm = std::max(3U, (result.boundsWidth * 3U) / 5U);
    const bool shape = result.boundsHeight >= 9 &&
        result.boundsHeight * 2U >= result.boundsWidth * 3U &&
        static_cast<uint64_t>(result.opaquePixels) * 2U <= area &&
        result.verticalCoverage * 5U >= result.boundsHeight * 3U &&
        result.topArmWidth >= minimumArm && result.bottomArmWidth >= minimumArm;
    const bool centeredHotspot = hotspotX + 3U >= mainColumnX && hotspotX <= mainColumnX + 3U &&
        hotspotY + std::max(3U, result.boundsHeight / 3U) >= centerY &&
        hotspotY <= centerY + std::max(3U, result.boundsHeight / 3U);

    if (metrics != nullptr) {
        *metrics = result;
    }
    return shape && centeredHotspot;
}

} // namespace rdp_bridge
