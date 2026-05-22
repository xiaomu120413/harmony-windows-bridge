#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/bridge_types.h"
#include "ohos/xrdp_ohos.h"

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
    std::string accessCode;
    bool accessCodeGateEnabled = false;
    bool restartIfRunning = false;
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
    std::string logPath;
    bool activeMstscSession = false;
    uint32_t port = 0;
};

struct XrdpServerDiagnostics {
    bool ok = true;
    bool running = false;
    bool activeMstscSession = false;
    uint32_t port = 0;
    uint32_t sessionWidth = 0;
    uint32_t sessionHeight = 0;
    uint32_t sessionBpp = 0;
    uint32_t backendEventCount = 0;
    uint32_t inputEventCount = 0;
    int lastExitCode = 0;
    std::string state;
    std::string message;
    std::string lastBackendEvent;
    std::string lastDisconnectReason;
    std::string libraryPath;
    std::string backendLibraryPath;
    std::string runtimeRoot;
    std::string configPath;
    std::string modulePath;
    std::string sharePath;
    std::string logPath;
    std::vector<std::string> logs;
};

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params);
XrdpServerDiagnostics GetXrdpServerDiagnostics();
bool SubmitXrdpRgbaFrame(const RgbaFrame& frame, std::string& message);

} // namespace rdp_bridge
