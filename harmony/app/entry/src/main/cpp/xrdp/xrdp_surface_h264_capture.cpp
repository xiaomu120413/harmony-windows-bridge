#include "xrdp/xrdp_surface_h264_capture.h"

#include "common/bridge_log.h"
#include "xrdp/xrdp_audio_capture_bridge.h"
#include "xrdp/xrdp_server_bridge.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <multimedia/player_framework/native_avbuffer.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcodec_videoencoder.h>
#include <multimedia/player_framework/native_averrors.h>
#include <multimedia/player_framework/native_avformat.h>
#include <multimedia/player_framework/native_avscreen_capture.h>
#include <native_window/external_window.h>

namespace rdp_bridge {
namespace {

constexpr uint32_t kMaxCaptureDimension = 8192;
constexpr uint32_t kDefaultCaptureFrameRate = 15;
constexpr int32_t kDefaultBitrate = 20000000;
constexpr int32_t kDefaultIFrameInterval = 1000;
constexpr int64_t kOutputTimeoutUs = 8000;

uint64_t NowUs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

std::string AvErrToString(OH_AVErrCode code)
{
    switch (code) {
        case AV_ERR_OK:
            return "OK";
        case AV_ERR_NO_MEMORY:
            return "NO_MEMORY";
        case AV_ERR_OPERATE_NOT_PERMIT:
            return "OPERATE_NOT_PERMIT";
        case AV_ERR_INVALID_VAL:
            return "INVALID_VAL";
        case AV_ERR_IO:
            return "IO";
        case AV_ERR_TIMEOUT:
            return "TIMEOUT";
        case AV_ERR_UNKNOWN:
            return "UNKNOWN";
        case AV_ERR_SERVICE_DIED:
            return "SERVICE_DIED";
        case AV_ERR_INVALID_STATE:
            return "INVALID_STATE";
        case AV_ERR_UNSUPPORT:
            return "UNSUPPORT";
        case AV_ERR_TRY_AGAIN_LATER:
            return "TRY_AGAIN_LATER";
        case AV_ERR_STREAM_CHANGED:
            return "STREAM_CHANGED";
        default:
            return "code=" + std::to_string(static_cast<int>(code));
    }
}

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

XrdpScreenCaptureOptions NormalizeOptions(XrdpScreenCaptureOptions options)
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

std::string DescribeOptions(const XrdpScreenCaptureOptions& options)
{
    return std::to_string(options.width) + "x" + std::to_string(options.height) +
        "@" + std::to_string(options.frameRate) +
        "fps cursor=" + (options.showCursor ? "on" : "off");
}

OH_AVScreenCaptureConfig BuildSurfaceScreenCaptureConfig(const XrdpScreenCaptureOptions& options)
{
    OH_AVScreenCaptureConfig config {};
    config.captureMode = OH_CAPTURE_HOME_SCREEN;
    config.dataType = OH_ORIGINAL_STREAM;

    ConfigureXrdpPlaybackAudioCapture(config);

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

bool AppendBytes(std::vector<uint8_t>& target, const uint8_t* data, size_t bytes)
{
    if (data == nullptr || bytes == 0U) {
        return true;
    }
    const size_t oldSize = target.size();
    target.resize(oldSize + bytes);
    std::memcpy(target.data() + oldSize, data, bytes);
    return true;
}

bool AppendStartCode(std::vector<uint8_t>& target)
{
    static const uint8_t kStartCode[] = { 0, 0, 0, 1 };
    return AppendBytes(target, kStartCode, sizeof(kStartCode));
}

uint32_t ReadBe32(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24U) |
        (static_cast<uint32_t>(data[1]) << 16U) |
        (static_cast<uint32_t>(data[2]) << 8U) |
        static_cast<uint32_t>(data[3]);
}

bool HasStartCode(const uint8_t* data, size_t bytes)
{
    if (data == nullptr || bytes < 4U) {
        return false;
    }
    for (size_t index = 0; index + 3U < bytes; ++index) {
        if (data[index] == 0 && data[index + 1U] == 0 && data[index + 2U] == 1) {
            return true;
        }
        if (index + 4U < bytes && data[index] == 0 && data[index + 1U] == 0 &&
            data[index + 2U] == 0 && data[index + 3U] == 1) {
            return true;
        }
    }
    return false;
}

bool AppendAvccPayload(std::vector<uint8_t>& target, const uint8_t* data, size_t bytes)
{
    if (data == nullptr || bytes < 7U || data[0] != 1U) {
        return false;
    }

    size_t offset = 5U;
    const uint8_t spsCount = data[offset++] & 0x1fU;
    for (uint8_t index = 0; index < spsCount; ++index) {
        if (offset + 2U > bytes) {
            return false;
        }
        const size_t nalBytes = (static_cast<size_t>(data[offset]) << 8U) | data[offset + 1U];
        offset += 2U;
        if (nalBytes == 0U || offset + nalBytes > bytes) {
            return false;
        }
        AppendStartCode(target);
        AppendBytes(target, data + offset, nalBytes);
        offset += nalBytes;
    }
    if (offset >= bytes) {
        return true;
    }

    const uint8_t ppsCount = data[offset++];
    for (uint8_t index = 0; index < ppsCount; ++index) {
        if (offset + 2U > bytes) {
            return false;
        }
        const size_t nalBytes = (static_cast<size_t>(data[offset]) << 8U) | data[offset + 1U];
        offset += 2U;
        if (nalBytes == 0U || offset + nalBytes > bytes) {
            return false;
        }
        AppendStartCode(target);
        AppendBytes(target, data + offset, nalBytes);
        offset += nalBytes;
    }
    return true;
}

bool AppendLengthPrefixedPayload(std::vector<uint8_t>& target, const uint8_t* data, size_t bytes)
{
    if (data == nullptr || bytes <= 4U) {
        return false;
    }

    size_t offset = 0;
    while (offset + 4U < bytes) {
        const size_t nalBytes = static_cast<size_t>(ReadBe32(data + offset));
        offset += 4U;
        if (nalBytes == 0U || nalBytes > bytes - offset) {
            return false;
        }
        AppendStartCode(target);
        AppendBytes(target, data + offset, nalBytes);
        offset += nalBytes;
    }
    return offset == bytes;
}

void AppendH264Payload(std::vector<uint8_t>& target, const uint8_t* data, size_t bytes)
{
    if (data == nullptr || bytes == 0U) {
        return;
    }
    if (HasStartCode(data, bytes)) {
        AppendBytes(target, data, bytes);
        return;
    }

    const size_t oldSize = target.size();
    if (AppendAvccPayload(target, data, bytes)) {
        return;
    }
    target.resize(oldSize);
    if (AppendLengthPrefixedPayload(target, data, bytes)) {
        return;
    }
    target.resize(oldSize);
    AppendBytes(target, data, bytes);
}

class XrdpSurfaceH264Capture {
public:
    static XrdpSurfaceH264Capture& Instance()
    {
        static XrdpSurfaceH264Capture capture;
        return capture;
    }

