#include "surface/avc420_gpu_compositor_internal.h"

#include "common/frame_utils.h"
#include "common/string_utils.h"
#include "freerdp/freerdp_runtime.h"
#include "surface/native_rgba_copy.h"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl3.h>
#include <multimedia/player_framework/native_avbuffer.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>
#include <multimedia/player_framework/native_avformat.h>
#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

namespace rdp_bridge {
namespace {
constexpr const char* kAvcMime = "video/avc";
constexpr int64_t kInputTimeoutUs = 20000;
constexpr int64_t kOutputTimeoutUs = 8000;
constexpr int64_t kFollowupOutputTimeoutUs = 4000;
constexpr int64_t kOutputSyncDeadlineUs = 32000;
constexpr uint32_t kOutputSyncMaxAttempts = 8;
constexpr uint64_t kTimingSampleInterval = 60U;
constexpr uint64_t kActiveFailureFallbackThreshold = 3U;
constexpr uint64_t kActiveResetIgnoreFallbackThreshold = 6U;
constexpr uint32_t kPresentSnapTolerancePx = 16U;
constexpr double kAvc420DecoderFrameRate = 60.0;

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef EGL_NATIVE_BUFFER_OHOS
#define EGL_NATIVE_BUFFER_OHOS 0x34E1
#endif

bool ShouldLogFrequent(uint64_t count)
{
    return count == 1U || (count % 600U) == 0U;
}

bool ShouldSampleTiming(uint64_t count)
{
    return count <= 3U || (count % kTimingSampleInterval) == 0U;
}

enum class ActiveAvc420UpdatePolicy {
    PreserveOwner,
    ResetDecoderAndPreserveOwner,
    ReleaseOwner,
};

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

uint64_t NowMicros()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

uint32_t DimensionDelta(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}

RenderViewport FitAvc420PresentViewport(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t sourceWidth, uint32_t sourceHeight, bool& snapped)
{
    snapped = false;
    RenderViewport viewport;
    if (targetWidth == 0 || targetHeight == 0 || sourceWidth == 0 || sourceHeight == 0) {
        return viewport;
    }

    if (DimensionDelta(targetWidth, sourceWidth) <= kPresentSnapTolerancePx &&
        DimensionDelta(targetHeight, sourceHeight) <= kPresentSnapTolerancePx) {
        viewport.width = targetWidth;
        viewport.height = targetHeight;
        snapped = true;
        return viewport;
    }

    return FitFrameIntoTarget(targetWidth, targetHeight, sourceWidth, sourceHeight);
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

struct TimingBucket {
    uint64_t count = 0;
    uint64_t totalUs = 0;
    uint64_t maxUs = 0;

    void Add(uint64_t valueUs)
    {
        ++count;
        totalUs += valueUs;
        maxUs = std::max(maxUs, valueUs);
    }

    void Reset()
    {
        count = 0;
        totalUs = 0;
        maxUs = 0;
    }

    std::string Text(const char* name) const
    {
        std::ostringstream out;
        out << name << ":";
        if (count == 0) {
            out << "0/0/0";
        } else {
            out << (totalUs / count) << "/" << maxUs << "/" << count;
        }
        return out.str();
    }
};

class ScopedTiming {
public:
    ScopedTiming(TimingBucket& bucket, bool enabled)
        : bucket_(enabled ? &bucket : nullptr), startUs_(enabled ? NowMicros() : 0)
    {}
    ScopedTiming(TimingBucket& bucket, uint64_t startUs, bool enabled)
        : bucket_(enabled ? &bucket : nullptr), startUs_(enabled ? startUs : 0)
    {}

    ~ScopedTiming()
    {
        if (bucket_ == nullptr) {
            return;
        }
        const uint64_t nowUs = NowMicros();
        bucket_->Add(nowUs >= startUs_ ? nowUs - startUs_ : 0);
    }

private:
    TimingBucket* bucket_ = nullptr;
    uint64_t startUs_ = 0;
};

uint32_t ReadBe32(const uint8_t* data)
{
    return (static_cast<uint32_t>(data[0]) << 24U) |
        (static_cast<uint32_t>(data[1]) << 16U) |
        (static_cast<uint32_t>(data[2]) << 8U) |
        static_cast<uint32_t>(data[3]);
}

bool FindAnnexBStartCode(const uint8_t* data, uint32_t size, uint32_t from,
    uint32_t& start, uint32_t& prefixLength)
{
    if (data == nullptr || size < 4 || from >= size) {
        return false;
    }
    for (uint32_t i = from; i + 3 < size; ++i) {
        if (data[i] == 0 && data[i + 1] == 0 && data[i + 2] == 1) {
            start = i;
            prefixLength = 3;
            return true;
        }
        if (i + 4 < size && data[i] == 0 && data[i + 1] == 0 &&
            data[i + 2] == 0 && data[i + 3] == 1) {
            start = i;
            prefixLength = 4;
            return true;
        }
    }
    return false;
}

bool ExtractH264ParameterSets(const uint8_t* data, uint32_t size,
    std::vector<uint8_t>& parameterSets, std::string* nalSummary)
{
    parameterSets.clear();
    if (data == nullptr || size < 5) {
        if (nalSummary != nullptr) {
            *nalSummary = "empty";
        }
        return false;
    }

    std::ostringstream summary;
    bool first = true;
    bool parsed = false;

    uint32_t start = 0;
    uint32_t prefixLength = 0;
    if (FindAnnexBStartCode(data, size, 0, start, prefixLength)) {
        parsed = true;
        uint32_t currentStart = start;
        uint32_t currentPrefix = prefixLength;
        while (currentStart < size) {
            const uint32_t nalOffset = currentStart + currentPrefix;
            if (nalOffset >= size) {
                break;
            }
            uint32_t nextStart = size;
            uint32_t nextPrefix = 0;
            FindAnnexBStartCode(data, size, nalOffset + 1, nextStart, nextPrefix);
            const uint8_t nalType = data[nalOffset] & 0x1FU;
            if (!first) {
                summary << ",";
            }
            summary << static_cast<uint32_t>(nalType);
            first = false;
            if (nalType == 7 || nalType == 8) {
                parameterSets.insert(parameterSets.end(), data + currentStart, data + nextStart);
            }
            if (nextStart == size) {
                break;
            }
            currentStart = nextStart;
            currentPrefix = nextPrefix;
        }
    } else {
        uint32_t offset = 0;
        while (offset + 4U < size) {
            const uint32_t nalSize = ReadBe32(data + offset);
            if (nalSize == 0 || nalSize > size - offset - 4U) {
                parsed = false;
                break;
            }
            parsed = true;
            const uint32_t nalOffset = offset + 4U;
            const uint8_t nalType = data[nalOffset] & 0x1FU;
            if (!first) {
                summary << ",";
            }
            summary << static_cast<uint32_t>(nalType);
            first = false;
            if (nalType == 7 || nalType == 8) {
                parameterSets.insert(parameterSets.end(), data + offset,
                    data + offset + 4U + nalSize);
            }
            offset += 4U + nalSize;
        }
    }

    if (nalSummary != nullptr) {
        *nalSummary = parsed ? summary.str() : "unparsed";
    }
    return !parameterSets.empty();
}

struct PreparedH264Packet {
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    bool hadParameterSets = false;
    bool prependedParameterSets = false;
    std::string nalSummary;
    std::vector<uint8_t> storage;
};

PreparedH264Packet PrepareH264Packet(const uint8_t* data, uint32_t size, bool decoderStarted,
    std::vector<uint8_t>& parameterSets, std::vector<std::string>& logs)
{
    PreparedH264Packet packet;
    packet.data = data;
    packet.size = size;
    if (data == nullptr || size == 0) {
        packet.nalSummary = "empty";
        return packet;
    }

    std::vector<uint8_t> extracted;
    packet.hadParameterSets = ExtractH264ParameterSets(data, size, extracted, &packet.nalSummary);
    if (packet.hadParameterSets) {
        parameterSets = std::move(extracted);
    } else if (!decoderStarted && !parameterSets.empty()) {
        packet.storage.reserve(parameterSets.size() + size);
        packet.storage.insert(packet.storage.end(), parameterSets.begin(), parameterSets.end());
        packet.storage.insert(packet.storage.end(), data, data + size);
        packet.data = packet.storage.data();
        packet.size = static_cast<uint32_t>(packet.storage.size());
        packet.prependedParameterSets = true;
        logs.push_back("AVC420 GPU prepended H264 parameter sets: parameterSets=" +
            std::to_string(parameterSets.size()) + " payload=" + std::to_string(size) +
            " nalTypes=" + packet.nalSummary);
    }
    return packet;
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

bool RectsValid(const RECTANGLE_16* rects, uint32_t count, uint32_t width, uint32_t height)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    return EnsureFreerdpRuntimeLoaded(api, error) && api.ohosRdpgfxRectsValid != nullptr &&
        api.ohosRdpgfxRectsValid(rects, count, width, height) != FALSE;
}

std::string FormatRectText(const RECTANGLE_16* rect)
{
    if (rect == nullptr) {
        return "none";
    }
    return std::to_string(rect->left) + "," + std::to_string(rect->top) + "-" +
        std::to_string(rect->right) + "," + std::to_string(rect->bottom);
}

std::string RectsText(const RECTANGLE_16* rects, uint32_t count)
{
    if (rects == nullptr || count == 0) {
        return "rects:0,first:none,last:none,bounds:none,area:0";
    }

    uint32_t left = rects[0].left;
    uint32_t top = rects[0].top;
    uint32_t right = rects[0].right;
    uint32_t bottom = rects[0].bottom;
    uint64_t area = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const RECTANGLE_16& rect = rects[i];
        left = std::min<uint32_t>(left, rect.left);
        top = std::min<uint32_t>(top, rect.top);
        right = std::max<uint32_t>(right, rect.right);
        bottom = std::max<uint32_t>(bottom, rect.bottom);
        area += static_cast<uint64_t>(rect.right - rect.left) * (rect.bottom - rect.top);
    }

    const RECTANGLE_16 bounds {
        static_cast<UINT16>(left),
        static_cast<UINT16>(top),
        static_cast<UINT16>(right),
        static_cast<UINT16>(bottom),
    };
    return "rects:" + std::to_string(count) +
        ",first:" + FormatRectText(rects) +
        ",last:" + FormatRectText(rects + count - 1U) +
        ",bounds:" + FormatRectText(&bounds) +
        ",area:" + std::to_string(area);
}

enum class DecodeResult {
    Decoded,
    NoOutput,
    Failed,
};

struct NativeDecodedFrame {
    OH_AVCodec* codec = nullptr;
    OH_AVBuffer* buffer = nullptr;
    OH_NativeBuffer* nativeBuffer = nullptr;
    uint32_t outputIndex = 0;
    bool hasOutputIndex = false;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t nativeWidth = 0;
    uint32_t nativeHeight = 0;
    uint32_t nativeStride = 0;
    int32_t nativeFormat = 0;
    int64_t pts = 0;

