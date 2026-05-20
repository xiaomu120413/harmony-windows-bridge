#include "surface/avc444_gpu_compositor_internal.h"

#include "common/string_utils.h"
#include "freerdp/freerdp_runtime.h"
#include "surface/native_rgba_copy.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <multimedia/player_framework/native_avbuffer.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>
#include <multimedia/player_framework/native_avformat.h>
#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>

namespace rdp_bridge {
namespace {
constexpr const char* kAvcMime = "video/avc";
constexpr int64_t kInputTimeoutUs = 20000;
constexpr int64_t kOutputTimeoutUs = 12000;
constexpr int64_t kFollowupOutputTimeoutUs = 6000;
constexpr int64_t kOutputSyncDeadlineUs = 120000;
constexpr uint32_t kOutputSyncMaxAttempts = 32;

bool ShouldLogFrequent(uint64_t count)
{
    return count <= 20U || (count % 60U) == 0U;
}

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
        } else if (i + 4 < size && data[i] == 0 && data[i + 1] == 0 &&
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
            if (FindAnnexBStartCode(data, size, nalOffset + 1, nextStart, nextPrefix)) {
                // Keep nextStart from the find call.
            }
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
    std::vector<uint8_t>& roleParameterSets, std::vector<uint8_t>& sharedParameterSets,
    const std::string& role, std::vector<std::string>& logs)
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
        roleParameterSets = extracted;
        sharedParameterSets = extracted;
    } else if (!decoderStarted) {
        const std::vector<uint8_t>& cache =
            !roleParameterSets.empty() ? roleParameterSets : sharedParameterSets;
        if (!cache.empty()) {
            packet.storage.reserve(cache.size() + size);
            packet.storage.insert(packet.storage.end(), cache.begin(), cache.end());
            packet.storage.insert(packet.storage.end(), data, data + size);
            packet.data = packet.storage.data();
            packet.size = static_cast<uint32_t>(packet.storage.size());
            packet.prependedParameterSets = true;
            logs.push_back("AVC444 GPU " + role + " prepended cached H264 parameter sets: cache=" +
                std::to_string(cache.size()) + " payload=" + std::to_string(size) +
                " nalTypes=" + packet.nalSummary);
        }
    }
    return packet;
}

uint32_t AlignUp(uint32_t value, uint32_t align)
{
    return align == 0 ? value : value + ((value % align) == 0 ? 0 : align - (value % align));
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
        default:
            return std::to_string(static_cast<int32_t>(format));
    }
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)

bool IsValidLcForCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    return EnsureFreerdpRuntimeLoaded(api, error) &&
        api.ohosRdpgfxAvc444CommandLcIsValid != nullptr &&
        api.ohosRdpgfxAvc444CommandLcIsValid(command) != FALSE;
}

bool RectsValid(const RECTANGLE_16* rects, uint32_t count, uint32_t width, uint32_t height)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    return EnsureFreerdpRuntimeLoaded(api, error) && api.ohosRdpgfxRectsValid != nullptr &&
        api.ohosRdpgfxRectsValid(rects, count, width, height) != FALSE;
}

bool RectsCoverFullSurface(const RECTANGLE_16* rects, uint32_t count, uint32_t width,
    uint32_t height)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    return EnsureFreerdpRuntimeLoaded(api, error) &&
        api.ohosRdpgfxRectsCoverFullSurface != nullptr &&
        api.ohosRdpgfxRectsCoverFullSurface(rects, count, width, height) != FALSE;
}

uint32_t RequiredChromaV1SourceYHeight(const RECTANGLE_16* rects, uint32_t count)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error) ||
        api.ohosRdpgfxAvc444ChromaV1RequiredYHeight == nullptr) {
        return UINT32_MAX;
    }
    return api.ohosRdpgfxAvc444ChromaV1RequiredYHeight(rects, count);
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

std::string StreamText(const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO& stream)
{
    return "bytes:" + std::to_string(stream.length) + "," +
        RectsText(stream.regionRects, stream.numRegionRects);
}

struct PlaneView {
    const uint8_t* data = nullptr;
    uint32_t rowStride = 0;
    uint32_t columnStride = 0;
};

struct DecodedFrame {
    OH_AVCodec* codec = nullptr;
    OH_AVBuffer* buffer = nullptr;
    OH_NativeBuffer* nativeBuffer = nullptr;
    uint32_t outputIndex = 0;
    bool hasOutputIndex = false;
    bool mapped = false;
    void* mappedAddress = nullptr;
    PlaneView y;
    PlaneView uv;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t alignedWidth = 0;
    uint32_t alignedHeight = 0;
    uint32_t yUploadWidth = 0;
    uint32_t yUploadHeight = 0;
    uint32_t uvUploadWidth = 0;
    uint32_t uvUploadHeight = 0;
    uint32_t nativeWidth = 0;
    uint32_t nativeHeight = 0;
    uint32_t nativeStride = 0;
    bool nv21 = false;
    int32_t nativeFormat = 0;
    int64_t pts = 0;

    DecodedFrame() = default;
    DecodedFrame(const DecodedFrame&) = delete;
    DecodedFrame& operator=(const DecodedFrame&) = delete;

    DecodedFrame(DecodedFrame&& other) noexcept
    {
        MoveFrom(other);
    }

    DecodedFrame& operator=(DecodedFrame&& other) noexcept
    {
        if (this != &other) {
            Release();
            MoveFrom(other);
        }
        return *this;
    }

    ~DecodedFrame()
    {
        Release();
    }

    bool Valid() const
    {
        return codec != nullptr && buffer != nullptr && PlanesValid();
    }

    bool PlanesValid() const
    {
        return y.data != nullptr && uv.data != nullptr && width > 0 && height > 0 &&
            y.rowStride > 0 && uv.rowStride > 0 && uv.columnStride == 2;
    }

    void Release()
    {
        if (nativeBuffer != nullptr && mapped) {
            OH_NativeBuffer_Unmap(nativeBuffer);
        }
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
        mapped = false;
        mappedAddress = nullptr;
        y = {};
        uv = {};
        width = 0;
        height = 0;
        alignedWidth = 0;
        alignedHeight = 0;
        yUploadWidth = 0;
        yUploadHeight = 0;
        uvUploadWidth = 0;
        uvUploadHeight = 0;
        nativeWidth = 0;
        nativeHeight = 0;
        nativeStride = 0;
        nv21 = false;
        nativeFormat = 0;
        pts = 0;
    }

private:
    void MoveFrom(DecodedFrame& other)
    {
        codec = other.codec;
        buffer = other.buffer;
        nativeBuffer = other.nativeBuffer;
        outputIndex = other.outputIndex;
        hasOutputIndex = other.hasOutputIndex;
        mapped = other.mapped;
        mappedAddress = other.mappedAddress;
        y = other.y;
        uv = other.uv;
        width = other.width;
        height = other.height;
        alignedWidth = other.alignedWidth;
        alignedHeight = other.alignedHeight;
        yUploadWidth = other.yUploadWidth;
        yUploadHeight = other.yUploadHeight;
        uvUploadWidth = other.uvUploadWidth;
        uvUploadHeight = other.uvUploadHeight;
        nativeWidth = other.nativeWidth;
        nativeHeight = other.nativeHeight;
        nativeStride = other.nativeStride;
        nv21 = other.nv21;
        nativeFormat = other.nativeFormat;
        pts = other.pts;

        other.codec = nullptr;
        other.buffer = nullptr;
        other.nativeBuffer = nullptr;
        other.hasOutputIndex = false;
        other.mapped = false;
        other.mappedAddress = nullptr;
        other.nativeWidth = 0;
        other.nativeHeight = 0;
        other.nativeStride = 0;
    }
};

std::string FramePlaneText(const DecodedFrame& frame)
{
    return "pts=" + std::to_string(frame.pts) +
        " logical=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
        " aligned=" + std::to_string(frame.alignedWidth) + "x" +
            std::to_string(frame.alignedHeight) +
        " native=" + std::to_string(frame.nativeWidth) + "x" +
            std::to_string(frame.nativeHeight) +
        " nativeStride=" + std::to_string(frame.nativeStride) +
        " nativeFormat=" +
            NativeBufferFormatName(static_cast<OH_NativeBuffer_Format>(frame.nativeFormat)) +
        " yStride=" + std::to_string(frame.y.rowStride) +
        " yColumn=" + std::to_string(frame.y.columnStride) +
        " uvStride=" + std::to_string(frame.uv.rowStride) +
        " uvColumn=" + std::to_string(frame.uv.columnStride) +
        " yUpload=" + std::to_string(frame.yUploadWidth) + "x" +
            std::to_string(frame.yUploadHeight) +
        " uvUpload=" + std::to_string(frame.uvUploadWidth) + "x" +
            std::to_string(frame.uvUploadHeight) +
        " order=" + std::string(frame.nv21 ? "NV21" : "NV12") +
        " mapped=" + std::string(frame.mapped ? "yes" : "no") +
        " nativeBuffer=" + std::string(frame.nativeBuffer != nullptr ? "yes" : "no");
}

