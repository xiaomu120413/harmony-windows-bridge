#include "surface/avc444_gpu_compositor.h"

#include "string_utils.h"
#include "surface/native_rgba_copy.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <utility>
#include <vector>

#include <dlfcn.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <multimedia/player_framework/native_avbuffer.h>
#include <multimedia/player_framework/native_avcapability.h>
#include <multimedia/player_framework/native_avcodec_base.h>
#include <multimedia/player_framework/native_avcodec_videodecoder.h>
#include <multimedia/player_framework/native_avformat.h>
#include <native_buffer/buffer_common.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace rdp_bridge {
namespace {

constexpr const char* kAvcMime = "video/avc";
constexpr int64_t kInputTimeoutUs = 6000;
constexpr int64_t kOutputTimeoutUs = 9000;
constexpr int64_t kFollowupOutputTimeoutUs = 4000;
constexpr uint32_t kOutputQueryAttempts = 6;

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

bool ContainsText(const char* text, const char* needle)
{
    return text != nullptr && needle != nullptr && std::strstr(text, needle) != nullptr;
}

uint32_t AlignUp(uint32_t value, uint32_t align)
{
    return align == 0 ? value : value + ((value % align) == 0 ? 0 : align - (value % align));
}

template <typename T>
std::string JoinNumbers(const T* values, uint32_t count, uint32_t limit = 12)
{
    if (values == nullptr || count == 0) {
        return "[]";
    }
    std::ostringstream out;
    out << "[";
    const uint32_t shown = std::min(count, limit);
    for (uint32_t i = 0; i < shown; ++i) {
        if (i != 0) {
            out << ",";
        }
        out << static_cast<int32_t>(values[i]);
    }
    if (shown < count) {
        out << ",...";
    }
    out << "]";
    return out.str();
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

bool IsRawYuvCandidate(OH_NativeBuffer_Format format)
{
    return format == NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP ||
        format == NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP ||
        format == NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P ||
        format == NATIVEBUFFER_PIXEL_FMT_YCRCB_420_P ||
        format == NATIVEBUFFER_PIXEL_FMT_Y8;
}

std::string JoinNativeBufferFormats(const OH_NativeBuffer_Format* formats, uint32_t count,
    bool& rawCandidate)
{
    rawCandidate = false;
    if (formats == nullptr || count == 0) {
        return "[]";
    }
    std::ostringstream out;
    out << "[";
    const uint32_t shown = std::min<uint32_t>(count, 12);
    for (uint32_t i = 0; i < shown; ++i) {
        if (i != 0) {
            out << ",";
        }
        rawCandidate = rawCandidate || IsRawYuvCandidate(formats[i]);
        out << NativeBufferFormatName(formats[i]);
    }
    for (uint32_t i = shown; i < count; ++i) {
        rawCandidate = rawCandidate || IsRawYuvCandidate(formats[i]);
    }
    if (shown < count) {
        out << ",...";
    }
    out << "]";
    return out.str();
}

struct DlHandle {
    void* handle = nullptr;

    ~DlHandle()
    {
        if (handle != nullptr) {
            dlclose(handle);
        }
    }

    bool OpenAny(const std::vector<const char*>& names)
    {
        for (const char* name : names) {
            handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
            if (handle != nullptr) {
                return true;
            }
        }
        return false;
    }

    template <typename T>
    T Symbol(const char* name) const
    {
        return handle == nullptr ? nullptr : reinterpret_cast<T>(dlsym(handle, name));
    }
};

std::vector<std::string> ProbeAvcCapability(uint32_t width, uint32_t height,
    Avc444GpuCompositor::SelfTestResult& result)
{
    using GetCapabilityByCategoryFn =
        OH_AVCapability* (*)(const char*, bool, OH_AVCodecCategory);
    using IsHardwareFn = bool (*)(OH_AVCapability*);
    using GetNameFn = const char* (*)(OH_AVCapability*);
    using GetMaxInstancesFn = int32_t (*)(OH_AVCapability*);
    using GetWidthAlignmentFn = OH_AVErrCode (*)(OH_AVCapability*, int32_t*);
    using GetHeightAlignmentFn = OH_AVErrCode (*)(OH_AVCapability*, int32_t*);
    using IsVideoSizeSupportedFn = bool (*)(OH_AVCapability*, int32_t, int32_t);
    using GetPixelFormatsFn = OH_AVErrCode (*)(OH_AVCapability*, const int32_t**, uint32_t*);
    using GetNativeBufferFormatsFn = OH_AVErrCode (*)(
        OH_AVCapability*, const OH_NativeBuffer_Format**, uint32_t*);

    std::vector<std::string> logs;
    DlHandle media;
    if (!media.OpenAny({
            "libnative_media_codecbase.so",
            "libnative_media_core.so",
            "libnative_media_vdec.so",
        })) {
        logs.push_back("AVC444 GPU compositor AVCodec probe: dlopen failed");
        return logs;
    }

    const auto getCapabilityByCategory =
        media.Symbol<GetCapabilityByCategoryFn>("OH_AVCodec_GetCapabilityByCategory");
    const auto isHardware = media.Symbol<IsHardwareFn>("OH_AVCapability_IsHardware");
    const auto getName = media.Symbol<GetNameFn>("OH_AVCapability_GetName");
    const auto getMaxInstances =
        media.Symbol<GetMaxInstancesFn>("OH_AVCapability_GetMaxSupportedInstances");
    const auto getWidthAlignment =
        media.Symbol<GetWidthAlignmentFn>("OH_AVCapability_GetVideoWidthAlignment");
    const auto getHeightAlignment =
        media.Symbol<GetHeightAlignmentFn>("OH_AVCapability_GetVideoHeightAlignment");
    const auto isVideoSizeSupported =
        media.Symbol<IsVideoSizeSupportedFn>("OH_AVCapability_IsVideoSizeSupported");
    const auto getPixelFormats =
        media.Symbol<GetPixelFormatsFn>("OH_AVCapability_GetVideoSupportedPixelFormats");
    const auto getNativeBufferFormats = media.Symbol<GetNativeBufferFormatsFn>(
        "OH_AVCapability_GetVideoSupportedNativeBufferFormats");

    if (getCapabilityByCategory == nullptr) {
        logs.push_back("AVC444 GPU compositor AVCodec probe: capability symbol missing");
        return logs;
    }

    OH_AVCapability* capability = getCapabilityByCategory(kAvcMime, false, HARDWARE);
    if (capability == nullptr) {
        logs.push_back("AVC444 GPU compositor AVCodec probe: no hardware H.264 decoder capability");
        return logs;
    }

    result.avcodecHardwareReady = isHardware == nullptr || isHardware(capability);
    const char* name = getName == nullptr ? nullptr : getName(capability);
    const int32_t maxInstances = getMaxInstances == nullptr ? -1 : getMaxInstances(capability);
    int32_t widthAlignment = 0;
    int32_t heightAlignment = 0;
    if (getWidthAlignment != nullptr) {
        getWidthAlignment(capability, &widthAlignment);
    }
    if (getHeightAlignment != nullptr) {
        getHeightAlignment(capability, &heightAlignment);
    }
    const bool sizeSupported = (isVideoSizeSupported == nullptr || width == 0 || height == 0) ?
        true : isVideoSizeSupported(capability, static_cast<int32_t>(width), static_cast<int32_t>(height));

    const int32_t* pixelFormats = nullptr;
    uint32_t pixelFormatCount = 0;
    std::string pixelFormatText = "unavailable";
    if (getPixelFormats != nullptr &&
        getPixelFormats(capability, &pixelFormats, &pixelFormatCount) == AV_ERR_OK) {
        pixelFormatText = JoinNumbers(pixelFormats, pixelFormatCount);
    }

    const OH_NativeBuffer_Format* nativeFormats = nullptr;
    uint32_t nativeFormatCount = 0;
    std::string nativeFormatText = "symbol-missing";
    bool rawCandidate = false;
    if (getNativeBufferFormats != nullptr) {
        result.nativeBufferFormatsKnown = true;
        if (getNativeBufferFormats(capability, &nativeFormats, &nativeFormatCount) == AV_ERR_OK) {
            nativeFormatText = JoinNativeBufferFormats(nativeFormats, nativeFormatCount, rawCandidate);
        } else {
            nativeFormatText = "query-failed";
        }
    }
    result.rawBufferCandidate = rawCandidate;

    logs.push_back("AVC444 GPU compositor AVCodec probe: hardware=" +
        std::string(result.avcodecHardwareReady ? "yes" : "no") +
        " name=" + SafeCString(name) +
        " maxInstances=" + std::to_string(maxInstances) +
        " align=" + std::to_string(widthAlignment) + "x" + std::to_string(heightAlignment) +
        " size=" + std::to_string(width) + "x" + std::to_string(height) +
        " sizeSupported=" + std::string(sizeSupported ? "yes" : "no") +
        " pixelFormats=" + pixelFormatText +
        " nativeBufferFormats=" + nativeFormatText +
        " rawYuvCandidate=" + std::string(rawCandidate ? "yes" : "no"));

    return logs;
}

std::vector<std::string> ProbeEglNativeImage(Avc444GpuCompositor::SelfTestResult& result)
{
    std::vector<std::string> logs;
    EGLDisplay display = EGL_NO_DISPLAY;
    EGLConfig config = nullptr;
    EGLSurface surface = EGL_NO_SURFACE;
    EGLContext context = EGL_NO_CONTEXT;
    GLuint texture = 0;
    OH_NativeImage* image = nullptr;

    auto cleanup = [&]() {
        if (display != EGL_NO_DISPLAY && context != EGL_NO_CONTEXT && surface != EGL_NO_SURFACE) {
            eglMakeCurrent(display, surface, surface, context);
        }
        if (image != nullptr) {
            OH_NativeImage_Destroy(&image);
        }
        if (texture != 0) {
            glDeleteTextures(1, &texture);
        }
        if (display != EGL_NO_DISPLAY) {
            eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        }
        if (display != EGL_NO_DISPLAY && context != EGL_NO_CONTEXT) {
            eglDestroyContext(display, context);
        }
        if (display != EGL_NO_DISPLAY && surface != EGL_NO_SURFACE) {
            eglDestroySurface(display, surface);
        }
        if (display != EGL_NO_DISPLAY) {
            eglTerminate(display);
        }
    };

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglGetDisplay failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }
    if (!eglInitialize(display, nullptr, nullptr)) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglInitialize failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }
    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglBindAPI failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }

    const EGLint configAttribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_NONE,
    };
    EGLint configCount = 0;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &configCount) || configCount <= 0) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglChooseConfig failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }

    const EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
    surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglCreatePbufferSurface failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }

    const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 3, EGL_NONE};
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglCreateContext failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }
    if (!eglMakeCurrent(display, surface, surface, context)) {
        logs.push_back("AVC444 GPU compositor EGL probe: eglMakeCurrent failed " +
            Hex32(static_cast<uint32_t>(eglGetError())));
        cleanup();
        return logs;
    }

    result.eglReady = true;
    const char* eglExtensions = eglQueryString(display, EGL_EXTENSIONS);
    const char* glExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    const bool externalTextureReady =
        ContainsText(glExtensions, "GL_OES_EGL_image_external") ||
        ContainsText(glExtensions, "GL_OES_EGL_image_external_essl3");

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    const GLenum textureError = glGetError();
    if (texture == 0 || textureError != GL_NO_ERROR) {
        logs.push_back("AVC444 GPU compositor EGL probe: external texture setup failed glError=" +
            Hex32(static_cast<uint32_t>(textureError)));
        cleanup();
        return logs;
    }

    image = OH_NativeImage_Create(texture, GL_TEXTURE_EXTERNAL_OES);
    OHNativeWindow* imageWindow = image == nullptr ? nullptr : OH_NativeImage_AcquireNativeWindow(image);
    uint64_t surfaceId = 0;
    const int32_t surfaceIdStatus =
        image == nullptr ? -1 : OH_NativeImage_GetSurfaceId(image, &surfaceId);
    result.nativeImageReady = image != nullptr && imageWindow != nullptr;

    logs.push_back("AVC444 GPU compositor EGL probe: egl=ready es3=yes externalTextureExt=" +
        std::string(externalTextureReady ? "yes" : "no") +
        " nativeImage=" + std::string(image != nullptr ? "yes" : "no") +
        " imageWindow=" + std::string(imageWindow != nullptr ? "yes" : "no") +
        " imageSurfaceIdStatus=" + std::to_string(surfaceIdStatus) +
        " imageSurfaceId=" + std::to_string(surfaceId) +
        " eglNativeBufferExt=" +
        std::string(ContainsText(eglExtensions, "EGL_OHOS_image_native_buffer") ? "yes" : "unknown"));

    cleanup();
    return logs;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)