    NativeDecodedFrame() = default;
    NativeDecodedFrame(const NativeDecodedFrame&) = delete;
    NativeDecodedFrame& operator=(const NativeDecodedFrame&) = delete;

    NativeDecodedFrame(NativeDecodedFrame&& other) noexcept
    {
        MoveFrom(other);
    }

    NativeDecodedFrame& operator=(NativeDecodedFrame&& other) noexcept
    {
        if (this != &other) {
            Release();
            MoveFrom(other);
        }
        return *this;
    }

    ~NativeDecodedFrame()
    {
        Release();
    }

    bool Valid() const
    {
        return codec != nullptr && buffer != nullptr && nativeBuffer != nullptr &&
            hasOutputIndex && width > 0 && height > 0;
    }

    void Release()
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

private:
    void MoveFrom(NativeDecodedFrame& other)
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
};

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

class Avc420HardwareDecoder {
public:
    ~Avc420HardwareDecoder()
    {
        Close();
    }

    void Close()
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

    bool Started() const
    {
        return decoder_ != nullptr && started_;
    }

    bool Ensure(uint32_t width, uint32_t height, std::vector<std::string>& logs)
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

    DecodeResult Decode(const uint8_t* data, uint32_t size, int64_t pts,
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

private:
    bool ConfigureWithPixelFormat(int32_t pixelFormat, std::vector<std::string>& logs)
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

    void PushEmptyInput(OH_AVBuffer* input, uint32_t inputIndex)
    {
        if (decoder_ == nullptr || input == nullptr) {
            return;
        }
        OH_AVCodecBufferAttr empty {};
        OH_AVBuffer_SetBufferAttr(input, &empty);
        OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
    }

    void UpdateOutputDescription(std::vector<std::string>& logs, const std::string& reason)
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

    bool AttachNativeOutput(NativeDecodedFrame& frame, std::vector<std::string>& logs)
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

    OH_AVCodec* decoder_ = nullptr;
    bool started_ = false;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    int32_t pixelFormat_ = 0;
    int32_t outputPixelFormat_ = 0;
    uint64_t pushed_ = 0;
    uint64_t outputs_ = 0;
    uint64_t noOutput_ = 0;
};

GLuint CompileShader(GLenum type, const char* source, std::vector<std::string>& logs)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        logs.push_back("AVC420 native-buffer GPU GLES create shader failed: " +
            Hex32(static_cast<uint32_t>(glGetError())));
        return 0;
    }
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled == GL_TRUE) {
        return shader;
    }

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
    std::string info;
    if (length > 1) {
        info.resize(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetShaderInfoLog(shader, length, &written, info.data());
        info.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
    }
    logs.push_back("AVC420 native-buffer GPU GLES shader compile failed type=" +
        std::to_string(type) + " log=" + (info.empty() ? "none" : info));
    glDeleteShader(shader);
    return 0;
}

GLuint LinkProgram(const char* vertexSource, const char* fragmentSource,
    std::vector<std::string>& logs)
{
    const GLuint vertex = CompileShader(GL_VERTEX_SHADER, vertexSource, logs);
    if (vertex == 0) {
        return 0;
    }
    const GLuint fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource, logs);
    if (fragment == 0) {
        glDeleteShader(vertex);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked == GL_TRUE) {
        return program;
    }

    GLint length = 0;
    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
    std::string info;
    if (length > 1) {
        info.resize(static_cast<size_t>(length), '\0');
        GLsizei written = 0;
        glGetProgramInfoLog(program, length, &written, info.data());
        info.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
    }
    logs.push_back("AVC420 native-buffer GPU GLES program link failed log=" +
        (info.empty() ? "none" : info));
    glDeleteProgram(program);
    return 0;
}

class Avc420NativeBufferRenderer {
public:
    ~Avc420NativeBufferRenderer()
    {
        Destroy();
    }

