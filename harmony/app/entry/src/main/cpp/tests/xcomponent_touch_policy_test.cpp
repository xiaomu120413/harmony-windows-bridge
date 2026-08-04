#include "input/xcomponent_touch_policy.h"

#include <cassert>

int main()
{
    using namespace rdp_bridge;
    assert(IsNativeDoubleTap(1000, 100.0f, 100.0f, 1050, 100.0f, 100.0f));
    assert(IsNativeDoubleTap(1000, 100.0f, 100.0f, 1350, 132.0f, 100.0f));
    assert(!IsNativeDoubleTap(1000, 100.0f, 100.0f, 1351, 100.0f, 100.0f));
    assert(!IsNativeDoubleTap(1000, 100.0f, 100.0f, 1200, 132.1f, 100.0f));
    assert(!IsNativeDoubleTap(0, 100.0f, 100.0f, 1200, 100.0f, 100.0f));
    assert(kNativeTouchDragThresholdPx > 10.0f);
    return 0;
}
