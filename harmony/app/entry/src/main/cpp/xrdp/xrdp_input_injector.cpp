#include "xrdp/xrdp_input_injector.h"

#include "common/bridge_log.h"
#include "xrdp/xrdp_display_geometry.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <cmath>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include <multimodalinput/oh_input_manager.h>
#include <multimodalinput/oh_key_code.h>

namespace rdp_bridge {
namespace {

constexpr int32_t kXrdpEventSessionConnect = XRDP_OHOS_INPUT_SESSION_CONNECT;
constexpr int32_t kXrdpEventSessionDisconnect = XRDP_OHOS_INPUT_SESSION_DISCONNECT;
constexpr int32_t kXrdpWmKeyDown = XRDP_OHOS_WM_KEYDOWN;
constexpr int32_t kXrdpWmKeyUp = XRDP_OHOS_WM_KEYUP;
constexpr int32_t kXrdpWmMouseMove = XRDP_OHOS_WM_MOUSEMOVE;
constexpr int32_t kXrdpWmLeftButtonUp = XRDP_OHOS_WM_LBUTTONUP;
constexpr int32_t kXrdpWmLeftButtonDown = XRDP_OHOS_WM_LBUTTONDOWN;
constexpr int32_t kXrdpWmRightButtonUp = XRDP_OHOS_WM_RBUTTONUP;
constexpr int32_t kXrdpWmRightButtonDown = XRDP_OHOS_WM_RBUTTONDOWN;
constexpr int32_t kXrdpWmMiddleButtonUp = XRDP_OHOS_WM_MBUTTONUP;
constexpr int32_t kXrdpWmMiddleButtonDown = XRDP_OHOS_WM_MBUTTONDOWN;
constexpr int32_t kXrdpWmWheelUpDown = XRDP_OHOS_WM_WHEELUPDOWN;
constexpr int32_t kXrdpWmWheelUpUp = XRDP_OHOS_WM_WHEELUPUP;
constexpr int32_t kXrdpWmWheelDownDown = XRDP_OHOS_WM_WHEELDOWNDOWN;
constexpr int32_t kXrdpWmWheelDownUp = XRDP_OHOS_WM_WHEELDOWNUP;
constexpr int32_t kXrdpWmHWheelLeftDown = XRDP_OHOS_WM_HWHEELLEFTDOWN;
constexpr int32_t kXrdpWmHWheelLeftUp = XRDP_OHOS_WM_HWHEELLEFTUP;
constexpr int32_t kXrdpWmHWheelRightDown = XRDP_OHOS_WM_HWHEELRIGHTDOWN;
constexpr int32_t kXrdpWmHWheelRightUp = XRDP_OHOS_WM_HWHEELRIGHTUP;
constexpr int32_t kXrdpWmXButton1Up = XRDP_OHOS_WM_XBUTTON1UP;
constexpr int32_t kXrdpWmXButton1Down = XRDP_OHOS_WM_XBUTTON1DOWN;
constexpr int32_t kXrdpWmXButton2Up = XRDP_OHOS_WM_XBUTTON2UP;
constexpr int32_t kXrdpWmXButton2Down = XRDP_OHOS_WM_XBUTTON2DOWN;
constexpr int32_t kXrdpWmTouchVScroll = XRDP_OHOS_WM_TOUCH_VSCROLL;
constexpr int32_t kXrdpWmTouchHScroll = XRDP_OHOS_WM_TOUCH_HSCROLL;
constexpr long kXrdpKeysymBackspace = 65288;
constexpr long kXrdpKeysymTab = 65289;
constexpr long kXrdpKeysymReturn = 65293;
constexpr long kXrdpKeysymEscape = 65307;
constexpr long kXrdpKeysymHome = 65360;
constexpr long kXrdpKeysymLeft = 65361;
constexpr long kXrdpKeysymUp = 65362;
constexpr long kXrdpKeysymRight = 65363;
constexpr long kXrdpKeysymDown = 65364;
constexpr long kXrdpKeysymPageUp = 65365;
constexpr long kXrdpKeysymPageDown = 65366;
constexpr long kXrdpKeysymEnd = 65367;
constexpr long kXrdpKeysymInsert = 65379;
constexpr long kXrdpKeysymDelete = 65535;
constexpr long kXrdpKeysymShiftLeft = 65505;
constexpr long kXrdpKeysymShiftRight = 65506;
constexpr long kXrdpKeysymCtrlLeft = 65507;
constexpr long kXrdpKeysymCtrlRight = 65508;
constexpr long kXrdpKeysymCapsLock = 65509;
constexpr long kXrdpKeysymAltLeft = 65513;
constexpr long kXrdpKeysymAltRight = 65514;
constexpr long kXrdpKeysymMetaLeft = 65511;
constexpr long kXrdpKeysymMetaRight = 65512;
constexpr long kXrdpKeysymSuperLeft = 65515;
constexpr long kXrdpKeysymSuperRight = 65516;
constexpr long kXrdpKeysymF1 = 65470;
constexpr int32_t kMaxInputQueue = 512;
constexpr int32_t kWheelStep = 120;
constexpr uint32_t kMouseButtonLeftMask = 1U;
constexpr uint32_t kMouseButtonMiddleMask = 1U << 1U;
constexpr uint32_t kMouseButtonRightMask = 1U << 2U;
constexpr uint32_t kMouseButtonForwardMask = 1U << 3U;
constexpr uint32_t kMouseButtonBackMask = 1U << 4U;
constexpr uint32_t kMouseButtonMaskAll = kMouseButtonLeftMask | kMouseButtonMiddleMask |
    kMouseButtonRightMask | kMouseButtonForwardMask | kMouseButtonBackMask;

std::atomic<Input_InjectionStatus> g_authorizedStatus { UNAUTHORIZED };
std::atomic<bool> g_authorizationRequested { false };
std::atomic<uint32_t> g_authorizationLogCount { 0 };
std::mutex g_authorizationMutex;
std::chrono::steady_clock::time_point g_lastAuthorizationRequest;

struct MouseCoordinates {
    int32_t displayId = 0;
    int32_t displayX = 0;
    int32_t displayY = 0;
    int32_t globalX = 0;
    int32_t globalY = 0;
    int32_t sourceWidth = 0;
    int32_t sourceHeight = 0;
    int32_t targetWidth = 0;
    int32_t targetHeight = 0;
    bool contentRectValid = false;
    int32_t contentLeft = 0;
    int32_t contentTop = 0;
    int32_t contentWidth = 0;
    int32_t contentHeight = 0;
    bool scaled = false;
    bool availableValid = false;
    int32_t availableLeft = 0;
    int32_t availableTop = 0;
    uint32_t availableWidth = 0;
    uint32_t availableHeight = 0;
    bool virtualPixelRatioValid = false;
    float virtualPixelRatio = 0.0F;
};

std::mutex g_displayGeometryMutex;
XrdpDisplayGeometry g_displayGeometry;
std::chrono::steady_clock::time_point g_lastDisplayGeometryQuery;

int64_t NowMs()
{
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

int32_t ClampToRange(long value, int32_t minValue, int32_t maxValue)
{
    if (maxValue < minValue) {
        return minValue;
    }
    if (value < minValue) {
        return minValue;
    }
    if (value > maxValue) {
        return maxValue;
    }
    return static_cast<int32_t>(value);
}

int32_t ScaleCoordinate(long value, int32_t sourceSize, int32_t targetSize)
{
    if (targetSize <= 1) {
        return 0;
    }
    if (sourceSize <= 1) {
        return ClampToRange(value, 0, targetSize - 1);
    }
    const int32_t source = ClampToRange(value, 0, sourceSize - 1);
    const double ratio = static_cast<double>(source) / static_cast<double>(sourceSize - 1);
    return ClampToRange(static_cast<long>(std::llround(ratio * static_cast<double>(targetSize - 1))),
        0, targetSize - 1);
}

int32_t ScaleCoordinateFromContent(long value, int32_t contentStart, int32_t contentSize, int32_t targetSize)
{
    if (targetSize <= 1) {
        return 0;
    }
    if (contentSize <= 1) {
        return ClampToRange(value - contentStart, 0, targetSize - 1);
    }
    const int32_t source = ClampToRange(value - contentStart, 0, contentSize - 1);
    const double ratio = static_cast<double>(source) / static_cast<double>(contentSize - 1);
    return ClampToRange(static_cast<long>(std::llround(ratio * static_cast<double>(targetSize - 1))),
        0, targetSize - 1);
}

void ResolveContentRect(int32_t sourceWidth, int32_t sourceHeight, int32_t targetWidth, int32_t targetHeight,
    MouseCoordinates& coordinates)
{
    coordinates.contentRectValid = true;
    coordinates.contentLeft = 0;
    coordinates.contentTop = 0;
    coordinates.contentWidth = sourceWidth;
    coordinates.contentHeight = sourceHeight;

    if (sourceWidth <= 0 || sourceHeight <= 0 || targetWidth <= 0 || targetHeight <= 0) {
        coordinates.contentRectValid = false;
        return;
    }

    const double scaleX = static_cast<double>(sourceWidth) / static_cast<double>(targetWidth);
    const double scaleY = static_cast<double>(sourceHeight) / static_cast<double>(targetHeight);
    const double scale = std::min(scaleX, scaleY);
    if (scale <= 0.0) {
        return;
    }

    coordinates.contentWidth = ClampToRange(
        static_cast<long>(std::llround(static_cast<double>(targetWidth) * scale)), 1, sourceWidth);
    coordinates.contentHeight = ClampToRange(
        static_cast<long>(std::llround(static_cast<double>(targetHeight) * scale)), 1, sourceHeight);
    coordinates.contentLeft = (sourceWidth - coordinates.contentWidth) / 2;
    coordinates.contentTop = (sourceHeight - coordinates.contentHeight) / 2;
}

XrdpDisplayGeometry GetDisplayGeometry()
{
    const auto now = std::chrono::steady_clock::now();
    std::lock_guard<std::mutex> lock(g_displayGeometryMutex);
    if (g_displayGeometry.valid &&
        now - g_lastDisplayGeometryQuery < std::chrono::seconds(1)) {
        return g_displayGeometry;
    }

    const XrdpDisplayGeometry previous = g_displayGeometry;
    g_displayGeometry = QueryXrdpDisplayGeometry();
    g_lastDisplayGeometryQuery = now;
    if (g_displayGeometry.valid &&
        (!previous.valid || previous.displayId != g_displayGeometry.displayId ||
            previous.width != g_displayGeometry.width || previous.height != g_displayGeometry.height ||
            previous.originX != g_displayGeometry.originX || previous.originY != g_displayGeometry.originY ||
            previous.availableValid != g_displayGeometry.availableValid ||
            previous.availableLeft != g_displayGeometry.availableLeft ||
            previous.availableTop != g_displayGeometry.availableTop ||
            previous.availableWidth != g_displayGeometry.availableWidth ||
            previous.availableHeight != g_displayGeometry.availableHeight ||
            previous.virtualPixelRatioValid != g_displayGeometry.virtualPixelRatioValid ||
            previous.virtualPixelRatio != g_displayGeometry.virtualPixelRatio ||
            previous.sourceModeValid != g_displayGeometry.sourceModeValid ||
            previous.sourceMode != g_displayGeometry.sourceMode)) {
        EmitHilogInfo("xrdp input " + FormatXrdpDisplayGeometry(g_displayGeometry));
    }
    return g_displayGeometry;
}

MouseCoordinates ResolveMouseCoordinates(const XrdpOhosInputEvent& event)
{
    MouseCoordinates coordinates;
    const XrdpDisplayGeometry geometry = GetDisplayGeometry();
    if (!geometry.valid) {
        coordinates.displayX = ClampToRange(event.param1, 0, 32767);
        coordinates.displayY = ClampToRange(event.param2, 0, 32767);
        coordinates.globalX = coordinates.displayX;
        coordinates.globalY = coordinates.displayY;
        return coordinates;
    }

    const int32_t sourceWidth = event.width > 0 ? event.width : geometry.width;
    const int32_t sourceHeight = event.height > 0 ? event.height : geometry.height;
    coordinates.displayId = static_cast<int32_t>(geometry.displayId);
    ResolveContentRect(sourceWidth, sourceHeight, geometry.width, geometry.height, coordinates);
    if (coordinates.contentRectValid) {
        coordinates.displayX = ScaleCoordinateFromContent(event.param1, coordinates.contentLeft,
            coordinates.contentWidth, geometry.width);
        coordinates.displayY = ScaleCoordinateFromContent(event.param2, coordinates.contentTop,
            coordinates.contentHeight, geometry.height);
    } else {
        coordinates.displayX = ScaleCoordinate(event.param1, sourceWidth, geometry.width);
        coordinates.displayY = ScaleCoordinate(event.param2, sourceHeight, geometry.height);
    }
    coordinates.globalX = geometry.originX + coordinates.displayX;
    coordinates.globalY = geometry.originY + coordinates.displayY;
    coordinates.sourceWidth = sourceWidth;
    coordinates.sourceHeight = sourceHeight;
    coordinates.targetWidth = geometry.width;
    coordinates.targetHeight = geometry.height;
    coordinates.scaled = sourceWidth != geometry.width || sourceHeight != geometry.height;
    coordinates.availableValid = geometry.availableValid;
    coordinates.availableLeft = geometry.availableLeft;
    coordinates.availableTop = geometry.availableTop;
    coordinates.availableWidth = geometry.availableWidth;
    coordinates.availableHeight = geometry.availableHeight;
    coordinates.virtualPixelRatioValid = geometry.virtualPixelRatioValid;
    coordinates.virtualPixelRatio = geometry.virtualPixelRatio;
    return coordinates;
}

void OnInjectionAuthorized(Input_InjectionStatus status)
{
    g_authorizedStatus.store(status);
    EmitHilogInfo("xrdp input injection authorization callback status=" + std::to_string(status));
}

bool EnsureInjectionAuthorized(std::string& message)
{
    if (g_authorizedStatus.load() == AUTHORIZED) {
        return true;
    }

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
        case kXrdpKeysymBackspace: return KEYCODE_DEL;
        case kXrdpKeysymTab: return KEYCODE_TAB;
        case kXrdpKeysymReturn: return KEYCODE_ENTER;
        case kXrdpKeysymEscape: return KEYCODE_ESCAPE;
        case kXrdpKeysymHome: return KEYCODE_MOVE_HOME;
        case kXrdpKeysymLeft: return KEYCODE_DPAD_LEFT;
        case kXrdpKeysymUp: return KEYCODE_DPAD_UP;
        case kXrdpKeysymRight: return KEYCODE_DPAD_RIGHT;
        case kXrdpKeysymDown: return KEYCODE_DPAD_DOWN;
        case kXrdpKeysymPageUp: return KEYCODE_PAGE_UP;
        case kXrdpKeysymPageDown: return KEYCODE_PAGE_DOWN;
        case kXrdpKeysymEnd: return KEYCODE_MOVE_END;
        case kXrdpKeysymInsert: return KEYCODE_INSERT;
        case kXrdpKeysymDelete: return KEYCODE_FORWARD_DEL;
        case kXrdpKeysymShiftLeft: return KEYCODE_SHIFT_LEFT;
        case kXrdpKeysymShiftRight: return KEYCODE_SHIFT_RIGHT;
        case kXrdpKeysymCtrlLeft: return KEYCODE_CTRL_LEFT;
        case kXrdpKeysymCtrlRight: return KEYCODE_CTRL_RIGHT;
        case kXrdpKeysymCapsLock: return KEYCODE_CAPS_LOCK;
        case kXrdpKeysymAltLeft: return KEYCODE_ALT_LEFT;
        case kXrdpKeysymAltRight: return KEYCODE_ALT_RIGHT;
        case kXrdpKeysymMetaLeft:
        case kXrdpKeysymSuperLeft: return KEYCODE_META_LEFT;
        case kXrdpKeysymMetaRight:
        case kXrdpKeysymSuperRight: return KEYCODE_META_RIGHT;
        default:
            if (keysym >= kXrdpKeysymF1 && keysym < kXrdpKeysymF1 + 12) {
                return KEYCODE_F1 + static_cast<int32_t>(keysym - kXrdpKeysymF1);
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

bool IsMouseButtonEvent(const XrdpOhosInputEvent& event)
{
    return event.msg >= kXrdpWmLeftButtonUp && event.msg <= kXrdpWmXButton2Down;
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
    bool beginAxisBeforeUpdate = false;
    bool endAxisAfterUpdate = false;
    uint32_t buttonMask = 0;
    bool buttonDown = false;
};

int32_t ActiveMouseButtonFromMask(uint32_t mask)
{
    if ((mask & kMouseButtonLeftMask) != 0U) {
        return MOUSE_BUTTON_LEFT;
    }
    if ((mask & kMouseButtonMiddleMask) != 0U) {
        return MOUSE_BUTTON_MIDDLE;
    }
    if ((mask & kMouseButtonRightMask) != 0U) {
        return MOUSE_BUTTON_RIGHT;
    }
    if ((mask & kMouseButtonForwardMask) != 0U) {
        return MOUSE_BUTTON_FORWARD;
    }
    if ((mask & kMouseButtonBackMask) != 0U) {
        return MOUSE_BUTTON_BACK;
    }
    return MOUSE_BUTTON_NONE;
}

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
            dispatch.axisValue = -static_cast<float>(kWheelStep);
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmWheelDownUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_VERTICAL;
            dispatch.axisValue = static_cast<float>(kWheelStep);
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmHWheelLeftUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = static_cast<float>(kWheelStep);
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmHWheelRightUp:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = -static_cast<float>(kWheelStep);
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmTouchVScroll:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_VERTICAL;
            dispatch.axisValue = static_cast<float>(event.param3);
            dispatch.beginAxisBeforeUpdate = true;
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmTouchHScroll:
            dispatch.wheel = true;
            dispatch.axisType = MOUSE_AXIS_SCROLL_HORIZONTAL;
            dispatch.axisValue = static_cast<float>(event.param3);
            dispatch.beginAxisBeforeUpdate = true;
            dispatch.endAxisAfterUpdate = true;
            break;
        case kXrdpWmWheelUpDown:
        case kXrdpWmWheelDownDown:
        case kXrdpWmHWheelLeftDown:
        case kXrdpWmHWheelRightDown:
            dispatch.wheel = true;
            dispatch.action = MOUSE_ACTION_AXIS_BEGIN;
            dispatch.axisValue = 0.0F;
            break;
        default:
            dispatch.supported = false;
            break;
    }

    if (dispatch.wheel && dispatch.action != MOUSE_ACTION_AXIS_BEGIN) {
        dispatch.action = MOUSE_ACTION_AXIS_UPDATE;
        dispatch.button = MOUSE_BUTTON_NONE;
    }
    return dispatch;
}

int32_t InjectMouseEventOnce(const MouseDispatch& dispatch, const MouseCoordinates& coordinates,
    int32_t action, float axisValue, std::string& message)
{
    Input_MouseEvent* mouseEvent = OH_Input_CreateMouseEvent();
    if (mouseEvent == nullptr) {
        message = "xrdp input mouse event allocation failed";
        return INPUT_PARAMETER_ERROR;
    }

    OH_Input_SetMouseEventAction(mouseEvent, action);
    OH_Input_SetMouseEventDisplayX(mouseEvent, coordinates.displayX);
    OH_Input_SetMouseEventDisplayY(mouseEvent, coordinates.displayY);
    OH_Input_SetMouseEventDisplayId(mouseEvent, coordinates.displayId);
    OH_Input_SetMouseEventGlobalX(mouseEvent, coordinates.globalX);
    OH_Input_SetMouseEventGlobalY(mouseEvent, coordinates.globalY);
    OH_Input_SetMouseEventButton(mouseEvent, dispatch.button);
    OH_Input_SetMouseEventActionTime(mouseEvent, NowMs());
    if (dispatch.wheel) {
        OH_Input_SetMouseEventAxisType(mouseEvent, dispatch.axisType);
        OH_Input_SetMouseEventAxisValue(mouseEvent, axisValue);
    }

    const int32_t rc = OH_Input_InjectMouseEventGlobal(mouseEvent);
    OH_Input_DestroyMouseEvent(&mouseEvent);
    return rc;
}

bool InjectMouseEvent(const XrdpOhosInputEvent& event, const MouseDispatch& dispatch,
    const MouseCoordinates& coordinates, std::string& message)
{
    int32_t beginRc = INPUT_SUCCESS;
    if (dispatch.beginAxisBeforeUpdate) {
        beginRc = InjectMouseEventOnce(dispatch, coordinates, MOUSE_ACTION_AXIS_BEGIN, 0.0F, message);
    }
    const int32_t rc = beginRc == INPUT_SUCCESS ?
        InjectMouseEventOnce(dispatch, coordinates, dispatch.action, dispatch.axisValue, message) :
        beginRc;
    int32_t endRc = INPUT_SUCCESS;
    if (rc == INPUT_SUCCESS && dispatch.endAxisAfterUpdate) {
        endRc = InjectMouseEventOnce(dispatch, coordinates, MOUSE_ACTION_AXIS_END, 0.0F, message);
    }

    message = "xrdp mouse inject msg=" + std::to_string(event.msg) +
        " action=" + std::to_string(dispatch.action) +
        " button=" + std::to_string(dispatch.button) +
        " remote=(" + std::to_string(event.param1) +
        "," + std::to_string(event.param2) + ")" +
        " display=(" + std::to_string(coordinates.displayX) +
        "," + std::to_string(coordinates.displayY) + ")" +
        " source=" + std::to_string(coordinates.sourceWidth) +
        "x" + std::to_string(coordinates.sourceHeight) +
        " target=" + std::to_string(coordinates.targetWidth) +
        "x" + std::to_string(coordinates.targetHeight);
    if (coordinates.contentRectValid) {
        message += " content=(" + std::to_string(coordinates.contentLeft) +
            "," + std::to_string(coordinates.contentTop) +
            "," + std::to_string(coordinates.contentWidth) +
            "," + std::to_string(coordinates.contentHeight) + ")";
    }
    if (coordinates.availableValid) {
        message += " available=(" + std::to_string(coordinates.availableLeft) +
            "," + std::to_string(coordinates.availableTop) +
            "," + std::to_string(coordinates.availableWidth) +
            "," + std::to_string(coordinates.availableHeight) + ")";
    }
    if (coordinates.virtualPixelRatioValid) {
        message += " vpr=" + std::to_string(coordinates.virtualPixelRatio);
    }
    if (dispatch.wheel) {
        message += " axisType=" + std::to_string(dispatch.axisType) +
            " axisValue=" + std::to_string(dispatch.axisValue);
        if (dispatch.beginAxisBeforeUpdate) {
            message += " beginRc=" + std::to_string(beginRc);
        }
        if (dispatch.endAxisAfterUpdate) {
            message += " endRc=" + std::to_string(endRc);
        }
    }
    message += " rc=" + std::to_string(rc);
    if (beginRc == INPUT_PERMISSION_DENIED || rc == INPUT_PERMISSION_DENIED || endRc == INPUT_PERMISSION_DENIED) {
        g_authorizedStatus.store(UNAUTHORIZED);
        g_authorizationRequested.store(false);
    }
    return beginRc == INPUT_SUCCESS && rc == INPUT_SUCCESS && endRc == INPUT_SUCCESS;
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
    if (rc == INPUT_PERMISSION_DENIED) {
        g_authorizedStatus.store(UNAUTHORIZED);
        g_authorizationRequested.store(false);
    }
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
        if (IsMouseButtonEvent(event)) {
            DropTrailingMoveLocked();
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

    void DropTrailingMoveLocked()
    {
        if (!queue_.empty() && IsMouseMove(queue_.back())) {
            queue_.pop_back();
        }
    }

    void DropQueuedEventsLocked(const std::string& reason)
    {
        const size_t count = queue_.size();
        if (count == 0) {
            return;
        }
        queue_.clear();
        dropped_.fetch_add(static_cast<uint32_t>(count));
        const uint32_t logCount = queueClearLogCount_.fetch_add(1U) + 1U;
        if (logCount <= 10U || (logCount % 100U) == 0U) {
            EmitHilogInfo("xrdp input queue cleared: " + reason +
                " dropped=" + std::to_string(count));
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
                        lock.lock();
                        DropQueuedEventsLocked("injection authorization pending");
                        lock.unlock();
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
        InjectMouseEvent(event, dispatch, ResolveMouseCoordinates(event), message);
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
            if (IsMouseMove(event) && dispatch.button == MOUSE_BUTTON_NONE) {
                dispatch.button = ActiveMouseButtonFromMask(pressedButtons_ & kMouseButtonMaskAll);
            }
            const MouseCoordinates coordinates = ResolveMouseCoordinates(event);
            lastMouseX_ = coordinates.displayX;
            lastMouseY_ = coordinates.displayY;
            ok = InjectMouseEvent(event, dispatch, coordinates, message);
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
    std::atomic<uint32_t> queueClearLogCount_ { 0 };
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
