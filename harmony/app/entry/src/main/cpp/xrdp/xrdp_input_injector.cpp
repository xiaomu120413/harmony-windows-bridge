#include "xrdp/xrdp_input_injector.h"

#include "common/bridge_log.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <multimodalinput/oh_input_manager.h>
#include <multimodalinput/oh_key_code.h>

namespace rdp_bridge {
namespace {

constexpr int32_t kXrdpEventSessionConnect = 0;
constexpr int32_t kXrdpEventSessionDisconnect = -1;
constexpr int32_t kXrdpWmKeyDown = 15;
constexpr int32_t kXrdpWmKeyUp = 16;
constexpr int32_t kXrdpWmMouseMove = 100;
constexpr int32_t kXrdpWmLeftButtonUp = 101;
constexpr int32_t kXrdpWmLeftButtonDown = 102;
constexpr int32_t kXrdpWmRightButtonUp = 103;
constexpr int32_t kXrdpWmRightButtonDown = 104;
constexpr int32_t kXrdpWmMiddleButtonUp = 105;
constexpr int32_t kXrdpWmMiddleButtonDown = 106;
constexpr int32_t kXrdpWmWheelUpDown = 108;
constexpr int32_t kXrdpWmWheelUpUp = 107;
constexpr int32_t kXrdpWmWheelDownDown = 110;
constexpr int32_t kXrdpWmWheelDownUp = 109;
constexpr int32_t kXrdpWmHWheelLeftDown = 112;
constexpr int32_t kXrdpWmHWheelLeftUp = 111;
constexpr int32_t kXrdpWmHWheelRightDown = 114;
constexpr int32_t kXrdpWmHWheelRightUp = 113;
constexpr int32_t kXrdpWmXButton1Up = 115;
constexpr int32_t kXrdpWmXButton1Down = 116;
constexpr int32_t kXrdpWmXButton2Up = 117;
constexpr int32_t kXrdpWmXButton2Down = 118;
constexpr int32_t kXrdpWmTouchVScroll = 140;
constexpr int32_t kXrdpWmTouchHScroll = 141;
constexpr long kX11Backspace = 65288;
constexpr long kX11Tab = 65289;
constexpr long kX11Return = 65293;
constexpr long kX11Escape = 65307;
constexpr long kX11Home = 65360;
constexpr long kX11Left = 65361;
constexpr long kX11Up = 65362;
constexpr long kX11Right = 65363;
constexpr long kX11Down = 65364;
constexpr long kX11PageUp = 65365;
constexpr long kX11PageDown = 65366;
constexpr long kX11End = 65367;
constexpr long kX11Insert = 65379;
constexpr long kX11Delete = 65535;
constexpr long kX11ShiftLeft = 65505;
constexpr long kX11ShiftRight = 65506;
constexpr long kX11CtrlLeft = 65507;
constexpr long kX11CtrlRight = 65508;
constexpr long kX11CapsLock = 65509;
constexpr long kX11AltLeft = 65513;
constexpr long kX11AltRight = 65514;
constexpr long kX11MetaLeft = 65511;
constexpr long kX11MetaRight = 65512;
constexpr long kX11SuperLeft = 65515;
constexpr long kX11SuperRight = 65516;
constexpr long kX11F1 = 65470;
constexpr int32_t kMaxInputQueue = 512;
constexpr int32_t kWheelStep = 120;
constexpr uint32_t kMouseButtonLeftMask = 1U;
constexpr uint32_t kMouseButtonMiddleMask = 1U << 1U;
constexpr uint32_t kMouseButtonRightMask = 1U << 2U;
constexpr uint32_t kMouseButtonForwardMask = 1U << 3U;
constexpr uint32_t kMouseButtonBackMask = 1U << 4U;

std::atomic<Input_InjectionStatus> g_authorizedStatus { UNAUTHORIZED };
std::atomic<bool> g_authorizationRequested { false };
std::atomic<uint32_t> g_authorizationLogCount { 0 };
std::mutex g_authorizationMutex;
std::chrono::steady_clock::time_point g_lastAuthorizationRequest;

int64_t NowMs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

int32_t ClampCoordinate(long value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 32767) {
        return 32767;
    }
    return static_cast<int32_t>(value);
}

void OnInjectionAuthorized(Input_InjectionStatus status)
{
    g_authorizedStatus.store(status);
    EmitHilogInfo("xrdp input injection authorization callback status=" + std::to_string(status));
}

bool EnsureInjectionAuthorized(std::string& message)
{
    Input_InjectionStatus status = UNAUTHORIZED;
    const Input_Result queryRc = OH_Input_QueryAuthorizedStatus(&status);
    if (queryRc == INPUT_SUCCESS) {
        g_authorizedStatus.store(status);
        if (status == AUTHORIZED) {
            return true;
        }
    }

    const auto now = std::chrono::steady_clock::now();
    bool shouldRequest = false;
    {
        std::lock_guard<std::mutex> lock(g_authorizationMutex);
        shouldRequest = !g_authorizationRequested.load() ||
            now - g_lastAuthorizationRequest > std::chrono::seconds(5);
        if (shouldRequest) {
            g_authorizationRequested.store(true);
            g_lastAuthorizationRequest = now;
        }
    }

    if (shouldRequest) {
        const Input_Result requestRc = OH_Input_RequestInjection(OnInjectionAuthorized);
        if (requestRc == INPUT_INJECTION_AUTHORIZED) {
            g_authorizedStatus.store(AUTHORIZED);
            return true;
        }
        message = "xrdp input injection authorization requested rc=" + std::to_string(requestRc) +
            " queryRc=" + std::to_string(queryRc) + " status=" + std::to_string(status);
    } else {
        message = "xrdp input injection authorization pending queryRc=" + std::to_string(queryRc) +
            " status=" + std::to_string(status);
    }

    const uint32_t count = g_authorizationLogCount.fetch_add(1) + 1U;
    if (count <= 20U || (count % 200U) == 0U) {
        EmitHilogInfo(message);
    }
    return false;
}

bool IsExtendedKeyboardEvent(const XrdpOhosInputEvent& event)
{
    return (event.param4 & 0x0100L) != 0;
}

int32_t MapScancodeToOhosKeyCode(long scancode, bool extended)
{
    switch (scancode) {
        case 1: return KEYCODE_ESCAPE;
        case 2: return KEYCODE_1;
        case 3: return KEYCODE_2;
        case 4: return KEYCODE_3;
        case 5: return KEYCODE_4;
        case 6: return KEYCODE_5;
        case 7: return KEYCODE_6;
        case 8: return KEYCODE_7;
        case 9: return KEYCODE_8;
        case 10: return KEYCODE_9;
        case 11: return KEYCODE_0;
        case 12: return KEYCODE_MINUS;
        case 13: return KEYCODE_EQUALS;
        case 14: return KEYCODE_DEL;
        case 15: return KEYCODE_TAB;
        case 16: return KEYCODE_Q;
        case 17: return KEYCODE_W;
        case 18: return KEYCODE_E;
        case 19: return KEYCODE_R;
        case 20: return KEYCODE_T;
        case 21: return KEYCODE_Y;
        case 22: return KEYCODE_U;
        case 23: return KEYCODE_I;
        case 24: return KEYCODE_O;
        case 25: return KEYCODE_P;
        case 26: return KEYCODE_LEFT_BRACKET;
        case 27: return KEYCODE_RIGHT_BRACKET;
        case 28: return extended ? KEYCODE_NUMPAD_ENTER : KEYCODE_ENTER;
        case 29: return extended ? KEYCODE_CTRL_RIGHT : KEYCODE_CTRL_LEFT;
        case 30: return KEYCODE_A;
        case 31: return KEYCODE_S;
        case 32: return KEYCODE_D;
        case 33: return KEYCODE_F;
        case 34: return KEYCODE_G;
        case 35: return KEYCODE_H;
        case 36: return KEYCODE_J;
        case 37: return KEYCODE_K;
        case 38: return KEYCODE_L;
        case 39: return KEYCODE_SEMICOLON;
        case 40: return KEYCODE_APOSTROPHE;
        case 41: return KEYCODE_GRAVE;
        case 42: return KEYCODE_SHIFT_LEFT;
        case 43: return KEYCODE_BACKSLASH;
        case 44: return KEYCODE_Z;
        case 45: return KEYCODE_X;
        case 46: return KEYCODE_C;
        case 47: return KEYCODE_V;
        case 48: return KEYCODE_B;
        case 49: return KEYCODE_N;
        case 50: return KEYCODE_M;
        case 51: return KEYCODE_COMMA;
        case 52: return KEYCODE_PERIOD;
        case 53: return extended ? KEYCODE_NUMPAD_DIVIDE : KEYCODE_SLASH;
        case 54: return KEYCODE_SHIFT_RIGHT;
        case 55: return KEYCODE_NUMPAD_MULTIPLY;
        case 56: return extended ? KEYCODE_ALT_RIGHT : KEYCODE_ALT_LEFT;
        case 57: return KEYCODE_SPACE;
        case 58: return KEYCODE_CAPS_LOCK;
        case 59: return KEYCODE_F1;
        case 60: return KEYCODE_F2;
        case 61: return KEYCODE_F3;
        case 62: return KEYCODE_F4;
        case 63: return KEYCODE_F5;
        case 64: return KEYCODE_F6;
        case 65: return KEYCODE_F7;
        case 66: return KEYCODE_F8;
        case 67: return KEYCODE_F9;
        case 68: return KEYCODE_F10;
        case 69: return KEYCODE_NUM_LOCK;
        case 70: return KEYCODE_SCROLL_LOCK;
        case 71: return extended ? KEYCODE_MOVE_HOME : KEYCODE_NUMPAD_7;
        case 72: return extended ? KEYCODE_DPAD_UP : KEYCODE_NUMPAD_8;
        case 73: return extended ? KEYCODE_PAGE_UP : KEYCODE_NUMPAD_9;
        case 74: return KEYCODE_NUMPAD_SUBTRACT;
        case 75: return extended ? KEYCODE_DPAD_LEFT : KEYCODE_NUMPAD_4;
        case 76: return KEYCODE_NUMPAD_5;
        case 77: return extended ? KEYCODE_DPAD_RIGHT : KEYCODE_NUMPAD_6;
        case 78: return KEYCODE_NUMPAD_ADD;
        case 79: return extended ? KEYCODE_MOVE_END : KEYCODE_NUMPAD_1;
        case 80: return extended ? KEYCODE_DPAD_DOWN : KEYCODE_NUMPAD_2;
        case 81: return extended ? KEYCODE_PAGE_DOWN : KEYCODE_NUMPAD_3;
        case 82: return extended ? KEYCODE_INSERT : KEYCODE_NUMPAD_0;
        case 83: return extended ? KEYCODE_FORWARD_DEL : KEYCODE_NUMPAD_DOT;
        case 87: return KEYCODE_F11;
        case 88: return KEYCODE_F12;
        case 91: return KEYCODE_META_LEFT;
        case 92: return KEYCODE_META_RIGHT;
        case 93: return KEYCODE_MENU;
        default: return -1;
    }
}

int32_t MapKeysymToOhosKeyCode(long keysym)
{
    if (keysym >= 'a' && keysym <= 'z') {
        return KEYCODE_A + static_cast<int32_t>(keysym - 'a');
    }
    if (keysym >= 'A' && keysym <= 'Z') {
        return KEYCODE_A + static_cast<int32_t>(keysym - 'A');
    }
    if (keysym >= '0' && keysym <= '9') {
        return keysym == '0' ? KEYCODE_0 : KEYCODE_1 + static_cast<int32_t>(keysym - '1');
    }

    switch (keysym) {
        case ' ': return KEYCODE_SPACE;
        case '-': return KEYCODE_MINUS;
        case '=': return KEYCODE_EQUALS;
        case '[': return KEYCODE_LEFT_BRACKET;
        case ']': return KEYCODE_RIGHT_BRACKET;
        case '\\': return KEYCODE_BACKSLASH;
        case ';': return KEYCODE_SEMICOLON;
        case '\'': return KEYCODE_APOSTROPHE;
        case ',': return KEYCODE_COMMA;
        case '.': return KEYCODE_PERIOD;
        case '/': return KEYCODE_SLASH;
        case '`': return KEYCODE_GRAVE;
        case kX11Backspace: return KEYCODE_DEL;
        case kX11Tab: return KEYCODE_TAB;
        case kX11Return: return KEYCODE_ENTER;
        case kX11Escape: return KEYCODE_ESCAPE;
        case kX11Home: return KEYCODE_MOVE_HOME;
        case kX11Left: return KEYCODE_DPAD_LEFT;
        case kX11Up: return KEYCODE_DPAD_UP;
        case kX11Right: return KEYCODE_DPAD_RIGHT;
        case kX11Down: return KEYCODE_DPAD_DOWN;
        case kX11PageUp: return KEYCODE_PAGE_UP;
        case kX11PageDown: return KEYCODE_PAGE_DOWN;
        case kX11End: return KEYCODE_MOVE_END;
        case kX11Insert: return KEYCODE_INSERT;
        case kX11Delete: return KEYCODE_FORWARD_DEL;
        case kX11ShiftLeft: return KEYCODE_SHIFT_LEFT;
        case kX11ShiftRight: return KEYCODE_SHIFT_RIGHT;
        case kX11CtrlLeft: return KEYCODE_CTRL_LEFT;
        case kX11CtrlRight: return KEYCODE_CTRL_RIGHT;
        case kX11CapsLock: return KEYCODE_CAPS_LOCK;
        case kX11AltLeft: return KEYCODE_ALT_LEFT;
        case kX11AltRight: return KEYCODE_ALT_RIGHT;
        case kX11MetaLeft:
        case kX11SuperLeft: return KEYCODE_META_LEFT;
        case kX11MetaRight:
        case kX11SuperRight: return KEYCODE_META_RIGHT;
        default:
            if (keysym >= kX11F1 && keysym < kX11F1 + 12) {
                return KEYCODE_F1 + static_cast<int32_t>(keysym - kX11F1);
            }
            return -1;
    }
}

int32_t MapXrdpKeyToOhosKeyCode(const XrdpOhosInputEvent& event)
{
    const int32_t byScancode = MapScancodeToOhosKeyCode(event.param3, IsExtendedKeyboardEvent(event));
    if (byScancode >= 0) {
        return byScancode;
    }
    return MapKeysymToOhosKeyCode(event.param2);
}

bool IsMouseMove(const XrdpOhosInputEvent& event)
{
    return event.msg == kXrdpWmMouseMove;
}

bool IsInputEvent(const XrdpOhosInputEvent& event)
{
    if (event.msg == kXrdpEventSessionConnect || event.msg == kXrdpEventSessionDisconnect) {
        return false;
    }
    return event.msg == kXrdpWmKeyDown || event.msg == kXrdpWmKeyUp ||
        (event.msg >= kXrdpWmMouseMove && event.msg <= kXrdpWmXButton2Down) ||
        event.msg == kXrdpWmTouchVScroll || event.msg == kXrdpWmTouchHScroll;
}

struct MouseDispatch {
    bool supported = false;
    bool wheel = false;
    int32_t action = MOUSE_ACTION_MOVE;
    int32_t button = MOUSE_BUTTON_NONE;
    int32_t axisType = MOUSE_AXIS_SCROLL_VERTICAL;
    float axisValue = 0.0f;
    uint32_t buttonMask = 0;
    bool buttonDown = false;
};

MouseDispatch MapMouseEvent(const XrdpOhosInputEvent& event)
{
    MouseDispatch dispatch;
    dispatch.supported = true;

    switch (event.msg) {
        case kXrdpWmMouseMove:
            dispatch.action = MOUSE_ACTION_MOVE;
            break;
        case kXrdpWmLeftButtonDown:
            dispatch.action = MOUSE_ACTION_BUTTON_DOWN;
            dispatch.button = MOUSE_BUTTON_LEFT;
            dispatch.buttonMask = kMouseButtonLeftMask;
            dispatch.buttonDown = true;
            break;
        case kXrdpWmLeftButtonUp:
            dispatch.action = MOUSE_ACTION_BUTTON_UP;
            dispatch.button = MOUSE_BUTTON_LEFT;
            dispatch.buttonMask = kMouseButtonLeftMask;
            break;
        case kXrdpWmRightButtonDown:
            dispatch.action = MOUSE_ACTION_BUTTON_DOWN;
            dispatch.button = MOUSE_BUTTON_RIGHT;
            dispatch.buttonMask = kMouseButtonRightMask;
            dispatch.buttonDown = true;
            break;
        case kXrdpWmRightButtonUp:
            dispatch.action = MOUSE_ACTION_BUTTON_UP;
            dispatch.button = MOUSE_BUTTON_RIGHT;
            dispatch.buttonMask = kMouseButtonRightMask;
            break;
        case kXrdpWmMiddleButtonDown:
            dispatch.action = MOUSE_ACTION_BUTTON_DOWN;
            dispatch.button = MOUSE_BUTTON_MIDDLE;
            dispatch.buttonMask = kMouseButtonMiddleMask;
            dispatch.buttonDown = true;
            break;
        case kXrdpWmMiddleButtonUp:
            dispatch.action = MOUSE_ACTION_BUTTON_UP;
            dispatch.button = MOUSE_BUTTON_MIDDLE;
            dispatch.buttonMask = kMouseButtonMiddleMask;
            break;
        case kXrdpWmXButton1Down:
            dispatch.action = MOUSE_ACTION_BUTTON_DOWN;
            dispatch.button = MOUSE_BUTTON_BACK;
            dispatch.buttonMask = kMouseButtonBackMask;
            dispatch.buttonDown = true;
            break;
        case kXrdpWmXButton1Up:
            dispatch.action = MOUSE_ACTION_BUTTON_UP;
            dispatch.button = MOUSE_BUTTON_BACK;
            dispatch.buttonMask = kMouseButtonBackMask;
            break;
        case kXrdpWmXButton2Down:
            dispatch.action = MOUSE_ACTION_BUTTON_DOWN;
            dispatch.button = MOUSE_BUTTON_FORWARD;
            dispatch.buttonMask = kMouseButtonForwardMask;
            dispatch.buttonDown = true;
            break;
        case kXrdpWmXButton2Up:
            dispatch.action = MOUSE_ACTION_BUTTON_UP;
            dispatch.button = MOUSE_BUTTON_FORWARD;
            dispatch.buttonMask = kMouseButtonForwardMask;
            break;
        case kXrdpWmWheelUpUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_VERTICAL;
            dispatch.axisValue = static_cast<float>(kWheelStep);
            break;
        case kXrdpWmWheelDownUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_VERTICAL;
            dispatch.axisValue = -static_cast<float>(kWheelStep);
            break;
        case kXrdpWmHWheelLeftUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = -static_cast<float>(kWheelStep);
            break;
        case kXrdpWmHWheelRightUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = static_cast<float>(kWheelStep);
            break;
        case kXrdpWmTouchVScroll:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_VERTICAL;
            dispatch.axisValue = static_cast<float>(event.param3);
            break;
        case kXrdpWmTouchHScroll:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = static_cast<float>(event.param3);
            break;
        case kXrdpWmWheelUpDown:
        case kXrdpWmWheelDownDown:
        case kXrdpWmHWheelLeftDown:
        case kXrdpWmHWheelRightDown:
            dispatch.supported = false;
            break;
        default:
            dispatch.supported = false;
            break;
    }

    if (dispatch.wheel) {
        dispatch.action = MOUSE_ACTION_AXIS_UPDATE;
        dispatch.button = MOUSE_BUTTON_NONE;
    }
    return dispatch;
}

bool InjectMouseEvent(const XrdpOhosInputEvent& event, const MouseDispatch& dispatch, std::string& message)
{
    Input_MouseEvent* mouseEvent = OH_Input_CreateMouseEvent();
    if (mouseEvent == nullptr) {
        message = "xrdp input mouse event allocation failed";
        return false;
    }

    const int32_t x = ClampCoordinate(event.param1);
    const int32_t y = ClampCoordinate(event.param2);
    OH_Input_SetMouseEventAction(mouseEvent, dispatch.action);
    OH_Input_SetMouseEventDisplayX(mouseEvent, x);
    OH_Input_SetMouseEventDisplayY(mouseEvent, y);
    OH_Input_SetMouseEventGlobalX(mouseEvent, x);
    OH_Input_SetMouseEventGlobalY(mouseEvent, y);
    OH_Input_SetMouseEventButton(mouseEvent, dispatch.button);
    OH_Input_SetMouseEventActionTime(mouseEvent, NowMs());
    if (dispatch.wheel) {
        OH_Input_SetMouseEventAxisType(mouseEvent, dispatch.axisType);
        OH_Input_SetMouseEventAxisValue(mouseEvent, dispatch.axisValue);
    }

    const int32_t rc = OH_Input_InjectMouseEventGlobal(mouseEvent);
    OH_Input_DestroyMouseEvent(&mouseEvent);
    message = "xrdp mouse inject msg=" + std::to_string(event.msg) +
        " action=" + std::to_string(dispatch.action) +
        " button=" + std::to_string(dispatch.button) +
        " x=" + std::to_string(x) +
        " y=" + std::to_string(y) +
        " rc=" + std::to_string(rc);
    return rc == INPUT_SUCCESS;
}

bool InjectKeyEvent(const XrdpOhosInputEvent& event, int32_t keyCode, std::string& message)
{
    Input_KeyEvent* keyEvent = OH_Input_CreateKeyEvent();
    if (keyEvent == nullptr) {
        message = "xrdp input key event allocation failed";
        return false;
    }

    const bool down = event.msg == kXrdpWmKeyDown;
    OH_Input_SetKeyEventAction(keyEvent, down ? KEY_ACTION_DOWN : KEY_ACTION_UP);
    OH_Input_SetKeyEventKeyCode(keyEvent, keyCode);
    OH_Input_SetKeyEventActionTime(keyEvent, NowMs());

    const int32_t rc = OH_Input_InjectKeyEvent(keyEvent);
    OH_Input_DestroyKeyEvent(&keyEvent);
    message = "xrdp key inject keyCode=" + std::to_string(keyCode) +
        (down ? " down" : " up") +
        " scancode=" + std::to_string(event.param3) +
        " keysym=" + std::to_string(event.param2) +
        " rc=" + std::to_string(rc);
    return rc == INPUT_SUCCESS;
}

void EmitInputResult(const std::string& message, bool ok, bool important)
{
    static std::atomic<uint32_t> logCount { 0 };
    const uint32_t count = logCount.fetch_add(1) + 1U;
    if (important || !ok || count <= 60U || (count % 200U) == 0U) {
        if (ok) {
            EmitHilogInfo(message);
        } else {
            EmitHilogError(message);
        }
    }
}

class XrdpInputDispatcher {
public:
    bool Enqueue(const XrdpOhosInputEvent& event, std::string& message)
    {
        if (!IsInputEvent(event)) {
            message = "xrdp input event ignored msg=" + std::to_string(event.msg);
            return false;
        }

        Start();
        std::lock_guard<std::mutex> lock(mutex_);
        if (IsMouseMove(event) && !queue_.empty() && IsMouseMove(queue_.back())) {
            queue_.back() = event;
            message = "xrdp input mouse move coalesced";
            condition_.notify_one();
            return true;
        }
        if (queue_.size() >= kMaxInputQueue) {
            if (IsMouseMove(event)) {
                message = "xrdp input queue full; mouse move dropped";
                dropped_.fetch_add(1U);
                return false;
            }
            DropOldestMoveLocked();
        }
        if (queue_.size() >= kMaxInputQueue) {
            message = "xrdp input queue full; event dropped msg=" + std::to_string(event.msg);
            dropped_.fetch_add(1U);
            return false;
        }

        queue_.push_back(event);
        queued_.fetch_add(1U);
        message = "xrdp input event queued msg=" + std::to_string(event.msg);
        condition_.notify_one();
        return true;
    }

