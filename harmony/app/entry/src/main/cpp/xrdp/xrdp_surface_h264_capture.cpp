#include "xrdp/xrdp_surface_h264_capture.h"

#include "ohos/ohos_capture_h264.h"
#include "xrdp/xrdp_server_bridge.h"

namespace rdp_bridge {
namespace {

bool SubmitEncodedFrame(const xrdp_ohos_encoded_frame& frame, std::string& message, void*)
{
    return QueueXrdpEncodedVideoFrame(frame, message);
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

xrdp_ohos::SurfaceH264Capture& Capture()
{
    static xrdp_ohos::SurfaceH264Capture capture({
        nullptr,
        SubmitEncodedFrame,
        SubmitAudioFrame,
        nullptr,
    });
    return capture;
}

} // namespace

bool StartXrdpSurfaceH264Capture(const XrdpScreenCaptureOptions& options, std::string& message)
{
    return Capture().Start(ToSourceOptions(options), message);
}

void StopXrdpSurfaceH264Capture(const std::string& reason)
{
    Capture().Stop(reason);
}

XrdpScreenCaptureDiagnostics GetXrdpSurfaceH264CaptureDiagnostics()
{
    return ToBridgeDiagnostics(Capture().Snapshot());
}

} // namespace rdp_bridge