    bool Start(XrdpScreenCaptureOptions options, std::string& message)
    {
        options = NormalizeOptions(options);
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_.load()) {
            message = "xrdp surface H264 capture already running " + DescribeOptions(target_);
            return true;
        }

        OH_AVCodec* codec = nullptr;
        OHNativeWindow* surface = nullptr;
        OH_AVScreenCapture* capture = nullptr;
        if (!CreateEncoder(options, &codec, &surface, message)) {
            Cleanup(codec, surface, capture, false);
            return false;
        }
        if (!CreateCapture(options, surface, &capture, message)) {
            Cleanup(codec, surface, capture, true);
            return false;
        }

        target_ = options;
        codec_ = codec;
        inputSurface_ = surface;
        capture_ = capture;
        running_.store(true);
        outputCount_.store(0);
        submittedCount_.store(0);
        droppedCount_.store(0);
        captureErrorCount_ = 0;
        codecConfig_.clear();
        pendingPayload_.clear();
        audioPump_.Start(capture, "surface-h264");
        outputThread_ = std::thread([this]() { OutputLoop(); });

        const OH_AVSCREEN_CAPTURE_ErrCode startRc =
            OH_AVScreenCapture_StartScreenCaptureWithSurface(capture_, inputSurface_);
        if (startRc != AV_SCREEN_CAPTURE_ERR_OK) {
            running_.store(false);
            if (codec_ != nullptr) {
                OH_VideoEncoder_NotifyEndOfStream(codec_);
            }
            std::thread outputThread = std::move(outputThread_);
            if (outputThread.joinable()) {
                outputThread.join();
            }
            codec_ = nullptr;
            inputSurface_ = nullptr;
            capture_ = nullptr;
            audioPump_.Stop("surface-h264 start failed");
            Cleanup(codec, surface, capture, true);
            message = "OH_AVScreenCapture_StartScreenCaptureWithSurface failed: " +
                CaptureErrToString(startRc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        RequestKeyFrameForCodec(codec_, "start");
        message = "xrdp surface H264 capture started " + DescribeOptions(options);
        EmitHilogInfo(message);
        return true;
    }

    void Stop(const std::string& reason)
    {
        OH_AVCodec* codec = nullptr;
        OHNativeWindow* surface = nullptr;
        OH_AVScreenCapture* capture = nullptr;
        std::thread outputThread;
        XrdpScreenCaptureOptions stoppedOptions;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_.load() && codec_ == nullptr && capture_ == nullptr && !outputThread_.joinable()) {
                return;
            }
            running_.store(false);
            codec = codec_;
            surface = inputSurface_;
            capture = capture_;
            codec_ = nullptr;
            inputSurface_ = nullptr;
            capture_ = nullptr;
            outputThread = std::move(outputThread_);
            stoppedOptions = target_;
        }

