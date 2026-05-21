#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>

#include <multimedia/player_framework/native_avscreen_capture.h>

#include "xrdp/xrdp_screen_capture_bridge.h"

namespace rdp_bridge {

void ConfigureXrdpPlaybackAudioCapture(OH_AVScreenCaptureConfig& config);

class XrdpAudioCapturePump {
public:
    bool Start(OH_AVScreenCapture* capture, const std::string& label);
    void Stop(const std::string& reason);
    void HandleAudioReady(OH_AVScreenCapture* capture, bool isReady, OH_AudioCaptureSourceType type);
    void FillDiagnostics(XrdpScreenCaptureDiagnostics& diagnostics);

private:
    void WorkerLoop();
    void ProcessAudioBuffers(OH_AVScreenCapture* capture, OH_AudioCaptureSourceType type,
        uint64_t audioReadyCount, uint64_t drainCount);
    bool ProcessOneAudioBuffer(OH_AVScreenCapture* capture, OH_AudioCaptureSourceType type,
        uint64_t audioReadyCount);
    void LogSampledError(const std::string& message, uint64_t count);

    std::mutex mutex_;
    std::condition_variable condition_;
    OH_AVScreenCapture* capture_ = nullptr;
    std::thread worker_;
    std::string label_;
    bool running_ = false;
    OH_AudioCaptureSourceType audioReadyType_ = OH_SOURCE_INVALID;
    uint64_t audioReadyCount_ = 0;
    uint64_t pendingAudioReadyCount_ = 0;
    uint64_t audioDroppedCount_ = 0;
    std::atomic<uint64_t> audioSubmittedCount_ { 0 };
    std::atomic<uint64_t> audioBytes_ { 0 };
};

} // namespace rdp_bridge
