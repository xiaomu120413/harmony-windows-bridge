#include "common/bridge_log.h"

#include <hilog/log.h>

namespace rdp_bridge {

namespace {

constexpr unsigned int kLogDomain = 0xF3D1;
constexpr const char* kLogTag = "FreeRDPBridge";
constexpr size_t kMaxHilogLine = 3500;

std::string ClipHilogLine(const std::string& line)
{
    return line.size() > kMaxHilogLine ? line.substr(0, kMaxHilogLine) : line;
}

} // namespace

void EmitHilogInfo(const std::string& line)
{
    const std::string clipped = ClipHilogLine(line);
    OH_LOG_Print(LOG_APP, LOG_INFO, kLogDomain, kLogTag, "%{public}s", clipped.c_str());
}

void EmitHilogError(const std::string& line)
{
    const std::string clipped = ClipHilogLine(line);
    OH_LOG_Print(LOG_APP, LOG_ERROR, kLogDomain, kLogTag, "%{public}s", clipped.c_str());
}

} // namespace rdp_bridge
