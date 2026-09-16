#include "surface/avc444_gpu_compositor_internal_types.h"
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

Avc444HardwareDecoder::~Avc444HardwareDecoder()
{
    Close();
}

void Avc444HardwareDecoder::Close()
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
    pixelFormat_ = AV_PIXEL_FORMAT_NV12;
    outputStride_ = 0;
    outputSliceHeight_ = 0;
    outputPixelFormat_ = 0;
    role_.clear();
}

bool Avc444HardwareDecoder::Ensure(uint32_t width, uint32_t height, const std::string& role,
    std::vector<std::string>& logs)
{
    if (decoder_ != nullptr && started_ && width_ == width && height_ == height &&
        role_ == role) {
        return true;
    }

    Close();
    role_ = role;
    width_ = width;
    height_ = height;

    OH_AVCapability* capability =
        OH_AVCodec_GetCapabilityByCategory(kAvcMime, false, HARDWARE);
    const char* name = capability == nullptr ? nullptr : OH_AVCapability_GetName(capability);
    decoder_ = (name != nullptr && name[0] != '\0') ?
        OH_VideoDecoder_CreateByName(name) : OH_VideoDecoder_CreateByMime(kAvcMime);
    if (decoder_ == nullptr) {
        logs.push_back(LogPrefix() + "decoder create failed name=" +
            SafeCString(name));
        return false;
    }

    bool isValid = false;
    OH_AVErrCode rc = OH_VideoDecoder_IsValid(decoder_, &isValid);
    if (rc != AV_ERR_OK || !isValid) {
        logs.push_back(LogPrefix() + "decoder invalid rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " valid=" + std::to_string(isValid ? 1 : 0));
        Close();
        return false;
    }

    if (!ConfigureWithPixelFormat(AV_PIXEL_FORMAT_NV12, logs) &&
        !ConfigureWithPixelFormat(AV_PIXEL_FORMAT_NV21, logs)) {
        Close();
        return false;
    }

    rc = OH_VideoDecoder_Prepare(decoder_);
    if (rc != AV_ERR_OK) {
        logs.push_back(LogPrefix() + "decoder prepare failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        Close();
        return false;
    }

    rc = OH_VideoDecoder_Start(decoder_);
    if (rc != AV_ERR_OK) {
        logs.push_back(LogPrefix() + "decoder start failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        Close();
        return false;
    }

    started_ = true;
    UpdateOutputDescription(logs, "start");
    logs.push_back(LogPrefix() + "decoder ready: " +
        std::to_string(width_) + "x" + std::to_string(height_) +
        " requestedPixelFormat=" + std::to_string(pixelFormat_) +
        " bounded-sync-mode outputDeadlineUs=" + std::to_string(kOutputSyncDeadlineUs));
    return true;
}

bool Avc444HardwareDecoder::Started() const
{
    return decoder_ != nullptr && started_;
}

DecodeResult Avc444HardwareDecoder::Decode(const uint8_t* data, uint32_t size, int64_t pts,
    DecodedFrame& frame, std::vector<std::string>& logs)
{
    if (decoder_ == nullptr || !started_ || data == nullptr || size == 0) {
        logs.push_back(LogPrefix() + "decode skipped: invalid input");
        return DecodeResult::Failed;
    }

    uint32_t inputIndex = 0;
    OH_AVErrCode rc = OH_VideoDecoder_QueryInputBuffer(decoder_, &inputIndex, kInputTimeoutUs);
    if (rc != AV_ERR_OK) {
        logs.push_back(LogPrefix() + "input unavailable rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " size=" + std::to_string(size));
        return DecodeResult::Failed;
    }

    OH_AVBuffer* input = OH_VideoDecoder_GetInputBuffer(decoder_, inputIndex);
    uint8_t* dst = input == nullptr ? nullptr : OH_AVBuffer_GetAddr(input);
    const int32_t capacity = input == nullptr ? -1 : OH_AVBuffer_GetCapacity(input);
    if (dst == nullptr || capacity < 0 || static_cast<uint32_t>(capacity) < size) {
        logs.push_back(LogPrefix() + "input buffer invalid capacity=" +
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
        logs.push_back(LogPrefix() + "set input attr failed rc=" +
            std::to_string(static_cast<int32_t>(rc)));
        PushEmptyInput(input, inputIndex);
        return DecodeResult::Failed;
    }

    rc = OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
    if (rc != AV_ERR_OK) {
        logs.push_back(LogPrefix() + "push input failed rc=" +
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
        rc = OH_VideoDecoder_QueryOutputBuffer(
            decoder_, &outputIndex, timeout);
        if (rc == AV_ERR_STREAM_CHANGED) {
            UpdateOutputDescription(logs, "stream-changed");
            continue;
        }
        if (rc == AV_ERR_TRY_AGAIN_LATER) {
            waitedUs += timeout;
            continue;
        }
        if (rc != AV_ERR_OK) {
            logs.push_back(LogPrefix() + "query output failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            return DecodeResult::Failed;
        }

        OH_AVBuffer* output = OH_VideoDecoder_GetOutputBuffer(decoder_, outputIndex);
        OH_AVCodecBufferAttr outputAttr {};
        if (output == nullptr ||
            OH_AVBuffer_GetBufferAttr(output, &outputAttr) != AV_ERR_OK) {
            OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
            logs.push_back(LogPrefix() + "output buffer invalid");
            return DecodeResult::Failed;
        }

        if (outputAttr.pts != pts) {
            OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
            logs.push_back(LogPrefix() + "discarded stale output pts=" +
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
        frame.alignedWidth = AlignUp(width_, 32);
        frame.alignedHeight = AlignUp(height_, 16);
        frame.pts = pts;
        if (!MapOutput(frame, logs)) {
            frame.Release();
            return DecodeResult::Failed;
        }

        ++outputs_;
        if (ShouldLogFrequent(outputs_)) {
            logs.push_back(LogPrefix() + "decoded output: pts=" +
                std::to_string(pts) +
                " nativeFormat=" + NativeBufferFormatName(
                    static_cast<OH_NativeBuffer_Format>(frame.nativeFormat)) +
                " yStride=" + std::to_string(frame.y.rowStride) +
                " uvStride=" + std::to_string(frame.uv.rowStride) +
                " uvColumn=" + std::to_string(frame.uv.columnStride) +
                " yUploadWidth=" + std::to_string(frame.yUploadWidth) +
                " uvUpload=" + std::to_string(frame.uvUploadWidth) + "x" +
                std::to_string(frame.uvUploadHeight) +
                " order=" + std::string(frame.nv21 ? "NV21" : "NV12"));
        }
        return DecodeResult::Decoded;
    }

    ++noOutput_;
    if (ShouldLogFrequent(noOutput_)) {
        logs.push_back(LogPrefix() + "synchronous output wait timed out: pts=" +
            std::to_string(pts) + " waitedUs=" + std::to_string(waitedUs) +
            " budgetUs=" + std::to_string(kOutputSyncDeadlineUs) +
            " attempts=" + std::to_string(kOutputSyncMaxAttempts) +
            " noOutput=" + std::to_string(noOutput_));
    }
    return DecodeResult::NoOutput;
}

bool Avc444HardwareDecoder::MapDecodedFrame(DecodedFrame& frame, std::vector<std::string>& logs)
{
    if (frame.Valid()) {
        return true;
    }
    if (decoder_ == nullptr || !started_ || frame.codec != decoder_ || frame.buffer == nullptr) {
        logs.push_back(LogPrefix() +
            " mapped fallback rejected: decoded output is not owned by this decoder");
        return false;
    }
    return MapOutput(frame, logs);
}

std::string Avc444HardwareDecoder::LogPrefix() const
{
    return CodecRoleLogPrefix(role_);
}

bool Avc444HardwareDecoder::ConfigureWithPixelFormat(int32_t pixelFormat,
    std::vector<std::string>& logs)
{
    OH_AVFormat* format = OH_AVFormat_CreateVideoFormat(
        kAvcMime, static_cast<int32_t>(width_), static_cast<int32_t>(height_));
    if (format == nullptr) {
        logs.push_back(LogPrefix() + "format create failed");
        return false;
    }

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, pixelFormat);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY, 1);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE,
        static_cast<int32_t>(std::max<uint32_t>(width_ * height_, 1024 * 1024)));

    const OH_AVErrCode rc = OH_VideoDecoder_Configure(decoder_, format);
    OH_AVFormat_Destroy(format);
    if (rc != AV_ERR_OK) {
        logs.push_back(LogPrefix() + "decoder configure failed rc=" +
            std::to_string(static_cast<int32_t>(rc)) +
            " pixelFormat=" + std::to_string(pixelFormat));
        return false;
    }

    pixelFormat_ = pixelFormat;
    return true;
}

void Avc444HardwareDecoder::PushEmptyInput(OH_AVBuffer* input, uint32_t inputIndex)
{
    if (decoder_ == nullptr || input == nullptr) {
        return;
    }
    OH_AVCodecBufferAttr empty {};
    OH_AVBuffer_SetBufferAttr(input, &empty);
    OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
}

void Avc444HardwareDecoder::UpdateOutputDescription(std::vector<std::string>& logs,
    const std::string& reason)
{
    if (decoder_ == nullptr) {
        return;
    }
    OH_AVFormat* description = OH_VideoDecoder_GetOutputDescription(decoder_);
    if (description == nullptr) {
        return;
    }
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    int32_t pixelFormat = 0;
    OH_AVFormat_GetIntValue(description, OH_MD_KEY_VIDEO_STRIDE, &stride);
    OH_AVFormat_GetIntValue(description, OH_MD_KEY_VIDEO_SLICE_HEIGHT, &sliceHeight);
    OH_AVFormat_GetIntValue(description, OH_MD_KEY_PIXEL_FORMAT, &pixelFormat);
    OH_AVFormat_Destroy(description);

    if (stride > 0) {
        outputStride_ = static_cast<uint32_t>(stride);
    }
    if (sliceHeight > 0) {
        outputSliceHeight_ = static_cast<uint32_t>(sliceHeight);
    }
    if (pixelFormat > 0) {
        outputPixelFormat_ = pixelFormat;
    }
    logs.push_back(LogPrefix() + "output description after " + reason +
        ": stride=" + std::to_string(outputStride_) +
        " sliceHeight=" + std::to_string(outputSliceHeight_) +
        " pixelFormat=" + std::to_string(outputPixelFormat_));
}

bool Avc444HardwareDecoder::MapOutput(DecodedFrame& frame, std::vector<std::string>& logs)
{
    if (frame.nativeBuffer == nullptr) {
        frame.nativeBuffer = OH_AVBuffer_GetNativeBuffer(frame.buffer);
    }
    if (frame.nativeBuffer != nullptr) {
        OH_NativeBuffer_Config config {};
        OH_NativeBuffer_GetConfig(frame.nativeBuffer, &config);
        OH_NativeBuffer_Planes planes {};
        void* address = nullptr;
        const int32_t mapRc = OH_NativeBuffer_MapPlanes(frame.nativeBuffer, &address, &planes);
        if (mapRc == 0 && address != nullptr && planes.planeCount >= 2) {
            frame.mapped = true;
            frame.mappedAddress = address;
            frame.nativeFormat = config.format;
            frame.nativeWidth = static_cast<uint32_t>(std::max(0, config.width));
            frame.nativeHeight = static_cast<uint32_t>(std::max(0, config.height));
            frame.nativeStride = static_cast<uint32_t>(std::max(0, config.stride));
            frame.nv21 = config.format == NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP;
            frame.y.data = static_cast<const uint8_t*>(address) + planes.planes[0].offset;
            frame.y.rowStride = planes.planes[0].rowStride;
            frame.y.columnStride = planes.planes[0].columnStride == 0 ?
                1 : planes.planes[0].columnStride;
            frame.uv.data = static_cast<const uint8_t*>(address) + planes.planes[1].offset;
            frame.uv.rowStride = planes.planes[1].rowStride;
            frame.uv.columnStride = planes.planes[1].columnStride == 0 ?
                2 : planes.planes[1].columnStride;
            const uint32_t yMinRow = frame.width;
            const uint32_t uvMinRow = frame.alignedWidth;
            const auto looksSwapped = [](uint32_t row, uint32_t col, uint32_t minRow) {
                return col >= minRow && row < minRow;
            };
            const bool ySwapped = looksSwapped(frame.y.rowStride, frame.y.columnStride, yMinRow);
            const bool uvSwapped = looksSwapped(frame.uv.rowStride, frame.uv.columnStride, uvMinRow);
            if (ySwapped || uvSwapped) {
                if (ShouldLogFrequent(outputs_ + 1U)) {
                    logs.push_back(LogPrefix() +
                        " normalized native plane strides ySwap=" +
                        std::string(ySwapped ? "yes" : "no") +
                        " uvSwap=" + std::string(uvSwapped ? "yes" : "no") +
                        " before: yRow=" + std::to_string(frame.y.rowStride) +
                        " yColumn=" + std::to_string(frame.y.columnStride) +
                        " uvRow=" + std::to_string(frame.uv.rowStride) +
                        " uvColumn=" + std::to_string(frame.uv.columnStride));
                }
                if (ySwapped) {
                    std::swap(frame.y.rowStride, frame.y.columnStride);
                }
                if (uvSwapped) {
                    std::swap(frame.uv.rowStride, frame.uv.columnStride);
                }
            }
            if (frame.y.columnStride != 1 || frame.uv.columnStride != 2) {
                logs.push_back(LogPrefix() +
                    " unexpected native plane columnStride yColumn=" +
                    std::to_string(frame.y.columnStride) +
                    " uvColumn=" + std::to_string(frame.uv.columnStride) +
                    " (NV12 expects 1/2); falling back to avbuffer-memory layout");
                OH_NativeBuffer_Unmap(frame.nativeBuffer);
                frame.mapped = false;
                frame.mappedAddress = nullptr;
                OH_NativeBuffer_Unreference(frame.nativeBuffer);
                frame.nativeBuffer = nullptr;
            } else {
                return FinishPlaneLayout(frame, logs, "native-buffer");
            }
        }

        logs.push_back(LogPrefix() + "native buffer map failed rc=" +
            std::to_string(mapRc) + " planeCount=" + std::to_string(planes.planeCount));
        OH_NativeBuffer_Unreference(frame.nativeBuffer);
        frame.nativeBuffer = nullptr;
    }

    uint8_t* address = OH_AVBuffer_GetAddr(frame.buffer);
    const int32_t capacity = OH_AVBuffer_GetCapacity(frame.buffer);
    const uint32_t stride = outputStride_ > 0 ? outputStride_ : AlignUp(frame.width, 16);
    const uint32_t sliceHeight = outputSliceHeight_ > 0 ? outputSliceHeight_ : AlignUp(frame.height, 16);
    const uint64_t uvOffset = static_cast<uint64_t>(stride) * sliceHeight;
    if (address == nullptr || capacity <= 0 ||
        uvOffset + static_cast<uint64_t>(stride) * ((frame.height + 1U) / 2U) >
            static_cast<uint64_t>(capacity)) {
        logs.push_back(LogPrefix() + "output has no mappable planes capacity=" +
            std::to_string(capacity) + " stride=" + std::to_string(stride) +
            " sliceHeight=" + std::to_string(sliceHeight));
        return false;
    }

    frame.nativeFormat = outputPixelFormat_;
    frame.nativeWidth = stride;
    frame.nativeHeight = sliceHeight;
    frame.nativeStride = stride;
    frame.nv21 = outputPixelFormat_ == AV_PIXEL_FORMAT_NV21;
    frame.y.data = address;
    frame.y.rowStride = stride;
    frame.y.columnStride = 1;
    frame.uv.data = address + uvOffset;
    frame.uv.rowStride = stride;
    frame.uv.columnStride = 2;
    return FinishPlaneLayout(frame, logs, "avbuffer-memory");
}

bool Avc444HardwareDecoder::FinishPlaneLayout(DecodedFrame& frame, std::vector<std::string>& logs,
    const std::string& source)
{
    if (frame.y.columnStride != 1 || frame.uv.columnStride != 2 ||
        frame.y.rowStride < frame.width || frame.uv.rowStride < frame.alignedWidth) {
        logs.push_back(LogPrefix() + "unsupported plane layout from " + source +
            ": yColumn=" + std::to_string(frame.y.columnStride) +
            " uvColumn=" + std::to_string(frame.uv.columnStride) +
            " yStride=" + std::to_string(frame.y.rowStride) +
            " uvStride=" + std::to_string(frame.uv.rowStride) +
            " alignedWidth=" + std::to_string(frame.alignedWidth));
        return false;
    }

    frame.yUploadWidth = std::min(frame.y.rowStride, std::max(frame.alignedWidth, frame.width));
    frame.yUploadHeight = std::max(frame.height,
        std::min(frame.nativeHeight > 0 ? frame.nativeHeight : frame.alignedHeight,
            std::max(frame.alignedHeight, frame.height)));
    frame.uvUploadWidth = std::min(frame.uv.rowStride / 2U, std::max(frame.alignedWidth / 2U,
        (frame.width + 1U) / 2U));
    frame.uvUploadHeight = (frame.height + 1U) / 2U;
    return frame.yUploadWidth >= frame.alignedWidth &&
        frame.yUploadHeight >= frame.height &&
        frame.uvUploadWidth >= (frame.alignedWidth / 2U) &&
        frame.uvUploadHeight > 0;
}

} // namespace rdp_bridge
