#include "input/xcomponent_touch_policy.h"

#include <cassert>
#include <cmath>
#include <initializer_list>

using namespace rdp_bridge;

namespace {

NativeTouchSample Sample(NativeTouchAction action, uint64_t atMs,
    std::initializer_list<NativeTouchPoint> points)
{
    return NativeTouchSample {action, atMs, points};
}

void AssertTypes(const std::vector<NativeGestureAction>& actions,
    std::initializer_list<NativeGestureActionType> expected)
{
    assert(actions.size() == expected.size());
    size_t index = 0;
    for (const auto type : expected) {
        assert(actions[index++].type == type);
    }
}

void TestDensityAndDoubleTapBoundaries()
{
    const NativeTouchThresholds thresholds = NativeTouchThresholdsForDensity(2.0f);
    assert(std::fabs(thresholds.dragDistancePx - 10.0f) < 0.001f);
    assert(std::fabs(thresholds.doubleTapDistancePx - 120.0f) < 0.001f);
    NativeTouchGesturePolicy policy(thresholds);

    policy.Handle(Sample(NativeTouchAction::Down, 1000, {{1, 100, 100}}));
    AssertTypes(policy.Handle(Sample(NativeTouchAction::Up, 1020, {{1, 100, 100}})),
        {NativeGestureActionType::LeftClick});
    AssertTypes(policy.Handle(Sample(NativeTouchAction::Down, 1320, {{2, 220, 100}})),
        {NativeGestureActionType::Move, NativeGestureActionType::LeftDown});
    AssertTypes(policy.Handle(Sample(NativeTouchAction::Up, 1340, {{2, 220, 100}})),
        {NativeGestureActionType::Move, NativeGestureActionType::LeftUp});

    policy.Handle(Sample(NativeTouchAction::Down, 2000, {{3, 100, 100}}));
    policy.Handle(Sample(NativeTouchAction::Up, 2020, {{3, 100, 100}}));
    assert(policy.Handle(Sample(NativeTouchAction::Down, 2321, {{4, 100, 100}})).empty());
    policy.Cancel();

    policy.Handle(Sample(NativeTouchAction::Down, 3000, {{5, 100, 100}}));
    policy.Handle(Sample(NativeTouchAction::Up, 3020, {{5, 100, 100}}));
    assert(policy.Handle(Sample(NativeTouchAction::Down, 3100, {{6, 220.1f, 100}})).empty());
}

void TestLongPressWithoutMoveAndCancelIdempotence()
{
    NativeTouchGesturePolicy policy(NativeTouchThresholdsForDensity(1.0f));
    policy.Handle(Sample(NativeTouchAction::Down, 1000, {{7, 20, 30}}));
    assert(policy.HandleLongPressTimeout(1499).empty());
    AssertTypes(policy.HandleLongPressTimeout(1500), {NativeGestureActionType::RightClick});
    assert(policy.HandleLongPressTimeout(1600).empty());
    assert(policy.Handle(Sample(NativeTouchAction::Up, 1700, {{7, 20, 30}})).empty());
    assert(policy.Cancel().empty());

    policy.Handle(Sample(NativeTouchAction::Down, 2000, {{8, 10, 10}}));
    AssertTypes(policy.Handle(Sample(NativeTouchAction::Move, 2010, {{8, 15, 10}})),
        {NativeGestureActionType::Move, NativeGestureActionType::LeftDown,
            NativeGestureActionType::Move});
    AssertTypes(policy.Cancel(), {NativeGestureActionType::LeftUp});
    assert(policy.Cancel().empty());
}

void TestPointerIdentityAndScrollRemainder()
{
    NativeTouchGesturePolicy policy(NativeTouchThresholdsForDensity(2.0f));
    assert(policy.Handle(Sample(NativeTouchAction::Down, 100,
        {{10, 0, 0}, {20, 20, 0}})).empty());
    assert(policy.Handle(Sample(NativeTouchAction::Move, 110,
        {{20, 20, 20}, {10, 0, 20}})).empty());
    const auto vertical = policy.Handle(Sample(NativeTouchAction::Move, 120,
        {{10, 0, 30}, {20, 20, 30}}));
    AssertTypes(vertical, {NativeGestureActionType::WheelVertical});
    assert(vertical[0].steps == 1);
    const auto horizontal = policy.Handle(Sample(NativeTouchAction::Move, 130,
        {{20, 70, 30}, {10, 50, 30}}));
    AssertTypes(horizontal, {NativeGestureActionType::WheelHorizontal});
    assert(horizontal[0].steps == 2);
    assert(policy.Handle(Sample(NativeTouchAction::Up, 140, {{10, 50, 30}})).empty());
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
    steps = axis.Update(4, 24.0, 23.0, 23.0);
    assert(steps.horizontal == 0 && steps.vertical == 0);
    steps = axis.Update(1, 15.0, 15.0, 30.0);
    assert(steps.horizontal == 1 && steps.vertical == 2);
}

} // namespace

int main()
{
    TestDensityAndDoubleTapBoundaries();
    TestLongPressWithoutMoveAndCancelIdempotence();
    TestPointerIdentityAndScrollRemainder();
    TestAxisSequenceAndSourceIsolation();
    return 0;
}
