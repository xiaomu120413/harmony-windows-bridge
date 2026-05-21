#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/bridge_types.h"
#include "xrdp/xrdp_video_frame.h"

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

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params);
bool QueueXrdpVideoFrame(const XrdpVideoFrame& frame, std::string& message);
bool QueueXrdpRgbaFrame(const RgbaFrame& frame, std::string& message);

} // namespace rdp_bridge
