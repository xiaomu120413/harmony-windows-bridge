#include "input/ohos_keyboard_adapter.h"

#include <iomanip>
#include <sstream>

namespace rdp_bridge {
namespace {

constexpr uint32_t OH_KEYCODE_0 = 2000;
constexpr uint32_t OH_KEYCODE_9 = 2009;
constexpr uint32_t OH_KEYCODE_DPAD_UP = 2012;
constexpr uint32_t OH_KEYCODE_DPAD_DOWN = 2013;
constexpr uint32_t OH_KEYCODE_DPAD_LEFT = 2014;
constexpr uint32_t OH_KEYCODE_DPAD_RIGHT = 2015;
constexpr uint32_t OH_KEYCODE_A = 2017;
constexpr uint32_t OH_KEYCODE_Z = 2042;
constexpr uint32_t OH_KEYCODE_COMMA = 2043;
constexpr uint32_t OH_KEYCODE_PERIOD = 2044;
constexpr uint32_t OH_KEYCODE_ALT_LEFT = 2045;
constexpr uint32_t OH_KEYCODE_ALT_RIGHT = 2046;
constexpr uint32_t OH_KEYCODE_SHIFT_LEFT = 2047;
constexpr uint32_t OH_KEYCODE_SHIFT_RIGHT = 2048;
constexpr uint32_t OH_KEYCODE_TAB = 2049;
constexpr uint32_t OH_KEYCODE_SPACE = 2050;
constexpr uint32_t OH_KEYCODE_ENTER = 2054;
constexpr uint32_t OH_KEYCODE_DEL = 2055;
constexpr uint32_t OH_KEYCODE_GRAVE = 2056;
constexpr uint32_t OH_KEYCODE_MINUS = 2057;
constexpr uint32_t OH_KEYCODE_EQUALS = 2058;
constexpr uint32_t OH_KEYCODE_LEFT_BRACKET = 2059;
constexpr uint32_t OH_KEYCODE_RIGHT_BRACKET = 2060;
constexpr uint32_t OH_KEYCODE_BACKSLASH = 2061;
constexpr uint32_t OH_KEYCODE_SEMICOLON = 2062;
constexpr uint32_t OH_KEYCODE_APOSTROPHE = 2063;
constexpr uint32_t OH_KEYCODE_SLASH = 2064;
constexpr uint32_t OH_KEYCODE_PAGE_UP = 2068;
constexpr uint32_t OH_KEYCODE_PAGE_DOWN = 2069;
constexpr uint32_t OH_KEYCODE_ESCAPE = 2070;
constexpr uint32_t OH_KEYCODE_FORWARD_DEL = 2071;
constexpr uint32_t OH_KEYCODE_CTRL_LEFT = 2072;
constexpr uint32_t OH_KEYCODE_CTRL_RIGHT = 2073;
constexpr uint32_t OH_KEYCODE_META_LEFT = 2076;
constexpr uint32_t OH_KEYCODE_META_RIGHT = 2077;
constexpr uint32_t OH_KEYCODE_MOVE_HOME = 2081;
constexpr uint32_t OH_KEYCODE_MOVE_END = 2082;
constexpr uint32_t OH_KEYCODE_INSERT = 2083;
constexpr uint32_t OH_KEYCODE_F1 = 2090;
constexpr uint32_t OH_KEYCODE_F12 = 2101;
constexpr uint32_t OH_KEYCODE_NUMPAD_0 = 2103;
constexpr uint32_t OH_KEYCODE_NUMPAD_9 = 2112;
constexpr uint32_t OH_KEYCODE_NUMPAD_DIVIDE = 2113;
constexpr uint32_t OH_KEYCODE_NUMPAD_MULTIPLY = 2114;
constexpr uint32_t OH_KEYCODE_NUMPAD_SUBTRACT = 2115;
constexpr uint32_t OH_KEYCODE_NUMPAD_ADD = 2116;
constexpr uint32_t OH_KEYCODE_NUMPAD_DOT = 2117;
constexpr uint32_t OH_KEYCODE_NUMPAD_ENTER = 2119;

constexpr uint32_t VK_BACK = 0x08;
constexpr uint32_t VK_TAB = 0x09;
constexpr uint32_t VK_RETURN = 0x0D;
constexpr uint32_t VK_ESCAPE = 0x1B;
constexpr uint32_t VK_SPACE = 0x20;
constexpr uint32_t VK_PRIOR = 0x21;
constexpr uint32_t VK_NEXT = 0x22;
constexpr uint32_t VK_END = 0x23;
constexpr uint32_t VK_HOME = 0x24;
constexpr uint32_t VK_LEFT = 0x25;
constexpr uint32_t VK_UP = 0x26;
constexpr uint32_t VK_RIGHT = 0x27;
constexpr uint32_t VK_DOWN = 0x28;
constexpr uint32_t VK_INSERT = 0x2D;
constexpr uint32_t VK_DELETE = 0x2E;
constexpr uint32_t VK_KEY_0 = 0x30;
constexpr uint32_t VK_KEY_A = 0x41;
constexpr uint32_t VK_LWIN = 0x5B;
constexpr uint32_t VK_RWIN = 0x5C;
constexpr uint32_t VK_NUMPAD0 = 0x60;
constexpr uint32_t VK_MULTIPLY = 0x6A;
constexpr uint32_t VK_ADD = 0x6B;
constexpr uint32_t VK_SUBTRACT = 0x6D;
constexpr uint32_t VK_DECIMAL = 0x6E;
constexpr uint32_t VK_DIVIDE = 0x6F;
constexpr uint32_t VK_F1 = 0x70;
constexpr uint32_t VK_LSHIFT = 0xA0;
constexpr uint32_t VK_RSHIFT = 0xA1;
constexpr uint32_t VK_LCONTROL = 0xA2;
constexpr uint32_t VK_RCONTROL = 0xA3;
constexpr uint32_t VK_LMENU = 0xA4;
constexpr uint32_t VK_RMENU = 0xA5;
constexpr uint32_t VK_OEM_1 = 0xBA;
constexpr uint32_t VK_OEM_PLUS = 0xBB;
constexpr uint32_t VK_OEM_COMMA = 0xBC;
constexpr uint32_t VK_OEM_MINUS = 0xBD;
constexpr uint32_t VK_OEM_PERIOD = 0xBE;
constexpr uint32_t VK_OEM_2 = 0xBF;
constexpr uint32_t VK_OEM_3 = 0xC0;
constexpr uint32_t VK_OEM_4 = 0xDB;
constexpr uint32_t VK_OEM_5 = 0xDC;
constexpr uint32_t VK_OEM_6 = 0xDD;
constexpr uint32_t VK_OEM_7 = 0xDE;

const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

std::string Hex32(uint32_t value)
{
    std::ostringstream line;
    line << "0x" << std::hex << std::uppercase << value;
    return line.str();
}

} // namespace

uint32_t MapOhosKeyCodeToWindowsVk(uint32_t keyCode)
{
    if (keyCode >= OH_KEYCODE_A && keyCode <= OH_KEYCODE_Z) {
        return VK_KEY_A + keyCode - OH_KEYCODE_A;
    }
    if (keyCode >= OH_KEYCODE_0 && keyCode <= OH_KEYCODE_9) {
        return VK_KEY_0 + keyCode - OH_KEYCODE_0;
    }
    if (keyCode >= OH_KEYCODE_F1 && keyCode <= OH_KEYCODE_F12) {
        return VK_F1 + keyCode - OH_KEYCODE_F1;
    }
    if (keyCode >= OH_KEYCODE_NUMPAD_0 && keyCode <= OH_KEYCODE_NUMPAD_9) {
        return VK_NUMPAD0 + keyCode - OH_KEYCODE_NUMPAD_0;
    }

    switch (keyCode) {
        case OH_KEYCODE_DPAD_UP:
            return VK_UP;
        case OH_KEYCODE_DPAD_DOWN:
            return VK_DOWN;
        case OH_KEYCODE_DPAD_LEFT:
            return VK_LEFT;
        case OH_KEYCODE_DPAD_RIGHT:
            return VK_RIGHT;
        case OH_KEYCODE_COMMA:
            return VK_OEM_COMMA;
        case OH_KEYCODE_PERIOD:
            return VK_OEM_PERIOD;
        case OH_KEYCODE_ALT_LEFT:
            return VK_LMENU;
        case OH_KEYCODE_ALT_RIGHT:
            return VK_RMENU;
        case OH_KEYCODE_SHIFT_LEFT:
            return VK_LSHIFT;
        case OH_KEYCODE_SHIFT_RIGHT:
            return VK_RSHIFT;
        case OH_KEYCODE_TAB:
            return VK_TAB;
        case OH_KEYCODE_SPACE:
            return VK_SPACE;
        case OH_KEYCODE_ENTER:
        case OH_KEYCODE_NUMPAD_ENTER:
            return VK_RETURN;
        case OH_KEYCODE_DEL:
            return VK_BACK;
        case OH_KEYCODE_GRAVE:
            return VK_OEM_3;
        case OH_KEYCODE_MINUS:
            return VK_OEM_MINUS;
        case OH_KEYCODE_EQUALS:
            return VK_OEM_PLUS;
        case OH_KEYCODE_LEFT_BRACKET:
            return VK_OEM_4;
        case OH_KEYCODE_RIGHT_BRACKET:
            return VK_OEM_6;
        case OH_KEYCODE_BACKSLASH:
            return VK_OEM_5;
        case OH_KEYCODE_SEMICOLON:
            return VK_OEM_1;
        case OH_KEYCODE_APOSTROPHE:
            return VK_OEM_7;
        case OH_KEYCODE_SLASH:
            return VK_OEM_2;
        case OH_KEYCODE_PAGE_UP:
            return VK_PRIOR;
        case OH_KEYCODE_PAGE_DOWN:
            return VK_NEXT;
        case OH_KEYCODE_ESCAPE:
            return VK_ESCAPE;
        case OH_KEYCODE_FORWARD_DEL:
            return VK_DELETE;
        case OH_KEYCODE_CTRL_LEFT:
            return VK_LCONTROL;
        case OH_KEYCODE_CTRL_RIGHT:
            return VK_RCONTROL;
        case OH_KEYCODE_META_LEFT:
            return VK_LWIN;
        case OH_KEYCODE_META_RIGHT:
            return VK_RWIN;
        case OH_KEYCODE_MOVE_HOME:
            return VK_HOME;
        case OH_KEYCODE_MOVE_END:
            return VK_END;
        case OH_KEYCODE_INSERT:
            return VK_INSERT;
        case OH_KEYCODE_NUMPAD_DIVIDE:
            return VK_DIVIDE;
        case OH_KEYCODE_NUMPAD_MULTIPLY:
            return VK_MULTIPLY;
        case OH_KEYCODE_NUMPAD_SUBTRACT:
            return VK_SUBTRACT;
        case OH_KEYCODE_NUMPAD_ADD:
            return VK_ADD;
        case OH_KEYCODE_NUMPAD_DOT:
            return VK_DECIMAL;
        default:
            return 0;
    }
}

bool OhosKeyCodeRequiresExtendedScancode(uint32_t keyCode)
{
    switch (keyCode) {
        case OH_KEYCODE_DPAD_UP:
        case OH_KEYCODE_DPAD_DOWN:
        case OH_KEYCODE_DPAD_LEFT:
        case OH_KEYCODE_DPAD_RIGHT:
        case OH_KEYCODE_ALT_RIGHT:
        case OH_KEYCODE_PAGE_UP:
        case OH_KEYCODE_PAGE_DOWN:
        case OH_KEYCODE_FORWARD_DEL:
        case OH_KEYCODE_CTRL_RIGHT:
        case OH_KEYCODE_META_LEFT:
        case OH_KEYCODE_META_RIGHT:
        case OH_KEYCODE_MOVE_HOME:
        case OH_KEYCODE_MOVE_END:
        case OH_KEYCODE_INSERT:
        case OH_KEYCODE_NUMPAD_DIVIDE:
        case OH_KEYCODE_NUMPAD_ENTER:
            return true;
        default:
            return false;
    }
}

std::string FormatOhosKeyEvent(const OhosKeyEvent& event)
{
    const uint32_t vk = MapOhosKeyCodeToWindowsVk(event.keyCode);
    std::ostringstream line;
    line << "ohos.key keyCode=" << event.keyCode
         << " vk=" << Hex32(vk)
         << " mapped=" << BoolText(vk != 0)
         << " extended=" << BoolText(OhosKeyCodeRequiresExtendedScancode(event.keyCode))
         << " down=" << BoolText(event.down)
         << " repeat=" << BoolText(event.repeat)
         << " ctrl=" << BoolText(event.ctrl)
         << " shift=" << BoolText(event.shift)
         << " alt=" << BoolText(event.alt)
         << " meta=" << BoolText(event.meta);
    return line.str();
}

bool OhosKeyboardAdapter::SendPlatformKey(const OhosKeyEvent& event, std::string& message) const
{
    message = FormatOhosKeyEvent(event) + " adapter=log-only";
    return true;
}

} // namespace rdp_bridge
