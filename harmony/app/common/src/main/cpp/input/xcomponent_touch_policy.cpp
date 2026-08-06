#include "input/xcomponent_touch_policy.h"

#include <cmath>

namespace rdp_bridge {
namespace {

int32_t ConsumeSteps(double& remainder, double quantum)
{
    if (quantum <= 0.0) {
        return 0;
    }
    const int32_t steps = static_cast<int32_t>(remainder / quantum);
    remainder -= static_cast<double>(steps) * quantum;
    return steps;
}

NativeGestureAction Action(NativeGestureActionType type, float x, float y,
    int32_t steps = 0, bool held = false)
{
    return NativeGestureAction {type, x, y, steps, held};
}

} // namespace

void NativeAxisScrollPolicy::Begin(int32_t source, double quantum)
{
    source_ = source;
    quantum_ = std::isfinite(quantum) && quantum > 0.0 ? quantum : 1.0;
    remainderX_ = 0.0;
    remainderY_ = 0.0;
}

NativeAxisSteps NativeAxisScrollPolicy::Update(
    int32_t source, double quantum, double deltaX, double deltaY)
{
    const double safeQuantum = std::isfinite(quantum) && quantum > 0.0 ? quantum : 1.0;
    if (source_ != source || std::fabs(quantum_ - safeQuantum) > 0.001) {
        Begin(source, safeQuantum);
    }
    remainderX_ += deltaX;
    remainderY_ += deltaY;
    return NativeAxisSteps {
        ConsumeSteps(remainderX_, quantum_),
        ConsumeSteps(remainderY_, quantum_),
    };
}

void NativeAxisScrollPolicy::End()
{
    source_ = -1;
    remainderX_ = 0.0;
    remainderY_ = 0.0;
}

std::vector<NativeGestureAction> BuildNativeClickActions(
    uint32_t clickCount, float x, float y)
{
    std::vector<NativeGestureAction> actions;
    actions.reserve(clickCount);
    for (uint32_t i = 0; i < clickCount; ++i) {
        actions.push_back(Action(NativeGestureActionType::LeftClick, x, y));
    }
    return actions;
}

std::vector<NativeGestureAction> BuildNativeRightClickActions(float x, float y)
{
    return {Action(NativeGestureActionType::RightClick, x, y)};
}

std::vector<NativeGestureAction> NativeSystemTapPolicy::SingleAccepted(float x, float y)
{
    firstAccepted_ = true;
    firstX_ = x;
    firstY_ = y;
    return BuildNativeClickActions(1, x, y);
}

std::vector<NativeGestureAction> NativeSystemTapPolicy::DoubleAccepted(float x, float y)
{
    const float clickX = firstAccepted_ ? firstX_ : x;
    const float clickY = firstAccepted_ ? firstY_ : y;
    firstAccepted_ = false;
    return BuildNativeClickActions(1, clickX, clickY);
}

void NativeSystemTapPolicy::Cancel()
{
    firstAccepted_ = false;
}

std::vector<NativeGestureAction> NativeSystemPanPolicy::Accept(
    float startX, float startY, float currentX, float currentY)
{
    active_ = true;
    lastX_ = currentX;
    lastY_ = currentY;
    std::vector<NativeGestureAction> actions {
        Action(NativeGestureActionType::Move, startX, startY),
        Action(NativeGestureActionType::LeftDown, startX, startY),
    };
    if (currentX != startX || currentY != startY) {
        actions.push_back(Action(NativeGestureActionType::Move,
            currentX, currentY, 0, true));
    }
    return actions;
}

std::vector<NativeGestureAction> NativeSystemPanPolicy::Update(float x, float y)
{
    if (!active_) {
        return {};
    }
    lastX_ = x;
    lastY_ = y;
    return {Action(NativeGestureActionType::Move, x, y, 0, true)};
}

std::vector<NativeGestureAction> NativeSystemPanPolicy::End(float x, float y)
{
    if (!active_) {
        return {};
    }
    active_ = false;
    lastX_ = x;
    lastY_ = y;
    return {
        Action(NativeGestureActionType::Move, x, y, 0, true),
        Action(NativeGestureActionType::LeftUp, x, y, 0, true),
    };
}

std::vector<NativeGestureAction> NativeSystemPanPolicy::Cancel()
{
    if (!active_) {
        return {};
    }
    active_ = false;
    return {Action(NativeGestureActionType::LeftUp, lastX_, lastY_, 0, true)};
}

void NativeSystemScrollPolicy::Begin(float offsetX, float offsetY, float quantumPx)
{
    active_ = true;
    lastOffsetX_ = offsetX;
    lastOffsetY_ = offsetY;
    steps_.Begin(2, quantumPx);
}

std::vector<NativeGestureAction> NativeSystemScrollPolicy::Update(
    float x, float y, float offsetX, float offsetY, float quantumPx)
{
    if (!active_) {
        Begin(offsetX, offsetY, quantumPx);
        return {};
    }
    const NativeAxisSteps delta = steps_.Update(2, quantumPx,
        offsetX - lastOffsetX_, offsetY - lastOffsetY_);
    lastOffsetX_ = offsetX;
    lastOffsetY_ = offsetY;
    std::vector<NativeGestureAction> actions;
    if (delta.vertical != 0) {
        actions.push_back(Action(NativeGestureActionType::WheelVertical,
            x, y, delta.vertical));
    }
    if (delta.horizontal != 0) {
        actions.push_back(Action(NativeGestureActionType::WheelHorizontal,
            x, y, delta.horizontal));
    }
    return actions;
}

void NativeSystemScrollPolicy::End()
{
    active_ = false;
    lastOffsetX_ = 0.0f;
    lastOffsetY_ = 0.0f;
    steps_.End();
}

} // namespace rdp_bridge