        OH_AVSCREEN_CAPTURE_ErrCode stopRc = AV_SCREEN_CAPTURE_ERR_OK;
        OH_AVSCREEN_CAPTURE_ErrCode releaseRc = AV_SCREEN_CAPTURE_ERR_OK;
        audioPump_.Stop(reason);
        if (capture != nullptr) {
            stopRc = OH_AVScreenCapture_StopScreenCapture(capture);
        }
        if (codec != nullptr) {
            OH_VideoEncoder_NotifyEndOfStream(codec);
        }
        if (outputThread.joinable()) {
            outputThread.join();
        }
        if (capture != nullptr) {
            releaseRc = OH_AVScreenCapture_Release(capture);
        }
        Cleanup(codec, surface, nullptr, true);
        EmitHilogInfo("xrdp surface H264 capture stopped after " + reason +
            " target=" + DescribeOptions(stoppedOptions) +
            " stop=" + CaptureErrToString(stopRc) +
            " release=" + CaptureErrToString(releaseRc));
    }

    XrdpScreenCaptureDiagnostics Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        XrdpScreenCaptureDiagnostics diagnostics;
        diagnostics.running = running_.load();
        diagnostics.width = target_.width;
        diagnostics.height = target_.height;
        diagnostics.frameRate = target_.frameRate;
        diagnostics.showCursor = target_.showCursor;
        diagnostics.readyCount = outputCount_.load();
        diagnostics.submittedCount = submittedCount_.load();
        diagnostics.droppedCount = droppedCount_.load();
        diagnostics.captureErrorCount = captureErrorCount_;
        audioPump_.FillDiagnostics(diagnostics);
        return diagnostics;
    }

private:
    static void OnCaptureError(OH_AVScreenCapture*, int32_t errorCode)
    {
        Instance().HandleCaptureError(errorCode);
    }

    static void OnAudioBufferAvailable(OH_AVScreenCapture* capture, bool isReady, OH_AudioCaptureSourceType type)
    {
        Instance().audioPump_.HandleAudioReady(capture, isReady, type);
    }

