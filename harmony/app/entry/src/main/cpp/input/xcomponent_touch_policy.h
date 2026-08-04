#pragma once

#include <cstdint>

namespace rdp_bridge {

constexpr float kNativeTouchDragThresholdPx = 18.0f;
constexpr float kNativeDoubleTapDistancePx = 32.0f;
constexpr uint64_t kNativeDoubleTapIntervalMs = 350;

bool IsNativeDoubleTap(uint64_t previousAtMs, float previousX, float previousY,
    uint64_t currentAtMs, float currentX, float currentY);

} // namespace rdp_bridge
