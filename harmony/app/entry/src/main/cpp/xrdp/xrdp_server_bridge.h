#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rdp_bridge {

struct XrdpServerParams {
    std::string appFilesDir;
    std::string runtimeRoot;
    std::string hnpRoot;
    std::string libraryPath;
    std::string libDir;
    std::string modulePath;
    std::string configPath;
    std::string sharePath;
    uint32_t port = 3390;
};

struct XrdpServerCommandResult {
    bool ok = false;
    std::string state;
    std::string message;
    std::vector<std::string> logs;
    std::string libraryPath;
    std::string runtimeRoot;
    std::string configPath;
    std::string modulePath;
};

XrdpServerCommandResult ProbeXrdpServer(const XrdpServerParams& params);
XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params);
XrdpServerCommandResult StopXrdpServer();

} // namespace rdp_bridge
