#include "input/xcomponent_input_bridge.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>
#include <utility>

#include <arkui/ui_input_event.h>
#include <ace/xcomponent/native_xcomponent_key_event.h>

namespace rdp_bridge {
namespace {

constexpr uint64_t kAxisEventDedupMs = 12;

struct NativeTouchState {
    bool singleActive = false;
    bool leftDragActive = false;
    bool longPressSent = false;
    bool scrollActive = false;
    uint64_t downAtMs = 0;
    float startX = 0.0f;
    float startY = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float scrollLastX = 0.0f;
    float scrollLastY = 0.0f;
};

struct NativeAxisState {
    uint64_t lastAtMs = 0;
    int32_t lastAction = -1;
    float lastX = 0.0f;
    float lastY = 0.0f;
    double lastDeltaX = 0.0;
    double lastDeltaY = 0.0;
};

RdpSession* g_inputSession = nullptr;
std::function<void(const std::string&)> g_inputLog;
std::atomic_uint32_t g_nativeMouseButtons{0};
std::mutex g_nativeTouchMutex;
NativeTouchState g_nativeTouch;
std::mutex g_nativeAxisMutex;
NativeAxisState g_nativeAxis;

void EmitInputLog(const std::string& line)
{
    if (g_inputLog) {
        g_inputLog(line);
    }
}

uint32_t RoundSurfaceCoordinate(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    const float maxValue = static_cast<float>(std::numeric_limits<uint32_t>::max());
    if (value >= maxValue) {
        return std::numeric_limits<uint32_t>::max();
    }
    return static_cast<uint32_t>(std::lround(value));
}

uint64_t NowMs()
{
    using Clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            Clock::now().time_since_epoch()).count());
}

const char* LocalPointerActionName(LocalPointerAction action)
{
    switch (action) {
        case LocalPointerAction::ButtonDown:
            return "buttonDown";
        case LocalPointerAction::ButtonUp:
            return "buttonUp";
        case LocalPointerAction::WheelVertical:
            return "wheelVertical";
        case LocalPointerAction::WheelHorizontal:
            return "wheelHorizontal";
        case LocalPointerAction::Move:
        default:
            return "move";
    }
}

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

LocalPointerEvent MakeNativePointer(LocalPointerAction action, float x, float y,
    uint32_t buttons = LocalPointerButtonNone, int32_t delta = 0)
{
    LocalPointerEvent event;
    event.action = action;
    event.buttons = buttons;
    event.x = RoundSurfaceCoordinate(x);
    event.y = RoundSurfaceCoordinate(y);
    event.delta = delta;
    event.allowClamp = true;
    return event;
}

