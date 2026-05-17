#include "input/ohos_keyboard_adapter.h"

#include "client/OHOS/ohos_keyboard.h"

#include <array>

namespace rdp_bridge {
namespace {

FREERDP_OHOS_KEY_EVENT ToFreerdpOhosKeyEvent(const OhosKeyEvent& event)
{
    return FREERDP_OHOS_KEY_EVENT {
        event.keyCode,
        event.down ? 1 : 0,
        event.repeat ? 1 : 0,
        event.ctrl ? 1 : 0,
        event.shift ? 1 : 0,
        event.alt ? 1 : 0,
        event.meta ? 1 : 0,
    };
}

} // namespace

uint32_t MapOhosKeyCodeToWindowsVk(uint32_t keyCode)
{
    return freerdp_ohos_keyboard_map_keycode_to_windows_vk(keyCode);
}

bool OhosKeyCodeRequiresExtendedScancode(uint32_t keyCode)
{
    return freerdp_ohos_keyboard_keycode_requires_extended_scancode(keyCode) != 0;
}

std::string FormatOhosKeyEvent(const OhosKeyEvent& event)
{
    std::array<char, 256> buffer {};
    const FREERDP_OHOS_KEY_EVENT nativeEvent = ToFreerdpOhosKeyEvent(event);
    if (freerdp_ohos_keyboard_format_event(&nativeEvent, buffer.data(), buffer.size()) == 0) {
        return "ohos.key format failed";
    }
    return buffer.data();
}

bool OhosKeyboardAdapter::SendPlatformKey(const OhosKeyEvent& event, std::string& message) const
{
    message = FormatOhosKeyEvent(event) + " adapter=log-only";
    return true;
}

} // namespace rdp_bridge