enum class DecodeResult {
    Decoded,
    NoOutput,
    Failed,
};

class Avc444HardwareDecoder {
public:
    ~Avc444HardwareDecoder()
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
        pixelFormat_ = AV_PIXEL_FORMAT_NV12;
        outputStride_ = 0;
        outputSliceHeight_ = 0;
        outputPixelFormat_ = 0;
        role_.clear();
    }

    bool Ensure(uint32_t width, uint32_t height, const std::string& role,
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
            logs.push_back("AVC444 GPU " + role_ + " decoder create failed name=" +
                SafeCString(name));
            return false;
        }

        bool isValid = false;
        OH_AVErrCode rc = OH_VideoDecoder_IsValid(decoder_, &isValid);
        if (rc != AV_ERR_OK || !isValid) {
            logs.push_back("AVC444 GPU " + role_ + " decoder invalid rc=" +
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
            logs.push_back("AVC444 GPU " + role_ + " decoder prepare failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            Close();
            return false;
        }

        rc = OH_VideoDecoder_Start(decoder_);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " decoder start failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            Close();
            return false;
        }

        started_ = true;
        UpdateOutputDescription(logs, "start");
        logs.push_back("AVC444 GPU " + role_ + " decoder ready: " +
            std::to_string(width_) + "x" + std::to_string(height_) +
            " requestedPixelFormat=" + std::to_string(pixelFormat_) +
            " bounded-sync-mode outputDeadlineUs=" + std::to_string(kOutputSyncDeadlineUs));
        return true;
    }

    bool Started() const
    {
        return decoder_ != nullptr && started_;
    }

    DecodeResult Decode(const uint8_t* data, uint32_t size, int64_t pts, DecodedFrame& frame,
        std::vector<std::string>& logs)
    {
        if (decoder_ == nullptr || !started_ || data == nullptr || size == 0) {
            logs.push_back("AVC444 GPU " + role_ + " decode skipped: invalid input");
            return DecodeResult::Failed;
        }

        uint32_t inputIndex = 0;
        OH_AVErrCode rc = OH_VideoDecoder_QueryInputBuffer(decoder_, &inputIndex, kInputTimeoutUs);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " input unavailable rc=" +
                std::to_string(static_cast<int32_t>(rc)) +
                " size=" + std::to_string(size));
            return DecodeResult::Failed;
        }

        OH_AVBuffer* input = OH_VideoDecoder_GetInputBuffer(decoder_, inputIndex);
        uint8_t* dst = input == nullptr ? nullptr : OH_AVBuffer_GetAddr(input);
        const int32_t capacity = input == nullptr ? -1 : OH_AVBuffer_GetCapacity(input);
        if (dst == nullptr || capacity < 0 || static_cast<uint32_t>(capacity) < size) {
            logs.push_back("AVC444 GPU " + role_ + " input buffer invalid capacity=" +
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
            logs.push_back("AVC444 GPU " + role_ + " set input attr failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            PushEmptyInput(input, inputIndex);
            return DecodeResult::Failed;
        }

        rc = OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " push input failed rc=" +
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
                logs.push_back("AVC444 GPU " + role_ + " query output failed rc=" +
                    std::to_string(static_cast<int32_t>(rc)));
                return DecodeResult::Failed;
            }

            OH_AVBuffer* output = OH_VideoDecoder_GetOutputBuffer(decoder_, outputIndex);
            OH_AVCodecBufferAttr outputAttr {};
            if (output == nullptr ||
                OH_AVBuffer_GetBufferAttr(output, &outputAttr) != AV_ERR_OK) {
                OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
                logs.push_back("AVC444 GPU " + role_ + " output buffer invalid");
                return DecodeResult::Failed;
            }

            if (outputAttr.pts != pts) {
                OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
                logs.push_back("AVC444 GPU " + role_ + " discarded stale output pts=" +
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
            if (outputs_ <= 8 || (outputs_ % 120) == 0) {
                logs.push_back("AVC444 GPU " + role_ + " decoded output: pts=" +
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
        if (noOutput_ <= 8 || (noOutput_ % 120) == 0) {
            logs.push_back("AVC444 GPU " + role_ + " synchronous output wait timed out: pts=" +
                std::to_string(pts) + " waitedUs=" + std::to_string(waitedUs) +
                " budgetUs=" + std::to_string(kOutputSyncDeadlineUs) +
                " attempts=" + std::to_string(kOutputSyncMaxAttempts) +
                " noOutput=" + std::to_string(noOutput_));
        }
        return DecodeResult::NoOutput;
    }

    bool MapDecodedFrame(DecodedFrame& frame, std::vector<std::string>& logs)
    {
        if (frame.Valid()) {
            return true;
        }
        if (decoder_ == nullptr || !started_ || frame.codec != decoder_ || frame.buffer == nullptr) {
            logs.push_back("AVC444 GPU " + role_ +
                " mapped fallback rejected: decoded output is not owned by this decoder");
            return false;
        }
        return MapOutput(frame, logs);
    }

private:
    bool ConfigureWithPixelFormat(int32_t pixelFormat, std::vector<std::string>& logs)
    {
        OH_AVFormat* format = OH_AVFormat_CreateVideoFormat(
            kAvcMime, static_cast<int32_t>(width_), static_cast<int32_t>(height_));
        if (format == nullptr) {
            logs.push_back("AVC444 GPU " + role_ + " format create failed");
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
            logs.push_back("AVC444 GPU " + role_ + " decoder configure failed rc=" +
                std::to_string(static_cast<int32_t>(rc)) +
                " pixelFormat=" + std::to_string(pixelFormat));
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
        logs.push_back("AVC444 GPU " + role_ + " output description after " + reason +
            ": stride=" + std::to_string(outputStride_) +
            " sliceHeight=" + std::to_string(outputSliceHeight_) +
            " pixelFormat=" + std::to_string(outputPixelFormat_));
    }

    bool MapOutput(DecodedFrame& frame, std::vector<std::string>& logs)
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
                        logs.push_back("AVC444 GPU " + role_ +
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
                    logs.push_back("AVC444 GPU " + role_ +
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

            logs.push_back("AVC444 GPU " + role_ + " native buffer map failed rc=" +
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
            logs.push_back("AVC444 GPU " + role_ + " output has no mappable planes capacity=" +
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

    bool FinishPlaneLayout(DecodedFrame& frame, std::vector<std::string>& logs,
        const std::string& source)
    {
        if (frame.y.columnStride != 1 || frame.uv.columnStride != 2 ||
            frame.y.rowStride < frame.width || frame.uv.rowStride < frame.alignedWidth) {
            logs.push_back("AVC444 GPU " + role_ + " unsupported plane layout from " + source +
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

    OH_AVCodec* decoder_ = nullptr;
    bool started_ = false;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    int32_t pixelFormat_ = AV_PIXEL_FORMAT_NV12;
    uint32_t outputStride_ = 0;
    uint32_t outputSliceHeight_ = 0;
    int32_t outputPixelFormat_ = 0;
    uint64_t pushed_ = 0;
    uint64_t outputs_ = 0;
    uint64_t noOutput_ = 0;
    std::string role_;
};

GLuint CompileShader(GLenum type, const char* source, std::vector<std::string>& logs)
{
    const GLuint shader = glCreateShader(type);
    if (shader == 0) {
        logs.push_back("AVC444 GPU GLES create shader failed: " +
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
    logs.push_back("AVC444 GPU GLES shader compile failed type=" + std::to_string(type) +
        " log=" + (info.empty() ? "none" : info));
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
    logs.push_back("AVC444 GPU GLES program link failed log=" +
        (info.empty() ? "none" : info));
    glDeleteProgram(program);
    return 0;
}

class Avc444GpuRenderer {
public:
    ~Avc444GpuRenderer()
    {
        Destroy();
    }

    void Destroy()
    {
        if (display_ != EGL_NO_DISPLAY) {
            eglMakeCurrent(display_, surface_, surface_, context_);
            DeleteTextures();
            if (copyYProgram_ != 0) {
                glDeleteProgram(copyYProgram_);
            }
            if (lumaUvProgram_ != 0) {
                glDeleteProgram(lumaUvProgram_);
            }
            if (chromaV1Program_ != 0) {
                glDeleteProgram(chromaV1Program_);
            }
            if (chromaV2Program_ != 0) {
                glDeleteProgram(chromaV2Program_);
            }
            if (presentProgram_ != 0) {
                glDeleteProgram(presentProgram_);
            }
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (context_ != EGL_NO_CONTEXT) {
                eglDestroyContext(display_, context_);
            }
            if (surface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, surface_);
            }
            eglTerminate(display_);
        }

        display_ = EGL_NO_DISPLAY;
        config_ = nullptr;
        surface_ = EGL_NO_SURFACE;
        context_ = EGL_NO_CONTEXT;
        window_ = nullptr;
        targetWidth_ = 0;
        targetHeight_ = 0;
        surfaceWidth_ = 0;
        surfaceHeight_ = 0;
        copyYProgram_ = 0;
        lumaUvProgram_ = 0;
        chromaV1Program_ = 0;
        chromaV2Program_ = 0;
        presentProgram_ = 0;
        hasLuma_ = false;
        hasChroma_ = false;
    }

    bool Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
        uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
    {
        if (surfaceWidth == 0 || surfaceHeight == 0 ||
            (window != nullptr && (targetWidth == 0 || targetHeight == 0))) {
            logs.push_back("AVC444 GPU renderer target invalid");
            return false;
        }

        const uint32_t effectiveTargetWidth = window == nullptr ? surfaceWidth : targetWidth;
        const uint32_t effectiveTargetHeight = window == nullptr ? surfaceHeight : targetHeight;

        if (display_ != EGL_NO_DISPLAY && surfaceWidth_ == surfaceWidth &&
            surfaceHeight_ == surfaceHeight && window == nullptr) {
            return true;
        }

        if (display_ != EGL_NO_DISPLAY && window_ == window &&
            targetWidth_ == effectiveTargetWidth && targetHeight_ == effectiveTargetHeight &&
            surfaceWidth_ == surfaceWidth && surfaceHeight_ == surfaceHeight) {
            return true;
        }

        if (display_ != EGL_NO_DISPLAY && surfaceWidth_ == surfaceWidth &&
            surfaceHeight_ == surfaceHeight && window != nullptr) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (surface_ != EGL_NO_SURFACE) {
                eglDestroySurface(display_, surface_);
                surface_ = EGL_NO_SURFACE;
            }
            surface_ = eglCreateWindowSurface(display_, config_,
                reinterpret_cast<EGLNativeWindowType>(window), nullptr);
            if (surface_ == EGL_NO_SURFACE) {
                logs.push_back("AVC444 GPU renderer attach window surface failed " +
                    Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }
            window_ = window;
            targetWidth_ = effectiveTargetWidth;
            targetHeight_ = effectiveTargetHeight;
            logs.push_back("AVC444 GPU renderer attached XComponent window: target=" +
                std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
                " surface=" + std::to_string(surfaceWidth_) + "x" +
                std::to_string(surfaceHeight_));
            return true;
        }

        Destroy();
        display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
        if (display_ == EGL_NO_DISPLAY) {
            logs.push_back("AVC444 GPU renderer eglGetDisplay failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglInitialize(display_, nullptr, nullptr)) {
            logs.push_back("AVC444 GPU renderer eglInitialize failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglBindAPI(EGL_OPENGL_ES_API)) {
            logs.push_back("AVC444 GPU renderer eglBindAPI failed " +
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
            logs.push_back("AVC444 GPU renderer eglChooseConfig failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        if (window == nullptr) {
            const EGLint pbufferAttribs[] = {
                EGL_WIDTH, 1,
                EGL_HEIGHT, 1,
                EGL_NONE,
            };
            surface_ = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
        } else {
            surface_ = eglCreateWindowSurface(display_, config_,
                reinterpret_cast<EGLNativeWindowType>(window), nullptr);
        }
        if (surface_ == EGL_NO_SURFACE) {
            logs.push_back("AVC444 GPU renderer eglCreateSurface failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
        context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
        if (context_ == EGL_NO_CONTEXT) {
            logs.push_back("AVC444 GPU renderer eglCreateContext ES3 failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }
        if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
            logs.push_back("AVC444 GPU renderer eglMakeCurrent failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            Destroy();
            return false;
        }

        if (!CreatePrograms(logs) || !CreateTextures(surfaceWidth, surfaceHeight, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            Destroy();
            return false;
        }

        window_ = window;
        targetWidth_ = effectiveTargetWidth;
        targetHeight_ = effectiveTargetHeight;
        surfaceWidth_ = surfaceWidth;
        surfaceHeight_ = surfaceHeight;
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        logs.push_back("AVC444 GPU renderer initialized: target=" +
            std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
            " surface=" + std::to_string(surfaceWidth_) + "x" +
            std::to_string(surfaceHeight_) +
            (window == nullptr ? " offscreen-pbuffer" : " window") +
            " GLES3 mapped-plane shader path present=direct-yuv444-rgb");
        return true;
    }

    bool ReadyToPresent() const
    {
        return hasLuma_ && hasChroma_;
    }

    std::string DebugState() const
    {
        std::ostringstream out;
        out << "renderer=window:" << (window_ != nullptr ? "yes" : "no")
            << ",eglSurface:" << (surface_ != EGL_NO_SURFACE ? "yes" : "no")
            << ",eglContext:" << (context_ != EGL_NO_CONTEXT ? "yes" : "no")
            << ",target:" << targetWidth_ << "x" << targetHeight_
            << ",surface:" << surfaceWidth_ << "x" << surfaceHeight_
            << ",luma:" << (hasLuma_ ? "yes" : "no")
            << ",chroma:" << (hasChroma_ ? "yes" : "no")
            << ",source:mapped-plane";
        return out.str();
    }

    bool HasWindowTarget() const
    {
        return window_ != nullptr && surface_ != EGL_NO_SURFACE;
    }

    void InvalidateComposedState()
    {
        hasLuma_ = false;
        hasChroma_ = false;
    }

    bool ApplyLuma(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs)
    {
        if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
            return false;
        }
        const bool hadChroma = hasChroma_;
        const bool fullSurfaceLuma =
            RectsCoverFullSurface(rects, rectCount, surfaceWidth_, surfaceHeight_);

        glUseProgram(copyYProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        glUniform1i(glGetUniformLocation(copyYProgram_, "uSrcY"), 0);
        glUniform1i(glGetUniformLocation(copyYProgram_, "uSrcHeight"),
            static_cast<GLint>(frame.height));
        glUniform1i(glGetUniformLocation(copyYProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        DrawRectsToTexture(yTexture_, rects, rectCount);

        glUseProgram(lumaUvProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcUV"), 0);
        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uTargetPlane"), 0);
        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        const GLint lumaRectLeft = glGetUniformLocation(lumaUvProgram_, "uRectLeft");
        const GLint lumaRectTop = glGetUniformLocation(lumaUvProgram_, "uRectTop");
        DrawRectsToTexture(uTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

        glUniform1i(glGetUniformLocation(lumaUvProgram_, "uTargetPlane"), 1);
        DrawRectsToTexture(vTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

        const GLenum error = glGetError();
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU luma shader failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        hasLuma_ = true;
        hasChroma_ = true;
        if (!hadChroma) {
            logs.push_back(
                "AVC444 GPU luma update initialized base chroma from AVC444_LUMA stream, "
                "matching FreeRDP LumaToYUV444"
                " fullSurface=" + std::string(fullSurfaceLuma ? "yes" : "no"));
        }
        return true;
    }

    bool ApplyChromaV1(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs)
    {
        const uint32_t requiredYHeight = RequiredChromaV1SourceYHeight(rects, rectCount);
        if (requiredYHeight > frame.yUploadHeight) {
            logs.push_back("AVC444 GPU chroma-v1 source rejected: requiredYHeight=" +
                std::to_string(requiredYHeight) + " uploadedYHeight=" +
                std::to_string(frame.yUploadHeight) +
                " frame=" + FramePlaneText(frame));
            return false;
        }
        if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
            return false;
        }

        glUseProgram(chromaV1Program_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcY"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcUV"), 1);
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        glUniform1i(glGetUniformLocation(chromaV1Program_, "uSrcYHeight"),
            static_cast<GLint>(frame.yUploadHeight));

        PingPongChromaPlane(true, rects, rectCount, chromaV1Program_);
        PingPongChromaPlane(false, rects, rectCount, chromaV1Program_);

        const GLenum error = glGetError();
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU chroma-v1 shader failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        hasChroma_ = true;
        return true;
    }

    bool ApplyChromaV2(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs)
    {
        if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
            return false;
        }

        glUseProgram(chromaV2Program_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcY"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcUV"), 1);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcUComponent"), frame.nv21 ? 1 : 0);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSrcVComponent"), frame.nv21 ? 0 : 1);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSurfaceWidth"),
            static_cast<GLint>(surfaceWidth_));
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uAlignedWidth"),
            static_cast<GLint>(frame.alignedWidth));

        PingPongChromaPlane(true, rects, rectCount, chromaV2Program_);
        PingPongChromaPlane(false, rects, rectCount, chromaV2Program_);

        const GLenum error = glGetError();
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU chroma-v2 shader failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        hasChroma_ = true;
        return true;
    }

    bool Present(std::vector<std::string>& logs, bool logSuccess)
    {
        if (!hasLuma_ || !hasChroma_) {
            logs.push_back("AVC444 GPU present skipped: luma=" +
                std::string(hasLuma_ ? "yes" : "no") +
                " chroma=" + std::string(hasChroma_ ? "yes" : "no"));
            return false;
        }
        if (!MakeCurrent(logs)) {
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, static_cast<GLsizei>(targetWidth_), static_cast<GLsizei>(targetHeight_));
        glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);

        const RenderViewport viewport = FitFrameIntoTarget(
            targetWidth_, targetHeight_, surfaceWidth_, surfaceHeight_);
        if (viewport.width == 0 || viewport.height == 0) {
            logs.push_back("AVC444 GPU present viewport invalid");
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glUseProgram(presentProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, yTexture_);
        glUniform1i(glGetUniformLocation(presentProgram_, "uY"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, uTexture_);
        glUniform1i(glGetUniformLocation(presentProgram_, "uU"), 1);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, vTexture_);
        glUniform1i(glGetUniformLocation(presentProgram_, "uV"), 2);
        glUniform1i(glGetUniformLocation(presentProgram_, "uSurfaceWidth"),
            static_cast<GLint>(surfaceWidth_));
        glUniform1i(glGetUniformLocation(presentProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));

        const GLfloat vertices[] = {
            -1.0F, -1.0F, 0.0F, 1.0F,
             1.0F, -1.0F, 1.0F, 1.0F,
            -1.0F,  1.0F, 0.0F, 0.0F,
             1.0F,  1.0F, 1.0F, 0.0F,
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
            logs.push_back("AVC444 GPU present draw failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        if (!eglSwapBuffers(display_, surface_)) {
            logs.push_back("AVC444 GPU present swap failed eglError=" +
                Hex32(static_cast<uint32_t>(eglGetError())));
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        const uint32_t leftBar = viewport.x;
        const uint32_t topBar = viewport.y;
        const uint32_t rightBar = targetWidth_ - viewport.x - viewport.width;
        const uint32_t bottomBar = targetHeight_ - viewport.y - viewport.height;
        if (logSuccess) {
            logs.push_back("AVC444 GPU compositor presented: target=" +
                std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
                " surface=" +
                std::to_string(surfaceWidth_) + "x" + std::to_string(surfaceHeight_) +
                " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y) +
                " " + std::to_string(viewport.width) + "x" + std::to_string(viewport.height) +
                " letterboxLTRB=" + std::to_string(leftBar) + "," +
                std::to_string(topBar) + "," + std::to_string(rightBar) + "," +
                std::to_string(bottomBar));
        }
        return true;
    }

private:
    bool MakeCurrent(std::vector<std::string>& logs)
    {
        if (display_ == EGL_NO_DISPLAY || surface_ == EGL_NO_SURFACE ||
            context_ == EGL_NO_CONTEXT) {
            logs.push_back("AVC444 GPU renderer is not initialized");
            return false;
        }
        if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
            logs.push_back("AVC444 GPU renderer eglMakeCurrent failed " +
                Hex32(static_cast<uint32_t>(eglGetError())));
            return false;
        }
        return true;
    }

    void DeleteTextures()
    {
        std::array<GLuint*, 8> textures {
            &srcYTexture_, &srcUVTexture_, &yTexture_, &uTexture_,
            &vTexture_, &uScratchTexture_, &vScratchTexture_, &framebuffer_
        };
        for (GLuint* value : textures) {
            if (*value == 0) {
                continue;
            }
            if (value == &framebuffer_) {
                glDeleteFramebuffers(1, value);
            } else {
                glDeleteTextures(1, value);
            }
            *value = 0;
        }
        srcYWidth_ = 0;
        srcYHeight_ = 0;
        srcUVWidth_ = 0;
        srcUVHeight_ = 0;
    }

    bool CreatePrograms(std::vector<std::string>& logs)
    {
        static constexpr const char* updateVertex =
            "#version 300 es\n"
            "layout(location = 0) in vec2 aPosition;\n"
            "void main() { gl_Position = vec4(aPosition, 0.0, 1.0); }\n";
        static constexpr const char* presentVertex =
            "#version 300 es\n"
            "layout(location = 0) in vec2 aPosition;\n"
            "layout(location = 1) in vec2 aTexCoord;\n"
            "out vec2 vTexCoord;\n"
            "void main() {\n"
            "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
            "  vTexCoord = aTexCoord;\n"
            "}\n";
        static constexpr const char* copyYFragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "uniform highp sampler2D uSrcY;\n"
            "uniform int uSrcHeight;\n"
            "uniform int uSurfaceHeight;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "  highp int x = int(floor(gl_FragCoord.x));\n"
            "  highp int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  float value = texelFetch(uSrcY, ivec2(x, y), 0).r;\n"
            "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* lumaUvFragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "uniform highp sampler2D uSrcUV;\n"
            "uniform int uSrcUComponent;\n"
            "uniform int uSrcVComponent;\n"
            "uniform int uTargetPlane;\n"
            "uniform int uSurfaceHeight;\n"
            "uniform int uRectLeft;\n"
            "uniform int uRectTop;\n"
            "out vec4 fragColor;\n"
            "void main() {\n"
            "  highp int x = int(floor(gl_FragCoord.x));\n"
            "  highp int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  int relX = x - uRectLeft;\n"
            "  int relY = y - uRectTop;\n"
            "  int srcX = uRectLeft / 2 + relX / 2;\n"
            "  int srcY = uRectTop / 2 + relY / 2;\n"
            "  vec2 uv = texelFetch(uSrcUV, ivec2(srcX, srcY), 0).rg;\n"
            "  int component = (uTargetPlane == 0) ? uSrcUComponent : uSrcVComponent;\n"
            "  float value = (component == 0) ? uv.r : uv.g;\n"
            "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* chromaV1Fragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "uniform highp sampler2D uPrev;\n"
            "uniform highp sampler2D uSrcY;\n"
            "uniform highp sampler2D uSrcUV;\n"
            "uniform int uSrcUComponent;\n"
            "uniform int uSrcVComponent;\n"
            "uniform int uTargetPlane;\n"
            "uniform int uSurfaceHeight;\n"
            "uniform int uSrcYHeight;\n"
            "uniform int uRectLeft;\n"
            "uniform int uRectTop;\n"
            "uniform int uRectRight;\n"
            "uniform int uRectBottom;\n"
            "out vec4 fragColor;\n"
            "float sampleUV(int x, int y, int component) {\n"
            "  vec2 uv = texelFetch(uSrcUV, ivec2(x, y), 0).rg;\n"
            "  return (component == 0) ? uv.r : uv.g;\n"
            "}\n"
            "void main() {\n"
            "  int x = int(floor(gl_FragCoord.x));\n"
            "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  int relX = x - uRectLeft;\n"
            "  int relY = y - uRectTop;\n"
            "  int rectWidth = uRectRight - uRectLeft;\n"
            "  int rectHeight = uRectBottom - uRectTop;\n"
            "  int stateY = uSurfaceHeight - 1 - y;\n"
            "  float value = texelFetch(uPrev, ivec2(x, stateY), 0).r;\n"
            "  if ((relY & 1) == 1) {\n"
            "    int oddRow = relY / 2;\n"
            "    int group = oddRow / 8;\n"
            "    int inGroup = oddRow - group * 8;\n"
            "    int srcY = uRectTop + group * 16 + inGroup + ((uTargetPlane == 1) ? 8 : 0);\n"
            "    srcY = min(srcY, uSrcYHeight - 1);\n"
            "    value = texelFetch(uSrcY, ivec2(uRectLeft + relX, srcY), 0).r;\n"
            "  } else if ((relX & 1) == 1 && relY < (rectHeight / 2) * 2 && relX < (rectWidth / 2) * 2) {\n"
            "    int srcX = uRectLeft / 2 + relX / 2;\n"
            "    int srcY = uRectTop / 2 + relY / 2;\n"
            "    int component = (uTargetPlane == 0) ? uSrcUComponent : uSrcVComponent;\n"
            "    value = sampleUV(srcX, srcY, component);\n"
            "  }\n"
            "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* chromaV2Fragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "uniform highp sampler2D uPrev;\n"
            "uniform highp sampler2D uSrcY;\n"
            "uniform highp sampler2D uSrcUV;\n"
            "uniform int uSrcUComponent;\n"
            "uniform int uSrcVComponent;\n"
            "uniform int uTargetPlane;\n"
            "uniform int uSurfaceWidth;\n"
            "uniform int uSurfaceHeight;\n"
            "uniform int uAlignedWidth;\n"
            "uniform int uRectLeft;\n"
            "uniform int uRectTop;\n"
            "out vec4 fragColor;\n"
            "float sampleUV(int x, int y, int component) {\n"
            "  vec2 uv = texelFetch(uSrcUV, ivec2(x, y), 0).rg;\n"
            "  return (component == 0) ? uv.r : uv.g;\n"
            "}\n"
            "void main() {\n"
            "  int x = int(floor(gl_FragCoord.x));\n"
            "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  int relX = x - uRectLeft;\n"
            "  int relY = y - uRectTop;\n"
            "  int stateY = uSurfaceHeight - 1 - y;\n"
            "  float value = texelFetch(uPrev, ivec2(x, stateY), 0).r;\n"
            "  if ((relX & 1) == 1) {\n"
            "    int srcX = uRectLeft / 2 + relX / 2 + ((uTargetPlane == 1) ? (uAlignedWidth / 2) : 0);\n"
            "    value = texelFetch(uSrcY, ivec2(srcX, y), 0).r;\n"
            "  } else if ((relY & 1) == 1 && (relX & 3) == 0) {\n"
            "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
            "    int srcY = uRectTop / 2 + relY / 2;\n"
            "    value = sampleUV(srcX, srcY, uSrcUComponent);\n"
            "  } else if ((relY & 1) == 1 && (relX & 3) == 2) {\n"
            "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
            "    int srcY = uRectTop / 2 + relY / 2;\n"
            "    value = sampleUV(srcX, srcY, uSrcVComponent);\n"
            "  }\n"
            "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* presentFragment =
            "#version 300 es\n"
            "precision mediump float;\n"
            "in vec2 vTexCoord;\n"
            "uniform highp sampler2D uY;\n"
            "uniform highp sampler2D uU;\n"
            "uniform highp sampler2D uV;\n"
            "uniform int uSurfaceWidth;\n"
            "uniform int uSurfaceHeight;\n"
            "out vec4 fragColor;\n"
            "float fetchPlane(sampler2D tex, int x, int y) {\n"
            "  x = clamp(x, 0, uSurfaceWidth - 1);\n"
            "  y = clamp(y, 0, uSurfaceHeight - 1);\n"
            "  return texelFetch(tex, ivec2(x, uSurfaceHeight - 1 - y), 0).r;\n"
            "}\n"
            "void main() {\n"
            "  int x = clamp(int(floor(vTexCoord.x * float(uSurfaceWidth))), 0, uSurfaceWidth - 1);\n"
            "  int y = clamp(int(floor(vTexCoord.y * float(uSurfaceHeight))), 0, uSurfaceHeight - 1);\n"
            "  float yy = fetchPlane(uY, x, y);\n"
            "  float uu = fetchPlane(uU, x, y);\n"
            "  float vv = fetchPlane(uV, x, y);\n"
            "  float d = uu - (128.0 / 255.0);\n"
            "  float e = vv - (128.0 / 255.0);\n"
            "  vec3 rgb = vec3(yy + 1.57421875 * e,\n"
            "                 yy - 0.1875 * d - 0.46875 * e,\n"
            "                 yy + 1.85546875 * d);\n"
            "  fragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);\n"
            "}\n";

        copyYProgram_ = LinkProgram(updateVertex, copyYFragment, logs);
        lumaUvProgram_ = LinkProgram(updateVertex, lumaUvFragment, logs);
        chromaV1Program_ = LinkProgram(updateVertex, chromaV1Fragment, logs);
        chromaV2Program_ = LinkProgram(updateVertex, chromaV2Fragment, logs);
        presentProgram_ = LinkProgram(presentVertex, presentFragment, logs);
        if (copyYProgram_ == 0 || lumaUvProgram_ == 0 ||
            chromaV1Program_ == 0 || chromaV2Program_ == 0 || presentProgram_ == 0) {
            return false;
        }
        return true;
    }

    static void ConfigureTexture(GLenum target)
    {
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    bool CreateTextures(uint32_t surfaceWidth, uint32_t surfaceHeight,
        std::vector<std::string>& logs)
    {
        glGenFramebuffers(1, &framebuffer_);
        glGenTextures(1, &srcYTexture_);
        glGenTextures(1, &srcUVTexture_);
        glGenTextures(1, &yTexture_);
        glGenTextures(1, &uTexture_);
        glGenTextures(1, &vTexture_);
        glGenTextures(1, &uScratchTexture_);
        glGenTextures(1, &vScratchTexture_);

        const std::array<GLuint, 5> stateTextures {
            yTexture_, uTexture_, vTexture_, uScratchTexture_, vScratchTexture_
        };
        for (GLuint texture : stateTextures) {
            glBindTexture(GL_TEXTURE_2D, texture);
            ConfigureTexture(GL_TEXTURE_2D);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(surfaceWidth),
                static_cast<GLsizei>(surfaceHeight), 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR || framebuffer_ == 0 || srcYTexture_ == 0 || srcUVTexture_ == 0 ||
            yTexture_ == 0 || uTexture_ == 0 || vTexture_ == 0 || uScratchTexture_ == 0 ||
            vScratchTexture_ == 0) {
            logs.push_back("AVC444 GPU texture allocation failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        return true;
    }

    bool UploadSource(const DecodedFrame& frame, std::vector<std::string>& logs)
    {
        if (!frame.PlanesValid()) {
            logs.push_back("AVC444 GPU source upload rejected: invalid decoded frame");
            return false;
        }
        if (frame.yUploadWidth == 0 || frame.yUploadHeight == 0 ||
            frame.uvUploadWidth == 0 || frame.uvUploadHeight == 0) {
            logs.push_back("AVC444 GPU source upload rejected: invalid upload dimensions");
            return false;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(frame.y.rowStride));
        if (srcYWidth_ != frame.yUploadWidth || srcYHeight_ != frame.yUploadHeight) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(frame.yUploadWidth),
                static_cast<GLsizei>(frame.yUploadHeight), 0, GL_RED, GL_UNSIGNED_BYTE,
                frame.y.data);
            srcYWidth_ = frame.yUploadWidth;
            srcYHeight_ = frame.yUploadHeight;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.yUploadWidth),
                static_cast<GLsizei>(frame.yUploadHeight), GL_RED, GL_UNSIGNED_BYTE,
                frame.y.data);
        }

        glBindTexture(GL_TEXTURE_2D, srcUVTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(frame.uv.rowStride / 2U));
        if (srcUVWidth_ != frame.uvUploadWidth || srcUVHeight_ != frame.uvUploadHeight) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RG8, static_cast<GLsizei>(frame.uvUploadWidth),
                static_cast<GLsizei>(frame.uvUploadHeight), 0, GL_RG, GL_UNSIGNED_BYTE,
                frame.uv.data);
            srcUVWidth_ = frame.uvUploadWidth;
            srcUVHeight_ = frame.uvUploadHeight;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.uvUploadWidth),
                static_cast<GLsizei>(frame.uvUploadHeight), GL_RG, GL_UNSIGNED_BYTE,
                frame.uv.data);
        }
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU source texture upload failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        return true;
    }

    void DrawRectsToTexture(GLuint texture, const RECTANGLE_16* rects, uint32_t rectCount,
        GLint rectLeftLocation = -1, GLint rectTopLocation = -1,
        GLint rectRightLocation = -1, GLint rectBottomLocation = -1)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        const GLfloat vertices[] = {
            -1.0F, -1.0F,
             1.0F, -1.0F,
            -1.0F,  1.0F,
             1.0F,  1.0F,
        };
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), vertices);
        glEnableVertexAttribArray(0);

        for (uint32_t i = 0; i < rectCount; ++i) {
            const RECTANGLE_16& rect = rects[i];
            const uint32_t width = rect.right - rect.left;
            const uint32_t height = rect.bottom - rect.top;
            if (rectLeftLocation >= 0) {
                glUniform1i(rectLeftLocation, static_cast<GLint>(rect.left));
            }
            if (rectTopLocation >= 0) {
                glUniform1i(rectTopLocation, static_cast<GLint>(rect.top));
            }
            if (rectRightLocation >= 0) {
                glUniform1i(rectRightLocation, static_cast<GLint>(rect.right));
            }
            if (rectBottomLocation >= 0) {
                glUniform1i(rectBottomLocation, static_cast<GLint>(rect.bottom));
            }
            glViewport(static_cast<GLint>(rect.left),
                static_cast<GLint>(surfaceHeight_ - rect.bottom),
                static_cast<GLsizei>(width), static_cast<GLsizei>(height));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void CopyPlaneTexture(GLuint source, GLuint target)
    {
        GLint activeTexture = GL_TEXTURE0;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &activeTexture);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, source, 0);
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, target);
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0,
            static_cast<GLsizei>(surfaceWidth_), static_cast<GLsizei>(surfaceHeight_));
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glActiveTexture(static_cast<GLenum>(activeTexture));
    }

    void PingPongChromaPlane(bool uPlane, const RECTANGLE_16* rects, uint32_t rectCount,
        GLuint program)
    {
        GLuint& current = uPlane ? uTexture_ : vTexture_;
        GLuint scratch = uPlane ? uScratchTexture_ : vScratchTexture_;

        CopyPlaneTexture(current, scratch);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, current);
        glUniform1i(glGetUniformLocation(program, "uPrev"), 2);
        glUniform1i(glGetUniformLocation(program, "uTargetPlane"), uPlane ? 0 : 1);
        DrawRectsToTexture(scratch, rects, rectCount,
            glGetUniformLocation(program, "uRectLeft"),
            glGetUniformLocation(program, "uRectTop"),
            glGetUniformLocation(program, "uRectRight"),
            glGetUniformLocation(program, "uRectBottom"));
        std::swap(current, scratch);
        if (uPlane) {
            uScratchTexture_ = scratch;
        } else {
            vScratchTexture_ = scratch;
        }
    }

    EGLDisplay display_ = EGL_NO_DISPLAY;
    EGLConfig config_ = nullptr;
    EGLSurface surface_ = EGL_NO_SURFACE;
    EGLContext context_ = EGL_NO_CONTEXT;
    OHNativeWindow* window_ = nullptr;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    uint32_t surfaceWidth_ = 0;
    uint32_t surfaceHeight_ = 0;

    GLuint framebuffer_ = 0;
    GLuint srcYTexture_ = 0;
    GLuint srcUVTexture_ = 0;
    GLuint yTexture_ = 0;
    GLuint uTexture_ = 0;
    GLuint vTexture_ = 0;
    GLuint uScratchTexture_ = 0;
    GLuint vScratchTexture_ = 0;
    uint32_t srcYWidth_ = 0;
    uint32_t srcYHeight_ = 0;
    uint32_t srcUVWidth_ = 0;
    uint32_t srcUVHeight_ = 0;

    GLuint copyYProgram_ = 0;
    GLuint lumaUvProgram_ = 0;
    GLuint chromaV1Program_ = 0;
    GLuint chromaV2Program_ = 0;
    GLuint presentProgram_ = 0;
    bool hasLuma_ = false;
    bool hasChroma_ = false;
};

#endif

} // namespace


#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool Avc444GpuCompositorImpl::CommandLcIsValid(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    return IsValidLcForCommand(command);
}

std::string Avc444GpuCompositorImpl::RectText(const RECTANGLE_16* rect)
{
    return FormatRectText(rect);
}
#endif
struct Avc444GpuCompositorImpl::State {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    Avc444HardwareDecoder avcDecoder;
    Avc444GpuRenderer renderer;
    std::vector<uint8_t> streamParameterSets;
    uint64_t streamPts = 0;
    uint64_t presented = 0;
    uint64_t queuedPresents = 0;
    uint64_t failures = 0;
    uint64_t ignoredUpdates = 0;
    uint64_t endFrameCallbacks = 0;
    uint64_t endFrameSkipInactive = 0;
    uint64_t endFrameSkipNoPending = 0;
    uint64_t endFrameMismatches = 0;
    uint64_t endFramePresentAttempts = 0;
    uint64_t pendingPresentOverwrites = 0;
    bool pendingPresent = false;
    bool resetDecodersBeforeNextDecode = false;
    uint32_t pendingFrameId = 0;
    uint32_t pendingSurfaceWidth = 0;
    uint32_t pendingSurfaceHeight = 0;

    void Destroy()
    {
        renderer.Destroy();
        avcDecoder.Close();
        streamParameterSets.clear();
        queuedPresents = 0;
        presented = 0;
        failures = 0;
        ignoredUpdates = 0;
        endFrameCallbacks = 0;
        endFrameSkipInactive = 0;
        endFrameSkipNoPending = 0;
        endFrameMismatches = 0;
        endFramePresentAttempts = 0;
        pendingPresentOverwrites = 0;
        pendingPresent = false;
        resetDecodersBeforeNextDecode = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
    }

    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
        const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
        std::vector<std::string>& logs)
    {
        const bool codecV1 = command->codecId == RDPGFX_CODECID_AVC444;
        const bool codecV2 = command->codecId == RDPGFX_CODECID_AVC444v2;
        if (!codecV1 && !codecV2) {
            logs.push_back("AVC444 GPU compositor supports AVC444/AVC444v2 only; codec=" +
                std::to_string(command->codecId) + " stays on GDI");
            return false;
        }
        if (!RectsValid(command->stream1.regionRects, command->stream1.numRegionRects,
                command->width, command->height) ||
            (command->LC == 0 && !RectsValid(command->stream2.regionRects,
                command->stream2.numRegionRects, command->width, command->height))) {
            logs.push_back("AVC444 GPU compositor rejected invalid dirty rects");
            return false;
        }
        const bool wasAuthoritative = authoritative;
        bool rendererStateTouched = false;

        auto ignoreActiveUpdate = [&](const std::string& reason, bool resetRenderer,
                                      bool resetDecoders) {
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            if (resetRenderer) {
                logs.push_back(
                    "AVC444 GPU compositor preserved the last composed state after an ignored "
                    "update, matching FreeRDP's ignore-this-update behavior");
            }
            if (resetDecoders) {
                resetDecodersBeforeNextDecode = true;
                logs.push_back(
                    "AVC444 GPU compositor scheduled hardware decoder reset for stream resync; "
                    "cached H264 parameter sets are preserved");
            }
            logs.push_back(
                "AVC444 GPU compositor ignored update while preserving GPU ownership: " + reason +
                "; GDI remains suppressed so its H264 context is not re-entered mid-stream"
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            return true;
        };

        auto dropActiveSynchronousTimeout = [&](const std::string& reason) {
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            logs.push_back(
                "AVC444 GPU compositor dropped active update after bounded synchronous decode wait: " +
                reason + "; preserving last presented GPU frame and trying the next command"
                " because FreeRDP native GDI H264 state is behind the suppressed GPU stream"
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            return true;
        };

        auto fail = [&](const std::string& reason) {
            ++failures;
            logs.push_back("AVC444 GPU compositor failed: " + reason +
                " failures=" + std::to_string(failures));
            if (wasAuthoritative) {
                return ignoreActiveUpdate(reason, true, false);
            }
            if (rendererStateTouched) {
                renderer.Destroy();
                logs.push_back(
                    "AVC444 GPU compositor reset warm-up renderer state after failed update; "
                    "FreeRDP native GDI path remains authoritative");
            }
            return false;
        };

        const bool needsLuma = command->LC == 0 || command->LC == 1;
        const bool needsChroma = command->LC == 0 || command->LC == 2;
        DecodedFrame lumaFrame;
        DecodedFrame chromaFrame;
        bool lumaUpdated = false;
        bool chromaUpdated = false;

        if (resetDecodersBeforeNextDecode) {
            avcDecoder.Close();
            resetDecodersBeforeNextDecode = false;
            logs.push_back(
                "AVC444 GPU compositor reset the single hardware decoder before decode; "
                "waiting for cached SPS/PPS or fresh parameter sets to bootstrap");
        }

        if ((needsLuma || needsChroma) &&
            !renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs)) {
            if (!authoritative) {
                logs.push_back("AVC444 GPU compositor offscreen renderer unavailable before decode; keeping GDI");
                return false;
            }
            rendererStateTouched = true;
            return fail("offscreen renderer init");
        }
        if (needsLuma) {
            PreparedH264Packet packet = PrepareH264Packet(command->stream1.data,
                command->stream1.length, avcDecoder.Started(), streamParameterSets,
                streamParameterSets, "luma", logs);
            if (!avcDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (authoritative) {
                    return ignoreActiveUpdate("single decoder missing initial SPS/PPS before luma stream",
                        true, true);
                }
                logs.push_back("AVC444 GPU compositor waits for luma SPS/PPS before single "
                    "hardware decode; keeping GDI nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!avcDecoder.Ensure(command->width, command->height, "avc444", logs)) {
                return fail("single avc444 decoder init before luma");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            const DecodeResult decode =
                avcDecoder.Decode(packet.data, packet.size, pts, lumaFrame, logs);
            if (decode != DecodeResult::Decoded) {
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor luma warm-up decode " +
                        std::string(decode == DecodeResult::NoOutput ? "has no output yet" :
                            "failed") + "; keeping GDI");
                    return false;
                }
                if (decode == DecodeResult::NoOutput) {
                    return dropActiveSynchronousTimeout("luma decode output not ready");
                }
                return ignoreActiveUpdate("luma decode failed", true, true);
            }
            lumaUpdated = true;
        }

        if (needsChroma) {
            const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
                command->LC == 0 ? &command->stream2 : &command->stream1;
            PreparedH264Packet packet = PrepareH264Packet(chromaStream->data,
                chromaStream->length, avcDecoder.Started(), streamParameterSets,
                streamParameterSets, "chroma", logs);
            if (!avcDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (authoritative) {
                    return ignoreActiveUpdate("single decoder missing initial SPS/PPS before chroma stream",
                        true, true);
                }
                logs.push_back("AVC444 GPU compositor waits for chroma SPS/PPS before single "
                    "hardware decode; keeping GDI nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!avcDecoder.Ensure(command->width, command->height, "avc444", logs)) {
                return fail("single avc444 decoder init before chroma");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            const DecodeResult decode =
                avcDecoder.Decode(packet.data, packet.size, pts, chromaFrame, logs);
            if (decode != DecodeResult::Decoded) {
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor chroma warm-up decode " +
                        std::string(decode == DecodeResult::NoOutput ? "has no output yet" :
                            "failed") + "; keeping GDI");
                    return false;
                }
                if (decode == DecodeResult::NoOutput) {
                    return dropActiveSynchronousTimeout("chroma decode output not ready");
                }
                return ignoreActiveUpdate("chroma decode failed", true, true);
            }
            chromaUpdated = true;
        }

        const uint32_t dirtyRectCount = command->stream1.numRegionRects +
            (command->LC == 0 ? command->stream2.numRegionRects : 0U);
        if (dirtyRectCount == 0) {
            logs.push_back("AVC444 GPU compositor decoded command with no dirty rects; "
                "matching FreeRDP UpdateSurfaceArea no-op frame=" +
                std::to_string(command->frameId) + " LC=" + std::to_string(command->LC));
            return authoritative;
        }

        if (needsLuma) {
            if (!lumaFrame.PlanesValid() && !avcDecoder.MapDecodedFrame(lumaFrame, logs)) {
                rendererStateTouched = true;
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor luma mapped-plane output failed; keeping GDI");
                    renderer.Destroy();
                    return false;
                }
                return fail("luma mapped-plane output");
            }
            if (!renderer.ApplyLuma(lumaFrame, command->stream1.regionRects,
                    command->stream1.numRegionRects, logs)) {
                rendererStateTouched = true;
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor luma offscreen update failed; keeping GDI");
                    renderer.Destroy();
                    return false;
                }
                return fail("luma shader update");
            }
            rendererStateTouched = true;
        }

        if (needsChroma) {
            const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
                command->LC == 0 ? &command->stream2 : &command->stream1;
            if (!chromaFrame.PlanesValid() && !avcDecoder.MapDecodedFrame(chromaFrame, logs)) {
                rendererStateTouched = true;
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor chroma mapped-plane output failed; keeping GDI");
                    renderer.Destroy();
                    return false;
                }
                return fail("chroma mapped-plane output");
            }
            const bool chromaApplied = codecV1 ?
                renderer.ApplyChromaV1(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs) :
                renderer.ApplyChromaV2(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs);
            if (!chromaApplied) {
                rendererStateTouched = true;
                if (!authoritative) {
                    logs.push_back("AVC444 GPU compositor chroma offscreen update failed; keeping GDI");
                    renderer.Destroy();
                    return false;
                }
                return fail(codecV1 ? "chroma-v1 shader update" : "chroma-v2 shader update");
            }
            rendererStateTouched = true;
        }

        if (!renderer.ReadyToPresent()) {
            logs.push_back("AVC444 GPU compositor warmed " +
                std::string(lumaUpdated ? "luma" : "-") + "/" +
                std::string(chromaUpdated ? "chroma" : "-") +
                " state; waiting for complete luma/base-chroma state before suppressing GDI");
            if (authoritative) {
                ++ignoredUpdates;
                pendingPresent = false;
                pendingFrameId = 0;
                pendingSurfaceWidth = 0;
                pendingSurfaceHeight = 0;
                logs.push_back(
                    "AVC444 GPU compositor ignored not-ready composed state while active; "
                    "GDI remains suppressed and the next command will continue the stream "
                    "ignoredUpdates=" + std::to_string(ignoredUpdates));
                return true;
            }
            return false;
        }

        if (pendingPresent) {
            ++pendingPresentOverwrites;
            if (ShouldLogFrequent(pendingPresentOverwrites)) {
                logs.push_back("AVC444 GPU compositor overwriting pending EndFrame present before "
                    "previous one was presented: oldFrame=" + std::to_string(pendingFrameId) +
                    " newFrame=" + std::to_string(command->frameId) +
                    " overwrites=" + std::to_string(pendingPresentOverwrites) +
                    " queued=" + std::to_string(queuedPresents) +
                    " presented=" + std::to_string(presented));
            }
        }
        const std::string route = std::string("hardware-decode+mapped-plane-gpu-combine+") +
            (codecV1 ? "avc444v1" : "avc444v2");
        if (ShouldLogFrequent(queuedPresents + 1U)) {
            logs.push_back("AVC444 GPU compositor update detail: frame=" +
                std::to_string(command->frameId) +
                " LC=" + std::to_string(command->LC) +
                " lumaUpdated=" + std::string(lumaUpdated ? "yes" : "no") +
                " chromaUpdated=" + std::string(chromaUpdated ? "yes" : "no") +
                " targetHint=" + std::to_string(command->targetWidth) + "x" +
                    std::to_string(command->targetHeight) +
                " remoteSurface=" + std::to_string(command->width) + "x" +
                    std::to_string(command->height) +
                " targetMinusSurface=" +
                    std::to_string(static_cast<int64_t>(command->targetWidth) -
                        static_cast<int64_t>(command->width)) + "x" +
                    std::to_string(static_cast<int64_t>(command->targetHeight) -
                        static_cast<int64_t>(command->height)) +
                " route=" + route +
                " stream1=" + StreamText(command->stream1) +
                " stream2=" + StreamText(command->stream2));
            if (lumaUpdated) {
                logs.push_back("AVC444 GPU compositor luma frame layout: " +
                    FramePlaneText(lumaFrame));
            }
            if (chromaUpdated) {
                logs.push_back("AVC444 GPU compositor chroma frame layout: " +
                    FramePlaneText(chromaFrame));
            }
        }
        pendingPresent = true;
        pendingFrameId = command->frameId;
        pendingSurfaceWidth = command->width;
        pendingSurfaceHeight = command->height;
        ++queuedPresents;
        if (ShouldLogFrequent(queuedPresents)) {
            logs.push_back("AVC444 GPU compositor queued EndFrame present: frame=" +
                std::to_string(command->frameId) + " LC=" + std::to_string(command->LC) +
                " queued=" + std::to_string(queuedPresents) +
                " route=" + route +
                " suppress=this-command");
        }
        if (!command->frameOpen) {
            logs.push_back("AVC444 GPU compositor queued inter-frame AVC444 update; "
                "the bridge will skip FreeRDP dirty state and trigger GPU present: frame=" +
                std::to_string(command->frameId) + " LC=" + std::to_string(command->LC));
        }
        if (!authoritative) {
            const RenderOutputOwner previousOwner =
                ExchangeRenderOutputOwner(RenderOutputOwner::Avc444Gpu);
            if (previousOwner != RenderOutputOwner::Avc444Gpu) {
                if (callbacks.stopRenderPipeline != nullptr) {
                    callbacks.stopRenderPipeline();
                }
                if (callbacks.releaseRenderTarget != nullptr) {
                    callbacks.releaseRenderTarget(
                        "before AVC444 GPU compositor SurfaceCommand takeover");
                }
                logs.push_back("AVC444 GPU compositor claimed render output ownership at "
                    "SurfaceCommand before suppressing FreeRDP GDI: previousOwner=" +
                    RenderOutputOwnerName(previousOwner) + " outputOwner=avc444-gpu");
            }
            authoritative = true;
            logs.push_back("AVC444 GPU compositor is authoritative after queued update; "
                "GDI is suppressed now and present is deferred until the matching frame boundary");
        }
        return true;
    }

    bool PresentQueuedUpdate(const std::string& trigger, uint32_t frameId,
        uint32_t activeFrameId, bool matchedFrame,
        const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
        std::vector<std::string>& logs)
    {
        auto releaseWarmupOwnership = [&](const std::string& reason) {
            if (presented != 0) {
                return;
            }
            const RenderOutputOwner previous =
                ExchangeRenderOutputOwner(RenderOutputOwner::Gdi);
            if (previous == RenderOutputOwner::Avc444Gpu) {
                authoritative = false;
                if (callbacks.startRenderPipeline != nullptr) {
                    callbacks.startRenderPipeline();
                }
                logs.push_back("AVC444 GPU compositor released output ownership before first "
                    "present completed: reason=" + reason + " outputOwner=gdi");
            }
        };

        if (!pendingPresent) {
            logs.push_back("AVC444 GPU compositor " + trigger +
                " present skipped: pending=no authoritative=" +
                std::string(authoritative ? "yes" : "no") + " " + renderer.DebugState());
            return false;
        }

        if (!matchedFrame || frameId != pendingFrameId) {
            ++endFrameMismatches;
            ++ignoredUpdates;
            pendingPresent = false;
            const uint32_t queuedFrameId = pendingFrameId;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            logs.push_back("AVC444 GPU compositor dropped pending present at " + trigger +
                " mismatch: frame=" + std::to_string(frameId) +
                " queuedFrame=" + std::to_string(queuedFrameId) +
                " activeFrame=" + std::to_string(activeFrameId) +
                " matched=" + std::string(matchedFrame ? "yes" : "no") +
                " mismatches=" + std::to_string(endFrameMismatches) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates) +
                " " + renderer.DebugState());
            releaseWarmupOwnership("EndFrame mismatch before first present");
            return authoritative;
        }

        if (pendingSurfaceWidth == 0 || pendingSurfaceHeight == 0) {
            ++failures;
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            logs.push_back("AVC444 GPU compositor dropped pending " + trigger +
                " present with invalid surface dimensions; authoritative=" +
                std::string(authoritative ? "yes" : "no") +
                " failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
            releaseWarmupOwnership("invalid pending surface dimensions");
            return authoritative;
        }

        DecoderSurfaceTarget target {};
        if (callbacks.decoderSurfaceTarget != nullptr) {
            target = callbacks.decoderSurfaceTarget();
        }
        if (target.window == nullptr || target.width == 0 || target.height == 0) {
            ++failures;
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            logs.push_back("AVC444 GPU compositor " + trigger + " target unavailable; "
                "authoritative=" + std::string(authoritative ? "yes" : "no") +
                " failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates) +
                (authoritative ? "; preserving GPU ownership and continuing with the next command" :
                    "; FreeRDP native GDI remains authoritative"));
            releaseWarmupOwnership("target unavailable");
            return authoritative;
        }

        const bool attachingWindowTarget = !renderer.HasWindowTarget();
        bool renderPipelineStopped = false;
        if (attachingWindowTarget) {
            if (callbacks.stopRenderPipeline != nullptr) {
                callbacks.stopRenderPipeline();
                renderPipelineStopped = true;
            }
            if (callbacks.releaseRenderTarget != nullptr) {
                callbacks.releaseRenderTarget(
                    "before AVC444 GPU compositor " + trigger + " takeover");
            }
            logs.push_back("AVC444 GPU compositor taking XComponent target at " + trigger +
                " for AVC444 GPU present: target=" +
                std::to_string(target.width) + "x" + std::to_string(target.height) +
                " surface=" + std::to_string(pendingSurfaceWidth) + "x" +
                std::to_string(pendingSurfaceHeight));
        }

        auto failPresent = [&](const std::string& reason) {
            ++failures;
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            if (!authoritative && attachingWindowTarget && renderPipelineStopped &&
                callbacks.startRenderPipeline != nullptr) {
                callbacks.startRenderPipeline();
            }
            logs.push_back("AVC444 GPU compositor " + trigger + " present failed: " + reason +
                " authoritative=" + std::string(authoritative ? "yes" : "no") +
                " failures=" + std::to_string(failures) +
                " ignoredUpdates=" + std::to_string(ignoredUpdates) +
                (authoritative ? "; preserving GPU ownership and continuing with the next command" :
                    "; FreeRDP native GDI remains authoritative"));
            releaseWarmupOwnership(reason);
            return authoritative;
        };

        if (!renderer.Ensure(target.window, target.width, target.height,
                pendingSurfaceWidth, pendingSurfaceHeight, logs)) {
            return failPresent("renderer window attach");
        }

        ++endFramePresentAttempts;
        const bool logPresentSummary = ShouldLogFrequent(endFramePresentAttempts);
        if (logPresentSummary) {
            logs.push_back("AVC444 GPU compositor " + trigger + " present attempt: "
                "frame=" + std::to_string(frameId) +
                " pendingFrame=" + std::to_string(pendingFrameId) +
                " attempts=" + std::to_string(endFramePresentAttempts) +
                " queued=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented) +
                " authoritative=" + std::string(authoritative ? "yes" : "no") +
                " " + renderer.DebugState());
        }

        if (!renderer.Present(logs, logPresentSummary)) {
            return failPresent("draw/swap");
        }

        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        ++presented;
        if (!authoritative) {
            authoritative = true;
            logs.push_back("AVC444 GPU compositor is authoritative after successful " + trigger +
                " present; future AVC444 SurfaceCommand updates may suppress FreeRDP native GDI");
        }
        if (ShouldLogFrequent(presented)) {
            logs.push_back("AVC444 GPU compositor presented at " + trigger + ": frame=" +
                std::to_string(frameId) + " presented=" + std::to_string(presented) +
                " attempts=" + std::to_string(endFramePresentAttempts) +
                " queued=" + std::to_string(queuedPresents) +
                " authoritative=" + std::string(authoritative ? "yes" : "no"));
        }
        return true;
    }

    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
        std::vector<std::string>& logs)
    {
        ++endFrameCallbacks;
        const uint32_t frameId = frame == nullptr ? 0 : frame->frameId;
        const uint32_t activeFrameId = frame == nullptr ? 0 : frame->activeFrameId;
        const bool matchedFrame = frame != nullptr && frame->matchedFrame;

        if (!pendingPresent) {
            ++endFrameSkipNoPending;
            if (ShouldLogFrequent(endFrameSkipNoPending) ||
                ShouldLogFrequent(endFrameCallbacks)) {
                logs.push_back("AVC444 GPU compositor EndFrame callback skipped: "
                    "endFrame=" + std::to_string(frameId) +
                    " activeFrame=" + std::to_string(activeFrameId) +
                    " matched=" + std::string(matchedFrame ? "yes" : "no") +
                    " authoritative=" + std::string(authoritative ? "yes" : "no") +
                    " pending=no "
                    " pendingFrame=" + std::to_string(pendingFrameId) +
                    " callbacks=" + std::to_string(endFrameCallbacks) +
                    " skipInactive=" + std::to_string(endFrameSkipInactive) +
                    " skipNoPending=" + std::to_string(endFrameSkipNoPending) +
                    " queued=" + std::to_string(queuedPresents) +
                    " presented=" + std::to_string(presented) +
                    " " + renderer.DebugState());
            }
            return false;
        }

        return PresentQueuedUpdate("EndFrame", frameId, activeFrameId, matchedFrame,
            callbacks, authoritative, logs);
    }
#else
    void Destroy() {}
#endif
};


Avc444GpuCompositorImpl::Avc444GpuCompositorImpl() : state_(std::make_unique<State>()) {}

Avc444GpuCompositorImpl::~Avc444GpuCompositorImpl() = default;

void Avc444GpuCompositorImpl::Destroy()
{
    if (state_) {
        state_->Destroy();
    }
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool Avc444GpuCompositorImpl::ProcessCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
    const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->ProcessCommand(command, callbacks, authoritative, logs);
}

bool Avc444GpuCompositorImpl::PresentEndFrame(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->PresentEndFrame(frame, callbacks, authoritative, logs);
}
#endif

} // namespace rdp_bridge
