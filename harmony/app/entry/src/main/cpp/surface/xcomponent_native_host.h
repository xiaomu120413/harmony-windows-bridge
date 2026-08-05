#pragma once

#include "napi/native_api.h"

#include <string>

namespace rdp_bridge {

bool AttachNativeXComponentContent(napi_env env, napi_value nodeContent, std::string& message);
void DetachNativeXComponentContent();

} // namespace rdp_bridge