    void HandleCaptureError(int32_t errorCode)
    {
        uint64_t count = 0;
        XrdpScreenCaptureOptions target;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            target = target_;
            count = ++captureErrorCount_;
        }
        EmitHilogError("xrdp surface H264 capture callback error=" +
            CaptureErrToString(static_cast<OH_AVSCREEN_CAPTURE_ErrCode>(errorCode)) +
            " raw=" + std::to_string(errorCode) +
            " target=" + DescribeOptions(target) +
            " count=" + std::to_string(count));
    }

    bool CreateEncoder(const XrdpScreenCaptureOptions& options, OH_AVCodec** outCodec,
        OHNativeWindow** outSurface, std::string& message)
    {
        OH_AVCapability* capability = OH_AVCodec_GetCapabilityByCategory(
            OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
        const char* codecName = capability == nullptr ? nullptr : OH_AVCapability_GetName(capability);
        if (codecName == nullptr || codecName[0] == '\0') {
            message = "no hardware AVC encoder capability";
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OH_AVCodec* codec = OH_VideoEncoder_CreateByName(codecName);
        if (codec == nullptr) {
            message = "OH_VideoEncoder_CreateByName failed name=" + std::string(codecName);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OH_AVFormat* format = OH_AVFormat_CreateVideoFormat(OH_AVCODEC_MIMETYPE_VIDEO_AVC,
            static_cast<int32_t>(options.width), static_cast<int32_t>(options.height));
        if (format == nullptr) {
            OH_VideoEncoder_Destroy(codec);
            message = "OH_AVFormat_CreateVideoFormat failed";
            return false;
        }

        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_SURFACE_FORMAT);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, kDefaultBitrate);
        OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, static_cast<double>(options.frameRate));
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, BITRATE_MODE_CBR);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, AVC_PROFILE_BASELINE);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, kDefaultIFrameInterval);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME, 0);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_PTS_BASED_RATECONTROL, 1);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, 1);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT709);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_BT709);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, MATRIX_COEFFICIENT_BT709);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, 1000 / options.frameRate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, 1);

        OH_AVErrCode rc = OH_VideoEncoder_Configure(codec, format);
        OH_AVFormat_Destroy(format);
        if (rc != AV_ERR_OK) {
            OH_VideoEncoder_Destroy(codec);
            message = "OH_VideoEncoder_Configure surface failed: " + AvErrToString(rc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OHNativeWindow* surface = nullptr;
        rc = OH_VideoEncoder_GetSurface(codec, &surface);
        if (rc != AV_ERR_OK || surface == nullptr) {
            OH_VideoEncoder_Destroy(codec);
            message = "OH_VideoEncoder_GetSurface failed: " + AvErrToString(rc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        rc = OH_VideoEncoder_Prepare(codec);
        if (rc != AV_ERR_OK) {
            OH_NativeWindow_DestroyNativeWindow(surface);
            OH_VideoEncoder_Destroy(codec);
            message = "OH_VideoEncoder_Prepare surface failed: " + AvErrToString(rc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        rc = OH_VideoEncoder_Start(codec);
        if (rc != AV_ERR_OK) {
            OH_NativeWindow_DestroyNativeWindow(surface);
            OH_VideoEncoder_Destroy(codec);
            message = "OH_VideoEncoder_Start surface failed: " + AvErrToString(rc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        *outCodec = codec;
        *outSurface = surface;
        EmitHilogInfo("xrdp surface H264 encoder ready name=" + std::string(codecName) +
            " size=" + std::to_string(options.width) + "x" + std::to_string(options.height) +
            " fps=" + std::to_string(options.frameRate) +
            " bitrate=" + std::to_string(kDefaultBitrate));
        return true;
    }

    bool CreateCapture(const XrdpScreenCaptureOptions& options, OHNativeWindow*,
        OH_AVScreenCapture** outCapture, std::string& message)
    {
        OH_AVScreenCapture* capture = OH_AVScreenCapture_Create();
        if (capture == nullptr) {
            message = "OH_AVScreenCapture_Create returned null";
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OH_AVScreenCaptureCallback callback {};
        callback.onError = &XrdpSurfaceH264Capture::OnCaptureError;
        callback.onAudioBufferAvailable = &XrdpSurfaceH264Capture::OnAudioBufferAvailable;
        OH_AVSCREEN_CAPTURE_ErrCode captureRc = OH_AVScreenCapture_SetCallback(capture, callback);
        if (captureRc != AV_SCREEN_CAPTURE_ERR_OK) {
            OH_AVScreenCapture_Release(capture);
            message = "OH_AVScreenCapture_SetCallback failed: " + CaptureErrToString(captureRc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OH_AVScreenCaptureConfig config = BuildSurfaceScreenCaptureConfig(options);
        captureRc = OH_AVScreenCapture_Init(capture, config);
        if (captureRc != AV_SCREEN_CAPTURE_ERR_OK) {
            OH_AVScreenCapture_Release(capture);
            message = "OH_AVScreenCapture_Init failed: " + CaptureErrToString(captureRc);
            EmitHilogError("xrdp surface H264 capture start failed: " + message);
            return false;
        }

        OH_AVScreenCapture_SetMicrophoneEnabled(capture, false);
        OH_AVScreenCapture_SetMaxVideoFrameRate(capture, static_cast<int32_t>(options.frameRate));
        OH_AVScreenCapture_ShowCursor(capture, options.showCursor);
        *outCapture = capture;
        return true;
    }

    void OutputLoop()
    {
        while (running_.load()) {
            DrainOneOutput();
        }
        for (int drain = 0; drain < 8; ++drain) {
            if (!DrainOneOutput()) {
                break;
            }
        }
    }

    bool DrainOneOutput()
    {
        OH_AVCodec* codec = nullptr;
        XrdpScreenCaptureOptions target;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            codec = codec_;
            target = target_;
        }
        if (codec == nullptr) {
            return false;
        }

        uint32_t outputIndex = 0;
        OH_AVErrCode rc = OH_VideoEncoder_QueryOutputBuffer(codec, &outputIndex, kOutputTimeoutUs);
        if (rc == AV_ERR_TRY_AGAIN_LATER) {
            return false;
        }
        if (rc == AV_ERR_STREAM_CHANGED) {
            UpdateOutputDescription(codec);
            return true;
        }
        if (rc != AV_ERR_OK) {
            const uint64_t dropped = droppedCount_.fetch_add(1) + 1;
            if (dropped <= 3 || (dropped % 120U) == 0U) {
                EmitHilogError("xrdp surface H264 query output failed rc=" + AvErrToString(rc) +
                    " dropped=" + std::to_string(dropped));
            }
            return false;
        }

        OH_AVBuffer* output = OH_VideoEncoder_GetOutputBuffer(codec, outputIndex);
        OH_AVCodecBufferAttr attr {};
        if (output == nullptr || OH_AVBuffer_GetBufferAttr(output, &attr) != AV_ERR_OK) {
            OH_VideoEncoder_FreeOutputBuffer(codec, outputIndex);
            const uint64_t dropped = droppedCount_.fetch_add(1) + 1;
            EmitHilogError("xrdp surface H264 output buffer invalid dropped=" + std::to_string(dropped));
            return false;
        }

        uint8_t* addr = OH_AVBuffer_GetAddr(output);
        const int capacity = OH_AVBuffer_GetCapacity(output);
        const bool payloadValid = addr != nullptr && attr.offset >= 0 && attr.size >= 0 &&
            capacity >= 0 && attr.offset + attr.size <= capacity;
        if (!payloadValid) {
            OH_VideoEncoder_FreeOutputBuffer(codec, outputIndex);
            const uint64_t dropped = droppedCount_.fetch_add(1) + 1;
            EmitHilogError("xrdp surface H264 output payload invalid capacity=" +
                std::to_string(capacity) + " offset=" + std::to_string(attr.offset) +
                " size=" + std::to_string(attr.size) +
                " dropped=" + std::to_string(dropped));
            return false;
        }

        const uint8_t* payload = addr + attr.offset;
        const size_t payloadBytes = static_cast<size_t>(attr.size);
        if ((attr.flags & AVCODEC_BUFFER_FLAGS_CODEC_DATA) != 0) {
            StoreCodecConfig(payload, payloadBytes);
            OH_VideoEncoder_FreeOutputBuffer(codec, outputIndex);
            return true;
        }

        const bool syncFrame = (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) != 0;
        const bool incomplete = (attr.flags & AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME) != 0;
        if (syncFrame) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!codecConfig_.empty() && pendingPayload_.empty()) {
                pendingPayload_.insert(pendingPayload_.end(), codecConfig_.begin(), codecConfig_.end());
            }
        }
        AppendOutputPayload(payload, payloadBytes);
        OH_VideoEncoder_FreeOutputBuffer(codec, outputIndex);
        if (incomplete) {
            return true;
        }

        std::vector<uint8_t> framePayload;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            framePayload.swap(pendingPayload_);
        }
        if (framePayload.empty()) {
            return true;
        }

        SubmitEncodedFrame(target, attr, framePayload);
        return true;
    }

    void UpdateOutputDescription(OH_AVCodec* codec)
    {
        OH_AVFormat* description = OH_VideoEncoder_GetOutputDescription(codec);
        if (description == nullptr) {
            return;
        }
        uint8_t* codecConfig = nullptr;
        size_t codecConfigBytes = 0;
        if (OH_AVFormat_GetBuffer(description, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &codecConfigBytes) &&
            codecConfig != nullptr && codecConfigBytes > 0U) {
            StoreCodecConfig(codecConfig, codecConfigBytes);
        }
        OH_AVFormat_Destroy(description);
    }

    void StoreCodecConfig(const uint8_t* data, size_t bytes)
    {
        std::vector<uint8_t> normalized;
        AppendH264Payload(normalized, data, bytes);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            codecConfig_ = std::move(normalized);
        }
        EmitHilogInfo("xrdp surface H264 stored codec config bytes=" + std::to_string(bytes));
    }

    void AppendOutputPayload(const uint8_t* data, size_t bytes)
    {
        std::vector<uint8_t> normalized;
        AppendH264Payload(normalized, data, bytes);
        if (normalized.empty()) {
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        pendingPayload_.insert(pendingPayload_.end(), normalized.begin(), normalized.end());
    }

    void SubmitEncodedFrame(const XrdpScreenCaptureOptions& target, const OH_AVCodecBufferAttr& attr,
        const std::vector<uint8_t>& payload)
    {
        const uint64_t sequence = outputCount_.fetch_add(1) + 1;
        const uint64_t outputUs = NowUs();
        const uint64_t captureUs = attr.pts > 0 ? static_cast<uint64_t>(attr.pts / 1000) : outputUs;
        xrdp_ohos_encoded_frame frame {};
        frame.data = payload.data();
        frame.bytes = static_cast<int>(payload.size());
        frame.width = static_cast<int>(target.width);
        frame.height = static_cast<int>(target.height);
        frame.format = XRDP_OHOS_ENCODED_FRAME_FORMAT_H264_AVC420;
        frame.flags = (attr.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) != 0 ?
            XRDP_OHOS_ENCODED_FRAME_FLAG_SYNC : 0U;
        frame.source_sequence = sequence;
        frame.capture_timestamp_us = captureUs;
        frame.capture_acquire_us = captureUs;
        frame.bridge_queue_us = outputUs;
        frame.encoder_output_us = outputUs;

        std::string message;
        const bool queued = QueueXrdpEncodedVideoFrame(frame, message);
        if (queued) {
            const uint64_t submitted = submittedCount_.fetch_add(1) + 1;
            if (submitted <= 5 || (submitted % 60U) == 0U) {
                EmitHilogInfo("xrdp surface H264 frame queued: seq=" + std::to_string(sequence) +
                    " size=" + std::to_string(target.width) + "x" + std::to_string(target.height) +
                    " bytes=" + std::to_string(payload.size()) +
                    " flags=0x" + std::to_string(static_cast<uint32_t>(attr.flags)) +
                    " pts=" + std::to_string(attr.pts) +
                    " submitted=" + std::to_string(submitted));
            }
            return;
        }

        if (message != "xrdp server is not running") {
            const uint64_t dropped = droppedCount_.fetch_add(1) + 1;
            if (message == "xrdp encoded video submit status=-6") {
                RequestKeyFrame("xrdp h264 backpressure");
            }
            if (dropped <= 5 || (dropped % 120U) == 0U) {
                EmitHilogInfo("xrdp surface H264 frame not queued: " + message +
                    " bytes=" + std::to_string(payload.size()) +
                    " dropped=" + std::to_string(dropped));
            }
        }
    }

    void RequestKeyFrame(const char* reason)
    {
        OH_AVCodec* codec = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            codec = codec_;
        }
        RequestKeyFrameForCodec(codec, reason);
    }

    static void RequestKeyFrameForCodec(OH_AVCodec* codec, const char* reason)
    {
        if (codec == nullptr) {
            return;
        }

        OH_AVFormat* format = OH_AVFormat_Create();
        if (format == nullptr) {
            return;
        }
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_REQUEST_I_FRAME, 1);
        const OH_AVErrCode rc = OH_VideoEncoder_SetParameter(codec, format);
        OH_AVFormat_Destroy(format);
        if (rc != AV_ERR_OK) {
            EmitHilogError("xrdp surface H264 request key frame failed reason=" +
                std::string(reason == nullptr ? "unknown" : reason) +
                " rc=" + AvErrToString(rc));
            return;
        }
        EmitHilogInfo("xrdp surface H264 requested key frame reason=" +
            std::string(reason == nullptr ? "unknown" : reason));
    }

    static void Cleanup(OH_AVCodec* codec, OHNativeWindow* surface, OH_AVScreenCapture* capture,
        bool stopCodec)
    {
        if (capture != nullptr) {
            OH_AVScreenCapture_Release(capture);
        }
        if (codec != nullptr) {
            if (stopCodec) {
                OH_VideoEncoder_Stop(codec);
            }
            OH_VideoEncoder_Destroy(codec);
        }
        if (surface != nullptr) {
            OH_NativeWindow_DestroyNativeWindow(surface);
        }
    }

    std::mutex mutex_;
    std::atomic<bool> running_ { false };
    OH_AVCodec* codec_ = nullptr;
    OHNativeWindow* inputSurface_ = nullptr;
    OH_AVScreenCapture* capture_ = nullptr;
    std::thread outputThread_;
    XrdpScreenCaptureOptions target_;
    std::vector<uint8_t> codecConfig_;
    std::vector<uint8_t> pendingPayload_;
    uint64_t captureErrorCount_ = 0;
    std::atomic<uint64_t> outputCount_ { 0 };
    std::atomic<uint64_t> submittedCount_ { 0 };
    std::atomic<uint64_t> droppedCount_ { 0 };
    XrdpAudioCapturePump audioPump_;
};

} // namespace

bool StartXrdpSurfaceH264Capture(const XrdpScreenCaptureOptions& options, std::string& message)
{
    return XrdpSurfaceH264Capture::Instance().Start(options, message);
}

void StopXrdpSurfaceH264Capture(const std::string& reason)
{
    XrdpSurfaceH264Capture::Instance().Stop(reason);
}

XrdpScreenCaptureDiagnostics GetXrdpSurfaceH264CaptureDiagnostics()
{
    return XrdpSurfaceH264Capture::Instance().Snapshot();
}

} // namespace rdp_bridge
