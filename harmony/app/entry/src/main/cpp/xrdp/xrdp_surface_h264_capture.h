#pragma once

#include <string>

#include "xrdp/xrdp_screen_capture_bridge.h"

namespace rdp_bridge {

bool StartXrdpSurfaceH264Capture(const XrdpScreenCaptureOptions& options, std::string& message);
void StopXrdpSurfaceH264Capture(const std::string& reason);
XrdpScreenCaptureDiagnostics GetXrdpSurfaceH264CaptureDiagnostics();

} // namespace rdp_bridge