bool SendNativePointer(const LocalPointerEvent& event, const std::string& label,
    bool forceLog = false)
{
    std::string message;
    const bool ok = g_inputSession != nullptr && g_inputSession->SendLocalPointer(event, message);
    if (g_inputSession == nullptr) {
        message = "input bridge not configured";
    }

    static std::atomic_uint32_t inputLogCount{0};
    const uint32_t logIndex = inputLogCount.fetch_add(1);
    if (forceLog || !ok || logIndex < 80 || (logIndex % 200) == 0) {
        EmitInputLog("XComponent native input " + label +
            ": action=" + LocalPointerActionName(event.action) +
            " buttons=" + std::to_string(event.buttons) +
            " delta=" + std::to_string(event.delta) +
            " x=" + std::to_string(event.x) +
            " y=" + std::to_string(event.y) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
    return ok;
}

void SendNativeTouchMove(float x, float y, uint32_t buttons, const std::string& label,
    bool forceLog = false)
{
    SendNativePointer(MakeNativePointer(LocalPointerAction::Move, x, y, buttons),
        label, forceLog);
}

void SendNativeTouchButton(uint32_t button, bool down, float x, float y,
    const std::string& label, bool forceLog = true)
{
    SendNativePointer(MakeNativePointer(
        down ? LocalPointerAction::ButtonDown : LocalPointerAction::ButtonUp, x, y, button),
        label, forceLog);
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
    if (!ok || logIndex < 60 || (logIndex % 200) == 0 ||
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

bool ShouldSkipDuplicateAxisEvent(int32_t action, float x, float y, double deltaX, double deltaY)
{
    const uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(g_nativeAxisMutex);
    const bool duplicate = now - g_nativeAxis.lastAtMs <= kAxisEventDedupMs &&
        g_nativeAxis.lastAction == action &&
        std::fabs(g_nativeAxis.lastX - x) < 0.01f &&
        std::fabs(g_nativeAxis.lastY - y) < 0.01f &&
        std::fabs(g_nativeAxis.lastDeltaX - deltaX) < 0.01 &&
        std::fabs(g_nativeAxis.lastDeltaY - deltaY) < 0.01;
    g_nativeAxis.lastAtMs = now;
    g_nativeAxis.lastAction = action;
    g_nativeAxis.lastX = x;
    g_nativeAxis.lastY = y;
    g_nativeAxis.lastDeltaX = deltaX;
    g_nativeAxis.lastDeltaY = deltaY;
    return duplicate;
}

void DispatchNativeAxisPointer(const ArkUI_UIInputEvent* event)
{
    const int32_t action = OH_ArkUI_UIInputEvent_GetAction(event);
    const double deltaY = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
    const double deltaX = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
    const float x = OH_ArkUI_PointerEvent_GetX(event);
    const float y = OH_ArkUI_PointerEvent_GetY(event);
    if (ShouldSkipDuplicateAxisEvent(action, x, y, deltaX, deltaY)) {
        return;
    }
    if (std::fabs(deltaY) < 0.01 && std::fabs(deltaX) < 0.01) {
        return;
    }
    if (action == UI_AXIS_EVENT_ACTION_BEGIN ||
        action == UI_AXIS_EVENT_ACTION_END ||
        action == UI_AXIS_EVENT_ACTION_CANCEL) {
        return;
    }

    const bool vertical = std::fabs(deltaY) >= std::fabs(deltaX);
    const double delta = vertical ? deltaY : deltaX;
    const LocalPointerAction pointerAction =
        vertical ? LocalPointerAction::WheelVertical : LocalPointerAction::WheelHorizontal;
    SendNativePointer(MakeNativePointer(pointerAction, x, y, LocalPointerButtonNone,
        static_cast<int32_t>(std::lround(delta))),
        vertical ? "axis.wheel.vertical" : "axis.wheel.horizontal");
}

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

void OnXComponentAxisEvent(OH_NativeXComponent*, ArkUI_UIInputEvent* event,
    ArkUI_UIInputEvent_Type type)
{
    if (event == nullptr || type != ARKUI_UIINPUTEVENT_TYPE_AXIS) {
        return;
    }
    DispatchNativeAxisPointer(event);
}

void OnXComponentFocusEvent(OH_NativeXComponent*, void*)
{
    EmitInputLog("XComponent focused for native input");
}

void OnXComponentBlurEvent(OH_NativeXComponent*, void*)
{
    g_nativeMouseButtons.store(0);
    {
        std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
        g_nativeTouch = NativeTouchState{};
    }

    std::string message;
    if (g_inputSession != nullptr && g_inputSession->ReleaseAllKeys(message)) {
        EmitInputLog("XComponent blurred; " + message);
    } else {
        EmitInputLog("XComponent blurred; release keys skipped: " + message);
    }
}

bool IsModifierPressed(uint64_t modifiers, ArkUI_ModifierKeyName modifier)
{
    return (modifiers & static_cast<uint64_t>(modifier)) != 0;
}

bool OnXComponentKeyEvent(OH_NativeXComponent* component, void*)
{
    if (component == nullptr) {
        return false;
    }

    OH_NativeXComponent_KeyEvent* keyEvent = nullptr;
    if (OH_NativeXComponent_GetKeyEvent(component, &keyEvent) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        keyEvent == nullptr) {
        EmitInputLog("XComponent native key skipped: key event unavailable");
        return false;
    }

    OH_NativeXComponent_KeyAction action = OH_NATIVEXCOMPONENT_KEY_ACTION_UNKNOWN;
    OH_NativeXComponent_KeyCode keyCode = KEY_UNKNOWN;
    if (OH_NativeXComponent_GetKeyEventAction(keyEvent, &action) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        OH_NativeXComponent_GetKeyEventCode(keyEvent, &keyCode) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native key skipped: action/code unavailable");
        return false;
    }

    if (action != OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN &&
        action != OH_NATIVEXCOMPONENT_KEY_ACTION_UP) {
        return false;
    }

    uint64_t modifiers = 0;
    OH_NativeXComponent_GetKeyEventModifierKeyStates(keyEvent, &modifiers);
    const OhosKeyEvent input {
        static_cast<uint32_t>(keyCode),
        action == OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN,
        false,
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_CTRL),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_SHIFT),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_ALT),
        false,
    };

    std::string message;
    const bool ok = g_inputSession != nullptr && g_inputSession->SendPlatformKey(input, message);
    if (g_inputSession == nullptr) {
        message = "input bridge not configured";
    }

    static std::atomic_uint32_t keyLogCount{0};
    const uint32_t logIndex = keyLogCount.fetch_add(1);
    const bool importantKey = input.keyCode == KEY_ENTER || input.keyCode == KEY_NUMPAD_ENTER ||
        input.keyCode == KEY_DEL || input.keyCode == KEY_FORWARD_DEL || input.ctrl ||
        input.keyCode == KEY_CTRL_LEFT || input.keyCode == KEY_CTRL_RIGHT;
    if (importantKey || logIndex < 40 || (logIndex % 200) == 0 || !ok) {
        EmitInputLog("XComponent native key: keyCode=" + std::to_string(input.keyCode) +
            (input.down ? " down " : " up ") +
            "mods=" + std::to_string(modifiers) +
            " ctrl=" + std::to_string(input.ctrl ? 1 : 0) +
            " shift=" + std::to_string(input.shift ? 1 : 0) +
            " alt=" + std::to_string(input.alt ? 1 : 0) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
    return ok;
}

} // namespace

void ConfigureXComponentInputBridge(
    RdpSession* session, std::function<void(const std::string&)> log)
{
    g_inputSession = session;
    g_inputLog = std::move(log);
}

void ResetXComponentInputBridge()
{
    g_nativeMouseButtons.store(0);
    {
        std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
        g_nativeTouch = NativeTouchState{};
    }
    {
        std::lock_guard<std::mutex> lock(g_nativeAxisMutex);
        g_nativeAxis = NativeAxisState{};
    }
}

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

XComponentInputRegisterResult RegisterXComponentInputCallbacks(OH_NativeXComponent* component)
{
    XComponentInputRegisterResult result;
    if (component == nullptr) {
        return result;
    }

    static OH_NativeXComponent_MouseEvent_Callback mouseCallback = {
        OnXComponentMouseEvent,
        OnXComponentHoverEvent,
    };

    result.mouseRc = OH_NativeXComponent_RegisterMouseEventCallback(component, &mouseCallback);
    result.focusRc = OH_NativeXComponent_RegisterFocusEventCallback(component, OnXComponentFocusEvent);
    result.blurRc = OH_NativeXComponent_RegisterBlurEventCallback(component, OnXComponentBlurEvent);
    result.keyRc = OH_NativeXComponent_RegisterKeyEventCallbackWithResult(component, OnXComponentKeyEvent);
    result.softKeyboardRc = OH_NativeXComponent_SetNeedSoftKeyboard(component, true);
    result.axisRc = OH_NativeXComponent_RegisterUIInputEventCallback(
        component, OnXComponentAxisEvent, ARKUI_UIINPUTEVENT_TYPE_AXIS);
    return result;
}

} // namespace rdp_bridge
