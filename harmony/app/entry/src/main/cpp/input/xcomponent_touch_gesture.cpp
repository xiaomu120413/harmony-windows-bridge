#include "input/xcomponent_input_internal.h"
#include "input/remote_pointer_text_detector.h"

#include <cstdlib>

namespace rdp_bridge {
namespace {

std::mutex g_nativeGestureDispatchMutex;

bool SendTouchButton(uint32_t button, bool down, float x, float y,
    const std::string& label, bool allowClamp)
{
    return SendNativePointer(MakeNativePointer(
        down ? LocalPointerAction::ButtonDown : LocalPointerAction::ButtonUp,
        x, y, button, 0, allowClamp), label, true);
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

} // namespace

bool DispatchNativeGestureActions(const std::vector<NativeGestureAction>& actions,
    const std::string& label)
{
    std::lock_guard<std::mutex> lock(g_nativeGestureDispatchMutex);
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

void OnXComponentTouchEvent(OH_NativeXComponent* component, void* window)
{
    if (component == nullptr) {
        return;
    }
    OH_NativeXComponent_TouchEvent event {};
    const int32_t rc = OH_NativeXComponent_GetTouchEvent(component, window, &event);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native touch skipped: get event rc=" + std::to_string(rc));
        return;
    }
    if (event.type == OH_NATIVEXCOMPONENT_DOWN &&
        IsDirectTouch(ResolveTouchSource(component, event)) && IsXComponentFocused()) {
        NotifyRemotePointerDirectTouch(NowMs());
    }
}

} // namespace rdp_bridge
