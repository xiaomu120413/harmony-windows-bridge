#pragma once

#include <string>

namespace rdp_bridge {

void EmitHilogInfo(const std::string& line);
void EmitHilogError(const std::string& line);

} // namespace rdp_bridge
