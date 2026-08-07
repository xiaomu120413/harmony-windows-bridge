#pragma once

#include <string>

namespace rdp_bridge {

enum class BridgeLogLevel {
    Debug,
    Info,
    Warn,
    Error,
};

class BridgeLogger {
public:
    static void Log(BridgeLogLevel level, const std::string& line);
    static void LogPublic(BridgeLogLevel level, const std::string& line);
    static void Debug(const std::string& line);
    static void DebugPublic(const std::string& line);
    static void Info(const std::string& line);
    static void Warn(const std::string& line);
    static void Error(const std::string& line);
};

void EmitHilogDebug(const std::string& line);
void EmitHilogInfo(const std::string& line);
void EmitHilogWarn(const std::string& line);
void EmitHilogError(const std::string& line);

} // namespace rdp_bridge
