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

struct XrdpScreenCaptureDiagnostics {
    bool running = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t frameRate = 0;
    bool showCursor = false;
    uint64_t readyCount = 0;
    uint64_t submittedCount = 0;
    uint64_t droppedCount = 0;
    uint64_t captureErrorCount = 0;
};

bool StartXrdpScreenCapture(const XrdpScreenCaptureOptions& options, std::string& message);
void StopXrdpScreenCapture(const std::string& reason);
void UpdateXrdpScreenCaptureTarget(uint32_t width, uint32_t height);
XrdpScreenCaptureDiagnostics GetXrdpScreenCaptureDiagnostics();

} // namespace rdp_bridge