bool IsValidLcForCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }
    switch (command->LC) {
        case 0:
            return command->stream1.data != nullptr && command->stream1.length > 0 &&
                command->stream2.data != nullptr && command->stream2.length > 0 &&
                command->stream1.regionRects != nullptr && command->stream1.numRegionRects > 0 &&
                command->stream2.regionRects != nullptr && command->stream2.numRegionRects > 0;
        case 1:
        case 2:
            return command->stream1.data != nullptr && command->stream1.length > 0 &&
                command->stream1.regionRects != nullptr && command->stream1.numRegionRects > 0;
        default:
            return false;
    }
}

bool RectsValid(const RECTANGLE_16* rects, uint32_t count, uint32_t width, uint32_t height)
{
    if (rects == nullptr || count == 0 || width == 0 || height == 0) {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        const RECTANGLE_16& r = rects[i];
        if (r.left >= r.right || r.top >= r.bottom || r.right > width || r.bottom > height) {
            return false;
        }
    }
    return true;
}

std::string RectText(const RECTANGLE_16* rect)
{
    if (rect == nullptr) {
        return "none";
    }
    return std::to_string(rect->left) + "," + std::to_string(rect->top) + "-" +
        std::to_string(rect->right) + "," + std::to_string(rect->bottom);
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
    uint32_t uvUploadWidth = 0;
    uint32_t uvUploadHeight = 0;
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
        return codec != nullptr && buffer != nullptr && y.data != nullptr && uv.data != nullptr &&
            width > 0 && height > 0 && y.rowStride > 0 && uv.rowStride > 0 && uv.columnStride == 2;
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
        uvUploadWidth = 0;
        uvUploadHeight = 0;
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
        uvUploadWidth = other.uvUploadWidth;
        uvUploadHeight = other.uvUploadHeight;
        nv21 = other.nv21;
        nativeFormat = other.nativeFormat;
        pts = other.pts;

        other.codec = nullptr;
        other.buffer = nullptr;
        other.nativeBuffer = nullptr;
        other.hasOutputIndex = false;
        other.mapped = false;
        other.mappedAddress = nullptr;
    }
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
            " sync-mode");
        return true;
    }

    bool Started() const
    {
        return decoder_ != nullptr && started_;
    }

    bool Decode(const uint8_t* data, uint32_t size, int64_t pts, DecodedFrame& frame,
        std::vector<std::string>& logs)
    {
        if (decoder_ == nullptr || !started_ || data == nullptr || size == 0) {
            logs.push_back("AVC444 GPU " + role_ + " decode skipped: invalid input");
            return false;
        }

        uint32_t inputIndex = 0;
        OH_AVErrCode rc = OH_VideoDecoder_QueryInputBuffer(decoder_, &inputIndex, kInputTimeoutUs);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " input unavailable rc=" +
                std::to_string(static_cast<int32_t>(rc)) +
                " size=" + std::to_string(size));
            return false;
        }

        OH_AVBuffer* input = OH_VideoDecoder_GetInputBuffer(decoder_, inputIndex);
        uint8_t* dst = input == nullptr ? nullptr : OH_AVBuffer_GetAddr(input);
        const int32_t capacity = input == nullptr ? -1 : OH_AVBuffer_GetCapacity(input);
        if (dst == nullptr || capacity < 0 || static_cast<uint32_t>(capacity) < size) {
            logs.push_back("AVC444 GPU " + role_ + " input buffer invalid capacity=" +
                std::to_string(capacity) + " size=" + std::to_string(size));
            PushEmptyInput(input, inputIndex);
            return false;
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
            return false;
        }

        rc = OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " push input failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            return false;
        }
        ++pushed_;

        for (uint32_t attempt = 0; attempt < kOutputQueryAttempts; ++attempt) {
            uint32_t outputIndex = 0;
            const int64_t timeout =
                attempt == 0 ? kOutputTimeoutUs : kFollowupOutputTimeoutUs;
            rc = OH_VideoDecoder_QueryOutputBuffer(
                decoder_, &outputIndex, timeout);
            if (rc == AV_ERR_STREAM_CHANGED) {
                UpdateOutputDescription(logs, "stream-changed");
                continue;
            }
            if (rc == AV_ERR_TRY_AGAIN_LATER) {
                if (attempt + 1U < kOutputQueryAttempts) {
                    continue;
                }
                ++noOutput_;
                if (noOutput_ <= 8 || (noOutput_ % 120) == 0) {
                    logs.push_back("AVC444 GPU " + role_ + " output not ready: pts=" +
                        std::to_string(pts) + " noOutput=" + std::to_string(noOutput_));
                }
                return false;
            }
            if (rc != AV_ERR_OK) {
                logs.push_back("AVC444 GPU " + role_ + " query output failed rc=" +
                    std::to_string(static_cast<int32_t>(rc)));
                return false;
            }

            OH_AVBuffer* output = OH_VideoDecoder_GetOutputBuffer(decoder_, outputIndex);
            OH_AVCodecBufferAttr outputAttr {};
            if (output == nullptr ||
                OH_AVBuffer_GetBufferAttr(output, &outputAttr) != AV_ERR_OK) {
                OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
                logs.push_back("AVC444 GPU " + role_ + " output buffer invalid");
                return false;
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
                return false;
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
            return true;
        }

        return false;
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
        frame.nativeBuffer = OH_AVBuffer_GetNativeBuffer(frame.buffer);
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
                frame.nv21 = config.format == NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP;
                frame.y.data = static_cast<const uint8_t*>(address) + planes.planes[0].offset;
                frame.y.rowStride = planes.planes[0].rowStride;
                frame.y.columnStride = planes.planes[0].columnStride == 0 ?
                    1 : planes.planes[0].columnStride;
                frame.uv.data = static_cast<const uint8_t*>(address) + planes.planes[1].offset;
                frame.uv.rowStride = planes.planes[1].rowStride;
                frame.uv.columnStride = planes.planes[1].columnStride;
                if (frame.y.rowStride < frame.width && frame.y.columnStride >= frame.width &&
                    frame.uv.rowStride < frame.alignedWidth &&
                    frame.uv.columnStride >= frame.alignedWidth) {
                    logs.push_back("AVC444 GPU " + role_ +
                        " normalized swapped native plane strides: yRow=" +
                        std::to_string(frame.y.rowStride) + " yColumn=" +
                        std::to_string(frame.y.columnStride) + " uvRow=" +
                        std::to_string(frame.uv.rowStride) + " uvColumn=" +
                        std::to_string(frame.uv.columnStride));
                    std::swap(frame.y.rowStride, frame.y.columnStride);
                    std::swap(frame.uv.rowStride, frame.uv.columnStride);
                }
                return FinishPlaneLayout(frame, logs, "native-buffer");
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
        frame.uvUploadWidth = std::min(frame.uv.rowStride / 2U, std::max(frame.alignedWidth / 2U,
            (frame.width + 1U) / 2U));
        frame.uvUploadHeight = (frame.height + 1U) / 2U;
        return frame.yUploadWidth >= frame.alignedWidth &&
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
            " GLES3 mapped-plane shader path");
        return true;
    }

    bool ReadyToPresent() const
    {
        return hasLuma_ && hasChroma_;
    }

    bool ApplyLuma(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs)
    {
        if (!MakeCurrent(logs) || !UploadSource(frame, logs)) {
            return false;
        }

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

        PingPongChromaPlane(true, rects, rectCount);
        PingPongChromaPlane(false, rects, rectCount);

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

    bool Present(std::vector<std::string>& logs)
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
        logs.push_back("AVC444 GPU compositor presented: surface=" +
            std::to_string(surfaceWidth_) + "x" + std::to_string(surfaceHeight_) +
            " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y) +
            " " + std::to_string(viewport.width) + "x" + std::to_string(viewport.height));
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
            &srcYTexture_, &srcUVTexture_, &yTexture_, &uTexture_, &vTexture_,
            &uScratchTexture_, &vScratchTexture_, &framebuffer_
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
            "  if ((x & 1) == 0 && (y & 1) == 0 && x + 1 < uSurfaceWidth && y + 1 < uSurfaceHeight) {\n"
            "    uu = clamp(4.0 * uu - fetchPlane(uU, x + 1, y) - fetchPlane(uU, x, y + 1) - fetchPlane(uU, x + 1, y + 1), 0.0, 1.0);\n"
            "    vv = clamp(4.0 * vv - fetchPlane(uV, x + 1, y) - fetchPlane(uV, x, y + 1) - fetchPlane(uV, x + 1, y + 1), 0.0, 1.0);\n"
            "  }\n"
            "  float d = uu - (128.0 / 255.0);\n"
            "  float e = vv - (128.0 / 255.0);\n"
            "  vec3 rgb = vec3(yy + 1.57421875 * e,\n"
            "                 yy - 0.1875 * d - 0.46875 * e,\n"
            "                 yy + 1.85546875 * d);\n"
            "  fragColor = vec4(clamp(rgb, 0.0, 1.0), 1.0);\n"
            "}\n";

        copyYProgram_ = LinkProgram(updateVertex, copyYFragment, logs);
        lumaUvProgram_ = LinkProgram(updateVertex, lumaUvFragment, logs);
        chromaV2Program_ = LinkProgram(updateVertex, chromaV2Fragment, logs);
        presentProgram_ = LinkProgram(presentVertex, presentFragment, logs);
        return copyYProgram_ != 0 && lumaUvProgram_ != 0 &&
            chromaV2Program_ != 0 && presentProgram_ != 0;
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
        if (!frame.Valid()) {
            logs.push_back("AVC444 GPU source upload rejected: invalid decoded frame");
            return false;
        }
        if (frame.yUploadWidth == 0 || frame.uvUploadWidth == 0 || frame.uvUploadHeight == 0) {
            logs.push_back("AVC444 GPU source upload rejected: invalid upload dimensions");
            return false;
        }

        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glBindTexture(GL_TEXTURE_2D, srcYTexture_);
        ConfigureTexture(GL_TEXTURE_2D);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, static_cast<GLint>(frame.y.rowStride));
        if (srcYWidth_ != frame.yUploadWidth || srcYHeight_ != frame.height) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, static_cast<GLsizei>(frame.yUploadWidth),
                static_cast<GLsizei>(frame.height), 0, GL_RED, GL_UNSIGNED_BYTE, frame.y.data);
            srcYWidth_ = frame.yUploadWidth;
            srcYHeight_ = frame.height;
        } else {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.yUploadWidth),
                static_cast<GLsizei>(frame.height), GL_RED, GL_UNSIGNED_BYTE, frame.y.data);
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
        GLint rectLeftLocation = -1, GLint rectTopLocation = -1)
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
            glViewport(static_cast<GLint>(rect.left),
                static_cast<GLint>(surfaceHeight_ - rect.bottom),
                static_cast<GLsizei>(width), static_cast<GLsizei>(height));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void PingPongChromaPlane(bool uPlane, const RECTANGLE_16* rects, uint32_t rectCount)
    {
        GLuint& current = uPlane ? uTexture_ : vTexture_;
        GLuint scratch = uPlane ? uScratchTexture_ : vScratchTexture_;

        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, current);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uPrev"), 2);
        glUniform1i(glGetUniformLocation(chromaV2Program_, "uTargetPlane"), uPlane ? 0 : 1);
        DrawRectsToTexture(scratch, rects, rectCount,
            glGetUniformLocation(chromaV2Program_, "uRectLeft"),
            glGetUniformLocation(chromaV2Program_, "uRectTop"));
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
    GLuint chromaV2Program_ = 0;
    GLuint presentProgram_ = 0;
    bool hasLuma_ = false;
    bool hasChroma_ = false;
};

