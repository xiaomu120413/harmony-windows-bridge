#pragma once

#include <cstdint>

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

} // namespace rdp_bridge
