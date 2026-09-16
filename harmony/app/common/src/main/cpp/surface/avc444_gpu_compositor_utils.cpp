#include "surface/avc444_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "freerdp/freerdp_runtime.h"

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>

namespace rdp_bridge {

// ---------------------------------------------------------------------------
// DecodedFrame method bodies (promoted from inline to out-of-line).
// ---------------------------------------------------------------------------

DecodedFrame::DecodedFrame(DecodedFrame&& other) noexcept
{
    MoveFrom(other);
}

DecodedFrame& DecodedFrame::operator=(DecodedFrame&& other) noexcept
{
    if (this != &other) {
        Release();
        MoveFrom(other);
    }
    return *this;
}

DecodedFrame::~DecodedFrame()
{
    Release();
}

void DecodedFrame::Release()
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

void DecodedFrame::MoveFrom(DecodedFrame& other)
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

// ---------------------------------------------------------------------------
// avc444-specific free functions.
// ---------------------------------------------------------------------------

bool IsGpuReadbackEnabled()
{
    static const bool enabled = []() {
        const char* value = std::getenv(kGpuReadbackEnv);
        return value != nullptr && value[0] == '1';
    }();
    return enabled;
}

std::string CodecRoleLogPrefix(const std::string& role)
{
    return "AVC444 GPU " + role + " ";
}

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
            logs.push_back(CodecRoleLogPrefix(role) +
                "prepended cached H264 parameter sets: cache=" +
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

// NativeBufferFormatName is defined once in avc420_gpu_compositor_utils.cpp
// (rdp_bridge namespace) and covers all YUV + RGB formats.  The avc444 code
// only passes YUV formats, so the shared definition is a behavioural
// superset — no need for a separate avc444 copy.

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

} // namespace rdp_bridge