#endif

} // namespace

struct Avc444GpuCompositor::Impl {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    Avc444HardwareDecoder lumaDecoder;
    Avc444HardwareDecoder chromaDecoder;
    Avc444GpuRenderer renderer;
    std::vector<uint8_t> sharedParameterSets;
    std::vector<uint8_t> lumaParameterSets;
    std::vector<uint8_t> chromaParameterSets;
    uint64_t streamPts = 0;
    uint64_t presented = 0;
    uint64_t failures = 0;

    void Destroy()
    {
        renderer.Destroy();
        lumaDecoder.Close();
        chromaDecoder.Close();
        sharedParameterSets.clear();
        lumaParameterSets.clear();
        chromaParameterSets.clear();
    }

    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
        const Avc444GpuCompositorCallbacks& callbacks, bool& active, std::vector<std::string>& logs)
    {
        if (!command->frameOpen) {
            logs.push_back("AVC444 GPU compositor rejected command outside an open RDPGFX frame: frame=" +
                std::to_string(command->frameId) + " LC=" + std::to_string(command->LC));
            return false;
        }
        if (command->codecId != RDPGFX_CODECID_AVC444v2) {
            logs.push_back("AVC444 GPU compositor supports AVC444v2 only for now; codec=" +
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
        const bool wasActive = active;
        bool activatedForThisCommand = false;

        auto fail = [&](const std::string& reason) {
            ++failures;
            logs.push_back("AVC444 GPU compositor failed: " + reason +
                " failures=" + std::to_string(failures));
            if (activatedForThisCommand || wasActive) {
                Destroy();
                active = false;
                if (callbacks.startRenderPipeline != nullptr) {
                    callbacks.startRenderPipeline();
                }
                logs.push_back("AVC444 GPU compositor returned target to GDI after failure");
            }
            return false;
        };

        const bool needsLuma = command->LC == 0 || command->LC == 1;
        const bool needsChroma = command->LC == 0 || command->LC == 2;
        DecodedFrame lumaFrame;
        DecodedFrame chromaFrame;
        bool lumaUpdated = false;
        bool chromaUpdated = false;

        if (needsLuma) {
            PreparedH264Packet packet = PrepareH264Packet(command->stream1.data,
                command->stream1.length, lumaDecoder.Started(), lumaParameterSets,
                sharedParameterSets, "luma", logs);
            if (!lumaDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (active) {
                    return fail("luma decoder missing initial SPS/PPS");
                }
                logs.push_back("AVC444 GPU compositor waits for luma SPS/PPS before hardware decode; keeping GDI"
                    " nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!lumaDecoder.Ensure(command->width, command->height, "luma", logs)) {
                return fail("luma decoder init");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            if (!lumaDecoder.Decode(packet.data, packet.size, pts, lumaFrame, logs)) {
                if (!active) {
                    logs.push_back("AVC444 GPU compositor luma warm-up decode not ready; keeping GDI");
                    return false;
                }
                return fail("luma decode");
            }
            lumaUpdated = true;
        }

        if (needsChroma) {
            const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
                command->LC == 0 ? &command->stream2 : &command->stream1;
            PreparedH264Packet packet = PrepareH264Packet(chromaStream->data,
                chromaStream->length, chromaDecoder.Started(), chromaParameterSets,
                sharedParameterSets, "chroma", logs);
            if (!chromaDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (active) {
                    return fail("chroma decoder missing initial SPS/PPS");
                }
                logs.push_back("AVC444 GPU compositor waits for chroma SPS/PPS before hardware decode; keeping GDI"
                    " nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!chromaDecoder.Ensure(command->width, command->height, "chroma", logs)) {
                return fail("chroma decoder init");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            if (!chromaDecoder.Decode(packet.data, packet.size, pts, chromaFrame, logs)) {
                if (!active) {
                    logs.push_back("AVC444 GPU compositor chroma warm-up decode not ready; keeping GDI");
                    return false;
                }
                return fail("chroma decode");
            }
            chromaUpdated = true;
        }

        if ((needsLuma || needsChroma) &&
            !renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs)) {
            if (!active) {
                logs.push_back("AVC444 GPU compositor offscreen renderer unavailable after decode; keeping GDI");
                return false;
            }
            return fail("offscreen renderer init");
        }

        if (needsLuma && !renderer.ApplyLuma(lumaFrame, command->stream1.regionRects,
                command->stream1.numRegionRects, logs)) {
            if (!active) {
                logs.push_back("AVC444 GPU compositor luma offscreen update failed; keeping GDI");
                return false;
            }
            return fail("luma shader update");
        }

        if (needsChroma) {
            const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
                command->LC == 0 ? &command->stream2 : &command->stream1;
            if (!renderer.ApplyChromaV2(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs)) {
                if (!active) {
                    logs.push_back("AVC444 GPU compositor chroma offscreen update failed; keeping GDI");
                    return false;
                }
                return fail("chroma-v2 shader update");
            }
        }

        if (!renderer.ReadyToPresent()) {
            logs.push_back("AVC444 GPU compositor warmed " +
                std::string(lumaUpdated ? "luma" : "-") + "/" +
                std::string(chromaUpdated ? "chroma" : "-") +
                " state; waiting for both luma and chroma before suppressing GDI");
            return false;
        }

        DecoderSurfaceTarget target {};
        if (callbacks.decoderSurfaceTarget != nullptr) {
            target = callbacks.decoderSurfaceTarget();
        }
        if (target.window == nullptr || target.width == 0 || target.height == 0) {
            if (active) {
                return fail("target unavailable");
            }
            logs.push_back("AVC444 GPU compositor target unavailable");
            return false;
        }

        if (!active) {
            if (callbacks.stopRenderPipeline != nullptr) {
                callbacks.stopRenderPipeline();
            }
            if (callbacks.releaseRenderTarget != nullptr) {
                callbacks.releaseRenderTarget("before AVC444 GPU compositor window bind");
            }
            active = true;
            activatedForThisCommand = true;
            logs.push_back("AVC444 GPU compositor took XComponent target after successful decode: " +
                std::to_string(target.width) + "x" + std::to_string(target.height));
        }

        if (!renderer.Ensure(target.window, target.width, target.height,
                command->width, command->height, logs)) {
            return fail("renderer init");
        }

        if (!renderer.Present(logs)) {
            return fail("present");
        }

        ++presented;
        logs.push_back("AVC444 GPU compositor consumed command: frame=" +
            std::to_string(command->frameId) + " LC=" + std::to_string(command->LC) +
            " presented=" + std::to_string(presented) +
            " route=hardware-decode+mapped-plane-gpu-combine");
        return true;
    }
#else
    void Destroy() {}
#endif
};

Avc444GpuCompositor::Avc444GpuCompositor() : impl_(std::make_unique<Impl>()) {}

Avc444GpuCompositor::~Avc444GpuCompositor() = default;

void Avc444GpuCompositor::Configure(bool enabled, Avc444GpuLogFn log,
    Avc444GpuCompositorCallbacks callbacks)
{
    std::lock_guard<std::mutex> processLock(processingMutex_);
    if (impl_) {
        impl_->Destroy();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    log_ = std::move(log);
    callbacks_ = std::move(callbacks);
    selfTestStarted_ = false;
    selfTestComplete_ = false;
    readyForGdiSuppression_ = false;
    active_ = false;
    candidates_ = 0;
    frameMismatchRejects_ = 0;
    invalidLcRejects_ = 0;
    lastFrameId_ = 0;
    lastLC_ = 0;
    lastStream1Bytes_ = 0;
    lastStream2Bytes_ = 0;
    lastTargetWidth_ = 0;
    lastTargetHeight_ = 0;
    diagnostics_ = enabled ?
        "avc444 gpu compositor: configured experimental mapped-plane compositor, gdi preserved until first successful present" :
        "avc444 gpu compositor: off";
}

void Avc444GpuCompositor::Reset()
{
    Configure(false, nullptr, {});
}

std::string Avc444GpuCompositor::Diagnostics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return diagnostics_;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool Avc444GpuCompositor::OnSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }

    bool shouldRunSelfTest = false;
    bool shouldLogCommand = false;
    bool frameOpen = false;
    bool lcValid = false;
    uint64_t candidate = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return false;
        }
        candidate = ++candidates_;
        shouldLogCommand = candidate <= 12 || (candidate % 120) == 0;
        shouldRunSelfTest = !selfTestStarted_;
        if (shouldRunSelfTest) {
            selfTestStarted_ = true;
        }

        frameOpen = command->frameOpen ? true : false;
        lcValid = IsValidLcForCommand(command);
        if (!frameOpen) {
            frameMismatchRejects_++;
        }
        if (!lcValid) {
            invalidLcRejects_++;
        }
        lastFrameId_ = command->frameId;
        lastLC_ = command->LC;
        lastStream1Bytes_ = command->stream1.length;
        lastStream2Bytes_ = command->stream2.length;
        lastTargetWidth_ = command->targetWidth;
        lastTargetHeight_ = command->targetHeight;
    }

    if (shouldRunSelfTest) {
        SelfTestResult result = RunSelfTest(command->targetWidth, command->targetHeight);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            selfTestComplete_ = true;
            readyForGdiSuppression_ =
                result.eglReady && result.avcodecHardwareReady && result.rawBufferCandidate;
            diagnostics_ = result.diagnostics;
        }
        Log("AVC444 GPU compositor self-test complete: " + result.diagnostics +
            "; GDI suppression still requires a successful decode+shader+present for this command");
    }

    if (shouldLogCommand || !frameOpen || !lcValid) {
        const RECTANGLE_16* stream1FirstRect = command->stream1.numRegionRects == 0 ?
            nullptr : command->stream1.regionRects;
        const RECTANGLE_16* stream2FirstRect = command->stream2.numRegionRects == 0 ?
            nullptr : command->stream2.regionRects;
        Log("AVC444 GPU compositor candidate: index=" + std::to_string(candidate) +
            " codec=" + std::to_string(command->codecId) +
            " surface=" + std::to_string(command->surfaceId) +
            " frame=" + std::to_string(command->frameId) +
            " frameOpen=" + std::string(frameOpen ? "yes" : "no") +
            " LC=" + std::to_string(command->LC) +
            " lcValid=" + std::string(lcValid ? "yes" : "no") +
            " commandRect=" + std::to_string(command->left) + "," +
            std::to_string(command->top) + " " +
            std::to_string(command->width) + "x" + std::to_string(command->height) +
            " target=" + std::to_string(command->targetWidth) + "x" +
            std::to_string(command->targetHeight) +
            " stream1=bytes:" + std::to_string(command->stream1.length) +
            ",rects:" + std::to_string(command->stream1.numRegionRects) +
            ",first:" + RectText(stream1FirstRect) +
            " stream2=bytes:" + std::to_string(command->stream2.length) +
            ",rects:" + std::to_string(command->stream2.numRegionRects) +
            ",first:" + RectText(stream2FirstRect));
    }

    bool selfTestGate = false;
    Avc444GpuCompositorCallbacks callbacks;
    bool active = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        selfTestGate = readyForGdiSuppression_;
        callbacks = callbacks_;
        active = active_;
    }

    if (!frameOpen || !lcValid || !selfTestGate) {
        if (!selfTestGate && shouldLogCommand) {
            Log("AVC444 GPU compositor gate closed; keeping FreeRDP native GDI path");
        }
        return false;
    }

    std::vector<std::string> logs;
    bool consumed = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active = active_;
        }
        consumed = impl_ != nullptr &&
            impl_->ProcessCommand(command, callbacks, active, logs);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_ = active;
            if (consumed) {
                readyForGdiSuppression_ = true;
            }
            diagnostics_ = "avc444 gpu compositor: enabled=yes selfTest=yes active=" +
                std::string(active_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " suppress=" + std::string(consumed ? "this-command" : "no");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return consumed;
}
#endif

