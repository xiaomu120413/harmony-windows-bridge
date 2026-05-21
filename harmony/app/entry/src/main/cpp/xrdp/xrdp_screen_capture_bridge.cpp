#include "xrdp/xrdp_screen_capture_bridge.h"

#include "common/bridge_log.h"
#include "common/bridge_types.h"
#include "xrdp/xrdp_server_bridge.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <limits>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <multimedia/player_framework/native_avscreen_capture.h>
#include <native_buffer/native_buffer.h>

namespace rdp_bridge {
namespace {

constexpr uint32_t kMaxCaptureDimension = 8192;
constexpr uint32_t kDefaultCaptureFrameRate = 15;

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

bool IsFourByteCaptureFormat(int32_t format)
{
    return format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888;
}

int32_t ResolveCaptureRowBytes(const OH_NativeBuffer_Config& config)
{
    if (config.width <= 0 || config.height <= 0) {
        return 0;
    }

    const int64_t tightRowBytes = static_cast<int64_t>(config.width) * 4;
    const int64_t stride = config.stride > 0 ? config.stride : config.width;
    if (stride >= tightRowBytes) {
        return static_cast<int32_t>(stride);
    }
    if (stride * 4 >= tightRowBytes) {
        return static_cast<int32_t>(stride * 4);
    }
    return static_cast<int32_t>(tightRowBytes);
}

OH_AVScreenCaptureConfig BuildRawScreenCaptureConfig(const XrdpScreenCaptureOptions& options)
{
    OH_AVScreenCaptureConfig config {};
    config.captureMode = OH_CAPTURE_HOME_SCREEN;
    config.dataType = OH_ORIGINAL_STREAM;

    config.audioInfo.micCapInfo.audioSource = OH_SOURCE_INVALID;
    config.audioInfo.innerCapInfo.audioSource = OH_SOURCE_INVALID;
    config.audioInfo.audioEncInfo.audioCodecformat = OH_AUDIO_DEFAULT;

    config.videoInfo.videoCapInfo.displayId = 0;
    config.videoInfo.videoCapInfo.missionIDs = nullptr;
    config.videoInfo.videoCapInfo.missionIDsLen = 0;
    config.videoInfo.videoCapInfo.videoFrameWidth = static_cast<int32_t>(options.width);
    config.videoInfo.videoCapInfo.videoFrameHeight = static_cast<int32_t>(options.height);
    config.videoInfo.videoCapInfo.videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA;
    config.videoInfo.videoEncInfo.videoCodec = OH_VIDEO_DEFAULT;
    config.videoInfo.videoEncInfo.videoBitrate = 0;
    config.videoInfo.videoEncInfo.videoFrameRate = static_cast<int32_t>(options.frameRate);
    return config;
}

class XrdpRawScreenCapture {
public:
    static XrdpRawScreenCapture& Instance()
    {
        static XrdpRawScreenCapture capture;
        return capture;
    }

    bool Start(XrdpScreenCaptureOptions options, std::string& message)
    {
        options = NormalizeOptions(options);

        std::unique_lock<std::mutex> lock(mutex_);
        target_ = options;
        if (running_) {
            message = "xrdp screen capture already running " + DescribeOptions(target_);
            return true;
        }

        OH_AVScreenCapture* capture = OH_AVScreenCapture_Create();
        if (capture == nullptr) {
            message = "OH_AVScreenCapture_Create returned null";
            EmitHilogError("xrdp screen capture start failed: " + message);
            return false;
        }

        OH_AVScreenCaptureCallback callback {};
        callback.onError = &XrdpRawScreenCapture::OnCaptureError;
        callback.onAudioBufferAvailable = &XrdpRawScreenCapture::OnAudioBufferAvailable;
        callback.onVideoBufferAvailable = &XrdpRawScreenCapture::OnVideoBufferAvailable;
        OH_AVSCREEN_CAPTURE_ErrCode rc = OH_AVScreenCapture_SetCallback(capture, callback);
        if (rc != AV_SCREEN_CAPTURE_ERR_OK) {
            ReleaseFailedCapture(capture);
            message = "OH_AVScreenCapture_SetCallback failed: " + CaptureErrToString(rc);
            EmitHilogError("xrdp screen capture start failed: " + message);
            return false;
        }

        OH_AVScreenCaptureConfig config = BuildRawScreenCaptureConfig(options);
        rc = OH_AVScreenCapture_Init(capture, config);
        if (rc != AV_SCREEN_CAPTURE_ERR_OK) {
            ReleaseFailedCapture(capture);
            message = "OH_AVScreenCapture_Init failed: " + CaptureErrToString(rc);
            EmitHilogError("xrdp screen capture start failed: " + message);
            return false;
        }

        OH_AVScreenCapture_SetMicrophoneEnabled(capture, false);
        OH_AVScreenCapture_SetMaxVideoFrameRate(capture, static_cast<int32_t>(options.frameRate));
        OH_AVScreenCapture_ShowCursor(capture, options.showCursor);

        running_ = true;
        capture_ = capture;
        videoReady_ = false;
        worker_ = std::thread([this]() { WorkerLoop(); });

        rc = OH_AVScreenCapture_StartScreenCapture(capture);
        if (rc != AV_SCREEN_CAPTURE_ERR_OK) {
            running_ = false;
            videoReady_ = false;
            condition_.notify_one();
            std::thread worker = std::move(worker_);
            OH_AVScreenCapture* failedCapture = capture_;
            capture_ = nullptr;
            if (worker.joinable()) {
                lock.unlock();
                worker.join();
                lock.lock();
            }
            ReleaseFailedCapture(failedCapture);
            message = "OH_AVScreenCapture_StartScreenCapture failed: " + CaptureErrToString(rc);
            EmitHilogError("xrdp screen capture start failed: " + message);
            return false;
        }

        message = "xrdp raw screen capture started " + DescribeOptions(options);
        EmitHilogInfo(message);
        return true;
    }