    void Destroy()
    {
        if (display_ != EGL_NO_DISPLAY) {
            if (context_ != EGL_NO_CONTEXT &&
                (pbufferSurface_ != EGL_NO_SURFACE || windowSurface_ != EGL_NO_SURFACE)) {
                EGLSurface currentSurface =
                    pbufferSurface_ != EGL_NO_SURFACE ? pbufferSurface_ : windowSurface_;
                if (eglMakeCurrent(display_, currentSurface, currentSurface, context_)) {
                    DeleteCompositeSurface();
                    if (program_ != 0) {
                        glDeleteProgram(program_);
                    }
                    if (presentProgram_ != 0) {
                        glDeleteProgram(presentProgram_);
                    }
                    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                }
            }
            if (windowSurface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, windowSurface_);
            }
            if (pbufferSurface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, pbufferSurface_);
            }
            if (context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(display_, context_);
            }
            eglTerminate(display_);
        }

        display_ = EGL_NO_DISPLAY;
        config_ = nullptr;
        pbufferSurface_ = EGL_NO_SURFACE;
        windowSurface_ = EGL_NO_SURFACE;
        context_ = EGL_NO_CONTEXT;
        window_ = nullptr;
        targetWidth_ = 0;
        targetHeight_ = 0;
        surfaceWidth_ = 0;
        surfaceHeight_ = 0;
        program_ = 0;
        presentProgram_ = 0;
        compositeTexture_ = 0;
        compositeFramebuffer_ = 0;
        compositeWidth_ = 0;
        compositeHeight_ = 0;
        compositeReady_ = false;
        composites_ = 0;
        backgroundComposites_ = 0;
        compositeFailures_ = 0;
        imports_ = 0;
        importFailures_ = 0;
        presents_ = 0;
        rgbaUploadBuffer_.clear();
    }

    bool Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
        uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
    {
        if (surfaceWidth == 0 || surfaceHeight == 0 ||
            (window != nullptr && (targetWidth == 0 || targetHeight == 0))) {
            logs.push_back("AVC420 native-buffer GPU renderer target invalid");
            return false;
        }

        const uint32_t effectiveTargetWidth = window == nullptr ? surfaceWidth : targetWidth;
        const uint32_t effectiveTargetHeight = window == nullptr ? surfaceHeight : targetHeight;

        if (!EnsureInitialized(logs)) {
            return false;
        }

        surfaceWidth_ = surfaceWidth;
        surfaceHeight_ = surfaceHeight;
        if (window == nullptr) {
            return true;
        }

        if (window_ != window || windowSurface_ == EGL_NO_SURFACE) {
            DestroyWindowSurface();
            windowSurface_ = CreateWindowSurface(window, logs);
            if (windowSurface_ == EGL_NO_SURFACE) {
                window_ = nullptr;
                targetWidth_ = 0;
                targetHeight_ = 0;
                return false;
            }
            window_ = window;
            logs.push_back("AVC420 native-buffer GPU renderer attached EGL window surface");
        }

        targetWidth_ = effectiveTargetWidth;
        targetHeight_ = effectiveTargetHeight;
        return true;
    }

    bool CompositeFrame(const NativeDecodedFrame& frame, const RECTANGLE_16* rects,
        uint32_t rectCount, std::vector<std::string>& logs, bool logSuccess)
    {
        if (!frame.Valid()) {
            logs.push_back("AVC420 native-buffer GPU composite rejected invalid frame");
            return false;
        }
        if (!MakePbufferCurrent(logs)) {
            return false;
        }
        if (!EnsureCompositeSurface(logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        ImportedTexture imported;
        if (!ImportFrame(frame, imported, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, compositeFramebuffer_);
        glViewport(0, 0, static_cast<GLsizei>(surfaceWidth_),
            static_cast<GLsizei>(surfaceHeight_));
        glUseProgram(program_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, imported.texture);
        glUniform1i(glGetUniformLocation(program_, "uFrame"), 0);
        glUniform2f(glGetUniformLocation(program_, "uCropScale"), 1.0F, 1.0F);
        glUniform2f(glGetUniformLocation(program_, "uCropClamp"), 1.0F, 1.0F);

        const GLfloat nativeWidth = static_cast<GLfloat>(
            std::max<uint32_t>(surfaceWidth_, frame.nativeWidth > 0 ? frame.nativeWidth : frame.width));
        const GLfloat nativeHeight = static_cast<GLfloat>(
            std::max<uint32_t>(surfaceHeight_, frame.nativeHeight > 0 ? frame.nativeHeight : frame.height));
        uint32_t drawnRects = 0;
        for (uint32_t index = 0; index < rectCount; ++index) {
            const RECTANGLE_16& rect = rects[index];
            if (rect.left >= rect.right || rect.top >= rect.bottom ||
                rect.right > surfaceWidth_ || rect.bottom > surfaceHeight_) {
                continue;
            }
            DrawExternalRectToComposite(rect, nativeWidth, nativeHeight);
            ++drawnRects;
        }

        GLenum error = glGetError();
        ReleaseImport(imported);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR || drawnRects == 0) {
            ++compositeFailures_;
            logs.push_back("AVC420 native-buffer GPU composite failed: glError=" +
                Hex32(static_cast<uint32_t>(error)) +
                " drawnRects=" + std::to_string(drawnRects) +
                " inputRects=" + std::to_string(rectCount) + " " + NativeFrameText(frame));
            return false;
        }

        compositeReady_ = true;
        ++composites_;
        if (logSuccess) {
            logs.push_back("AVC420 native-buffer GPU composited dirty frame: rects=" +
                std::to_string(drawnRects) + "/" + std::to_string(rectCount) +
                " surface=" + std::to_string(surfaceWidth_) + "x" +
                std::to_string(surfaceHeight_) +
                " composites=" + std::to_string(composites_) +
                " " + NativeFrameText(frame));
        }
        return true;
    }

    bool CompositeRgbaFrame(const RgbaFrame& frame, std::vector<std::string>& logs,
        bool logSuccess)
    {
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            logs.push_back("AVC420 native-buffer GPU rejected invalid GDI background frame");
            return false;
        }
        if (frame.width != surfaceWidth_ || frame.height != surfaceHeight_) {
            logs.push_back("AVC420 native-buffer GPU rejected GDI background size mismatch: frame=" +
                std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                " surface=" + std::to_string(surfaceWidth_) + "x" +
                std::to_string(surfaceHeight_));
            return false;
        }
        if (!MakePbufferCurrent(logs)) {
            return false;
        }
        if (!EnsureCompositeSurface(logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        DirtyFrameStats dirty = frame.dirty;
        if (!dirty.valid || dirty.width == 0 || dirty.height == 0 ||
            dirty.x >= frame.width || dirty.y >= frame.height) {
            dirty.valid = true;
            dirty.rectCount = 1;
            dirty.x = 0;
            dirty.y = 0;
            dirty.width = frame.width;
            dirty.height = frame.height;
            dirty.areaPermille = 1000;
        }
        if (dirty.x + dirty.width > frame.width) {
            dirty.width = frame.width - dirty.x;
        }
        if (dirty.y + dirty.height > frame.height) {
            dirty.height = frame.height - dirty.y;
        }
        if (dirty.width == 0 || dirty.height == 0) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        const size_t rowBytes = static_cast<size_t>(dirty.width) * 4U;
        rgbaUploadBuffer_.resize(rowBytes * dirty.height);
        for (uint32_t row = 0; row < dirty.height; ++row) {
            const uint32_t srcY = dirty.y + dirty.height - 1U - row;
            const uint8_t* src = frame.data +
                static_cast<int64_t>(srcY) * sourceStride +
                static_cast<size_t>(dirty.x) * 4U;
            std::memcpy(rgbaUploadBuffer_.data() + rowBytes * row, src, rowBytes);
        }

        const uint32_t dstY = frame.height - dirty.y - dirty.height;
        glBindTexture(GL_TEXTURE_2D, compositeTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(dirty.x),
            static_cast<GLint>(dstY), static_cast<GLsizei>(dirty.width),
            static_cast<GLsizei>(dirty.height), GL_RGBA, GL_UNSIGNED_BYTE,
            rgbaUploadBuffer_.data());
        const GLenum error = glGetError();
        glBindTexture(GL_TEXTURE_2D, 0);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            ++compositeFailures_;
            logs.push_back("AVC420 native-buffer GPU GDI background upload failed: glError=" +
                Hex32(static_cast<uint32_t>(error)) + " " + DescribeDirtyStats(dirty));
            return false;
        }

        compositeReady_ = true;
        ++backgroundComposites_;
        if (logSuccess) {
            logs.push_back("AVC420 native-buffer GPU composited GDI background: " +
                DescribeDirtyStats(dirty) +
                " composites=" + std::to_string(backgroundComposites_) +
                " surface=" + std::to_string(surfaceWidth_) + "x" +
                std::to_string(surfaceHeight_));
        }
        return true;
    }

    bool PresentComposite(std::vector<std::string>& logs, bool logSuccess)
    {
        if (!compositeReady_ || compositeTexture_ == 0 ||
            compositeWidth_ == 0 || compositeHeight_ == 0) {
            logs.push_back("AVC420 native-buffer GPU composite present skipped: retained surface missing");
            return false;
        }
        if (window_ == nullptr || windowSurface_ == EGL_NO_SURFACE ||
            targetWidth_ == 0 || targetHeight_ == 0) {
            logs.push_back("AVC420 native-buffer GPU composite present skipped: window target missing");
            return false;
        }
        if (!MakeWindowCurrent(logs)) {
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, static_cast<GLsizei>(targetWidth_), static_cast<GLsizei>(targetHeight_));
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        bool snappedViewport = false;
        const RenderViewport viewport = FitAvc420PresentViewport(
            targetWidth_, targetHeight_, compositeWidth_, compositeHeight_, snappedViewport);
        if (viewport.width == 0 || viewport.height == 0) {
            logs.push_back("AVC420 native-buffer GPU composite present viewport invalid");
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glUseProgram(presentProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, compositeTexture_);
        glUniform1i(glGetUniformLocation(presentProgram_, "uFrame"), 0);

        const GLfloat vertices[] = {
            -1.0F, -1.0F, 0.0F, 0.0F,
             1.0F, -1.0F, 1.0F, 0.0F,
            -1.0F,  1.0F, 0.0F, 1.0F,
             1.0F,  1.0F, 1.0F, 1.0F,
        };
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
        glEnableVertexAttribArray(1);

        const GLint viewportY = static_cast<GLint>(targetHeight_ - viewport.y - viewport.height);
        glViewport(static_cast<GLint>(viewport.x), viewportY, static_cast<GLsizei>(viewport.width),
            static_cast<GLsizei>(viewport.height));
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC420 native-buffer GPU composite present draw failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        if (!eglSwapBuffers(display_, windowSurface_)) {
            logs.push_back("AVC420 native-buffer GPU composite present swap failed eglError=" +
                Hex32(static_cast<uint32_t>(eglGetError())));
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        ++presents_;
        if (logSuccess) {
            const uint32_t leftBar = viewport.x;
            const uint32_t topBar = viewport.y;
            const uint32_t rightBar = targetWidth_ - viewport.x - viewport.width;
            const uint32_t bottomBar = targetHeight_ - viewport.y - viewport.height;
            logs.push_back("AVC420 native-buffer GPU presented retained composite: target=" +
                std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
                " surface=" + std::to_string(compositeWidth_) + "x" +
                std::to_string(compositeHeight_) +
                " viewport=" + std::to_string(viewport.x) + "," +
                std::to_string(viewport.y) + " " + std::to_string(viewport.width) +
                "x" + std::to_string(viewport.height) +
                " letterboxLTRB=" + std::to_string(leftBar) + "," +
                std::to_string(topBar) + "," + std::to_string(rightBar) + "," +
                std::to_string(bottomBar) +
                " snapFill=" + std::string(snappedViewport ? "yes" : "no") +
                " composites=" + std::to_string(composites_) +
                " failures=" + std::to_string(compositeFailures_) +
                " retainFrames=yes");
        }
        return true;
    }

    void DetachWindowSurface(const std::string& reason, std::vector<std::string>& logs)
    {
        if (display_ == EGL_NO_DISPLAY || windowSurface_ == EGL_NO_SURFACE ||
            window_ == nullptr) {
            return;
        }
        DestroyWindowSurface();
        logs.push_back("AVC420 native-buffer GPU renderer detached window after " + reason +
            "; EGL context and pbuffer preserved");
    }

    std::string DebugState() const
    {
        std::ostringstream out;
        out << "renderer=window:" << (window_ != nullptr ? "yes" : "no")
            << ",windowSurface:" << (windowSurface_ != EGL_NO_SURFACE ? "yes" : "no")
            << ",pbufferSurface:" << (pbufferSurface_ != EGL_NO_SURFACE ? "yes" : "no")
            << ",eglContext:" << (context_ != EGL_NO_CONTEXT ? "yes" : "no")
            << ",target:" << targetWidth_ << "x" << targetHeight_
            << ",surface:" << surfaceWidth_ << "x" << surfaceHeight_
            << ",source:native-buffer-oes"
            << ",imports:" << imports_ << "/" << importFailures_
            << ",composite:" << (compositeReady_ ? "yes" : "no")
            << ",compositeSize:" << compositeWidth_ << "x" << compositeHeight_
            << ",composites:" << composites_ << "/" << backgroundComposites_ <<
                "/" << compositeFailures_
            << ",presents:" << presents_
            << ",retainFrames:yes";
        return out.str();
    }

private:
    struct ImportedTexture {
        OHNativeWindowBuffer* windowBuffer = nullptr;
        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        GLuint texture = 0;
    };

    EGLSurface CreatePbufferSurface(std::vector<std::string>& logs)
    {
        const EGLint pbufferAttribs[] = {
            EGL_WIDTH, 1,
            EGL_HEIGHT, 1,
            EGL_NONE,
        };
        EGLSurface pbuffer = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
        if (pbuffer == EGL_NO_SURFACE) {
            logs.push_back("AVC420 native-buffer GPU renderer create pbuffer failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
        }
        return pbuffer;
    }

    EGLSurface CreateWindowSurface(OHNativeWindow* window, std::vector<std::string>& logs)
    {
        EGLSurface windowSurface = eglCreateWindowSurface(display_, config_,
            reinterpret_cast<EGLNativeWindowType>(window), nullptr);
        if (windowSurface == EGL_NO_SURFACE) {
            logs.push_back("AVC420 native-buffer GPU renderer attach window surface failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
        }
        return windowSurface;
    }

    void DestroyWindowSurface()
    {
        if (display_ == EGL_NO_DISPLAY) {
            return;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (windowSurface_ != EGL_NO_SURFACE) {
            eglDestroySurface(display_, windowSurface_);
        }
        windowSurface_ = EGL_NO_SURFACE;
        window_ = nullptr;
        targetWidth_ = 0;
        targetHeight_ = 0;
    }

    void DeleteCompositeSurface()
    {
        if (compositeFramebuffer_ != 0) {
            glDeleteFramebuffers(1, &compositeFramebuffer_);
            compositeFramebuffer_ = 0;
        }
        if (compositeTexture_ != 0) {
            glDeleteTextures(1, &compositeTexture_);
            compositeTexture_ = 0;
        }
        compositeWidth_ = 0;
        compositeHeight_ = 0;
        compositeReady_ = false;
    }

    bool EnsureCompositeSurface(std::vector<std::string>& logs)
    {
        if (surfaceWidth_ == 0 || surfaceHeight_ == 0) {
            logs.push_back("AVC420 native-buffer GPU composite surface size invalid");
            return false;
        }
        if (compositeTexture_ != 0 && compositeFramebuffer_ != 0 &&
            compositeWidth_ == surfaceWidth_ && compositeHeight_ == surfaceHeight_) {
            return true;
        }

        DeleteCompositeSurface();
        glGenTextures(1, &compositeTexture_);
        glBindTexture(GL_TEXTURE_2D, compositeTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(surfaceWidth_),
            static_cast<GLsizei>(surfaceHeight_), 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        glGenFramebuffers(1, &compositeFramebuffer_);
        glBindFramebuffer(GL_FRAMEBUFFER, compositeFramebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
            compositeTexture_, 0);
        const GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            ++compositeFailures_;
            logs.push_back("AVC420 native-buffer GPU composite framebuffer incomplete: status=" +
                Hex32(static_cast<uint32_t>(status)) +
                " surface=" + std::to_string(surfaceWidth_) + "x" +
                std::to_string(surfaceHeight_));
            DeleteCompositeSurface();
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }

        glViewport(0, 0, static_cast<GLsizei>(surfaceWidth_),
            static_cast<GLsizei>(surfaceHeight_));
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        compositeWidth_ = surfaceWidth_;
        compositeHeight_ = surfaceHeight_;
        compositeReady_ = false;
        logs.push_back("AVC420 native-buffer GPU allocated retained composite surface: " +
            std::to_string(compositeWidth_) + "x" + std::to_string(compositeHeight_));
        return true;
    }

    void DrawExternalRectToComposite(const RECTANGLE_16& rect,
        GLfloat nativeWidth, GLfloat nativeHeight)
    {
        const GLfloat surfaceWidth = static_cast<GLfloat>(surfaceWidth_);
        const GLfloat surfaceHeight = static_cast<GLfloat>(surfaceHeight_);
        const GLfloat left = static_cast<GLfloat>(rect.left);
        const GLfloat top = static_cast<GLfloat>(rect.top);
        const GLfloat right = static_cast<GLfloat>(rect.right);
        const GLfloat bottom = static_cast<GLfloat>(rect.bottom);

        const GLfloat x0 = (left / surfaceWidth) * 2.0F - 1.0F;
        const GLfloat x1 = (right / surfaceWidth) * 2.0F - 1.0F;
        const GLfloat y0 = 1.0F - (top / surfaceHeight) * 2.0F;
        const GLfloat y1 = 1.0F - (bottom / surfaceHeight) * 2.0F;
        const GLfloat u0 = left / nativeWidth;
        const GLfloat u1 = right / nativeWidth;
        const GLfloat v0 = top / nativeHeight;
        const GLfloat v1 = bottom / nativeHeight;

        const GLfloat vertices[] = {
            x0, y1, u0, v1,
            x1, y1, u1, v1,
            x0, y0, u0, v0,
            x1, y0, u1, v0,
        };
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
        glEnableVertexAttribArray(1);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    bool EnsureInitialized(std::vector<std::string>& logs)
    {
        if (display_ != EGL_NO_DISPLAY && pbufferSurface_ != EGL_NO_SURFACE &&
            context_ != EGL_NO_CONTEXT && program_ != 0) {
            return true;
        }

        Destroy();
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) {
            logs.push_back("AVC420 native-buffer GPU renderer eglGetDisplay failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglInitialize(display_, nullptr, nullptr)) {
            logs.push_back("AVC420 native-buffer GPU renderer eglInitialize failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            logs.push_back("AVC420 native-buffer GPU renderer eglBindAPI failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        const EGLint configAttribs[] = {
            EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT,
            EGL_RED_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_BLUE_SIZE, 8,
            EGL_ALPHA_SIZE, 8,
            EGL_NONE,
        };
        EGLint configCount = 0;
        if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) ||
            configCount <= 0) {
            logs.push_back("AVC420 native-buffer GPU renderer eglChooseConfig failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        pbufferSurface_ = CreatePbufferSurface(logs);
        if (pbufferSurface_ == EGL_NO_SURFACE) {
            Destroy();
            return false;
        }

        const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
        if (context_ == EGL_NO_CONTEXT) {
            logs.push_back("AVC420 native-buffer GPU renderer eglCreateContext ES3 failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!MakePbufferCurrent(logs)) {
            Destroy();
            return false;
        }

        if (!CreateProgram(logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            Destroy();
            return false;
        }

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        logs.push_back("AVC420 native-buffer GPU renderer initialized: "
            "path=OH_NativeBuffer->EGLImage->GL_TEXTURE_EXTERNAL_OES "
            "pbuffer=persistent retainFrames=yes");
        return true;
    }

    bool CreateProgram(std::vector<std::string>& logs)
    {
        static constexpr const char* vertex =
            "#version 300 es\n"
            "layout(location = 0) in vec2 aPosition;\n"
            "layout(location = 1) in vec2 aTexCoord;\n"
            "out vec2 vTexCoord;\n"
            "void main() {\n"
            "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
            "  vTexCoord = aTexCoord;\n"
            "}\n";
        static constexpr const char* fragment =
            "#version 300 es\n"
            "#extension GL_OES_EGL_image_external_essl3 : require\n"
            "precision highp float;\n"
            "in vec2 vTexCoord;\n"
            "uniform samplerExternalOES uFrame;\n"
            "uniform vec2 uCropScale;\n"
            "uniform vec2 uCropClamp;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "  vec2 uv = min(vTexCoord * uCropScale, uCropClamp);\n"
            "  fragColor = texture(uFrame, uv);\n"
            "}\n";
        static constexpr const char* presentFragment =
            "#version 300 es\n"
            "precision highp float;\n"
            "in vec2 vTexCoord;\n"
            "uniform sampler2D uFrame;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "  fragColor = texture(uFrame, vTexCoord);\n"
            "}\n";

        program_ = LinkProgram(vertex, fragment, logs);
        presentProgram_ = LinkProgram(vertex, presentFragment, logs);
        return program_ != 0 && presentProgram_ != 0;
    }

    static void ConfigureTexture(GLenum target)
    {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    bool MakeCurrent(EGLSurface surface, const char* label, std::vector<std::string>& logs)
    {
        if (display_ == EGL_NO_DISPLAY || surface == EGL_NO_SURFACE ||
            context_ == EGL_NO_CONTEXT) {
            logs.push_back("AVC420 native-buffer GPU renderer " +
                std::string(label) + " surface is not initialized");
            return false;
        }
        if (!eglMakeCurrent(display_, surface, surface, context_)) {
            logs.push_back("AVC420 native-buffer GPU renderer eglMakeCurrent(" +
                std::string(label) + ") failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            return false;
        }
        return true;
    }

    bool MakePbufferCurrent(std::vector<std::string>& logs)
    {
        return MakeCurrent(pbufferSurface_, "pbuffer", logs);
    }

    bool MakeWindowCurrent(std::vector<std::string>& logs)
    {
        return MakeCurrent(windowSurface_, "window", logs);
    }

    bool ImportFrame(const NativeDecodedFrame& frame, ImportedTexture& imported,
        std::vector<std::string>& logs)
    {
        if (frame.nativeBuffer == nullptr) {
            ++importFailures_;
            logs.push_back("AVC420 native-buffer GPU import rejected missing OH_NativeBuffer");
            return false;
        }

        const auto createImageKhr =
            reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        const auto destroyImageKhr =
            reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
        const auto imageTargetTexture =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                eglGetProcAddress("glEGLImageTargetTexture2DOES"));
        if (createImageKhr == nullptr || destroyImageKhr == nullptr ||
            imageTargetTexture == nullptr) {
            ++importFailures_;
            logs.push_back("AVC420 native-buffer GPU import failed: EGLImage/OES entry point unavailable");
            return false;
        }

        imported.windowBuffer =
            OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer(frame.nativeBuffer);
        if (imported.windowBuffer == nullptr) {
            ++importFailures_;
            logs.push_back("AVC420 native-buffer GPU import failed: "
                "OH_NativeWindow_CreateNativeWindowBufferFromNativeBuffer returned null " +
                NativeFrameText(frame));
            return false;
        }

        const EGLint imageAttribs[] = {EGL_NONE};
        imported.image = createImageKhr(display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS,
            reinterpret_cast<EGLClientBuffer>(imported.windowBuffer), imageAttribs);
        if (imported.image == EGL_NO_IMAGE_KHR) {
            const EGLint error = eglGetError();
            ReleaseImport(imported);
            ++importFailures_;
            logs.push_back("AVC420 native-buffer GPU import failed: "
                "eglCreateImageKHR(EGL_NATIVE_BUFFER_OHOS) eglError=" +
                Hex32(static_cast<uint32_t>(error)) + " " + NativeFrameText(frame));
            return false;
        }

        glGenTextures(1, &imported.texture);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, imported.texture);
        ConfigureTexture(GL_TEXTURE_EXTERNAL_OES);
        imageTargetTexture(GL_TEXTURE_EXTERNAL_OES,
            reinterpret_cast<GLeglImageOES>(imported.image));
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR || imported.texture == 0) {
            ReleaseImport(imported);
            ++importFailures_;
            logs.push_back("AVC420 native-buffer GPU import failed: "
                "glEGLImageTargetTexture2DOES glError=" +
                Hex32(static_cast<uint32_t>(error)) + " " + NativeFrameText(frame));
            return false;
        }

        ++imports_;
        return true;
    }

    void ReleaseImport(ImportedTexture& imported)
    {
        if (imported.texture != 0) {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
            glDeleteTextures(1, &imported.texture);
            imported.texture = 0;
        }
        if (imported.image != EGL_NO_IMAGE_KHR && display_ != EGL_NO_DISPLAY) {
            const auto destroyImageKhr =
                reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(
                    eglGetProcAddress("eglDestroyImageKHR"));
            if (destroyImageKhr != nullptr) {
                destroyImageKhr(display_, imported.image);
            }
            imported.image = EGL_NO_IMAGE_KHR;
        }
        if (imported.windowBuffer != nullptr) {
            OH_NativeWindow_DestroyNativeWindowBuffer(imported.windowBuffer);
            imported.windowBuffer = nullptr;
        }
    }

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLSurface pbufferSurface_ = EGL_NO_SURFACE;
    EGLSurface windowSurface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    OHNativeWindow* window_ = nullptr;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    uint32_t surfaceWidth_ = 0;
    uint32_t surfaceHeight_ = 0;
    GLuint program_ = 0;
    GLuint presentProgram_ = 0;
    GLuint compositeTexture_ = 0;
    GLuint compositeFramebuffer_ = 0;
    uint32_t compositeWidth_ = 0;
    uint32_t compositeHeight_ = 0;
    bool compositeReady_ = false;
    uint64_t composites_ = 0;
    uint64_t backgroundComposites_ = 0;
    uint64_t compositeFailures_ = 0;
    uint64_t imports_ = 0;
    uint64_t importFailures_ = 0;
    uint64_t presents_ = 0;
    std::vector<uint8_t> rgbaUploadBuffer_;
};

} // namespace

struct Avc420GpuCompositorImpl::State {
    Avc420HardwareDecoder avcDecoder;
    Avc420NativeBufferRenderer renderer;
    std::vector<uint8_t> streamParameterSets;
    uint64_t streamPts = 0;
    uint64_t processedCommands = 0;
    uint64_t decoded = 0;
    uint64_t queuedPresents = 0;
    uint64_t presented = 0;
    uint64_t failures = 0;
    uint64_t ignoredUpdates = 0;
    uint64_t importFallbacks = 0;
    uint64_t skippedWarmups = 0;
    uint64_t prewarms = 0;
    uint64_t prewarmFailures = 0;
    uint64_t gdiBackgroundUpdates = 0;
    uint64_t gdiBackgroundPresents = 0;
    bool gdiBackgroundPendingPresent = false;
    uint64_t endFrameCallbacks = 0;
    uint64_t endFrameSkipNoPending = 0;
    uint64_t endFrameMismatches = 0;
    uint64_t endFramePresentAttempts = 0;
    uint64_t pendingPresentOverwrites = 0;
    uint32_t pendingFrameId = 0;
    uint32_t pendingSurfaceWidth = 0;
    uint32_t pendingSurfaceHeight = 0;
    uint32_t currentSurfaceWidth = 0;
    uint32_t currentSurfaceHeight = 0;
    bool pendingPresent = false;
    bool resetDecoderBeforeNextDecode = false;
    bool nativeImportUnsupported = false;
    uint32_t nativeImportUnsupportedWidth = 0;
    uint32_t nativeImportUnsupportedHeight = 0;
    int32_t nativeImportUnsupportedFormat = 0;
    uint64_t lastSampledProcessStartUs = 0;
    uint64_t lastCommandStartUs = 0;
    uint64_t lastEndFrameUs = 0;
    uint64_t lastPresentUs = 0;
    uint64_t maxCommandGapUs = 0;
    uint64_t maxEndFrameGapUs = 0;
    uint64_t maxPresentGapUs = 0;
    uint64_t lastStatsUs = NowMicros();
    uint64_t lastStatsPresented = 0;
    TimingBucket commandTiming;
    TimingBucket commandIntervalTiming;
    TimingBucket offscreenEnsureTiming;
    TimingBucket decodeTiming;
    TimingBucket windowEnsureTiming;
    TimingBucket presentTiming;

    void Destroy()
    {
        renderer.Destroy();
        avcDecoder.Close();
        streamParameterSets.clear();
        streamPts = 0;
        processedCommands = 0;
        decoded = 0;
        queuedPresents = 0;
        presented = 0;
        failures = 0;
        ignoredUpdates = 0;
        importFallbacks = 0;
        skippedWarmups = 0;
        prewarms = 0;
        prewarmFailures = 0;
        gdiBackgroundUpdates = 0;
        gdiBackgroundPresents = 0;
        gdiBackgroundPendingPresent = false;
        endFrameCallbacks = 0;
        endFrameSkipNoPending = 0;
        endFrameMismatches = 0;
        endFramePresentAttempts = 0;
        pendingPresentOverwrites = 0;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        currentSurfaceWidth = 0;
        currentSurfaceHeight = 0;
        pendingPresent = false;
        resetDecoderBeforeNextDecode = false;
        nativeImportUnsupported = false;
        nativeImportUnsupportedWidth = 0;
        nativeImportUnsupportedHeight = 0;
        nativeImportUnsupportedFormat = 0;
        lastSampledProcessStartUs = 0;
        lastCommandStartUs = 0;
        lastEndFrameUs = 0;
        lastPresentUs = 0;
        maxCommandGapUs = 0;
        maxEndFrameGapUs = 0;
        maxPresentGapUs = 0;
        lastStatsUs = NowMicros();
        lastStatsPresented = 0;
        commandTiming.Reset();
        commandIntervalTiming.Reset();
        offscreenEnsureTiming.Reset();
        decodeTiming.Reset();
        windowEnsureTiming.Reset();
        presentTiming.Reset();
    }

    void RecordCommandGap(uint64_t nowUs)
    {
        if (lastCommandStartUs != 0 && nowUs >= lastCommandStartUs) {
            maxCommandGapUs = std::max(maxCommandGapUs, nowUs - lastCommandStartUs);
        }
        lastCommandStartUs = nowUs;
    }

    void RecordEndFrameGap(uint64_t nowUs)
    {
        if (lastEndFrameUs != 0 && nowUs >= lastEndFrameUs) {
            maxEndFrameGapUs = std::max(maxEndFrameGapUs, nowUs - lastEndFrameUs);
        }
        lastEndFrameUs = nowUs;
    }

    void RecordPresentGap(uint64_t nowUs)
    {
        if (lastPresentUs != 0 && nowUs >= lastPresentUs) {
            maxPresentGapUs = std::max(maxPresentGapUs, nowUs - lastPresentUs);
        }
        lastPresentUs = nowUs;
    }

    void ClearPendingPresent()
    {
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
    }

    void OnSurfaceTargetChanged(const std::string& reason,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs)
    {
        if (!outputActive) {
            renderer.DetachWindowSurface(reason, logs);
            logs.push_back("AVC420 native-buffer GPU noted target change while inactive after " +
                reason + "; decoder preserved with SPS/PPS bootstrap parameter sets");
            return;
        }

        DecoderSurfaceTarget target {};
        if (callbacks.decoderSurfaceTarget != nullptr) {
            target = callbacks.decoderSurfaceTarget();
        }
        if (target.window == nullptr || target.width == 0 || target.height == 0) {
            renderer.DetachWindowSurface(reason, logs);
            logs.push_back("AVC420 native-buffer GPU detached missing target after " + reason +
                "; decoder preserved, waiting for next target");
            return;
        }

        const uint32_t surfaceWidth =
            pendingSurfaceWidth != 0 ? pendingSurfaceWidth : currentSurfaceWidth;
        const uint32_t surfaceHeight =
            pendingSurfaceHeight != 0 ? pendingSurfaceHeight : currentSurfaceHeight;
        if (surfaceWidth == 0 || surfaceHeight == 0) {
            renderer.DetachWindowSurface(reason, logs);
            logs.push_back("AVC420 native-buffer GPU target changed after " + reason +
                " with no retained surface size; decoder preserved, waiting for next AVC420 frame");
            return;
        }

        ++endFramePresentAttempts;
        const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
        bool windowReady = false;
        {
            ScopedTiming timing(windowEnsureTiming, sampleTiming);
            windowReady = renderer.Ensure(target.window, target.width, target.height,
                surfaceWidth, surfaceHeight, logs);
        }
        if (!windowReady) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU target change window attach failed after " +
                reason + " failures=" + std::to_string(failures));
            return;
        }

        bool presentOk = false;
        {
            ScopedTiming timing(presentTiming, sampleTiming);
            presentOk = renderer.PresentComposite(logs, true);
        }
        if (!presentOk) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU target change pending present failed after " +
                reason + " failures=" + std::to_string(failures));
            return;
        }

        const bool includedGdiBackground = gdiBackgroundPendingPresent;
        if (includedGdiBackground) {
            ++gdiBackgroundPresents;
            gdiBackgroundPendingPresent = false;
        }
        const uint32_t frameId = pendingFrameId;
        ClearPendingPresent();
        ++presented;
        logs.push_back("AVC420 native-buffer GPU re-presented retained composite after " +
            reason + ": frame=" + std::to_string(frameId) +
            " presented=" + std::to_string(presented) +
            " gdiBgIncluded=" + std::string(includedGdiBackground ? "yes" : "no") +
            " decoder=preserved retainFrames=yes");
    }

    bool Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
    {
        if (surfaceWidth == 0 || surfaceHeight == 0) {
            logs.push_back("AVC420 native-buffer GPU prewarm skipped: invalid surface size");
            return false;
        }

        ++prewarms;
        const bool ready = renderer.Ensure(nullptr, 0, 0, surfaceWidth, surfaceHeight, logs);
        if (!ready) {
            ++prewarmFailures;
            logs.push_back("AVC420 native-buffer GPU prewarm failed: surface=" +
                std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
                " prewarms=" + std::to_string(prewarms) +
                " failures=" + std::to_string(prewarmFailures));
            return false;
        }
        if (ShouldLogFrequent(prewarms)) {
            logs.push_back("AVC420 native-buffer GPU prewarmed: surface=" +
                std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
                " prewarms=" + std::to_string(prewarms) +
                " " + renderer.DebugState());
        }
        return true;
    }

    bool ProcessGdiFrame(const RgbaFrame& frame, bool outputActive,
        std::vector<std::string>& logs)
    {
        if (!outputActive) {
            return false;
        }
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            logs.push_back("AVC420 native-buffer GPU rejected invalid GDI background task");
            return false;
        }

        const bool sampleTiming = ShouldSampleTiming(gdiBackgroundUpdates + 1);
        bool offscreenReady = false;
        {
            ScopedTiming timing(offscreenEnsureTiming, sampleTiming);
            offscreenReady = renderer.Ensure(nullptr, 0, 0, frame.width, frame.height, logs);
        }
        if (!offscreenReady) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU GDI background renderer init failed: " +
                renderer.DebugState());
            return false;
        }

        const bool logSummary = ShouldLogFrequent(gdiBackgroundUpdates + 1);
        if (!renderer.CompositeRgbaFrame(frame, logs, logSummary)) {
            ++failures;
            return false;
        }
        ++gdiBackgroundUpdates;
        gdiBackgroundPendingPresent = true;
        currentSurfaceWidth = frame.width;
        currentSurfaceHeight = frame.height;

        if (logSummary) {
            logs.push_back("AVC420 native-buffer GPU retained GDI background composite: "
                "updates=" + std::to_string(gdiBackgroundUpdates) +
                " presents=" + std::to_string(gdiBackgroundPresents) +
                " pendingPresent=yes"
                " presentDeferred=next-retained-present"
                " retainFrames=yes");
        }
        return true;
    }

    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
        const Avc420GpuCompositorCallbacks&, bool outputActive, std::vector<std::string>& logs)
    {
        const uint64_t commandIndex = ++processedCommands;
        const bool sampleTiming = ShouldSampleTiming(commandIndex);
        const uint64_t processStartUs = NowMicros();
        RecordCommandGap(processStartUs);
        ScopedTiming commandTimer(commandTiming, processStartUs, sampleTiming);
        if (sampleTiming) {
            if (lastSampledProcessStartUs != 0 && processStartUs >= lastSampledProcessStartUs) {
                commandIntervalTiming.Add(processStartUs - lastSampledProcessStartUs);
            }
            lastSampledProcessStartUs = processStartUs;
        }

        if (command == nullptr || command->codecId != RDPGFX_CODECID_AVC420) {
            logs.push_back("AVC420 native-buffer GPU supports AVC420 only");
            return false;
        }
        if (command->stream.data == nullptr || command->stream.length == 0) {
            logs.push_back("AVC420 native-buffer GPU rejected empty H264 stream");
            return false;
        }
        if (!RectsValid(command->stream.regionRects, command->stream.numRegionRects,
                command->width, command->height)) {
            logs.push_back("AVC420 native-buffer GPU rejected invalid dirty rects");
            return false;
        }
        if (nativeImportUnsupported && (nativeImportUnsupportedWidth != command->width ||
                nativeImportUnsupportedHeight != command->height)) {
            logs.push_back("AVC420 native-buffer GPU clears native import fallback after surface "
                "size changed: old=" + std::to_string(nativeImportUnsupportedWidth) + "x" +
                std::to_string(nativeImportUnsupportedHeight) + " new=" +
                std::to_string(command->width) + "x" + std::to_string(command->height));
            nativeImportUnsupported = false;
            nativeImportUnsupportedWidth = 0;
            nativeImportUnsupportedHeight = 0;
            nativeImportUnsupportedFormat = 0;
        }
        if (nativeImportUnsupported && !outputActive) {
            ++skippedWarmups;
            if (ShouldLogFrequent(skippedWarmups)) {
                logs.push_back("AVC420 native-buffer GPU skipped warm-up after native import "
                    "fallback: surface=" + std::to_string(command->width) + "x" +
                    std::to_string(command->height) +
                    " nativeFormat=" +
                    NativeBufferFormatName(
                        static_cast<OH_NativeBuffer_Format>(nativeImportUnsupportedFormat)) +
                    " skippedWarmups=" + std::to_string(skippedWarmups) +
                    "; preserving FreeRDP GDI path without duplicate decoder work");
            }
            return false;
        }

        auto applyActiveUpdatePolicy = [&](const std::string& reason,
            ActiveAvc420UpdatePolicy policy) {
            ++ignoredUpdates;
            ClearPendingPresent();
            if (policy == ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner) {
                resetDecoderBeforeNextDecode = true;
                logs.push_back("AVC420 native-buffer GPU active update policy=" +
                    std::string(ActiveAvc420UpdatePolicyName(policy)) +
                    " scheduled decoder reset for stream resync; SPS/PPS bootstrap parameter "
                    "sets are preserved");
                if (ignoredUpdates >= kActiveResetIgnoreFallbackThreshold) {
                    renderer.Destroy();
                    avcDecoder.Close();
                    resetDecoderBeforeNextDecode = false;
                    logs.push_back("AVC420 native-buffer GPU active update policy=" +
                        std::string(ActiveAvc420UpdatePolicyName(
                            ActiveAvc420UpdatePolicy::ReleaseOwner)) +
                        " after repeated reset-required updates: ignoredUpdates=" +
                        std::to_string(ignoredUpdates) +
                        " threshold=" + std::to_string(kActiveResetIgnoreFallbackThreshold) +
                        "; GDI may recover on the next decodable frame");
                    return false;
                }
            }
            logs.push_back("AVC420 native-buffer GPU active update policy=" +
                std::string(ActiveAvc420UpdatePolicyName(policy)) +
                " reason=" + reason +
                "; GDI remains suppressed so its H264 context is not re-entered mid-stream "
                "ignoredUpdates=" + std::to_string(ignoredUpdates));
            return true;
        };

        auto fail = [&](const std::string& reason) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU failed: " + reason +
                " failures=" + std::to_string(failures));
            if (outputActive) {
                if (failures >= kActiveFailureFallbackThreshold) {
                    ClearPendingPresent();
                    renderer.Destroy();
                    avcDecoder.Close();
                    resetDecoderBeforeNextDecode = false;
                    logs.push_back("AVC420 native-buffer GPU releasing output after repeated "
                        "active failures: failures=" + std::to_string(failures) +
                        " threshold=" + std::to_string(kActiveFailureFallbackThreshold) +
                        "; GDI may recover on the next decodable frame");
                    return false;
                }
                return applyActiveUpdatePolicy(
                    reason, ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner);
            }
            ClearPendingPresent();
            renderer.Destroy();
            return false;
        };

        if (resetDecoderBeforeNextDecode) {
            avcDecoder.Close();
            resetDecoderBeforeNextDecode = false;
            logs.push_back("AVC420 native-buffer GPU reset hardware decoder before decode; "
                "waiting for stored SPS/PPS or a fresh parameter set to bootstrap");
        }

        bool offscreenReady = false;
        {
            ScopedTiming timing(offscreenEnsureTiming, sampleTiming);
            offscreenReady = renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs);
        }
        if (!offscreenReady) {
            if (!outputActive) {
                logs.push_back("AVC420 native-buffer GPU offscreen EGL context unavailable before "
                    "decode; keeping GDI");
                return false;
            }
            return fail("offscreen renderer init");
        }

        PreparedH264Packet packet = PrepareH264Packet(command->stream.data,
            command->stream.length, avcDecoder.Started(), streamParameterSets, logs);
        if (!avcDecoder.Started() && !packet.hadParameterSets &&
            !packet.prependedParameterSets) {
            if (outputActive) {
                return applyActiveUpdatePolicy(
                    "missing initial SPS/PPS before AVC420 stream",
                    ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner);
            }
            logs.push_back("AVC420 native-buffer GPU waits for SPS/PPS before hardware decode; "
                "keeping GDI nalTypes=" + packet.nalSummary);
            return false;
        }

        if (pendingPresent && (pendingSurfaceWidth != command->width ||
                pendingSurfaceHeight != command->height)) {
            ClearPendingPresent();
        }

        if (!avcDecoder.Ensure(command->width, command->height, logs)) {
            return fail("decoder init");
        }

        NativeDecodedFrame frame;
        DecodeResult decode = DecodeResult::Failed;
        {
            ScopedTiming timing(decodeTiming, sampleTiming);
            const int64_t pts = MakeDecoderPts(command->frameId, ++streamPts);
            decode = avcDecoder.Decode(packet.data, packet.size, pts, frame, logs);
        }
        if (decode != DecodeResult::Decoded) {
            if (!outputActive) {
                logs.push_back("AVC420 native-buffer GPU warm-up decode " +
                    std::string(decode == DecodeResult::NoOutput ? "has no output yet" : "failed") +
                    "; keeping GDI");
                return false;
            }
            if (decode == DecodeResult::NoOutput) {
                return applyActiveUpdatePolicy(
                    "decode output not ready", ActiveAvc420UpdatePolicy::PreserveOwner);
            }
            return fail("decode failed");
        }
        ++decoded;

        nativeImportUnsupported = false;
        nativeImportUnsupportedWidth = 0;
        nativeImportUnsupportedHeight = 0;
        nativeImportUnsupportedFormat = 0;

        const bool logCompositeSummary = ShouldLogFrequent(queuedPresents + 1);
        if (!renderer.CompositeFrame(frame, command->stream.regionRects,
                command->stream.numRegionRects, logs, logCompositeSummary)) {
            if (!outputActive) {
                ++importFallbacks;
                nativeImportUnsupported = true;
                nativeImportUnsupportedWidth = command->width;
                nativeImportUnsupportedHeight = command->height;
                nativeImportUnsupportedFormat = frame.nativeFormat;
                frame.Release();
                ClearPendingPresent();
                avcDecoder.Close();
                renderer.Destroy();
                logs.push_back("AVC420 native-buffer GPU fallback before takeover: "
                    "native buffer composite/import is unavailable for " +
                    std::to_string(command->width) + "x" + std::to_string(command->height) +
                    " nativeFormat=" +
                    NativeBufferFormatName(
                        static_cast<OH_NativeBuffer_Format>(nativeImportUnsupportedFormat)) +
                    " importFallbacks=" + std::to_string(importFallbacks) +
                    "; preserving FreeRDP GDI path and disabling duplicate warm-up until "
                    "reset/resize");
                return false;
            }
            frame.Release();
            return fail("retained dirty-rect composite");
        }

        if (pendingPresent) {
            ++pendingPresentOverwrites;
            if (ShouldLogFrequent(pendingPresentOverwrites)) {
                logs.push_back("AVC420 native-buffer GPU overwriting pending EndFrame present: "
                    "oldFrame=" + std::to_string(pendingFrameId) + " newFrame=" +
                    std::to_string(command->frameId) +
                    " overwrites=" + std::to_string(pendingPresentOverwrites));
            }
        }

        pendingPresent = true;
        pendingFrameId = command->frameId;
        pendingSurfaceWidth = command->width;
        pendingSurfaceHeight = command->height;
        currentSurfaceWidth = command->width;
        currentSurfaceHeight = command->height;
        ++queuedPresents;
        if (ShouldLogFrequent(queuedPresents)) {
            logs.push_back("AVC420 native-buffer GPU queued EndFrame present: frame=" +
                std::to_string(command->frameId) +
                " surface=" + std::to_string(command->width) + "x" +
                std::to_string(command->height) +
                " targetHint=" + std::to_string(command->targetWidth) + "x" +
                std::to_string(command->targetHeight) +
                " fullSurface=" + std::string(command->fullSurface ? "yes" : "no") +
                " stream=bytes:" + std::to_string(command->stream.length) +
                "," + RectsText(command->stream.regionRects, command->stream.numRegionRects) +
                " route=hardware-decode+native-buffer-eglimage-gles-avc420-retained-composite "
                "retainFrames=yes " + NativeFrameText(frame));
        }
        if (!command->frameOpen) {
            logs.push_back("AVC420 native-buffer GPU queued inter-frame update; bridge will trigger "
                "GPU present without entering FreeRDP dirty state frame=" +
                std::to_string(command->frameId));
        }
        return true;
    }

    bool PresentQueuedUpdate(const std::string& trigger, uint32_t frameId,
        uint32_t activeFrameId, bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks,
        bool outputActive, std::vector<std::string>& logs)
    {
        if (!pendingPresent) {
            logs.push_back("AVC420 native-buffer GPU " + trigger +
                " present skipped: pending=no policyActive=" +
                std::string(outputActive ? "yes" : "no") + " " + renderer.DebugState());
            return false;
        }

        if (!matchedFrame || frameId != pendingFrameId) {
            ++endFrameMismatches;
            ++ignoredUpdates;
            const uint32_t queuedFrameId = pendingFrameId;
            ClearPendingPresent();
            logs.push_back("AVC420 native-buffer GPU dropped pending present at " + trigger +
                " mismatch: frame=" + std::to_string(frameId) +
                " queuedFrame=" + std::to_string(queuedFrameId) +
                " activeFrame=" + std::to_string(activeFrameId) +
                " matched=" + std::string(matchedFrame ? "yes" : "no") +
                " mismatches=" + std::to_string(endFrameMismatches) +
                " queuedPresents=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented) +
                " endCallbacks=" + std::to_string(endFrameCallbacks));
            return outputActive;
        }

        DecoderSurfaceTarget target {};
        if (callbacks.decoderSurfaceTarget != nullptr) {
            target = callbacks.decoderSurfaceTarget();
        }
        if (target.window == nullptr || target.width == 0 || target.height == 0) {
            ++ignoredUpdates;
            if (outputActive) {
                logs.push_back("AVC420 native-buffer GPU " + trigger +
                    " target unavailable; retained pending present while AVC420 output is "
                    "target-paused policyActive=yes ignoredUpdates=" +
                    std::to_string(ignoredUpdates));
                return true;
            }
            ++failures;
            ClearPendingPresent();
            logs.push_back("AVC420 native-buffer GPU " + trigger +
                " target unavailable; policyActive=" +
                std::string(outputActive ? "yes" : "no") +
                " failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            return false;
        }

        ++endFramePresentAttempts;
        const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
        bool windowReady = false;
        {
            ScopedTiming timing(windowEnsureTiming, sampleTiming);
            windowReady = renderer.Ensure(target.window, target.width, target.height,
                pendingSurfaceWidth, pendingSurfaceHeight, logs);
        }
        if (!windowReady) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU " + trigger +
                " renderer window attach failed failures=" + std::to_string(failures));
            return false;
        }

        const bool logPresentSummary = ShouldLogFrequent(endFramePresentAttempts);
        bool presentOk = false;
        {
            ScopedTiming timing(presentTiming, sampleTiming);
            presentOk = renderer.PresentComposite(logs, logPresentSummary);
        }
        if (!presentOk) {
            ++failures;
            ++ignoredUpdates;
            ClearPendingPresent();
            logs.push_back("AVC420 native-buffer GPU " + trigger +
                " present failed failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            return false;
        }

        const bool includedGdiBackground = gdiBackgroundPendingPresent;
        if (includedGdiBackground) {
            ++gdiBackgroundPresents;
            gdiBackgroundPendingPresent = false;
        }
        RecordPresentGap(NowMicros());
        ClearPendingPresent();
        ++presented;
        if (ShouldLogFrequent(presented)) {
            logs.push_back("AVC420 native-buffer GPU presented at " + trigger + ": frame=" +
                std::to_string(frameId) + " presented=" + std::to_string(presented) +
                " attempts=" + std::to_string(endFramePresentAttempts) +
                " queued=" + std::to_string(queuedPresents) +
                " policyActive=" + std::string(outputActive ? "yes" : "no") +
                " gdiBgIncluded=" + std::string(includedGdiBackground ? "yes" : "no") +
                " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
                std::to_string(gdiBackgroundPresents) +
                " retainFrames=yes");
        }
        return true;
    }

    bool PresentGdiBackgroundAtEndFrame(uint32_t frameId, uint32_t activeFrameId,
        bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs)
    {
        if (!outputActive || !gdiBackgroundPendingPresent) {
            return false;
        }
        if (!matchedFrame) {
            if (ShouldLogFrequent(endFrameCallbacks)) {
                logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
                    "endFrame=" + std::to_string(frameId) +
                    " activeFrame=" + std::to_string(activeFrameId) +
                    " matched=no gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
                    std::to_string(gdiBackgroundPresents));
            }
            return false;
        }
        if (currentSurfaceWidth == 0 || currentSurfaceHeight == 0) {
            logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
                "retained surface size missing gdiBg=" +
                std::to_string(gdiBackgroundUpdates) + "/" +
                std::to_string(gdiBackgroundPresents));
            return false;
        }

        DecoderSurfaceTarget target {};
        if (callbacks.decoderSurfaceTarget != nullptr) {
            target = callbacks.decoderSurfaceTarget();
        }
        if (target.window == nullptr || target.width == 0 || target.height == 0) {
            ++ignoredUpdates;
            if (ShouldLogFrequent(ignoredUpdates)) {
                logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
                    "target unavailable while output active ignoredUpdates=" +
                    std::to_string(ignoredUpdates));
            }
            return false;
        }

        ++endFramePresentAttempts;
        const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
        bool windowReady = false;
        {
            ScopedTiming timing(windowEnsureTiming, sampleTiming);
            windowReady = renderer.Ensure(target.window, target.width, target.height,
                currentSurfaceWidth, currentSurfaceHeight, logs);
        }
        if (!windowReady) {
            ++failures;
            logs.push_back("AVC420 native-buffer GPU EndFrame GDI-only renderer window attach "
                "failed failures=" + std::to_string(failures));
            return false;
        }

        const bool logPresentSummary = ShouldLogFrequent(gdiBackgroundPresents + 1) ||
            ShouldLogFrequent(endFramePresentAttempts);
        bool presentOk = false;
        {
            ScopedTiming timing(presentTiming, sampleTiming);
            presentOk = renderer.PresentComposite(logs, logPresentSummary);
        }
        if (!presentOk) {
            ++failures;
            ++ignoredUpdates;
            logs.push_back("AVC420 native-buffer GPU EndFrame GDI-only present failed "
                "failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            return false;
        }

        ++gdiBackgroundPresents;
        gdiBackgroundPendingPresent = false;
        RecordPresentGap(NowMicros());
        ++presented;
        if (ShouldLogFrequent(gdiBackgroundPresents) || ShouldLogFrequent(presented)) {
            logs.push_back("AVC420 native-buffer GPU presented retained GDI background at "
                "EndFrame: frame=" + std::to_string(frameId) +
                " activeFrame=" + std::to_string(activeFrameId) +
                " presented=" + std::to_string(presented) +
                " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
                std::to_string(gdiBackgroundPresents) +
                " surface=" + std::to_string(currentSurfaceWidth) + "x" +
                std::to_string(currentSurfaceHeight) +
                " policyActive=yes retainFrames=yes");
        }
        return true;
    }

    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs)
    {
        RecordEndFrameGap(NowMicros());
        ++endFrameCallbacks;
        const uint32_t frameId = frame == nullptr ? 0 : frame->frameId;
        const uint32_t activeFrameId = frame == nullptr ? 0 : frame->activeFrameId;
        const bool matchedFrame = frame != nullptr && frame->matchedFrame;
        if (!pendingPresent) {
            if (PresentGdiBackgroundAtEndFrame(frameId, activeFrameId, matchedFrame,
                    callbacks, outputActive, logs)) {
                return true;
            }
            ++endFrameSkipNoPending;
            if (ShouldLogFrequent(endFrameSkipNoPending) ||
                ShouldLogFrequent(endFrameCallbacks)) {
                logs.push_back("AVC420 native-buffer GPU EndFrame callback skipped: endFrame=" +
                    std::to_string(frameId) + " activeFrame=" + std::to_string(activeFrameId) +
                    " matched=" + std::string(matchedFrame ? "yes" : "no") +
                    " pending=no callbacks=" + std::to_string(endFrameCallbacks) +
                    " skipNoPending=" + std::to_string(endFrameSkipNoPending) +
                    " queued=" + std::to_string(queuedPresents) +
                    " presented=" + std::to_string(presented) +
                    " decoded=" + std::to_string(decoded) +
                    " gdiBgPending=" +
                    std::string(gdiBackgroundPendingPresent ? "yes" : "no") +
                    " " + renderer.DebugState());
            }
            return false;
        }
        return PresentQueuedUpdate("EndFrame", frameId, activeFrameId, matchedFrame,
            callbacks, outputActive, logs);
    }

    std::string DebugSummary() const
    {
        std::ostringstream out;
        out << "impl=queued:" << queuedPresents
            << ",decoded:" << decoded
            << ",presented:" << presented
            << ",failures:" << failures
            << ",prewarm:" << prewarms << "/" << prewarmFailures
            << ",ignored:" << ignoredUpdates
            << ",importFallbacks:" << importFallbacks
            << ",skippedWarmups:" << skippedWarmups
            << ",gdiBg:" << gdiBackgroundUpdates << "/" << gdiBackgroundPresents
            << ",gdiBgPending:" << (gdiBackgroundPendingPresent ? "yes" : "no")
            << ",nativeImportUnsupported:" << (nativeImportUnsupported ? "yes" : "no")
            << ",endCallbacks:" << endFrameCallbacks
            << ",skipNoPending:" << endFrameSkipNoPending
            << ",mismatch:" << endFrameMismatches
            << ",attempts:" << endFramePresentAttempts
            << ",commands:" << processedCommands
            << ",pending:" << (pendingPresent ? "yes" : "no")
            << ",pendingFrame:" << pendingFrameId
            << ",pendingSize:" << pendingSurfaceWidth << "x" << pendingSurfaceHeight
            << ",currentSurface:" << currentSurfaceWidth << "x" << currentSurfaceHeight
            << ",retainedComposite:yes"
            << ",lastSampleAgeUs:" << (lastSampledProcessStartUs == 0 ?
                0 : NowMicros() - lastSampledProcessStartUs)
            << ",timingSample=1/" << kTimingSampleInterval
            << ",timingUs(avg/max/count)="
            << commandTiming.Text("cmd") << ";"
            << commandIntervalTiming.Text("cmdGap") << ";"
            << offscreenEnsureTiming.Text("offEns") << ";"
            << decodeTiming.Text("dec") << ";"
            << windowEnsureTiming.Text("winEns") << ";"
            << presentTiming.Text("present")
            << "," << renderer.DebugState();
        return out.str();
    }

    std::string StatsSummary()
    {
        const uint64_t nowUs = NowMicros();
        const uint64_t intervalUs = nowUs >= lastStatsUs ? nowUs - lastStatsUs : 0;
        const uint64_t presentDelta =
            presented >= lastStatsPresented ? presented - lastStatsPresented : 0;
        const double fps = intervalUs == 0 ? 0.0 :
            (static_cast<double>(presentDelta) * 1000000.0) /
                static_cast<double>(intervalUs);
        const uint64_t maxCommandGapSnapshotUs = maxCommandGapUs;
        const uint64_t maxEndFrameGapSnapshotUs = maxEndFrameGapUs;
        const uint64_t maxPresentGapSnapshotUs = maxPresentGapUs;
        const uint64_t presentGap = decoded > presented ? decoded - presented : 0;
        const uint64_t callbackGap =
            endFrameCallbacks > presented ? endFrameCallbacks - presented : 0;
        lastStatsUs = nowUs;
        lastStatsPresented = presented;
        maxCommandGapUs = 0;
        maxEndFrameGapUs = 0;
        maxPresentGapUs = 0;

        std::ostringstream out;
        out << "decoded=" << decoded
            << " queuedPresents=" << queuedPresents
            << " presented=" << presented
            << " presentDelta=" << presentDelta
            << " fps=" << FormatFixed(fps, 1)
            << " maxCommandGapMs=" << FormatMs(maxCommandGapSnapshotUs)
            << " maxEndFrameGapMs=" << FormatMs(maxEndFrameGapSnapshotUs)
            << " maxPresentGapMs=" << FormatMs(maxPresentGapSnapshotUs)
            << " presentGap=" << presentGap
            << " pending=" << (pendingPresent ? "yes" : "no")
            << " pendingFrame=" << pendingFrameId
            << " endCallbacks=" << endFrameCallbacks
            << " callbackGap=" << callbackGap
            << " skipNoPending=" << endFrameSkipNoPending
            << " mismatch=" << endFrameMismatches
            << " overwrites=" << pendingPresentOverwrites
            << " failures=" << failures
            << " ignored=" << ignoredUpdates
            << " importFallbacks=" << importFallbacks
            << " skippedWarmups=" << skippedWarmups
            << " gdiBg=" << gdiBackgroundUpdates << "/" << gdiBackgroundPresents
            << " gdiBgPending=" << (gdiBackgroundPendingPresent ? "yes" : "no")
            << " nativeImportUnsupported=" << (nativeImportUnsupported ? "yes" : "no")
            << " timingUs="
            << commandTiming.Text("cmd") << ";"
            << commandIntervalTiming.Text("cmdGap") << ";"
            << offscreenEnsureTiming.Text("offEns") << ";"
            << decodeTiming.Text("dec") << ";"
            << windowEnsureTiming.Text("winEns") << ";"
            << presentTiming.Text("present")
            << " " << renderer.DebugState();
        return out.str();
    }
};

