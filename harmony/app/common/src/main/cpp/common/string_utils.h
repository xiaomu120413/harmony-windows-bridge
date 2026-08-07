#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

bool ParseUInt32(const std::string& value, uint32_t& result);
bool ParsePort(const std::string& value, uint32_t& port);
bool IsAutoResolution(const std::string& value);
bool ParseResolution(const std::string& value, uint32_t& width, uint32_t& height);

std::string ToLowerAscii(std::string value);
std::string TrimAscii(const std::string& value);
std::string TrimTrailingSlashes(std::string value);
std::string JoinPath(const std::string& base, const std::string& child);
bool EnsureDirectory(const std::string& path, std::string& error);

std::string Hex32(uint32_t value);
std::string SafeCString(const char* value);

} // namespace rdp_bridge