    void Stop(const std::string& reason)
    {
        OH_AVScreenCapture* capture = nullptr;
        std::thread worker;
        XrdpScreenCaptureOptions stoppedOptions;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && capture_ == nullptr && !worker_.joinable()) {
                return;
            }
            running_ = false;
            videoReady_ = false;
            capture = capture_;
            capture_ = nullptr;
            worker = std::move(worker_);
            stoppedOptions = target_;
        }

        condition_.notify_one();
        if (worker.joinable()) {
            worker.join();
        }
        if (capture != nullptr) {
            const OH_AVSCREEN_CAPTURE_ErrCode stopRc = OH_AVScreenCapture_StopScreenCapture(capture);
            const OH_AVSCREEN_CAPTURE_ErrCode releaseRc = OH_AVScreenCapture_Release(capture);
            EmitHilogInfo("xrdp screen capture stopped after " + reason +
                " target=" + DescribeOptions(stoppedOptions) +
                " stop=" + CaptureErrToString(stopRc) +
                " release=" + CaptureErrToString(releaseRc));
        }
    }

    void UpdateTarget(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 ||
            width > kMaxCaptureDimension || height > kMaxCaptureDimension) {
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (target_.width == width && target_.height == height) {
            return;
        }
        target_.width = width;
        target_.height = height;
        EmitHilogInfo("xrdp screen capture target updated for next start: " +
            std::to_string(width) + "x" + std::to_string(height));
    }

private:
    static XrdpScreenCaptureOptions NormalizeOptions(XrdpScreenCaptureOptions options)
    {
        if (options.width == 0 || options.width > kMaxCaptureDimension) {
            options.width = 2560;
        }
        if (options.height == 0 || options.height > kMaxCaptureDimension) {
            options.height = 1440;
        }
        if (options.frameRate == 0 || options.frameRate > 60) {
            options.frameRate = kDefaultCaptureFrameRate;
        }
        return options;
    }

    static std::string DescribeOptions(const XrdpScreenCaptureOptions& options)
    {
        return std::to_string(options.width) + "x" + std::to_string(options.height) +
            "@" + std::to_string(options.frameRate) +
            "fps cursor=" + (options.showCursor ? "on" : "off");
    }

    static void ReleaseFailedCapture(OH_AVScreenCapture* capture)
    {
        if (capture != nullptr) {
            OH_AVScreenCapture_Release(capture);
        }
    }

    static void OnCaptureError(OH_AVScreenCapture*, int32_t errorCode)
    {
        Instance().HandleCaptureError(errorCode);
    }

    static void OnAudioBufferAvailable(OH_AVScreenCapture*, bool, OH_AudioCaptureSourceType)
    {
    }

    static void OnVideoBufferAvailable(OH_AVScreenCapture* capture, bool isReady)
    {
        Instance().HandleVideoReady(capture, isReady);
    }

    void HandleCaptureError(int32_t errorCode)
    {
        EmitHilogError("xrdp screen capture callback error=" + std::to_string(errorCode));
    }

