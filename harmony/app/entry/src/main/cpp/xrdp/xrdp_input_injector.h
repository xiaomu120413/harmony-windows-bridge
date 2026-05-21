#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

struct XrdpOhosInputEvent {
    int32_t version = 0;
    int32_t msg = 0;
    long param1 = 0;
    long param2 = 0;
    long param3 = 0;
    long param4 = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t bpp = 0;
    int32_t connected = 0;
};

bool DispatchXrdpInputEvent(const XrdpOhosInputEvent& event, std::string& message);
void PrimeXrdpInputInjectorAuthorization(const std::string& reason);
void ResetXrdpInputInjector(const std::string& reason);

} // namespace rdp_bridge
