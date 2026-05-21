#include "xrdp/xrdp_screen_capture_bridge.h"

#include "common/bridge_log.h"
#include "ohos/ohos_capture_raw.h"
#include "xrdp/xrdp_server_bridge.h"
#include "xrdp/xrdp_surface_h264_capture.h"

namespace rdp_bridge {
namespace {

bool SubmitVideoFrame(const xrdp_ohos_frame& frame, std::string& message, void*)
{
    return QueueXrdpVideoFrame(frame, message);
}

bool SubmitAudioFrame(const xrdp_ohos_audio_frame& frame, std::string& message, void*)
{
    return QueueXrdpAudioFrame(frame, message);
}

xrdp_ohos::CaptureOptions ToSourceOptions(const XrdpScreenCaptureOptions& options)
{
    xrdp_ohos::CaptureOptions source;
    source.width = options.width;
    source.height = options.height;
    source.frameRate = options.frameRate;
    source.showCursor = options.showCursor;
    return source;
}

XrdpScreenCaptureDiagnostics ToBridgeDiagnostics(const xrdp_ohos::CaptureDiagnostics& source)
{
    XrdpScreenCaptureDiagnostics diagnostics;
    diagnostics.running = source.running;
    diagnostics.width = source.width;
    diagnostics.height = source.height;
    diagnostics.frameRate = source.frameRate;
    diagnostics.showCursor = source.showCursor;
    diagnostics.readyCount = source.readyCount;
    diagnostics.submittedCount = source.submittedCount;
    diagnostics.droppedCount = source.droppedCount;
    diagnostics.audioReadyCount = source.audioReadyCount;
    diagnostics.audioSubmittedCount = source.audioSubmittedCount;
    diagnostics.audioDroppedCount = source.audioDroppedCount;
    diagnostics.audioBytes = source.audioBytes;
    diagnostics.captureErrorCount = source.captureErrorCount;
    return diagnostics;
}

xrdp_ohos::RawScreenCapture& RawCapture()
{
    static xrdp_ohos::RawScreenCapture capture({
        SubmitVideoFrame,
        nullptr,
        SubmitAudioFrame,
        nullptr,
    });
    return capture;
}

} // namespace

bool StartXrdpScreenCapture(const XrdpScreenCaptureOptions& options, std::string& message)
{
    std::string surfaceMessage;
    if (StartXrdpSurfaceH264Capture(options, surfaceMessage)) {
        message = surfaceMessage;
        return true;
    }

    EmitHilogError("xrdp surface H264 capture unavailable, falling back to raw path: " + surfaceMessage);
    std::string rawMessage;
    const bool rawStarted = RawCapture().Start(ToSourceOptions(options), rawMessage);
    message = rawStarted ? rawMessage : surfaceMessage + "; raw fallback failed: " + rawMessage;
    return rawStarted;
}

void StopXrdpScreenCapture(const std::string& reason)
{
    StopXrdpSurfaceH264Capture(reason);
    RawCapture().Stop(reason);
}

void UpdateXrdpScreenCaptureTarget(uint32_t width, uint32_t height)
{
    RawCapture().UpdateTarget(width, height);
}

XrdpScreenCaptureDiagnostics GetXrdpScreenCaptureDiagnostics()
{
    const XrdpScreenCaptureDiagnostics surface = GetXrdpSurfaceH264CaptureDiagnostics();
    if (surface.running) {
        return surface;
    }
    return ToBridgeDiagnostics(RawCapture().Snapshot());
}

} // namespace rdp_bridge
