#include "xrdp/xrdp_audio_capture_bridge.h"

#include "xrdp/xrdp_server_bridge.h"

namespace rdp_bridge {
namespace {

bool SubmitXrdpAudioFrame(const xrdp_ohos_audio_frame& frame, std::string& message, void*)
{
    return QueueXrdpAudioFrame(frame, message);
}

} // namespace

void ConfigureXrdpPlaybackAudioCapture(OH_AVScreenCaptureConfig& config)
{
    xrdp_ohos::ConfigurePlaybackAudioCapture(config);
}

XrdpAudioCapturePump::XrdpAudioCapturePump()
    : pump_(SubmitXrdpAudioFrame, nullptr)
{
}

XrdpAudioCapturePump::~XrdpAudioCapturePump() = default;

bool XrdpAudioCapturePump::Start(OH_AVScreenCapture* capture, const std::string& label)
{
    return pump_.Start(capture, label);
}

void XrdpAudioCapturePump::Stop(const std::string& reason)
{
    pump_.Stop(reason);
}

void XrdpAudioCapturePump::HandleAudioReady(OH_AVScreenCapture* capture, bool isReady,
    OH_AudioCaptureSourceType type)
{
    pump_.HandleAudioReady(capture, isReady, type);
}

void XrdpAudioCapturePump::FillDiagnostics(XrdpScreenCaptureDiagnostics& diagnostics)
{
    const xrdp_ohos::AudioCaptureStats stats = pump_.Snapshot();
    diagnostics.audioReadyCount = stats.readyCount;
    diagnostics.audioSubmittedCount = stats.submittedCount;
    diagnostics.audioDroppedCount = stats.droppedCount;
    diagnostics.audioBytes = stats.bytes;
}

} // namespace rdp_bridge
