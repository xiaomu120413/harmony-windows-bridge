#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

struct OhosKeyEvent {
    uint32_t keyCode = 0;
    bool down = false;
    bool repeat = false;
    bool ctrl = false;
    bool shift = false;
    bool alt = false;
    bool meta = false;
};

class OhosKeyboardAdapter {
public:
    bool SendPlatformKey(const OhosKeyEvent& event, std::string& message) const;
};

uint32_t MapOhosKeyCodeToWindowsVk(uint32_t keyCode);
bool OhosKeyCodeRequiresExtendedScancode(uint32_t keyCode);
std::string FormatOhosKeyEvent(const OhosKeyEvent& event);

} // namespace rdp_bridge
