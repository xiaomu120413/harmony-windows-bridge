#include "surface/avc420_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "common/string_utils.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <multimedia/player_framework/native_avbuffer.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>
#include <multimedia/player_framework/native_avformat.h>
#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>

namespace rdp_bridge {
namespace {

std::string DecoderPixelFormatName(int32_t format)
{
    switch (format) {
        case AV_PIXEL_FORMAT_YUVI420:
            return "YUVI420";
        case AV_PIXEL_FORMAT_NV12:
            return "NV12";
        case AV_PIXEL_FORMAT_NV21:
            return "NV21";
        case AV_PIXEL_FORMAT_SURFACE_FORMAT:
            return "SURFACE_FORMAT";
        case AV_PIXEL_FORMAT_RGBA:
            return "RGBA";
        case AV_PIXEL_FORMAT_RGBA1010102:
            return "RGBA1010102";
        default:
            return std::to_string(format);
    }
}

} // namespace

Avc420HardwareDecoder::~Avc420HardwareDecoder()
{
    Close();
}

void Avc420HardwareDecoder::Close()
{
    if (decoder_ != nullptr) {
        if (started_) {
            OH_VideoDecoder_Stop(decoder_);
        }
        OH_VideoDecoder_Destroy(decoder_);
    }
    decoder_ = nullptr;
    started_ = false;
    width_ = 0;
    height_ = 0;
    pixelFormat_ = 0;
    outputPixelFormat_ = 0;
}

bool Avc420HardwareDecoder::Started() const
{
    return decoder_ != nullptr && started_;
}

bool Avc420HardwareDecoder::Ensure(uint32_t width, uint32_t height, std::vector<std::string>& logs)
{
    if (decoder_ != nullptr && started_ && width_ == width && height_ == height) {
        return true;
    }

    Close();
    width_ = width;
    height_ = height;

    OH_AVCapability* capability =
        OH_AVCodec_GetCapabilityByCategory(kAvcMime, false, HARDWARE);
    const char* name = capability == nullptr ? nullptr : OH_AVCapability_GetName(capability);
    decoder_ = (name != nullptr && name[0] != '\0') ?
        OH_VideoDecoder_CreateByName(name) : OH_VideoDecoder_CreateByMime(kAvcMime);
    if (decoder_ == nullptr) {
        logs.push_back("AVC420 GPU decoder create failed name=" + SafeCString(name));
        return false;
    }

    bool isValid = false;
    OH_AVErrCode rc = OH_VideoDecoder_IsValid(decoder_, &isValid);
    if (rc != AV_ERR_OK || !isValid) {
        logs.push_back("AVC420 GPU decoder invalid rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " valid=" + std::to_string(isValid ? 1 : 0));
        Close();
        return false;
    }

    if (!ConfigureWithPixelFormat(AV_PIXEL_FORMAT_SURFACE_FORMAT, logs) &&
        !ConfigureWithPixelFormat(AV_PIXEL_FORMAT_RGBA, logs)) {
        logs.push_back("AVC420 GPU decoder has no direct-sampleable native output format; "
            "raw NV12/NV21 buffer import is disabled to avoid green screen");
        Close();
        return false;
    }

    rc = OH_VideoDecoder_Prepare(decoder_);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU decoder prepare failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        Close();
        return false;
    }

    rc = OH_VideoDecoder_Start(decoder_);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU decoder start failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        Close();
        return false;
    }

    started_ = true;
    UpdateOutputDescription(logs, "start");
    logs.push_back("AVC420 GPU decoder ready: " +
        std::to_string(width_) + "x" + std::to_string(height_) +
        " requestedPixelFormat=" + DecoderPixelFormatName(pixelFormat_) +
        "(" + std::to_string(pixelFormat_) + ")" +
        " syncMode=buffer nativeBufferOnly=yes outputDeadlineUs=" +
        std::to_string(kOutputSyncDeadlineUs));
    return true;
}

