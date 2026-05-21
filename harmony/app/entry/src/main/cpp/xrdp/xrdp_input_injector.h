#pragma once

#include <cstdint>
#include <string>

#include "ohos/xrdp_ohos.h"

namespace rdp_bridge {

using XrdpOhosInputEvent = ::xrdp_ohos_input_event;

bool DispatchXrdpInputEvent(const XrdpOhosInputEvent& event, std::string& message);
void PrimeXrdpInputInjectorAuthorization(const std::string& reason);
void ResetXrdpInputInjector(const std::string& reason);

} // namespace rdp_bridge
