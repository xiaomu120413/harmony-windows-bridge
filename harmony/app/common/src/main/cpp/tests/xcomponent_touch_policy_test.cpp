#include "input/xcomponent_touch_policy.h"

#include <cassert>
#include <initializer_list>

using namespace rdp_bridge;

namespace {

void AssertTypes(const std::vector<NativeGestureAction>& actions,
    std::initializer_list<NativeGestureActionType> expected)
{
    assert(actions.size() == expected.size());
    size_t index = 0;
    for (const auto type : expected) {
        assert(actions[index++].type == type);
    }
}

void TestSystemTapTranslationAvoidsThirdClick()
{
    NativeSystemTapPolicy tap;
    const auto first = tap.SingleAccepted(100.0f, 120.0f);
    AssertTypes(first, {NativeGestureActionType::LeftClick});
    const auto second = tap.DoubleAccepted(108.0f, 126.0f);
    AssertTypes(second, {NativeGestureActionType::LeftClick});
    assert(second[0].x == 100.0f && second[0].y == 120.0f);

    const auto fallback = tap.DoubleAccepted(50.0f, 60.0f);
    AssertTypes(fallback, {NativeGestureActionType::LeftClick});
    assert(fallback[0].x == 50.0f && fallback[0].y == 60.0f);
    assert(BuildNativeClickActions(0, 1.0f, 2.0f).empty());
    AssertTypes(BuildNativeRightClickActions(3.0f, 4.0f),
        {NativeGestureActionType::RightClick});
}

void TestSystemPanButtonPairing()
{
    NativeSystemPanPolicy pan;
    AssertTypes(pan.Accept(10.0f, 20.0f, 15.0f, 24.0f),
        {NativeGestureActionType::Move, NativeGestureActionType::LeftDown,
            NativeGestureActionType::Move});
    const auto update = pan.Update(30.0f, 40.0f);
    AssertTypes(update, {NativeGestureActionType::Move});
    assert(update[0].held);
    AssertTypes(pan.End(35.0f, 45.0f),
        {NativeGestureActionType::Move, NativeGestureActionType::LeftUp});
    assert(pan.End(35.0f, 45.0f).empty());

    pan.Accept(1.0f, 2.0f, 1.0f, 2.0f);
    AssertTypes(pan.Cancel(), {NativeGestureActionType::LeftUp});
    assert(pan.Cancel().empty());
}

void TestSystemTwoFingerScrollUsesIncrementalOffsets()
{
    NativeSystemScrollPolicy scroll;
    scroll.Begin(12.0f, 12.0f, 24.0f);
    assert(scroll.Update(100.0f, 100.0f, 20.0f, 30.0f, 24.0f).empty());
    const auto vertical = scroll.Update(100.0f, 110.0f, 20.0f, 38.0f, 24.0f);
    AssertTypes(vertical, {NativeGestureActionType::WheelVertical});
    assert(vertical[0].steps == 1);
    const auto both = scroll.Update(120.0f, 130.0f, 68.0f, 86.0f, 24.0f);
    AssertTypes(both,
        {NativeGestureActionType::WheelVertical, NativeGestureActionType::WheelHorizontal});
    assert(both[0].steps == 2 && both[1].steps == 2);
    scroll.End();
    assert(scroll.Update(0.0f, 0.0f, 100.0f, 100.0f, 24.0f).empty());
}

void TestAxisSequenceAndSourceIsolation()
{
    NativeAxisScrollPolicy axis;
    axis.Begin(4, 24.0);
    NativeAxisSteps steps = axis.Update(4, 24.0, 5.0, 10.0);
    assert(steps.horizontal == 0 && steps.vertical == 0);
    steps = axis.Update(4, 24.0, 43.0, 40.0);
    assert(steps.horizontal == 2 && steps.vertical == 2);
    steps = axis.Update(4, 24.0, -24.0, -26.0);
    assert(steps.horizontal == -1 && steps.vertical == -1);
    axis.End();
    steps = axis.Update(1, 15.0, 15.0, 30.0);
    assert(steps.horizontal == 1 && steps.vertical == 2);
}

} // namespace

int main()
{
    TestSystemTapTranslationAvoidsThirdClick();
    TestSystemPanButtonPairing();
    TestSystemTwoFingerScrollUsesIncrementalOffsets();
    TestAxisSequenceAndSourceIsolation();
    return 0;
}
