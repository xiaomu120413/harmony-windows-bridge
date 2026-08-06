#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace rdp_bridge {

struct XrdpServerParams {
    std::string appFilesDir;
    std::string accessCode;
    bool accessCodeGateEnabled = false;
    bool restartIfRunning = false;
};

struct XrdpServerCommandResult {
    bool ok = false;
    std::string state;
    std::string message;
    std::vector<std::string> logs;
    int32_t pid = 0;
    int32_t lastExitCode = 0;
    uint32_t port = 3390;
};

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params);
XrdpServerCommandResult GetXrdpServerDiagnostics();
XrdpServerCommandResult StopXrdpServer(const std::string& reason);

} // namespace rdp_bridge
