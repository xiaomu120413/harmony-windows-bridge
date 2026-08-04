#include "input/xcomponent_touch_policy.h"

#include <cmath>

namespace rdp_bridge {

bool IsNativeDoubleTap(uint64_t previousAtMs, float previousX, float previousY,
    uint64_t currentAtMs, float currentX, float currentY)
{
    if (previousAtMs == 0 || currentAtMs < previousAtMs ||
        currentAtMs - previousAtMs > kNativeDoubleTapIntervalMs) {
        return false;
    }
    const float deltaX = currentX - previousX;
    const float deltaY = currentY - previousY;
    return std::sqrt((deltaX * deltaX) + (deltaY * deltaY)) <= kNativeDoubleTapDistancePx;
}

} // namespace rdp_bridge
