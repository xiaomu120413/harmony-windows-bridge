#pragma once

#include <arkui/native_type.h>

#include <string>

namespace rdp_bridge {

bool BindXComponentNativeGestures(ArkUI_NodeHandle node, std::string& message);
void CancelXComponentNativeGestures(const std::string& reason);
void UnbindXComponentNativeGestures();

} // namespace rdp_bridge
