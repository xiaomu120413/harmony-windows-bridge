#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

struct XrdpScreenCaptureOptions {
    uint32_t width = 2560;
    uint32_t height = 1440;
    uint32_t frameRate = 15;
    bool showCursor = true;
};

bool StartXrdpScreenCapture(const XrdpScreenCaptureOptions& options, std::string& message);
void StopXrdpScreenCapture(const std::string& reason);
void UpdateXrdpScreenCaptureTarget(uint32_t width, uint32_t height);

} // namespace rdp_bridge
