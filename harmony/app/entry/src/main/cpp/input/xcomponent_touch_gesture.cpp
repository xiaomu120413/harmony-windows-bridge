#include "input/xcomponent_input_internal.h"
#include "input/remote_pointer_text_detector.h"

#include <condition_variable>
#include <cstdlib>
#include <thread>

namespace rdp_bridge {
namespace {

class LongPressScheduler {
public:
    LongPressScheduler() : thread_([this]() { Run(); }) {}
    ~LongPressScheduler()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
            ++generation_;
        }
        condition_.notify_all();
        thread_.join();
    }

    void Schedule(uint64_t deadlineMs)
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            deadlineMs_ = deadlineMs;
            scheduled_ = deadlineMs != 0;
            ++generation_;
        }
        condition_.notify_all();
    }

    void Cancel()
    {
        Schedule(0);
    }

private:
    void Run();

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread thread_;
    uint64_t generation_ = 0;
    uint64_t deadlineMs_ = 0;
    bool scheduled_ = false;
    bool stopped_ = false;
};

LongPressScheduler& Scheduler()
{
    static LongPressScheduler scheduler;
    return scheduler;
}

bool SendTouchButton(uint32_t button, bool down, float x, float y, const std::string& label,
    bool allowClamp)
{
    return SendNativePointer(MakeNativePointer(
        down ? LocalPointerAction::ButtonDown : LocalPointerAction::ButtonUp,
        x, y, button, 0, allowClamp), label, true);
}

bool DispatchGestureActions(const std::vector<NativeGestureAction>& actions,
    const std::string& label)
{
    bool remoteLeftDownAccepted = true;
    for (const NativeGestureAction& action : actions) {
        switch (action.type) {
            case NativeGestureActionType::Move:
                if (remoteLeftDownAccepted) {
                    SendNativePointer(MakeNativePointer(LocalPointerAction::Move,
                        action.x, action.y,
                        action.held ? LocalPointerButtonLeft : LocalPointerButtonNone,
                        0, action.held), label + ".move", action.held);
                }
                break;
            case NativeGestureActionType::LeftDown:
                remoteLeftDownAccepted = SendTouchButton(LocalPointerButtonLeft, true,
                    action.x, action.y, label + ".leftDown", false);
                break;
            case NativeGestureActionType::LeftUp:
                if (remoteLeftDownAccepted) {
                    SendTouchButton(LocalPointerButtonLeft, false, action.x, action.y,
                        label + ".leftUp", true);
                }
                break;
            case NativeGestureActionType::LeftClick:
                SendNativePointer(MakeNativePointer(LocalPointerAction::Move,
                    action.x, action.y), label + ".leftClick.move", true);
                if (SendTouchButton(LocalPointerButtonLeft, true, action.x, action.y,
                    label + ".leftClick.down", false)) {
                    SendTouchButton(LocalPointerButtonLeft, false, action.x, action.y,
                        label + ".leftClick.up", true);
                }
                break;
            case NativeGestureActionType::RightClick:
                SendNativePointer(MakeNativePointer(LocalPointerAction::Move,
                    action.x, action.y), label + ".rightClick.move", true);
                if (SendTouchButton(LocalPointerButtonRight, true, action.x, action.y,
                    label + ".rightClick.down", false)) {
                    SendTouchButton(LocalPointerButtonRight, false, action.x, action.y,
                        label + ".rightClick.up", true);
                }
                break;
            case NativeGestureActionType::WheelVertical:
            case NativeGestureActionType::WheelHorizontal: {
                const LocalPointerAction wheel = action.type == NativeGestureActionType::WheelVertical
                    ? LocalPointerAction::WheelVertical : LocalPointerAction::WheelHorizontal;
                const int32_t direction = action.steps < 0 ? -1 : 1;
                for (int32_t i = 0; i < std::abs(action.steps); ++i) {
                    SendNativePointer(MakeNativePointer(wheel, action.x, action.y,
                        LocalPointerButtonNone, direction), label + ".wheel");
                }
                break;
            }
        }
    }
    return remoteLeftDownAccepted;
}