Avc420GpuCompositorImpl::Avc420GpuCompositorImpl() : state_(std::make_unique<State>()) {}

Avc420GpuCompositorImpl::~Avc420GpuCompositorImpl() = default;

void Avc420GpuCompositorImpl::Destroy()
{
    if (state_) {
        state_->Destroy();
    }
}

void Avc420GpuCompositorImpl::OnSurfaceTargetChanged(const std::string& reason,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (state_) {
        state_->OnSurfaceTargetChanged(reason, callbacks, outputActive, logs);
    }
}

bool Avc420GpuCompositorImpl::Prewarm(
    uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->Prewarm(surfaceWidth, surfaceHeight, logs);
}

bool Avc420GpuCompositorImpl::ProcessGdiFrame(
    const RgbaFrame& frame, bool outputActive, std::vector<std::string>& logs)
{
    return state_ != nullptr &&
        state_->ProcessGdiFrame(frame, outputActive, logs);
}

bool Avc420GpuCompositorImpl::ProcessCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->ProcessCommand(command, callbacks, outputActive, logs);
}

bool Avc420GpuCompositorImpl::PresentEndFrame(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->PresentEndFrame(frame, callbacks, outputActive, logs);
}

std::string Avc420GpuCompositorImpl::DebugSummary() const
{
    return state_ == nullptr ? "impl=null" : state_->DebugSummary();
}

std::string Avc420GpuCompositorImpl::StatsSummary()
{
    return state_ == nullptr ? "decoded=0 queuedPresents=0 presented=0 mismatch=0" :
        state_->StatsSummary();
}

} // namespace rdp_bridge