    void HandleVideoReady(OH_AVScreenCapture* capture, bool isReady)
    {
        if (!isReady) {
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ || capture_ != capture) {
                return;
            }
            videoReady_ = true;
            ++readyCount_;
        }
        condition_.notify_one();
    }

    void WorkerLoop()
    {
        for (;;) {
            OH_AVScreenCapture* capture = nullptr;
            uint64_t readyCount = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() { return !running_ || videoReady_; });
                if (!running_) {
                    return;
                }
                capture = capture_;
                videoReady_ = false;
                readyCount = readyCount_;
            }

            if (capture != nullptr) {
                ProcessOneVideoBuffer(capture, readyCount);
            }
        }
    }

    void ProcessOneVideoBuffer(OH_AVScreenCapture* capture, uint64_t readyCount)
    {
        int32_t fence = -1;
        int64_t timestamp = 0;
        OH_Rect region {};
        OH_NativeBuffer* buffer = OH_AVScreenCapture_AcquireVideoBuffer(capture, &fence, &timestamp, &region);
        if (buffer == nullptr) {
            LogSampledError("xrdp screen capture acquire video buffer returned null", readyCount);
            return;
        }

        OH_NativeBuffer_Config config {};
        OH_NativeBuffer_GetConfig(buffer, &config);

        void* mapped = nullptr;
        const int32_t mapRc = OH_NativeBuffer_Map(buffer, &mapped);
        if (mapRc != 0 || mapped == nullptr) {
            OH_AVScreenCapture_ReleaseVideoBuffer(capture);
            LogSampledError("xrdp screen capture native buffer map failed rc=" + std::to_string(mapRc),
                readyCount);
            return;
        }

        QueueMappedFrame(capture, buffer, mapped, config, timestamp, region, readyCount);

        OH_NativeBuffer_Unmap(buffer);
        OH_AVScreenCapture_ReleaseVideoBuffer(capture);
    }

    void QueueMappedFrame(OH_AVScreenCapture*, OH_NativeBuffer*, void* mapped,
        const OH_NativeBuffer_Config& config, int64_t timestamp, const OH_Rect& region, uint64_t readyCount)
    {
        XrdpScreenCaptureOptions target;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            target = target_;
        }

        if (config.width <= 0 || config.height <= 0 ||
            static_cast<uint32_t>(config.width) > kMaxCaptureDimension ||
            static_cast<uint32_t>(config.height) > kMaxCaptureDimension ||
            !IsFourByteCaptureFormat(config.format)) {
            LogSampledError("xrdp screen capture unsupported buffer: " +
                    std::to_string(config.width) + "x" + std::to_string(config.height) +
                    " format=" + std::to_string(config.format),
                readyCount);
            return;
        }

        const int32_t rowBytes = ResolveCaptureRowBytes(config);
        if (rowBytes <= 0 || rowBytes < config.width * 4) {
            LogSampledError("xrdp screen capture invalid stride=" + std::to_string(config.stride) +
                    " rowBytes=" + std::to_string(rowBytes),
                readyCount);
            return;
        }

        const auto start = std::chrono::steady_clock::now();
        const auto* source = static_cast<const uint8_t*>(mapped);
        xrdp_ohos_frame frame {};
        if (config.format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 ||
            config.format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888) {
            frame.format = XRDP_OHOS_FRAME_FORMAT_BGRA_8888;
        } else {
            frame.format = XRDP_OHOS_FRAME_FORMAT_RGBA_8888;
        }
        frame.data = source;
        frame.stride = rowBytes;
        frame.width = config.width;
        frame.height = config.height;
        frame.source_sequence = readyCount;

        std::string message;
        const bool queued = QueueXrdpVideoFrame(frame, message);
        const auto end = std::chrono::steady_clock::now();
        const uint32_t queueUs = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count());

        if (queued) {
            const uint64_t submitted = submittedCount_.fetch_add(1) + 1;
            if (submitted <= 3 || (submitted % 60U) == 0U) {
                EmitHilogInfo("xrdp screen capture frame queued: seq=" + std::to_string(readyCount) +
                    " size=" + std::to_string(config.width) + "x" + std::to_string(config.height) +
                    " target=" + DescribeOptions(target) +
                    " stride=" + std::to_string(rowBytes) +
                    " format=" + std::to_string(config.format) +
                    " pixel=" + std::string(frame.format == XRDP_OHOS_FRAME_FORMAT_BGRA_8888 ? "bgra" : "rgba") +
                    " ts=" + std::to_string(timestamp) +
                    " region=(" + std::to_string(region.x) + "," + std::to_string(region.y) +
                    "," + std::to_string(region.width) + "," + std::to_string(region.height) + ")" +
                    " queue=" + std::to_string(queueUs / 1000.0) +
                    "ms submitted=" + std::to_string(submitted));
            }
        } else if (message != "xrdp server is not running") {
            const uint64_t dropped = droppedCount_.fetch_add(1) + 1;
            if (dropped <= 3 || (dropped % 120U) == 0U) {
                EmitHilogInfo("xrdp screen capture frame not queued: " + message +
                    " dropped=" + std::to_string(dropped));
            }
        }
    }

    void LogSampledError(const std::string& message, uint64_t count)
    {
        if (count <= 3 || (count % 120U) == 0U) {
            EmitHilogError(message + " count=" + std::to_string(count));
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    OH_AVScreenCapture* capture_ = nullptr;
    std::thread worker_;
    XrdpScreenCaptureOptions target_;
    bool running_ = false;
    bool videoReady_ = false;
    uint64_t readyCount_ = 0;
    std::atomic<uint64_t> submittedCount_ { 0 };
    std::atomic<uint64_t> droppedCount_ { 0 };
};

XrdpRawScreenCapture& ScreenCapture()
{
    return XrdpRawScreenCapture::Instance();
}

} // namespace

bool StartXrdpScreenCapture(const XrdpScreenCaptureOptions& options, std::string& message)
{
    return ScreenCapture().Start(options, message);
}

void StopXrdpScreenCapture(const std::string& reason)
{
    ScreenCapture().Stop(reason);
}

void UpdateXrdpScreenCaptureTarget(uint32_t width, uint32_t height)
{
    ScreenCapture().UpdateTarget(width, height);
}

} // namespace rdp_bridge
