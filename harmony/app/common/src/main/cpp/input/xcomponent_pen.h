#pragma once

#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {

bool TryHandleXComponentPenEvent(OH_NativeXComponent* component,
    const OH_NativeXComponent_TouchEvent& event);
void CancelXComponentPenInput(const std::string& reason);

} // namespace rdp_bridge
