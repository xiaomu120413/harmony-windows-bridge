#include "input/xcomponent_input_internal.h"

namespace rdp_bridge {
namespace {

uint32_t NativeMouseButtonMask(OH_NativeXComponent_MouseEventButton button)
{
    switch (button) {
        case OH_NATIVEXCOMPONENT_LEFT_BUTTON:
            return LocalPointerButtonLeft;
        case OH_NATIVEXCOMPONENT_RIGHT_BUTTON:
            return LocalPointerButtonRight;
        case OH_NATIVEXCOMPONENT_MIDDLE_BUTTON:
            return LocalPointerButtonMiddle;
        default:
            return LocalPointerButtonNone;
    }
}

const char* NativeMouseActionName(OH_NativeXComponent_MouseEventAction action)
{
    switch (action) {
        case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
            return "press";
        case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
            return "release";
        case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
            return "move";
        default:
            return "none";
    }
}

void DispatchNativeMousePointer(const OH_NativeXComponent_MouseEvent& mouseEvent)
{
    LocalPointerEvent event;
    const uint32_t buttonMask = NativeMouseButtonMask(mouseEvent.button);
    const uint32_t currentButtons = g_nativeMouseButtons.load();

    event.x = RoundSurfaceCoordinate(mouseEvent.x);
    event.y = RoundSurfaceCoordinate(mouseEvent.y);
    event.allowClamp = true;

    switch (mouseEvent.action) {
        case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
            if (buttonMask == LocalPointerButtonNone) {
                return;
            }
            event.action = LocalPointerAction::ButtonDown;
            event.buttons = buttonMask;
            g_nativeMouseButtons.fetch_or(buttonMask);
            break;
        case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
            event.action = LocalPointerAction::ButtonUp;
            event.buttons = buttonMask != LocalPointerButtonNone ? buttonMask : currentButtons;
            if (event.buttons == LocalPointerButtonNone) {
                return;
            }
            g_nativeMouseButtons.fetch_and(~event.buttons);
            break;
        case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
            event.action = LocalPointerAction::Move;
            event.buttons = currentButtons;
            break;
        default:
            return;
    }

    std::string message;
    const bool ok = g_inputSession != nullptr && g_inputSession->SendLocalPointer(event, message);
    if (g_inputSession == nullptr) {
        message = "input bridge not configured";
    }

    static std::atomic_uint32_t mouseLogCount{0};
    const uint32_t logIndex = mouseLogCount.fetch_add(1);
    (void)logIndex;
    if (!ok ||
        mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_PRESS ||
        mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_RELEASE) {
        EmitInputLog("XComponent native mouse: action=" +
            std::string(NativeMouseActionName(mouseEvent.action)) +
            " button=" + std::to_string(static_cast<int32_t>(mouseEvent.button)) +
            " buttons=" + std::to_string(event.buttons) +
            " x=" + std::to_string(event.x) +
            " y=" + std::to_string(event.y) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
}

} // namespace

void OnXComponentMouseEvent(OH_NativeXComponent* component, void* window)
{
    if (component == nullptr) {
        return;
    }

    OH_NativeXComponent_MouseEvent mouseEvent{};
    const int32_t rc = OH_NativeXComponent_GetMouseEvent(component, window, &mouseEvent);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native mouse skipped: get mouse event failed rc=" +
            std::to_string(rc));
        return;
    }

    DispatchNativeMousePointer(mouseEvent);
}

void OnXComponentHoverEvent(OH_NativeXComponent*, bool)
{
}

} // namespace rdp_bridge