    void Reset(const std::string& reason)
    {
        Start();
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.clear();
        resetRequested_ = true;
        resetReason_ = reason;
        condition_.notify_one();
    }

private:
    void Start()
    {
        bool expected = false;
        if (!started_.compare_exchange_strong(expected, true)) {
            return;
        }
        worker_ = std::thread([this]() {
            Run();
        });
        worker_.detach();
        EmitHilogInfo("xrdp input dispatcher started");
    }

    void DropOldestMoveLocked()
    {
        for (auto iter = queue_.begin(); iter != queue_.end(); ++iter) {
            if (IsMouseMove(*iter)) {
                queue_.erase(iter);
                dropped_.fetch_add(1U);
                return;
            }
        }
    }

    void Run()
    {
        while (true) {
            XrdpOhosInputEvent event;
            bool hasEvent = false;
            bool reset = false;
            std::string resetReason;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() {
                    return resetRequested_ || !queue_.empty();
                });
                if (resetRequested_) {
                    reset = true;
                    resetRequested_ = false;
                    resetReason.swap(resetReason_);
                } else if (!queue_.empty()) {
                    lock.unlock();
                    std::string authMessage;
                    if (!EnsureInjectionAuthorized(authMessage)) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(50));
                        continue;
                    }
                    lock.lock();
                    if (resetRequested_) {
                        reset = true;
                        resetRequested_ = false;
                        resetReason.swap(resetReason_);
                    } else if (!queue_.empty()) {
                        event = queue_.front();
                        queue_.pop_front();
                        hasEvent = true;
                    }
                }
            }

            if (reset) {
                ReleasePressedState(resetReason);
                continue;
            }
            if (hasEvent) {
                ProcessEvent(event);
            }
        }
    }

    void ReleasePressedState(const std::string& reason)
    {
        std::string authMessage;
        if (!EnsureInjectionAuthorized(authMessage)) {
            pressedKeys_.clear();
            pressedButtons_ = 0;
            EmitHilogInfo("xrdp input reset without injection authorization: " + reason);
            return;
        }

        for (int32_t keyCode : pressedKeys_) {
            XrdpOhosInputEvent event;
            event.msg = kXrdpWmKeyUp;
            std::string message;
            InjectKeyEvent(event, keyCode, message);
        }
        pressedKeys_.clear();

        ReleaseMouseButton(kMouseButtonLeftMask, MOUSE_BUTTON_LEFT, reason);
        ReleaseMouseButton(kMouseButtonMiddleMask, MOUSE_BUTTON_MIDDLE, reason);
        ReleaseMouseButton(kMouseButtonRightMask, MOUSE_BUTTON_RIGHT, reason);
        ReleaseMouseButton(kMouseButtonForwardMask, MOUSE_BUTTON_FORWARD, reason);
        ReleaseMouseButton(kMouseButtonBackMask, MOUSE_BUTTON_BACK, reason);
        pressedButtons_ = 0;
        EmitHilogInfo("xrdp input pressed state reset: " + reason);
    }

    void ReleaseMouseButton(uint32_t mask, int32_t button, const std::string&)
    {
        if ((pressedButtons_ & mask) == 0U) {
            return;
        }
        XrdpOhosInputEvent event;
        event.msg = kXrdpWmLeftButtonUp;
        event.param1 = lastMouseX_;
        event.param2 = lastMouseY_;
        MouseDispatch dispatch;
        dispatch.supported = true;
        dispatch.action = MOUSE_ACTION_BUTTON_UP;
        dispatch.button = button;
        std::string message;
        InjectMouseEvent(event, dispatch, message);
    }

    void ProcessEvent(const XrdpOhosInputEvent& event)
    {
        std::string message;
        bool ok = false;
        bool important = false;
        if (event.msg == kXrdpWmKeyDown || event.msg == kXrdpWmKeyUp) {
            const int32_t keyCode = MapXrdpKeyToOhosKeyCode(event);
            if (keyCode < 0) {
                EmitInputResult("xrdp key inject skipped: unmapped scancode=" +
                    std::to_string(event.param3) + " keysym=" + std::to_string(event.param2),
                    false, true);
                return;
            }
            ok = InjectKeyEvent(event, keyCode, message);
            if (ok) {
                if (event.msg == kXrdpWmKeyDown) {
                    pressedKeys_.insert(keyCode);
                } else {
                    pressedKeys_.erase(keyCode);
                }
            }
            important = keyCode == KEYCODE_ENTER || keyCode == KEYCODE_DEL ||
                keyCode == KEYCODE_FORWARD_DEL || keyCode == KEYCODE_CTRL_LEFT ||
                keyCode == KEYCODE_CTRL_RIGHT || keyCode == KEYCODE_ALT_LEFT ||
                keyCode == KEYCODE_ALT_RIGHT;
        } else {
            MouseDispatch dispatch = MapMouseEvent(event);
            if (!dispatch.supported) {
                return;
            }
            lastMouseX_ = ClampCoordinate(event.param1);
            lastMouseY_ = ClampCoordinate(event.param2);
            ok = InjectMouseEvent(event, dispatch, message);
            if (ok && dispatch.buttonMask != 0U) {
                if (dispatch.buttonDown) {
                    pressedButtons_ |= dispatch.buttonMask;
                } else {
                    pressedButtons_ &= ~dispatch.buttonMask;
                }
            }
            important = event.msg != kXrdpWmMouseMove;
        }

        if (ok) {
            sent_.fetch_add(1U);
        } else {
            dropped_.fetch_add(1U);
        }
        EmitInputResult(message + " queued=" + std::to_string(queued_.load()) +
            " sent=" + std::to_string(sent_.load()) +
            " dropped=" + std::to_string(dropped_.load()), ok, important);
    }

    std::atomic<bool> started_ { false };
    std::thread worker_;
    std::mutex mutex_;
    std::condition_variable condition_;
    std::deque<XrdpOhosInputEvent> queue_;
    bool resetRequested_ = false;
    std::string resetReason_;
    std::set<int32_t> pressedKeys_;
    uint32_t pressedButtons_ = 0;
    int32_t lastMouseX_ = 0;
    int32_t lastMouseY_ = 0;
    std::atomic<uint32_t> queued_ { 0 };
    std::atomic<uint32_t> sent_ { 0 };
    std::atomic<uint32_t> dropped_ { 0 };
};

XrdpInputDispatcher& InputDispatcher()
{
    static XrdpInputDispatcher dispatcher;
    return dispatcher;
}

} // namespace

bool DispatchXrdpInputEvent(const XrdpOhosInputEvent& event, std::string& message)
{
    return InputDispatcher().Enqueue(event, message);
}

void PrimeXrdpInputInjectorAuthorization(const std::string& reason)
{
    std::thread([reason]() {
        std::string message;
        if (EnsureInjectionAuthorized(message)) {
            EmitHilogInfo("xrdp input injection authorization ready: " + reason);
        }
    }).detach();
}

void ResetXrdpInputInjector(const std::string& reason)
{
    InputDispatcher().Reset(reason);
}

} // namespace rdp_bridge
