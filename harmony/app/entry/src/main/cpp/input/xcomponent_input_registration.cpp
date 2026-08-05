#include "input/xcomponent_input_internal.h"
#include "input/remote_ime_client.h"
#include "input/remote_pointer_text_detector.h"
#include "input/xcomponent_native_gesture.h"

#include <utility>
#include <window_manager/oh_display_manager.h>

namespace rdp_bridge {

RdpSession* g_inputSession = nullptr;
RemoteImeClient* g_remoteIme = nullptr;
std::function<void(const std::string&)> g_inputLog;
std::atomic_bool g_xcomponentFocused{false};
std::atomic<float> g_inputDensity{1.0f};
std::mutex g_nativeMouseMutex;
NativeMouseState g_nativeMouse;
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
    uint32_t buttons, int32_t delta, bool allowClamp)
{
    LocalPointerEvent event;
    event.action = action;
    event.buttons = buttons;
    event.x = RoundSurfaceCoordinate(x);
    event.y = RoundSurfaceCoordinate(y);
    event.delta = delta;
    event.allowClamp = allowClamp;
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
    RdpSession* session, RemoteImeClient* remoteIme,
    std::function<void(const std::string&)> log)
{
    g_inputSession = session;
    g_remoteIme = remoteIme;
    g_inputLog = std::move(log);
    ConfigureRemotePointerTextDetector([](bool visible) {
        if (!IsXComponentFocused() || g_remoteIme == nullptr) {
            return false;
        }
        std::string message;
        if (visible && g_remoteIme->IsKeyboardVisible()) {
            return true;
        }
        if (!visible && !g_remoteIme->IsKeyboardVisible()) {
            return true;
        }
        const bool applied = visible ? g_remoteIme->Open(message) : g_remoteIme->Close(message);
        EmitInputLog(std::string("RDP_IME event=touch_pointer_visibility desired=") +
            (visible ? "shown" : "hidden") + " result=" +
            (applied ? "applied " : "rejected ") + message);
        return applied;
    }, EmitInputLog);
}

void RefreshXComponentInputDensity()
{
    int32_t densityDpi = 0;
    const auto rc = OH_NativeDisplayManager_GetDefaultDisplayDensityDpi(&densityDpi);
    const float density = rc == DISPLAY_MANAGER_OK && densityDpi > 0
        ? static_cast<float>(densityDpi) / 160.0f : 1.0f;
    g_inputDensity.store(density);
    EmitInputLog("XComponent input density: dpi=" + std::to_string(densityDpi) +
        " density=" + std::to_string(density) + " rc=" + std::to_string(rc));
}

void ReleaseAllXComponentInput(const std::string& reason)
{
    NativeMouseState mouse;
    {
        std::lock_guard<std::mutex> lock(g_nativeMouseMutex);
        mouse = g_nativeMouse;
        g_nativeMouse = NativeMouseState{};
    }
    for (uint32_t button : {LocalPointerButtonLeft, LocalPointerButtonRight,
        LocalPointerButtonMiddle}) {
        if ((mouse.buttons & button) != 0) {
            SendNativePointer(MakeNativePointer(LocalPointerAction::ButtonUp,
                mouse.lastX, mouse.lastY, button, 0, true),
                "releaseAllInput.mouse." + reason, true);
        }
    }
    CancelXComponentNativeGestures(reason);
    {
        std::lock_guard<std::mutex> lock(g_nativeAxisMutex);
        g_nativeAxis = NativeAxisState{};
    }
    std::string message;
    if (g_inputSession != nullptr && !g_inputSession->ReleaseAllKeys(message) &&
        message != "no active FreeRDP session") {
        EmitInputLog("releaseAllInput keys skipped: reason=" + reason + " " + message);
    }
}

void ResetXComponentInputBridge()
{
    ReleaseAllXComponentInput("bridgeReset");
    g_xcomponentFocused.store(false);
    ResetRemotePointerTextDetector();
    if (g_remoteIme != nullptr) {
        std::string message;
        (void)g_remoteIme->Close(message);
    }
}

bool IsXComponentFocused()
{
    return g_xcomponentFocused.load();
}

XComponentInputRegisterResult RegisterXComponentInputCallbacks(OH_NativeXComponent* component)
{
    XComponentInputRegisterResult result;
    if (component == nullptr) {
        return result;
    }
    RefreshXComponentInputDensity();

    static OH_NativeXComponent_MouseEvent_Callback mouseCallback = {
        OnXComponentMouseEvent,
        OnXComponentHoverEvent,
    };

    result.mouseRc = OH_NativeXComponent_RegisterMouseEventCallback(component, &mouseCallback);
    result.focusRc = OH_NativeXComponent_RegisterFocusEventCallback(component, OnXComponentFocusEvent);
    result.blurRc = OH_NativeXComponent_RegisterBlurEventCallback(component, OnXComponentBlurEvent);
    result.keyRc = OH_NativeXComponent_RegisterKeyEventCallbackWithResult(component, OnXComponentKeyEvent);
    result.softKeyboardRc = OH_NativeXComponent_SetNeedSoftKeyboard(component, false);
    result.axisRc = OH_NativeXComponent_RegisterUIInputEventCallback(
        component, OnXComponentAxisEvent, ARKUI_UIINPUTEVENT_TYPE_AXIS);
    return result;
}

} // namespace rdp_bridge
