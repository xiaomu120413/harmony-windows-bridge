#include "surface/avc420_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "surface/native_rgba_copy.h"

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>

namespace rdp_bridge {

const char* ActiveAvc420UpdatePolicyName(ActiveAvc420UpdatePolicy policy)
{
    switch (policy) {
        case ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner:
            return "reset-decoder-preserve-owner";
        case ActiveAvc420UpdatePolicy::ReleaseOwner:
            return "release-owner";
        case ActiveAvc420UpdatePolicy::PreserveOwner:
        default:
            return "preserve-owner";
    }
}

RenderViewport FitAvc420PresentViewport(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t sourceWidth, uint32_t sourceHeight, bool& snapped)
{
    const RenderViewport viewport = FitFrameIntoTarget(
        targetWidth, targetHeight, sourceWidth, sourceHeight);
    snapped = viewport.width == targetWidth && viewport.height == targetHeight;
    return viewport;
}

std::string FormatFixed(double value, int precision)
{
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

std::string FormatMs(uint64_t valueUs)
{
    return FormatFixed(static_cast<double>(valueUs) / 1000.0, 1);
}

int64_t MakeDecoderPts(uint32_t frameId, uint64_t sequence)
{
    return static_cast<int64_t>(frameId) * 1000LL +
        static_cast<int64_t>(sequence % 1000ULL);
}

std::string NativeBufferFormatName(OH_NativeBuffer_Format format)
{
    switch (format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            return "YCBCR_420_SP";
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            return "YCRCB_420_SP";
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            return "YCBCR_420_P";
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_P:
            return "YCRCB_420_P";
        case NATIVEBUFFER_PIXEL_FMT_Y8:
            return "Y8";
        case NATIVEBUFFER_PIXEL_FMT_RGBX_8888:
            return "RGBX_8888";
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            return "RGBA_8888";
        case NATIVEBUFFER_PIXEL_FMT_BGRX_8888:
            return "BGRX_8888";
        case NATIVEBUFFER_PIXEL_FMT_BGRA_8888:
            return "BGRA_8888";
        default:
            return std::to_string(static_cast<int32_t>(format));
    }
}

std::string NativeFrameText(const NativeDecodedFrame& frame)
{
    return "pts=" + std::to_string(frame.pts) +
        " logical=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
        " native=" + std::to_string(frame.nativeWidth) + "x" +
        std::to_string(frame.nativeHeight) +
        " nativeStride=" + std::to_string(frame.nativeStride) +
        " nativeFormat=" +
        NativeBufferFormatName(static_cast<OH_NativeBuffer_Format>(frame.nativeFormat)) +
        " nativeBuffer=" + std::string(frame.nativeBuffer != nullptr ? "yes" : "no");
}

NativeDecodedFrame::NativeDecodedFrame() = default;

NativeDecodedFrame::NativeDecodedFrame(NativeDecodedFrame&& other) noexcept
{
    MoveFrom(other);
}

NativeDecodedFrame& NativeDecodedFrame::operator=(NativeDecodedFrame&& other) noexcept
{
    if (this != &other) {
        Release();
        MoveFrom(other);
    }
    return *this;
}

NativeDecodedFrame::~NativeDecodedFrame()
{
    Release();
}

void NativeDecodedFrame::Release()
{
    if (nativeBuffer != nullptr) {
        OH_NativeBuffer_Unreference(nativeBuffer);
    }
    if (codec != nullptr && hasOutputIndex) {
        OH_VideoDecoder_FreeOutputBuffer(codec, outputIndex);
    }
    codec = nullptr;
    buffer = nullptr;
    nativeBuffer = nullptr;
    outputIndex = 0;
    hasOutputIndex = false;
    width = 0;
    height = 0;
    nativeWidth = 0;
    nativeHeight = 0;
    nativeStride = 0;
    nativeFormat = 0;
    pts = 0;
}

void NativeDecodedFrame::MoveFrom(NativeDecodedFrame& other)
{
    codec = other.codec;
    buffer = other.buffer;
    nativeBuffer = other.nativeBuffer;
    outputIndex = other.outputIndex;
    hasOutputIndex = other.hasOutputIndex;
    width = other.width;
    height = other.height;
    nativeWidth = other.nativeWidth;
    nativeHeight = other.nativeHeight;
    nativeStride = other.nativeStride;
    nativeFormat = other.nativeFormat;
    pts = other.pts;

    other.codec = nullptr;
    other.buffer = nullptr;
    other.nativeBuffer = nullptr;
    other.hasOutputIndex = false;
}

} // namespace rdp_bridge