DecodeResult Avc420HardwareDecoder::Decode(const uint8_t* data, uint32_t size, int64_t pts,
    NativeDecodedFrame& frame, std::vector<std::string>& logs)
{
    if (decoder_ == nullptr || !started_ || data == nullptr || size == 0) {
        logs.push_back("AVC420 GPU decode skipped: invalid input");
        return DecodeResult::Failed;
    }

    uint32_t inputIndex = 0;
    OH_AVErrCode rc = OH_VideoDecoder_QueryInputBuffer(decoder_, &inputIndex, kInputTimeoutUs);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU input unavailable rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " size=" + std::to_string(size));
        return DecodeResult::Failed;
    }

    OH_AVBuffer* input = OH_VideoDecoder_GetInputBuffer(decoder_, inputIndex);
    uint8_t* dst = input == nullptr ? nullptr : OH_AVBuffer_GetAddr(input);
    const int32_t capacity = input == nullptr ? -1 : OH_AVBuffer_GetCapacity(input);
    if (dst == nullptr || capacity < 0 || static_cast<uint32_t>(capacity) < size) {
        logs.push_back("AVC420 GPU input buffer invalid capacity=" +
            std::to_string(capacity) + " size=" + std::to_string(size));
        PushEmptyInput(input, inputIndex);
        return DecodeResult::Failed;
    }

    std::memcpy(dst, data, size);
    OH_AVCodecBufferAttr attr {};
    attr.pts = pts;
    attr.size = static_cast<int32_t>(size);
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    rc = OH_AVBuffer_SetBufferAttr(input, &attr);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU set input attr failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        PushEmptyInput(input, inputIndex);
        return DecodeResult::Failed;
    }

    rc = OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU push input failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        return DecodeResult::Failed;
    }
    ++pushed_;

    int64_t waitedUs = 0;
    for (uint32_t attempt = 0;
         attempt < kOutputSyncMaxAttempts && waitedUs < kOutputSyncDeadlineUs; ++attempt) {
        uint32_t outputIndex = 0;
        const int64_t remainingUs = kOutputSyncDeadlineUs - waitedUs;
        const int64_t timeout = std::min(
            attempt == 0 ? kOutputTimeoutUs : kFollowupOutputTimeoutUs, remainingUs);
        rc = OH_VideoDecoder_QueryOutputBuffer(decoder_, &outputIndex, timeout);
        if (rc == AV_ERR_STREAM_CHANGED) {
            UpdateOutputDescription(logs, "stream-changed");
            continue;
        }
        if (rc == AV_ERR_TRY_AGAIN_LATER) {
            waitedUs += timeout;
            continue;
        }
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC420 GPU query output failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            return DecodeResult::Failed;
        }

        OH_AVBuffer* output = OH_VideoDecoder_GetOutputBuffer(decoder_, outputIndex);
        OH_AVCodecBufferAttr outputAttr {};
        if (output == nullptr ||
            OH_AVBuffer_GetBufferAttr(output, &outputAttr) != AV_ERR_OK) {
            OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
            logs.push_back("AVC420 GPU output buffer invalid");
            return DecodeResult::Failed;
        }

        if (outputAttr.pts != pts) {
            OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
            logs.push_back("AVC420 GPU discarded stale output pts=" +
                std::to_string(outputAttr.pts) + " expected=" + std::to_string(pts));
            continue;
        }

        frame.Release();
        frame.codec = decoder_;
        frame.buffer = output;
        frame.outputIndex = outputIndex;
        frame.hasOutputIndex = true;
        frame.width = width_;
        frame.height = height_;
        frame.pts = pts;
        if (!AttachNativeOutput(frame, logs)) {
            frame.Release();
            return DecodeResult::Failed;
        }

        ++outputs_;
        if (ShouldLogFrequent(outputs_)) {
            logs.push_back("AVC420 GPU decoded native output: " +
                NativeFrameText(frame) + " pushed=" + std::to_string(pushed_) +
                " outputs=" + std::to_string(outputs_));
        }
        return DecodeResult::Decoded;
    }

    ++noOutput_;
    if (ShouldLogFrequent(noOutput_)) {
        logs.push_back("AVC420 GPU synchronous output wait timed out: pts=" +
            std::to_string(pts) + " waitedUs=" + std::to_string(waitedUs) +
            " budgetUs=" + std::to_string(kOutputSyncDeadlineUs) +
            " attempts=" + std::to_string(kOutputSyncMaxAttempts) +
            " noOutput=" + std::to_string(noOutput_));
    }
    return DecodeResult::NoOutput;
}

