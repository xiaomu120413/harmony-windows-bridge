#include "input/xcomponent_input_internal.h"

#include <utility>

namespace rdp_bridge {

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

LocalPointerEvent MakeNativePointer(LocalPointerAction action, float x, float y,
    uint32_t buttons, int32_t delta)
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
    bool forceLog)
{
    std::string message;
    const bool ok = g_inputSession != nullptr && g_inputSession->SendLocalPointer(event, message);
    if (g_inputSession == nullptr) {
        message = "input bridge not configured";
    }

    static std::atomic_uint32_t inputLogCount{0};
    const uint32_t logIndex = inputLogCount.fetch_add(1);
    (void)logIndex;
    if (forceLog || !ok) {
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
