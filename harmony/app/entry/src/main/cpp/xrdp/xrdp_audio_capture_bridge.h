#pragma once

#include <string>

#include <multimedia/player_framework/native_avscreen_capture.h>

#include "ohos/ohos_audio_capture_bridge.h"
#include "xrdp/xrdp_screen_capture_bridge.h"

namespace rdp_bridge {

void ConfigureXrdpPlaybackAudioCapture(OH_AVScreenCaptureConfig& config);

class XrdpAudioCapturePump {
public:
    XrdpAudioCapturePump();
    ~XrdpAudioCapturePump();

    bool Start(OH_AVScreenCapture* capture, const std::string& label);
    void Stop(const std::string& reason);
    void HandleAudioReady(OH_AVScreenCapture* capture, bool isReady, OH_AudioCaptureSourceType type);
    void FillDiagnostics(XrdpScreenCaptureDiagnostics& diagnostics);

private:
    xrdp_ohos::AudioCapturePump pump_;
};

} // namespace rdp_bridge
