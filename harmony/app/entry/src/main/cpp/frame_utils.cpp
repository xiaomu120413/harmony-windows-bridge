#include "frame_utils.h"

#include <algorithm>
#include <limits>
#include <sstream>

namespace rdp_bridge {

std::string DescribeDirtyStats(const DirtyFrameStats& dirty)
{
    if (!dirty.valid) {
        return "dirty=none";
    }

    std::ostringstream out;
    out << "dirtyRects=" << dirty.rectCount
        << " dirtyBox=" << dirty.x << "," << dirty.y << " "
        << dirty.width << "x" << dirty.height
        << " dirtyArea=" << (dirty.areaPermille / 10) << "."
        << (dirty.areaPermille % 10) << "%";
    return out.str();
}

DirtyFrameStats MergeDirtyStats(const DirtyFrameStats& first, const DirtyFrameStats& second,
    uint32_t frameWidth, uint32_t frameHeight)
{
    if (!first.valid) {
        return second;
    }
    if (!second.valid) {
        return first;
    }
    if (frameWidth == 0 || frameHeight == 0) {
        return DirtyFrameStats{};
    }

    DirtyFrameStats merged;
    const uint64_t firstRight = static_cast<uint64_t>(first.x) + first.width;
    const uint64_t firstBottom = static_cast<uint64_t>(first.y) + first.height;
    const uint64_t secondRight = static_cast<uint64_t>(second.x) + second.width;
    const uint64_t secondBottom = static_cast<uint64_t>(second.y) + second.height;
    const uint32_t right = static_cast<uint32_t>(
        std::min<uint64_t>(frameWidth, std::max(firstRight, secondRight)));
    const uint32_t bottom = static_cast<uint32_t>(
        std::min<uint64_t>(frameHeight, std::max(firstBottom, secondBottom)));

    merged.valid = true;
    merged.rectCount = first.rectCount + second.rectCount;
    if (merged.rectCount < first.rectCount) {
        merged.rectCount = UINT32_MAX;
    }
    merged.x = std::min(first.x, second.x);
    merged.y = std::min(first.y, second.y);
    merged.width = right > merged.x ? right - merged.x : 0;
    merged.height = bottom > merged.y ? bottom - merged.y : 0;
    const uint64_t frameArea = static_cast<uint64_t>(frameWidth) * frameHeight;
    const uint64_t dirtyArea = static_cast<uint64_t>(merged.width) * merged.height;
    merged.areaPermille = frameArea == 0 ? 0 :
        static_cast<uint32_t>((std::min(dirtyArea, frameArea) * 1000U + frameArea / 2U) / frameArea);
    return merged;
}

} // namespace rdp_bridge
