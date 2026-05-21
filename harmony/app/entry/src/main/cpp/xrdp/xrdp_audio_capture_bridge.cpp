#include "xrdp/xrdp_audio_capture_bridge.h"

#include "common/bridge_log.h"
#include "xrdp/xrdp_server_bridge.h"

#include <algorithm>

namespace rdp_bridge {
namespace {

constexpr int32_t kXrdpAudioSampleRate = 44100;
constexpr int32_t kXrdpAudioChannels = 2;
constexpr int32_t kXrdpAudioBitsPerSample = 16;
constexpr uint64_t kMaxAudioBuffersPerWake = 32;

std::string CaptureErrToString(OH_AVSCREEN_CAPTURE_ErrCode code)
{
    switch (code) {
        case AV_SCREEN_CAPTURE_ERR_OK:
            return "OK";
        case AV_SCREEN_CAPTURE_ERR_NO_MEMORY:
            return "NO_MEMORY";
        case AV_SCREEN_CAPTURE_ERR_OPERATE_NOT_PERMIT:
            return "OPERATE_NOT_PERMIT";
        case AV_SCREEN_CAPTURE_ERR_INVALID_VAL:
            return "INVALID_VAL";
        case AV_SCREEN_CAPTURE_ERR_IO:
            return "IO";
        case AV_SCREEN_CAPTURE_ERR_TIMEOUT:
            return "TIMEOUT";
        case AV_SCREEN_CAPTURE_ERR_UNKNOWN:
            return "UNKNOWN";
        case AV_SCREEN_CAPTURE_ERR_SERVICE_DIED:
            return "SERVICE_DIED";
        case AV_SCREEN_CAPTURE_ERR_INVALID_STATE:
            return "INVALID_STATE";
        case AV_SCREEN_CAPTURE_ERR_UNSUPPORT:
            return "UNSUPPORT";
        default:
            return "code=" + std::to_string(static_cast<int>(code));
    }
}

} // namespace

void ConfigureXrdpPlaybackAudioCapture(OH_AVScreenCaptureConfig& config)
{
    config.audioInfo.micCapInfo.audioSource = OH_SOURCE_INVALID;
    config.audioInfo.innerCapInfo.audioSampleRate = kXrdpAudioSampleRate;
    config.audioInfo.innerCapInfo.audioChannels = kXrdpAudioChannels;
    config.audioInfo.innerCapInfo.audioSource = OH_ALL_PLAYBACK;
    config.audioInfo.audioEncInfo.audioBitrate = 0;
    config.audioInfo.audioEncInfo.audioCodecformat = OH_AUDIO_DEFAULT;
}

bool XrdpAudioCapturePump::Start(OH_AVScreenCapture* capture, const std::string& label)
{
    if (capture == nullptr) {
        EmitHilogError("xrdp audio capture pump start failed: capture is null label=" + label);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (running_) {
        return true;
    }
    capture_ = capture;
    label_ = label;
    running_ = true;
    audioReadyType_ = OH_SOURCE_INVALID;
    audioReadyCount_ = 0;
    pendingAudioReadyCount_ = 0;
    audioDroppedCount_ = 0;
    audioSubmittedCount_.store(0);
    audioBytes_.store(0);
    worker_ = std::thread([this]() { WorkerLoop(); });
    EmitHilogInfo("xrdp audio capture pump started label=" + label_);
    return true;
}

void XrdpAudioCapturePump::Stop(const std::string& reason)
{
    std::thread worker;
    std::string label;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ && capture_ == nullptr && !worker_.joinable()) {
            return;
        }
        running_ = false;
        capture_ = nullptr;
        pendingAudioReadyCount_ = 0;
        audioReadyType_ = OH_SOURCE_INVALID;
        worker = std::move(worker_);
        label = label_;
    }

    condition_.notify_one();
    if (worker.joinable()) {
        worker.join();
    }
    EmitHilogInfo("xrdp audio capture pump stopped after " + reason +
        " label=" + label +
        " submitted=" + std::to_string(audioSubmittedCount_.load()) +
        " dropped=" + std::to_string(audioDroppedCount_) +
        " bytes=" + std::to_string(audioBytes_.load()));
}

void XrdpAudioCapturePump::HandleAudioReady(OH_AVScreenCapture* capture, bool isReady,
    OH_AudioCaptureSourceType type)
{
    uint64_t audioReadyCount = 0;
    if (!isReady) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_ || capture_ != capture) {
            return;
        }
        ++pendingAudioReadyCount_;
        audioReadyType_ = type;
        audioReadyCount = ++audioReadyCount_;
    }
    if (type != OH_ALL_PLAYBACK && (audioReadyCount <= 3 || (audioReadyCount % 120U) == 0U)) {
        EmitHilogInfo("xrdp audio capture ready type=" + std::to_string(static_cast<int>(type)) +
            " label=" + label_ +
            " count=" + std::to_string(audioReadyCount));
    }
    condition_.notify_one();
}

void XrdpAudioCapturePump::FillDiagnostics(XrdpScreenCaptureDiagnostics& diagnostics)
{
    std::lock_guard<std::mutex> lock(mutex_);
    diagnostics.audioReadyCount = audioReadyCount_;
    diagnostics.audioSubmittedCount = audioSubmittedCount_.load();
    diagnostics.audioDroppedCount = audioDroppedCount_;
    diagnostics.audioBytes = audioBytes_.load();
}

