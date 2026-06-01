#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "ohos/xrdp_ohos.h"

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
    std::string libraryPath;
    std::string runtimeRoot;
    std::string configPath;
    std::string modulePath;
    std::string logPath;
    bool activeMstscSession = false;
    uint32_t port = 0;
};

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params);
XrdpServerCommandResult GetXrdpServerDiagnostics();

} // namespace rdp_bridge
