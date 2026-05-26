#include "input/xcomponent_input_internal.h"

namespace rdp_bridge {
namespace {

const char* NativeTouchTypeName(OH_NativeXComponent_TouchEventType type)
{
    switch (type) {
        case OH_NATIVEXCOMPONENT_DOWN:
            return "down";
        case OH_NATIVEXCOMPONENT_UP:
            return "up";
        case OH_NATIVEXCOMPONENT_MOVE:
            return "move";
        case OH_NATIVEXCOMPONENT_CANCEL:
            return "cancel";
        default:
            return "unknown";
    }
}

const char* NativeTouchSourceName(OH_NativeXComponent_EventSourceType source)
{
    switch (source) {
        case OH_NATIVEXCOMPONENT_SOURCE_TYPE_MOUSE:
            return "mouse";
        case OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHSCREEN:
            return "touchscreen";
        case OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHPAD:
            return "touchpad";
        case OH_NATIVEXCOMPONENT_SOURCE_TYPE_JOYSTICK:
            return "joystick";
        case OH_NATIVEXCOMPONENT_SOURCE_TYPE_KEYBOARD:
            return "keyboard";
        default:
            return "unknown";
    }
}

OH_NativeXComponent_EventSourceType ResolveNativeTouchSource(
    OH_NativeXComponent* component, const OH_NativeXComponent_TouchEvent& touchEvent)
{
    OH_NativeXComponent_EventSourceType source = OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
    if (component == nullptr) {
        return source;
    }

    if (OH_NativeXComponent_GetTouchEventSourceType(component, touchEvent.id, &source) ==
            OH_NATIVEXCOMPONENT_RESULT_SUCCESS &&
        source != OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN) {
        return source;
    }

    for (uint32_t i = 0; i < touchEvent.numPoints; ++i) {
        if (OH_NativeXComponent_GetTouchEventSourceType(
                component, touchEvent.touchPoints[i].id, &source) ==
                OH_NATIVEXCOMPONENT_RESULT_SUCCESS &&
            source != OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN) {
            return source;
        }
    }
    return OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
}

bool NativePrimaryTouchPoint(const OH_NativeXComponent_TouchEvent& touchEvent, float& x, float& y)
{
    if (touchEvent.numPoints < 1) {
        return false;
    }
    x = touchEvent.touchPoints[0].x;
    y = touchEvent.touchPoints[0].y;
    return true;
}

bool NativeTouchCenter(const OH_NativeXComponent_TouchEvent& touchEvent, float& x, float& y)
{
    if (touchEvent.numPoints < 2) {
        return false;
    }
    x = (touchEvent.touchPoints[0].x + touchEvent.touchPoints[1].x) / 2.0f;
    y = (touchEvent.touchPoints[0].y + touchEvent.touchPoints[1].y) / 2.0f;
    return true;
}

float NativeTouchDistance(float x1, float y1, float x2, float y2)
{
    const float dx = x1 - x2;
    const float dy = y1 - y2;
    return std::sqrt((dx * dx) + (dy * dy));
}

bool IsNativeTouchscreenLikeSource(OH_NativeXComponent_EventSourceType source)
{
    return source == OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHSCREEN ||
        source == OH_NATIVEXCOMPONENT_SOURCE_TYPE_UNKNOWN;
}

void SendNativeTouchMove(float x, float y, uint32_t buttons, const std::string& label,
    bool forceLog = false)
{
    SendNativePointer(MakeNativePointer(LocalPointerAction::Move, x, y, buttons),
        label, forceLog);
}

void SendNativeTouchButton(uint32_t button, bool down, float x, float y,
    const std::string& label)
{
    SendNativePointer(MakeNativePointer(
        down ? LocalPointerAction::ButtonDown : LocalPointerAction::ButtonUp, x, y, button),
        label, true);
}

void SendNativeTouchClick(uint32_t button, float x, float y, const std::string& label)
{
    SendNativeTouchMove(x, y, LocalPointerButtonNone, label + ".move", true);
    SendNativeTouchButton(button, true, x, y, label + ".down");
    SendNativeTouchButton(button, false, x, y, label + ".up");
}

void ReleaseNativeTouchLeftDrag(float x, float y, const std::string& label)
{
    SendNativeTouchMove(x, y, LocalPointerButtonNone, label + ".move", true);
    SendNativeTouchButton(LocalPointerButtonLeft, false, x, y, label + ".up");
}

void ResetNativeTouchGesture()
{
    g_nativeTouch.singleActive = false;
    g_nativeTouch.leftDragActive = false;
    g_nativeTouch.longPressSent = false;
}

bool DispatchNativeTouchScroll(const OH_NativeXComponent_TouchEvent& touchEvent,
    OH_NativeXComponent_EventSourceType source)
{
    constexpr float kWheelThreshold = 24.0f;
    const bool scrollSource = source == OH_NATIVEXCOMPONENT_SOURCE_TYPE_TOUCHPAD ||
        IsNativeTouchscreenLikeSource(source);
    if (!scrollSource) {
        g_nativeTouch.scrollActive = false;
        return false;
    }

    if (touchEvent.type == OH_NATIVEXCOMPONENT_UP ||
        touchEvent.type == OH_NATIVEXCOMPONENT_CANCEL ||
        touchEvent.numPoints < 2) {
        if (g_nativeTouch.scrollActive) {
            g_nativeTouch.scrollActive = false;
            ResetNativeTouchGesture();
            return true;
        }
        return false;
    }

    float centerX = 0.0f;
    float centerY = 0.0f;
    if (!NativeTouchCenter(touchEvent, centerX, centerY)) {
        return false;
    }

    if (g_nativeTouch.leftDragActive) {
        ReleaseNativeTouchLeftDrag(g_nativeTouch.lastX, g_nativeTouch.lastY,
            "touch.scroll.releaseDrag");
    }
    ResetNativeTouchGesture();

    if (touchEvent.type == OH_NATIVEXCOMPONENT_DOWN || !g_nativeTouch.scrollActive) {
        g_nativeTouch.scrollActive = true;
        g_nativeTouch.scrollLastX = centerX;
        g_nativeTouch.scrollLastY = centerY;
        EmitInputLog("XComponent native touch scroll start: source=" +
            std::string(NativeTouchSourceName(source)) +
            " type=" + NativeTouchTypeName(touchEvent.type) +
            " points=" + std::to_string(touchEvent.numPoints) +
            " center=" + std::to_string(RoundSurfaceCoordinate(centerX)) + "," +
            std::to_string(RoundSurfaceCoordinate(centerY)));
        return true;
    }

    if (touchEvent.type != OH_NATIVEXCOMPONENT_MOVE) {
        return true;
    }

    const float deltaX = centerX - g_nativeTouch.scrollLastX;
    const float deltaY = centerY - g_nativeTouch.scrollLastY;
    if (std::fabs(deltaY) < kWheelThreshold && std::fabs(deltaX) < kWheelThreshold) {
        return true;
    }

    g_nativeTouch.scrollLastX = centerX;
    g_nativeTouch.scrollLastY = centerY;
    const bool vertical = std::fabs(deltaY) >= std::fabs(deltaX);
    SendNativePointer(MakeNativePointer(
        vertical ? LocalPointerAction::WheelVertical : LocalPointerAction::WheelHorizontal,
        centerX, centerY, LocalPointerButtonNone,
        static_cast<int32_t>(std::lround(vertical ? deltaY : deltaX))),
        vertical ? "touch.scroll.vertical" : "touch.scroll.horizontal");
    return true;
}

bool DispatchNativeSingleTouch(const OH_NativeXComponent_TouchEvent& touchEvent,
    OH_NativeXComponent_EventSourceType source)
{
    constexpr float kDragThreshold = 8.0f;
    constexpr uint64_t kLongPressTimeoutMs = 550;
    if (!IsNativeTouchscreenLikeSource(source)) {
        return false;
    }

    float x = g_nativeTouch.lastX;
    float y = g_nativeTouch.lastY;
    NativePrimaryTouchPoint(touchEvent, x, y);

    if (touchEvent.type == OH_NATIVEXCOMPONENT_DOWN) {
        g_nativeTouch.scrollActive = false;
        g_nativeTouch.singleActive = true;
        g_nativeTouch.leftDragActive = false;
        g_nativeTouch.longPressSent = false;
        g_nativeTouch.downAtMs = NowMs();
        g_nativeTouch.startX = x;
        g_nativeTouch.startY = y;
        g_nativeTouch.lastX = x;
        g_nativeTouch.lastY = y;
        EmitInputLog("XComponent native touch down: source=" +
            std::string(NativeTouchSourceName(source)) +
            " x=" + std::to_string(RoundSurfaceCoordinate(x)) +
            " y=" + std::to_string(RoundSurfaceCoordinate(y)));
        return true;
    }

    if (!g_nativeTouch.singleActive) {
        return false;
    }

    const uint64_t ageMs = NowMs() - g_nativeTouch.downAtMs;
    const float distance = NativeTouchDistance(x, y, g_nativeTouch.startX, g_nativeTouch.startY);
    g_nativeTouch.lastX = x;
    g_nativeTouch.lastY = y;

    if (touchEvent.type == OH_NATIVEXCOMPONENT_MOVE) {
        if (!g_nativeTouch.leftDragActive && !g_nativeTouch.longPressSent &&
            ageMs >= kLongPressTimeoutMs && distance < kDragThreshold) {
            SendNativeTouchClick(LocalPointerButtonRight, g_nativeTouch.startX,
                g_nativeTouch.startY, "touch.longPress.right");
            g_nativeTouch.longPressSent = true;
            ResetNativeTouchGesture();
            return true;
        }
        if (!g_nativeTouch.leftDragActive && distance >= kDragThreshold) {
            g_nativeTouch.leftDragActive = true;
            SendNativeTouchMove(g_nativeTouch.startX, g_nativeTouch.startY,
                LocalPointerButtonNone, "touch.drag.start.move", true);
            SendNativeTouchButton(LocalPointerButtonLeft, true, g_nativeTouch.startX,
                g_nativeTouch.startY, "touch.drag.start.down");
        }
        if (g_nativeTouch.leftDragActive) {
            SendNativeTouchMove(x, y, LocalPointerButtonLeft, "touch.drag.move");
        }
        return true;
    }

    if (touchEvent.type == OH_NATIVEXCOMPONENT_UP ||
        touchEvent.type == OH_NATIVEXCOMPONENT_CANCEL) {
        const bool cancelled = touchEvent.type == OH_NATIVEXCOMPONENT_CANCEL;
        if (g_nativeTouch.leftDragActive) {
            ReleaseNativeTouchLeftDrag(x, y, "touch.drag.release");
        } else if (!cancelled && !g_nativeTouch.longPressSent) {
            if (ageMs >= kLongPressTimeoutMs && distance < kDragThreshold) {
                SendNativeTouchClick(LocalPointerButtonRight, g_nativeTouch.startX,
                    g_nativeTouch.startY, "touch.longPress.right");
            } else {
                SendNativeTouchClick(LocalPointerButtonLeft, x, y, "touch.tap.left");
            }
        }
        ResetNativeTouchGesture();
        return true;
    }
    return false;
}

} // namespace

void OnXComponentTouchEvent(OH_NativeXComponent* component, void* window)
{
    if (component == nullptr) {
        return;
    }

    OH_NativeXComponent_TouchEvent touchEvent{};
    const int32_t rc = OH_NativeXComponent_GetTouchEvent(component, window, &touchEvent);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native touch skipped: get touch event failed rc=" +
            std::to_string(rc));
        return;
    }

    const OH_NativeXComponent_EventSourceType source =
        ResolveNativeTouchSource(component, touchEvent);
    std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
    if (DispatchNativeTouchScroll(touchEvent, source)) {
        return;
    }
    (void)DispatchNativeSingleTouch(touchEvent, source);
}

} // namespace rdp_bridge
