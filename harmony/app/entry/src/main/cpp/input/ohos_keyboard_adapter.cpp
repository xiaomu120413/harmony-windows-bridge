#include "input/ohos_keyboard_adapter.h"

#include "client/OHOS/ohos_keyboard.h"
#include "freerdp_runtime.h"

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
    auto& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error) ||
        api.ohosKeyboardMapKeyCodeToWindowsVk == nullptr) {
        return 0;
    }
    return api.ohosKeyboardMapKeyCodeToWindowsVk(keyCode);
}

bool OhosKeyCodeRequiresExtendedScancode(uint32_t keyCode)
{
    auto& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error) ||
        api.ohosKeyboardKeyCodeRequiresExtendedScancode == nullptr) {
        return false;
    }
    return api.ohosKeyboardKeyCodeRequiresExtendedScancode(keyCode) != 0;
}

std::string FormatOhosKeyEvent(const OhosKeyEvent& event)
{
    std::array<char, 256> buffer {};
    const FREERDP_OHOS_KEY_EVENT nativeEvent = ToFreerdpOhosKeyEvent(event);
    auto& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error) || api.ohosKeyboardFormatEvent == nullptr) {
        return "ohos.key backend unavailable";
    }
    if (api.ohosKeyboardFormatEvent(&nativeEvent, buffer.data(), buffer.size()) == 0) {
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