bool Avc420HardwareDecoder::ConfigureWithPixelFormat(int32_t pixelFormat, std::vector<std::string>& logs)
{
    OH_AVFormat* format = OH_AVFormat_CreateVideoFormat(
        kAvcMime, static_cast<int32_t>(width_), static_cast<int32_t>(height_));
    if (format == nullptr) {
        logs.push_back("AVC420 GPU format create failed");
        return false;
    }

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, pixelFormat);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
    OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, kAvc420DecoderFrameRate);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE,
        static_cast<int32_t>(std::max<uint32_t>(width_ * height_, 1024 * 1024)));

    const OH_AVErrCode rc = OH_VideoDecoder_Configure(decoder_, format);
    OH_AVFormat_Destroy(format);
    if (rc != AV_ERR_OK) {
        logs.push_back("AVC420 GPU decoder configure failed rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " pixelFormat=" + DecoderPixelFormatName(pixelFormat) +
            "(" + std::to_string(pixelFormat) + ")");
        return false;
    }

    pixelFormat_ = pixelFormat;
    return true;
}

void Avc420HardwareDecoder::PushEmptyInput(OH_AVBuffer* input, uint32_t inputIndex)
{
    if (decoder_ == nullptr || input == nullptr) {
        return;
    }
    OH_AVCodecBufferAttr empty {};
    OH_AVBuffer_SetBufferAttr(input, &empty);
    OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
}

void Avc420HardwareDecoder::UpdateOutputDescription(std::vector<std::string>& logs, const std::string& reason)
{
    if (decoder_ == nullptr) {
        return;
    }
    OH_AVFormat* description = OH_VideoDecoder_GetOutputDescription(decoder_);
    if (description == nullptr) {
        return;
    }
    int32_t pixelFormat = 0;
    OH_AVFormat_GetIntValue(description, OH_MD_KEY_PIXEL_FORMAT, &pixelFormat);
    OH_AVFormat_Destroy(description);
    if (pixelFormat > 0) {
        outputPixelFormat_ = pixelFormat;
    }
    logs.push_back("AVC420 GPU output description after " + reason +
        ": pixelFormat=" + DecoderPixelFormatName(outputPixelFormat_) +
        "(" + std::to_string(outputPixelFormat_) + ")" +
        " configuredFrameRate=" + FormatFixed(kAvc420DecoderFrameRate, 1));
}

bool Avc420HardwareDecoder::AttachNativeOutput(NativeDecodedFrame& frame, std::vector<std::string>& logs)
{
    frame.nativeBuffer = OH_AVBuffer_GetNativeBuffer(frame.buffer);
    if (frame.nativeBuffer == nullptr) {
        logs.push_back("AVC420 GPU decoded output has no OH_NativeBuffer; "
            "native-buffer-only path rejects mapped fallback");
        return false;
    }

    OH_NativeBuffer_Config config {};
    OH_NativeBuffer_GetConfig(frame.nativeBuffer, &config);
    frame.nativeFormat = config.format;
    frame.nativeWidth = static_cast<uint32_t>(std::max(0, config.width));
    frame.nativeHeight = static_cast<uint32_t>(std::max(0, config.height));
    frame.nativeStride = static_cast<uint32_t>(std::max(0, config.stride));
    return true;
}

} // namespace rdp_bridge
