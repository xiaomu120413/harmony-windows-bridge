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

LogLevel ToHilogLevel(BridgeLogLevel level)
{
    switch (level) {
        case BridgeLogLevel::Debug:
            return LOG_DEBUG;
        case BridgeLogLevel::Info:
            return LOG_INFO;
        case BridgeLogLevel::Warn:
            return LOG_WARN;
        case BridgeLogLevel::Error:
            return LOG_ERROR;
        default:
            return LOG_DEBUG;
    }
}

} // namespace

void BridgeLogger::Log(BridgeLogLevel level, const std::string& line)
{
    const std::string clipped = ClipHilogLine(line);
    OH_LOG_Print(LOG_APP, ToHilogLevel(level), kLogDomain, kLogTag, "%{private}s", clipped.c_str());
}

void BridgeLogger::LogPublic(BridgeLogLevel level, const std::string& line)
{
    const std::string clipped = ClipHilogLine(line);
    OH_LOG_Print(LOG_APP, ToHilogLevel(level), kLogDomain, kLogTag, "%{public}s", clipped.c_str());
}

void BridgeLogger::Debug(const std::string& line)
{
    Log(BridgeLogLevel::Debug, line);
}

void BridgeLogger::DebugPublic(const std::string& line)
{
    LogPublic(BridgeLogLevel::Debug, line);
}

void BridgeLogger::Info(const std::string& line)
{
    Log(BridgeLogLevel::Info, line);
}

void BridgeLogger::Warn(const std::string& line)
{
    Log(BridgeLogLevel::Warn, line);
}

void BridgeLogger::Error(const std::string& line)
{
    Log(BridgeLogLevel::Error, line);
}

void EmitHilogDebug(const std::string& line)
{
    BridgeLogger::Debug(line);
}

void EmitHilogInfo(const std::string& line)
{
    BridgeLogger::Info(line);
}

void EmitHilogWarn(const std::string& line)
{
    BridgeLogger::Warn(line);
}

void EmitHilogError(const std::string& line)
{
    BridgeLogger::Error(line);
}

} // namespace rdp_bridge