Avc444GpuCompositor::SelfTestResult Avc444GpuCompositor::RunSelfTest(
    uint32_t width, uint32_t height)
{
    SelfTestResult result;
    std::vector<std::string> logs = ProbeEglNativeImage(result);
    std::vector<std::string> avcodecLogs = ProbeAvcCapability(width, height, result);
    logs.insert(logs.end(), avcodecLogs.begin(), avcodecLogs.end());

    result.diagnostics =
        "avc444 gpu compositor: egl=" + std::string(result.eglReady ? "yes" : "no") +
        " nativeImage=" + std::string(result.nativeImageReady ? "yes" : "no") +
        " avcodecHardware=" + std::string(result.avcodecHardwareReady ? "yes" : "no") +
        " nativeBufferFormats=" + std::string(result.nativeBufferFormatsKnown ? "known" : "unknown") +
        " rawYuvCandidate=" + std::string(result.rawBufferCandidate ? "yes" : "no") +
        " readyForSuppressGdi=" +
        std::string((result.eglReady && result.avcodecHardwareReady && result.rawBufferCandidate) ?
            "after-successful-present" : "no");

    Avc444GpuCompositor& compositor = SharedAvc444GpuCompositor();
    for (const std::string& line : logs) {
        compositor.Log(line);
    }
    return result;
}

void Avc444GpuCompositor::Log(const std::string& message) const
{
    Avc444GpuLogFn log;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        log = log_;
    }
    if (log != nullptr) {
        log(message);
    }
}

Avc444GpuCompositor& SharedAvc444GpuCompositor()
{
    static Avc444GpuCompositor compositor;
    return compositor;
}

} // namespace rdp_bridge
