#include "input/xcomponent_touch_policy.h"

#include <algorithm>
#include <cmath>

namespace rdp_bridge {
namespace {

float Distance(float x1, float y1, float x2, float y2)
{
    return std::hypot(x1 - x2, y1 - y2);
}

int32_t ConsumeSteps(float& remainder, float quantum)
{
    if (quantum <= 0.0f) {
        return 0;
    }
    const int32_t steps = static_cast<int32_t>(remainder / quantum);
    remainder -= static_cast<float>(steps) * quantum;
    return steps;
}

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

NativeTouchThresholds NativeTouchThresholdsForDensity(float density)
{
    const float safeDensity = std::isfinite(density) && density > 0.0f ? density : 1.0f;
    return NativeTouchThresholds {
        kNativeDoubleTapDistanceVp * safeDensity,
        kNativeTouchDragThresholdVp * safeDensity,
        kNativeTouchWheelQuantumVp * safeDensity,
    };
}

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

NativeTouchGesturePolicy::NativeTouchGesturePolicy(NativeTouchThresholds thresholds)
    : thresholds_(thresholds)
{
}

const NativeTouchPoint* NativeTouchGesturePolicy::FindPoint(
    const NativeTouchSample& sample, int32_t id) const
{
    const auto it = std::find_if(sample.points.begin(), sample.points.end(),
        [id](const NativeTouchPoint& point) { return point.id == id; });
    return it == sample.points.end() ? nullptr : &*it;
}

void NativeTouchGesturePolicy::ResetActive()
{
    state_ = State::Idle;
    primaryId_ = -1;
    secondaryId_ = -1;
    downAtMs_ = 0;
    remoteLeftDown_ = false;
    scrollRemainderX_ = 0.0f;
    scrollRemainderY_ = 0.0f;
}

std::vector<NativeGestureAction> NativeTouchGesturePolicy::Cancel()
{
    std::vector<NativeGestureAction> actions;
    if (remoteLeftDown_) {
        actions.push_back(Action(NativeGestureActionType::LeftUp, lastX_, lastY_, 0, true));
    }
    ResetActive();
    lastTapValid_ = false;
    return actions;
}

void NativeTouchGesturePolicy::SetThresholds(NativeTouchThresholds thresholds)
{
    thresholds_ = thresholds;
}

bool NativeTouchGesturePolicy::IsLongPressPending() const
{
    return state_ == State::Pressed;
}

uint64_t NativeTouchGesturePolicy::LongPressDeadlineMs() const
{
    return IsLongPressPending() ? downAtMs_ + kNativeLongPressIntervalMs : 0;
}

std::vector<NativeGestureAction> NativeTouchGesturePolicy::HandleLongPressTimeout(uint64_t atMs)
{
    if (state_ != State::Pressed || atMs < downAtMs_ + kNativeLongPressIntervalMs ||
        Distance(startX_, startY_, lastX_, lastY_) >= thresholds_.dragDistancePx) {
        return {};
    }
    state_ = State::LongPressRecognized;
    lastTapValid_ = false;
    return {Action(NativeGestureActionType::RightClick, startX_, startY_)};
}

std::vector<NativeGestureAction> NativeTouchGesturePolicy::Handle(const NativeTouchSample& sample)
{
    std::vector<NativeGestureAction> actions;
    if (sample.action == NativeTouchAction::Cancel) {
        return Cancel();
    }

    if (sample.points.size() >= 2) {
        const NativeTouchPoint& first = sample.points[0];
        const NativeTouchPoint& second = sample.points[1];
        if (state_ != State::Scrolling) {
            if (remoteLeftDown_) {
                actions.push_back(Action(NativeGestureActionType::LeftUp, lastX_, lastY_, 0, true));
            }
            state_ = State::Scrolling;
            remoteLeftDown_ = false;
            primaryId_ = first.id;
            secondaryId_ = second.id;
            scrollLastX_ = (first.x + second.x) * 0.5f;
            scrollLastY_ = (first.y + second.y) * 0.5f;
            lastX_ = scrollLastX_;
            lastY_ = scrollLastY_;
            scrollRemainderX_ = 0.0f;
            scrollRemainderY_ = 0.0f;
            lastTapValid_ = false;
            return actions;
        }

        const NativeTouchPoint* primary = FindPoint(sample, primaryId_);
        const NativeTouchPoint* secondary = FindPoint(sample, secondaryId_);
        if (primary == nullptr || secondary == nullptr) {
            return Cancel();
        }
        const float centerX = (primary->x + secondary->x) * 0.5f;
        const float centerY = (primary->y + secondary->y) * 0.5f;
        if (sample.action == NativeTouchAction::Move) {
            scrollRemainderX_ += centerX - scrollLastX_;
            scrollRemainderY_ += centerY - scrollLastY_;
            const int32_t horizontal = ConsumeSteps(scrollRemainderX_, thresholds_.wheelQuantumPx);
            const int32_t vertical = ConsumeSteps(scrollRemainderY_, thresholds_.wheelQuantumPx);
            if (vertical != 0) {
                actions.push_back(Action(NativeGestureActionType::WheelVertical,
                    centerX, centerY, vertical));
            }
            if (horizontal != 0) {
                actions.push_back(Action(NativeGestureActionType::WheelHorizontal,
                    centerX, centerY, horizontal));
            }
        }
        scrollLastX_ = centerX;
        scrollLastY_ = centerY;
        lastX_ = centerX;
        lastY_ = centerY;
        return actions;
    }

    if (state_ == State::Scrolling) {
        ResetActive();
        return actions;
    }
    if (sample.points.empty()) {
        if (sample.action == NativeTouchAction::Up) {
            return Cancel();
        }
        return actions;
    }

    const NativeTouchPoint& point = sample.points[0];
    if (sample.action == NativeTouchAction::Down) {
        if (state_ != State::Idle) {
            actions = Cancel();
        }
        const bool doubleTap = lastTapValid_ && sample.atMs >= lastTapAtMs_ &&
            sample.atMs - lastTapAtMs_ <= kNativeDoubleTapIntervalMs &&
            Distance(lastTapX_, lastTapY_, point.x, point.y) <= thresholds_.doubleTapDistancePx;
        state_ = doubleTap ? State::DoubleSecondDown : State::Pressed;
        primaryId_ = point.id;
        downAtMs_ = sample.atMs;
        startX_ = lastX_ = point.x;
        startY_ = lastY_ = point.y;
        if (doubleTap) {
            lastTapValid_ = false;
            remoteLeftDown_ = true;
            actions.push_back(Action(NativeGestureActionType::Move, point.x, point.y));
            actions.push_back(Action(NativeGestureActionType::LeftDown, point.x, point.y));
        }
        return actions;
    }

    if (point.id != primaryId_ || state_ == State::Idle) {
        return actions;
    }
    lastX_ = point.x;
    lastY_ = point.y;

    if (sample.action == NativeTouchAction::Move) {
        if (state_ == State::Pressed &&
            Distance(startX_, startY_, point.x, point.y) >= thresholds_.dragDistancePx) {
            state_ = State::Dragging;
            lastTapValid_ = false;
            remoteLeftDown_ = true;
            actions.push_back(Action(NativeGestureActionType::Move, startX_, startY_));
            actions.push_back(Action(NativeGestureActionType::LeftDown, startX_, startY_));
        }
        if (state_ == State::Dragging || state_ == State::DoubleSecondDown) {
            actions.push_back(Action(NativeGestureActionType::Move, point.x, point.y, 0, true));
        }
        return actions;
    }

    if (sample.action != NativeTouchAction::Up) {
        return actions;
    }
    if (state_ == State::Dragging || state_ == State::DoubleSecondDown) {
        actions.push_back(Action(NativeGestureActionType::Move, point.x, point.y, 0, true));
        actions.push_back(Action(NativeGestureActionType::LeftUp, point.x, point.y, 0, true));
        lastTapValid_ = false;
    } else if (state_ == State::Pressed) {
        if (sample.atMs >= downAtMs_ + kNativeLongPressIntervalMs) {
            actions.push_back(Action(NativeGestureActionType::RightClick, startX_, startY_));
            lastTapValid_ = false;
        } else {
            actions.push_back(Action(NativeGestureActionType::LeftClick, point.x, point.y));
            lastTapValid_ = true;
            lastTapAtMs_ = sample.atMs;
            lastTapX_ = point.x;
            lastTapY_ = point.y;
        }
    }
    ResetActive();
    return actions;
}

} // namespace rdp_bridge