void XrdpAudioCapturePump::WorkerLoop()
{
    for (;;) {
        OH_AVScreenCapture* capture = nullptr;
        OH_AudioCaptureSourceType audioType = OH_SOURCE_INVALID;
        uint64_t audioReadyCount = 0;
        uint64_t audioDrainCount = 0;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            condition_.wait(lock, [this]() {
                return !running_ || pendingAudioReadyCount_ > 0;
            });
            if (!running_) {
                return;
            }
            capture = capture_;
            audioType = audioReadyType_;
            audioDrainCount = std::min(pendingAudioReadyCount_, kMaxAudioBuffersPerWake);
            pendingAudioReadyCount_ -= audioDrainCount;
            if (pendingAudioReadyCount_ == 0) {
                audioReadyType_ = OH_SOURCE_INVALID;
            }
            audioReadyCount = audioReadyCount_;
        }

        if (capture != nullptr && audioDrainCount > 0) {
            ProcessAudioBuffers(capture, audioType, audioReadyCount, audioDrainCount);
        }
    }
}

void XrdpAudioCapturePump::ProcessAudioBuffers(OH_AVScreenCapture* capture, OH_AudioCaptureSourceType type,
    uint64_t audioReadyCount, uint64_t drainCount)
{
    for (uint64_t i = 0; i < drainCount; ++i) {
        if (!ProcessOneAudioBuffer(capture, type, audioReadyCount)) {
            break;
        }
    }
}

bool XrdpAudioCapturePump::ProcessOneAudioBuffer(OH_AVScreenCapture* capture,
    OH_AudioCaptureSourceType type, uint64_t audioReadyCount)
{
    OH_AudioBuffer audioBufferStorage {};
    OH_AudioBuffer* audioBuffer = &audioBufferStorage;
    const OH_AVSCREEN_CAPTURE_ErrCode acquireRc =
        OH_AVScreenCapture_AcquireAudioBuffer(capture, &audioBuffer, type);
    if (acquireRc != AV_SCREEN_CAPTURE_ERR_OK) {
        const uint64_t dropped = ++audioDroppedCount_;
        LogSampledError("xrdp audio capture acquire buffer failed rc=" +
                CaptureErrToString(acquireRc) +
                " type=" + std::to_string(static_cast<int>(type)) +
                " dropped=" + std::to_string(dropped),
            audioReadyCount);
        return false;
    }

    const OH_AudioCaptureSourceType releaseType = audioBuffer != nullptr ? audioBuffer->type : type;
    if (audioBuffer == nullptr || audioBuffer->buf == nullptr || audioBuffer->size <= 0) {
        const uint64_t dropped = ++audioDroppedCount_;
        OH_AVScreenCapture_ReleaseAudioBuffer(capture, releaseType);
        LogSampledError("xrdp audio capture invalid buffer type=" +
                std::to_string(static_cast<int>(releaseType)) +
                " dropped=" + std::to_string(dropped),
            audioReadyCount);
        return false;
    }

    xrdp_ohos_audio_frame frame {};
    frame.data = audioBuffer->buf;
    frame.bytes = audioBuffer->size;
    frame.sample_rate = kXrdpAudioSampleRate;
    frame.channels = kXrdpAudioChannels;
    frame.bits_per_sample = kXrdpAudioBitsPerSample;
    frame.format = XRDP_OHOS_AUDIO_FORMAT_PCM_S16LE;
    frame.source_timestamp = audioBuffer->timestamp > 0 ?
        static_cast<uint64_t>(audioBuffer->timestamp) : 0;

    std::string message;
    const bool queued = QueueXrdpAudioFrame(frame, message);
    const OH_AVSCREEN_CAPTURE_ErrCode releaseRc =
        OH_AVScreenCapture_ReleaseAudioBuffer(capture, releaseType);
    if (releaseRc != AV_SCREEN_CAPTURE_ERR_OK) {
        LogSampledError("xrdp audio capture release buffer failed rc=" +
                CaptureErrToString(releaseRc) +
                " type=" + std::to_string(static_cast<int>(releaseType)),
            audioReadyCount);
    }

    if (queued) {
        const uint64_t submitted = audioSubmittedCount_.fetch_add(1) + 1;
        const uint64_t totalBytes = audioBytes_.fetch_add(static_cast<uint64_t>(frame.bytes)) +
            static_cast<uint64_t>(frame.bytes);
        if (submitted <= 3 || (submitted % 120U) == 0U) {
            EmitHilogInfo("xrdp audio capture queued: seq=" +
                std::to_string(audioReadyCount) +
                " bytes=" + std::to_string(frame.bytes) +
                " type=" + std::to_string(static_cast<int>(releaseType)) +
                " ts=" + std::to_string(frame.source_timestamp) +
                " submitted=" + std::to_string(submitted) +
                " totalBytes=" + std::to_string(totalBytes));
        }
    } else if (message != "xrdp server is not running") {
        const uint64_t dropped = ++audioDroppedCount_;
        if (dropped <= 3 || (dropped % 120U) == 0U) {
            EmitHilogInfo("xrdp audio capture not queued: " + message +
                " bytes=" + std::to_string(frame.bytes) +
                " dropped=" + std::to_string(dropped));
        }
    }
    return true;
}

void XrdpAudioCapturePump::LogSampledError(const std::string& message, uint64_t count)
{
    if (count <= 3 || (count % 120U) == 0U) {
        EmitHilogError(message + " count=" + std::to_string(count) +
            " label=" + label_);
    }
}

} // namespace rdp_bridge