void LongPressScheduler::Run()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopped_) {
        condition_.wait(lock, [this]() { return stopped_ || scheduled_; });
        if (stopped_) {
            break;
        }
        const uint64_t generation = generation_;
        const uint64_t deadline = deadlineMs_;
        const uint64_t now = NowMs();
        if (deadline > now) {
            condition_.wait_for(lock, std::chrono::milliseconds(deadline - now),
                [this, generation]() { return stopped_ || generation_ != generation; });
        }
        if (stopped_ || generation_ != generation || !scheduled_) {
            continue;
        }
        scheduled_ = false;
        lock.unlock();
        {
            std::lock_guard<std::mutex> touchLock(g_nativeTouchMutex);
            DispatchGestureActions(g_nativeTouchPolicy.HandleLongPressTimeout(NowMs()),
                "touch.longPress.timeout");
        }
        lock.lock();
    }
}

NativeTouchAction ResolveTouchAction(OH_NativeXComponent_TouchEventType type)
{
    switch (type) {
        case OH_NATIVEXCOMPONENT_DOWN:
            return NativeTouchAction::Down;
        case OH_NATIVEXCOMPONENT_MOVE:
            return NativeTouchAction::Move;
        case OH_NATIVEXCOMPONENT_UP:
            return NativeTouchAction::Up;
        case OH_NATIVEXCOMPONENT_CANCEL:
        default:
            return NativeTouchAction::Cancel;
    }
}

OH_NativeXComponent_EventSourceType ResolveTouchSource(
    OH_NativeXComponent* component, const OH_NativeXComponent_TouchEvent& event)
{
    OH_NativeXComponent_EventSourceType source = OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
    if (OH_NativeXComponent_GetTouchEventSourceType(component, event.id, &source) ==
        OH_NATIVEXCOMPONENT_RESULT_SUCCESS &&
        source != OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN) {
        return source;
    }
    for (uint32_t i = 0; i < event.numPoints; ++i) {
        if (OH_NativeXComponent_GetTouchEventSourceType(component,
            event.touchPoints[i].id, &source) == OH_NATIVEXCOMPONENT_RESULT_SUCCESS &&
            source != OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN) {
            return source;
        }
    }
    return OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
}

bool IsDirectTouch(OH_NativeXComponent_EventSourceType source)
{
    return source == OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHSCREEN ||
        source == OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
}

NativeTouchSample MakeTouchSample(const OH_NativeXComponent_TouchEvent& event)
{
    NativeTouchSample sample;
    sample.action = ResolveTouchAction(event.type);
    sample.atMs = NowMs();
    sample.points.reserve(event.numPoints == 0 ? 1 : event.numPoints);
    for (uint32_t i = 0; i < event.numPoints; ++i) {
        sample.points.push_back(NativeTouchPoint {
            static_cast<int32_t>(event.touchPoints[i].id),
            event.touchPoints[i].x,
            event.touchPoints[i].y,
        });
    }
    if (sample.points.empty() && event.type != OH_NATIVEXCOMPONENT_CANCEL) {
        sample.points.push_back(NativeTouchPoint {
            static_cast<int32_t>(event.id), event.x, event.y});
    }
    return sample;
}

} // namespace

void CancelNativeTouchGesture(const std::string& reason)
{
    Scheduler().Cancel();
    std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
    DispatchGestureActions(g_nativeTouchPolicy.Cancel(), "touch.cancel." + reason);
}

void OnXComponentTouchEvent(OH_NativeXComponent* component, void* window)
{
    if (component == nullptr) {
        return;
    }
    OH_NativeXComponent_TouchEvent event{};
    const int32_t rc = OH_NativeXComponent_GetTouchEvent(component, window, &event);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native touch skipped: get event rc=" + std::to_string(rc));
        return;
    }

    const auto source = ResolveTouchSource(component, event);
    if (!IsDirectTouch(source)) {
        return;
    }
    if (event.type == OH_NATIVEXCOMPONENT_DOWN && IsXComponentFocused()) {
        NotifyRemotePointerDirectTouch(NowMs());
    }

    const NativeTouchSample sample = MakeTouchSample(event);
    uint64_t deadline = 0;
    {
        std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
        const auto actions = g_nativeTouchPolicy.Handle(sample);
        const bool downAccepted = DispatchGestureActions(actions, "touch.gesture");
        if (!downAccepted) {
            DispatchGestureActions(g_nativeTouchPolicy.Cancel(), "touch.rejected");
        }
        deadline = g_nativeTouchPolicy.LongPressDeadlineMs();
    }
    if (deadline == 0) {
        Scheduler().Cancel();
    } else {
        Scheduler().Schedule(deadline);
    }
}

} // namespace rdp_bridge
