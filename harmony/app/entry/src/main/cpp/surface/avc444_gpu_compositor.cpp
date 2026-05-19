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
#include <native_image/native_image.h>

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

namespace rdp_bridge {
namespace {

constexpr const char* kAvcMime = "video/avc";
constexpr int64_t kInputTimeoutUs = 20000;
constexpr int64_t kOutputTimeoutUs = 12000;
constexpr int64_t kFollowupOutputTimeoutUs = 6000;
constexpr int64_t kOutputSyncDeadlineUs = 120000;
constexpr uint32_t kOutputSyncMaxAttempts = 32;
bool gZeroCopyNativeBufferDisabledBySelfTest = false;

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
    const bool externalTextureEssl3Ready =
        ContainsText(glExtensions, "GL_OES_EGL_image_external_essl3");
    const bool yuvTargetReady = ContainsText(glExtensions, "GL_EXT_YUV_target");
    const bool eglNativeBufferReady = ContainsText(eglExtensions, "EGL_OHOS_image_native_buffer");
    const bool eglImageProcReady =
        eglGetProcAddress("eglCreateImageKHR") != nullptr &&
        eglGetProcAddress("eglDestroyImageKHR") != nullptr &&
        eglGetProcAddress("glEGLImageTargetTexture2DOES") != nullptr;
    result.zeroCopyNativeBufferReady =
        externalTextureEssl3Ready && yuvTargetReady && eglNativeBufferReady && eglImageProcReady;

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
        " externalTextureEssl3=" + std::string(externalTextureEssl3Ready ? "yes" : "no") +
        " yuvTarget=" + std::string(yuvTargetReady ? "yes" : "no") +
        " nativeImage=" + std::string(image != nullptr ? "yes" : "no") +
        " imageWindow=" + std::string(imageWindow != nullptr ? "yes" : "no") +
        " imageSurfaceIdStatus=" + std::to_string(surfaceIdStatus) +
        " imageSurfaceId=" + std::to_string(surfaceId) +
        " eglNativeBufferExt=" +
        std::string(eglNativeBufferReady ? "yes" : "unknown") +
        " eglImageProc=" + std::string(eglImageProcReady ? "yes" : "no") +
        " zeroCopyNativeBuffer=" +
        std::string(result.zeroCopyNativeBufferReady ? "yes" : "no"));

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

bool RectsContainFullSurface(const RECTANGLE_16* rects, uint32_t count, uint32_t width,
    uint32_t height)
{
    if (rects == nullptr || count == 0 || width == 0 || height == 0) {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        const RECTANGLE_16& r = rects[i];
        if (r.left == 0 && r.top == 0 && r.right == width && r.bottom == height) {
            return true;
        }
    }
    return false;
}

bool RectsCoverFullSurface(const RECTANGLE_16* rects, uint32_t count, uint32_t width,
    uint32_t height)
{
    if (!RectsValid(rects, count, width, height)) {
        return false;
    }
    if (RectsContainFullSurface(rects, count, width, height)) {
        return true;
    }

    std::vector<uint32_t> edges;
    edges.reserve((static_cast<size_t>(count) * 2U) + 2U);
    edges.push_back(0);
    edges.push_back(height);
    for (uint32_t i = 0; i < count; ++i) {
        edges.push_back(rects[i].top);
        edges.push_back(rects[i].bottom);
    }
    std::sort(edges.begin(), edges.end());
    edges.erase(std::unique(edges.begin(), edges.end()), edges.end());

    for (size_t band = 0; band + 1U < edges.size(); ++band) {
        const uint32_t top = edges[band];
        const uint32_t bottom = edges[band + 1U];
        if (top == bottom) {
            continue;
        }

        std::vector<std::pair<uint32_t, uint32_t>> intervals;
        intervals.reserve(count);
        for (uint32_t i = 0; i < count; ++i) {
            const RECTANGLE_16& r = rects[i];
            if (r.top <= top && r.bottom >= bottom) {
                intervals.emplace_back(r.left, r.right);
            }
        }
        if (intervals.empty()) {
            return false;
        }
        std::sort(intervals.begin(), intervals.end());

        uint32_t coveredRight = 0;
        bool started = false;
        for (const auto& interval : intervals) {
            if (!started) {
                if (interval.first != 0) {
                    return false;
                }
                coveredRight = interval.second;
                started = true;
            } else if (interval.first <= coveredRight) {
                coveredRight = std::max(coveredRight, interval.second);
            } else {
                return false;
            }
            if (coveredRight >= width) {
                break;
            }
        }
        if (!started || coveredRight < width) {
            return false;
        }
    }
    return true;
}

bool IsFullAvc444Refresh(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    return command != nullptr && command->LC == 0 &&
        RectsCoverFullSurface(command->stream1.regionRects, command->stream1.numRegionRects,
            command->width, command->height) &&
        RectsCoverFullSurface(command->stream2.regionRects, command->stream2.numRegionRects,
            command->width, command->height);
}

std::string RectText(const RECTANGLE_16* rect);

uint8_t TestByte(uint32_t x, uint32_t y, uint32_t seed)
{
    return static_cast<uint8_t>((x * 37U + y * 17U + seed * 29U + 13U) & 0xFFU);
}

void ApplyChromaV2Reference(const std::vector<uint8_t>& srcY,
    const std::vector<uint8_t>& srcU, const std::vector<uint8_t>& srcV, uint32_t srcYStep,
    uint32_t srcUvStep, uint32_t totalWidth, std::vector<uint8_t>& dstU,
    std::vector<uint8_t>& dstV, uint32_t dstStep, const RECTANGLE_16& roi)
{
    const uint32_t width = roi.right - roi.left;
    const uint32_t height = roi.bottom - roi.top;
    const uint32_t halfWidth = (width + 1U) / 2U;
    const uint32_t halfHeight = (height + 1U) / 2U;
    const uint32_t quarterWidth = (width + 3U) / 4U;

    for (uint32_t y = 0; y < height; ++y) {
        const uint32_t yTop = y + roi.top;
        const uint32_t srcOddBase = yTop * srcYStep + roi.left / 2U;
        const uint32_t srcOddVBase = srcOddBase + totalWidth / 2U;
        const uint32_t dstBase = yTop * dstStep + roi.left;
        for (uint32_t x = 0; x < halfWidth; ++x) {
            const uint32_t odd = 2U * x + 1U;
            dstU[dstBase + odd] = srcY[srcOddBase + x];
            dstV[dstBase + odd] = srcY[srcOddVBase + x];
        }
    }

    for (uint32_t y = 0; y < halfHeight; ++y) {
        const uint32_t srcRow = y + roi.top / 2U;
        const uint32_t srcBase = srcRow * srcUvStep + roi.left / 4U;
        const uint32_t srcVBase = srcBase + totalWidth / 4U;
        const uint32_t dstBase = (2U * y + 1U + roi.top) * dstStep + roi.left;
        for (uint32_t x = 0; x < quarterWidth; ++x) {
            dstU[dstBase + 4U * x + 0U] = srcU[srcBase + x];
            dstV[dstBase + 4U * x + 0U] = srcU[srcVBase + x];
            dstU[dstBase + 4U * x + 2U] = srcV[srcBase + x];
            dstV[dstBase + 4U * x + 2U] = srcV[srcVBase + x];
        }
    }
}

void ApplyChromaV2ShaderModel(const std::vector<uint8_t>& srcY,
    const std::vector<uint8_t>& srcU, const std::vector<uint8_t>& srcV, uint32_t srcYStep,
    uint32_t srcUvStep, uint32_t totalWidth, std::vector<uint8_t>& dstU,
    std::vector<uint8_t>& dstV, uint32_t dstStep, const RECTANGLE_16& roi)
{
    for (uint32_t y = roi.top; y < roi.bottom; ++y) {
        for (uint32_t x = roi.left; x < roi.right; ++x) {
            const uint32_t relX = x - roi.left;
            const uint32_t relY = y - roi.top;
            const uint32_t dst = y * dstStep + x;
            if ((relX & 1U) == 1U) {
                const uint32_t srcX = roi.left / 2U + relX / 2U;
                dstU[dst] = srcY[y * srcYStep + srcX];
                dstV[dst] = srcY[y * srcYStep + srcX + totalWidth / 2U];
            } else if ((relY & 1U) == 1U && (relX & 3U) == 0U) {
                const uint32_t srcX = roi.left / 4U + relX / 4U;
                const uint32_t srcYCoord = roi.top / 2U + relY / 2U;
                dstU[dst] = srcU[srcYCoord * srcUvStep + srcX];
                dstV[dst] = srcU[srcYCoord * srcUvStep + srcX + totalWidth / 4U];
            } else if ((relY & 1U) == 1U && (relX & 3U) == 2U) {
                const uint32_t srcX = roi.left / 4U + relX / 4U;
                const uint32_t srcYCoord = roi.top / 2U + relY / 2U;
                dstU[dst] = srcV[srcYCoord * srcUvStep + srcX];
                dstV[dst] = srcV[srcYCoord * srcUvStep + srcX + totalWidth / 4U];
            }
        }
    }
}

bool RunAvc444V2LayoutSelfTest(std::vector<std::string>& logs)
{
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 32;
    constexpr uint32_t yStep = width;
    constexpr uint32_t uvStep = width / 2U;
    std::vector<uint8_t> srcY(yStep * height);
    std::vector<uint8_t> srcU(uvStep * (height / 2U));
    std::vector<uint8_t> srcV(uvStep * (height / 2U));
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < yStep; ++x) {
            srcY[y * yStep + x] = TestByte(x, y, 1);
        }
    }
    for (uint32_t y = 0; y < height / 2U; ++y) {
        for (uint32_t x = 0; x < uvStep; ++x) {
            srcU[y * uvStep + x] = TestByte(x, y, 2);
            srcV[y * uvStep + x] = TestByte(x, y, 3);
        }
    }

    const std::array<RECTANGLE_16, 5> rects {{
        { 0, 0, 64, 32 },
        { 4, 2, 36, 18 },
        { 2, 6, 34, 22 },
        { 8, 10, 56, 30 },
        { 12, 4, 28, 20 },
    }};
    for (const RECTANGLE_16& rect : rects) {
        std::vector<uint8_t> refU(width * height, 0x80);
        std::vector<uint8_t> refV(width * height, 0x81);
        std::vector<uint8_t> shaderU = refU;
        std::vector<uint8_t> shaderV = refV;
        ApplyChromaV2Reference(srcY, srcU, srcV, yStep, uvStep, width, refU, refV, width, rect);
        ApplyChromaV2ShaderModel(srcY, srcU, srcV, yStep, uvStep, width, shaderU, shaderV, width,
            rect);
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t offset = y * width + x;
                if (refU[offset] != shaderU[offset] || refV[offset] != shaderV[offset]) {
                    logs.push_back("AVC444 GPU compositor AVC444v2 layout self-test failed: rect=" +
                        RectText(&rect) + " at=" + std::to_string(x) + "," +
                        std::to_string(y) + " ref=" + std::to_string(refU[offset]) + "/" +
                        std::to_string(refV[offset]) + " shaderModel=" +
                        std::to_string(shaderU[offset]) + "/" + std::to_string(shaderV[offset]));
                    return false;
                }
            }
        }
    }

    logs.push_back("AVC444 GPU compositor AVC444v2 layout self-test passed against FreeRDP "
        "general_ChromaV2ToYUV444 model");
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

    bool NativeBufferValid() const
    {
        return nativeBuffer != nullptr && width > 0 && height > 0;
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

    DecodeResult DecodeNativeBuffer(const uint8_t* data, uint32_t size, int64_t pts,
        DecodedFrame& frame, std::vector<std::string>& logs)
    {
        if (decoder_ == nullptr || !started_ || data == nullptr || size == 0) {
            logs.push_back("AVC444 GPU " + role_ + " zero-copy decode skipped: invalid input");
            return DecodeResult::Failed;
        }

        uint32_t inputIndex = 0;
        OH_AVErrCode rc = OH_VideoDecoder_QueryInputBuffer(decoder_, &inputIndex, kInputTimeoutUs);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " zero-copy input unavailable rc=" +
                std::to_string(static_cast<int32_t>(rc)) + " size=" + std::to_string(size));
            return DecodeResult::Failed;
        }

        OH_AVBuffer* input = OH_VideoDecoder_GetInputBuffer(decoder_, inputIndex);
        uint8_t* dst = input == nullptr ? nullptr : OH_AVBuffer_GetAddr(input);
        const int32_t capacity = input == nullptr ? -1 : OH_AVBuffer_GetCapacity(input);
        if (dst == nullptr || capacity < 0 || static_cast<uint32_t>(capacity) < size) {
            logs.push_back("AVC444 GPU " + role_ +
                " zero-copy input buffer invalid capacity=" + std::to_string(capacity) +
                " size=" + std::to_string(size));
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
            logs.push_back("AVC444 GPU " + role_ + " zero-copy set input attr failed rc=" +
                std::to_string(static_cast<int32_t>(rc)));
            PushEmptyInput(input, inputIndex);
            return DecodeResult::Failed;
        }

        rc = OH_VideoDecoder_PushInputBuffer(decoder_, inputIndex);
        if (rc != AV_ERR_OK) {
            logs.push_back("AVC444 GPU " + role_ + " zero-copy push input failed rc=" +
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
                UpdateOutputDescription(logs, "zero-copy-stream-changed");
                continue;
            }
            if (rc == AV_ERR_TRY_AGAIN_LATER) {
                waitedUs += timeout;
                continue;
            }
            if (rc != AV_ERR_OK) {
                logs.push_back("AVC444 GPU " + role_ + " zero-copy query output failed rc=" +
                    std::to_string(static_cast<int32_t>(rc)));
                return DecodeResult::Failed;
            }

            OH_AVBuffer* output = OH_VideoDecoder_GetOutputBuffer(decoder_, outputIndex);
            OH_AVCodecBufferAttr outputAttr {};
            if (output == nullptr ||
                OH_AVBuffer_GetBufferAttr(output, &outputAttr) != AV_ERR_OK) {
                OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
                logs.push_back("AVC444 GPU " + role_ + " zero-copy output buffer invalid");
                return DecodeResult::Failed;
            }
            if (outputAttr.pts != pts) {
                OH_VideoDecoder_FreeOutputBuffer(decoder_, outputIndex);
                logs.push_back("AVC444 GPU " + role_ + " zero-copy discarded stale output pts=" +
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
            frame.nativeBuffer = OH_AVBuffer_GetNativeBuffer(frame.buffer);
            if (frame.nativeBuffer == nullptr) {
                frame.Release();
                logs.push_back("AVC444 GPU " + role_ +
                    " zero-copy output has no native buffer");
                return DecodeResult::Failed;
            }
            OH_NativeBuffer_Config config {};
            OH_NativeBuffer_GetConfig(frame.nativeBuffer, &config);
            frame.nativeFormat = config.format;
            frame.nativeWidth = static_cast<uint32_t>(std::max(0, config.width));
            frame.nativeHeight = static_cast<uint32_t>(std::max(0, config.height));
            frame.nativeStride = static_cast<uint32_t>(std::max(0, config.stride));

            ++outputs_;
            if (outputs_ <= 8 || (outputs_ % 120) == 0) {
                logs.push_back("AVC444 GPU " + role_ + " zero-copy decoded native buffer: pts=" +
                    std::to_string(pts) + " nativeFormat=" +
                    NativeBufferFormatName(static_cast<OH_NativeBuffer_Format>(frame.nativeFormat)) +
                    " size=" + std::to_string(config.width) + "x" +
                    std::to_string(config.height) + " stride=" + std::to_string(config.stride));
            }
            return DecodeResult::Decoded;
        }

        ++noOutput_;
        if (noOutput_ <= 8 || (noOutput_ % 120) == 0) {
            logs.push_back("AVC444 GPU " + role_ +
                " zero-copy synchronous output wait timed out: pts=" + std::to_string(pts) +
                " waitedUs=" + std::to_string(waitedUs) +
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
            if (copyYExternalProgram_ != 0) {
                glDeleteProgram(copyYExternalProgram_);
            }
            if (lumaUvExternalProgram_ != 0) {
                glDeleteProgram(lumaUvExternalProgram_);
            }
            if (chromaV2ExternalProgram_ != 0) {
                glDeleteProgram(chromaV2ExternalProgram_);
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
        copyYExternalProgram_ = 0;
        lumaUvExternalProgram_ = 0;
        chromaV2ExternalProgram_ = 0;
        presentProgram_ = 0;
        zeroCopyNativeBufferReady_ = false;
        eglCreateImageKHRFn_ = nullptr;
        eglDestroyImageKHRFn_ = nullptr;
        glEGLImageTargetTexture2DOESFn_ = nullptr;
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

        DetectZeroCopyNativeBufferSupport(logs);
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
            " GLES3 " +
            std::string(zeroCopyNativeBufferReady_ ?
                "native-buffer zero-copy preferred, mapped-plane fallback" :
                "mapped-plane shader path"));
        return true;
    }

    bool ZeroCopyNativeBufferReady() const
    {
        return zeroCopyNativeBufferReady_;
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
            << ",zeroCopy:" << (zeroCopyNativeBufferReady_ ? "yes" : "no");
        return out.str();
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

    bool ApplyLumaNativeBuffer(const DecodedFrame& frame, const RECTANGLE_16* rects,
        uint32_t rectCount, std::vector<std::string>& logs)
    {
        if (!MakeCurrent(logs)) {
            return false;
        }

        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        if (!BindNativeBufferExternal(frame, image, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glUseProgram(copyYExternalProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, srcExternalTexture_);
        glUniform1i(glGetUniformLocation(copyYExternalProgram_, "uSrcYuv"), 0);
        glUniform1i(glGetUniformLocation(copyYExternalProgram_, "uNativeWidth"),
            static_cast<GLint>(frame.nativeWidth));
        glUniform1i(glGetUniformLocation(copyYExternalProgram_, "uNativeHeight"),
            static_cast<GLint>(frame.nativeHeight));
        glUniform1i(glGetUniformLocation(copyYExternalProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        DrawRectsToTexture(yTexture_, rects, rectCount);

        glUseProgram(lumaUvExternalProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, srcExternalTexture_);
        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uSrcYuv"), 0);
        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uTargetPlane"), 0);
        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uNativeWidth"),
            static_cast<GLint>(frame.nativeWidth));
        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uNativeHeight"),
            static_cast<GLint>(frame.nativeHeight));
        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        const GLint lumaRectLeft = glGetUniformLocation(lumaUvExternalProgram_, "uRectLeft");
        const GLint lumaRectTop = glGetUniformLocation(lumaUvExternalProgram_, "uRectTop");
        DrawRectsToTexture(uTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

        glUniform1i(glGetUniformLocation(lumaUvExternalProgram_, "uTargetPlane"), 1);
        DrawRectsToTexture(vTexture_, rects, rectCount, lumaRectLeft, lumaRectTop);

        glFinish();
        const GLenum error = glGetError();
        DestroyEglImage(image);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU zero-copy luma shader failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        hasLuma_ = true;
        return true;
    }

    bool ApplyChromaV2NativeBuffer(const DecodedFrame& frame, const RECTANGLE_16* rects,
        uint32_t rectCount, std::vector<std::string>& logs)
    {
        if (!MakeCurrent(logs)) {
            return false;
        }

        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        if (!BindNativeBufferExternal(frame, image, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }

        glUseProgram(chromaV2ExternalProgram_);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, srcExternalTexture_);
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uSrcYuv"), 0);
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uSurfaceWidth"),
            static_cast<GLint>(surfaceWidth_));
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uSurfaceHeight"),
            static_cast<GLint>(surfaceHeight_));
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uAlignedWidth"),
            static_cast<GLint>(frame.alignedWidth));
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uNativeWidth"),
            static_cast<GLint>(frame.nativeWidth));
        glUniform1i(glGetUniformLocation(chromaV2ExternalProgram_, "uNativeHeight"),
            static_cast<GLint>(frame.nativeHeight));

        PingPongChromaPlane(true, rects, rectCount, chromaV2ExternalProgram_);
        PingPongChromaPlane(false, rects, rectCount, chromaV2ExternalProgram_);

        glFinish();
        const GLenum error = glGetError();
        DestroyEglImage(image);
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU zero-copy chroma-v2 shader failed glError=" +
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

    bool RunChromaV2ShaderReadbackSelfTest(std::vector<std::string>& logs)
    {
        constexpr uint32_t width = 64;
        constexpr uint32_t height = 32;
        constexpr uint32_t yStep = width;
        constexpr uint32_t uvStep = width / 2U;
        std::vector<uint8_t> srcY(yStep * height);
        std::vector<uint8_t> srcU(uvStep * (height / 2U));
        std::vector<uint8_t> srcV(uvStep * (height / 2U));
        std::vector<uint8_t> srcUV(width * (height / 2U));
        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < yStep; ++x) {
                srcY[y * yStep + x] = TestByte(x, y, 7);
            }
        }
        for (uint32_t y = 0; y < height / 2U; ++y) {
            for (uint32_t x = 0; x < uvStep; ++x) {
                const uint8_t u = TestByte(x, y, 8);
                const uint8_t v = TestByte(x, y, 9);
                srcU[y * uvStep + x] = u;
                srcV[y * uvStep + x] = v;
                srcUV[y * width + 2U * x + 0U] = u;
                srcUV[y * width + 2U * x + 1U] = v;
            }
        }

        const std::array<RECTANGLE_16, 5> rects {{
            { 0, 0, 64, 32 },
            { 4, 2, 36, 18 },
            { 2, 6, 34, 22 },
            { 8, 10, 56, 30 },
            { 12, 4, 28, 20 },
        }};
        std::vector<uint8_t> refU(width * height, 0x80);
        std::vector<uint8_t> refV(width * height, 0x81);
        for (const RECTANGLE_16& rect : rects) {
            ApplyChromaV2Reference(srcY, srcU, srcV, yStep, uvStep, width, refU, refV, width,
                rect);
        }

        DecodedFrame frame;
        frame.y.data = srcY.data();
        frame.y.rowStride = yStep;
        frame.y.columnStride = 1;
        frame.uv.data = srcUV.data();
        frame.uv.rowStride = width;
        frame.uv.columnStride = 2;
        frame.width = width;
        frame.height = height;
        frame.alignedWidth = width;
        frame.alignedHeight = height;
        frame.yUploadWidth = width;
        frame.uvUploadWidth = uvStep;
        frame.uvUploadHeight = height / 2U;
        frame.nv21 = false;

        std::vector<uint8_t> baselineU(width * height, 0x80);
        std::vector<uint8_t> baselineV(width * height, 0x81);
        if (!MakeCurrent(logs) ||
            !UploadPlaneTexture(uTexture_, baselineU, logs) ||
            !UploadPlaneTexture(vTexture_, baselineV, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (!ApplyChromaV2(frame, rects.data(), static_cast<uint32_t>(rects.size()), logs)) {
            logs.push_back("AVC444 GPU compositor AVC444v2 shader readback self-test failed: "
                "shader update did not complete");
            return false;
        }

        std::vector<uint8_t> gotU;
        std::vector<uint8_t> gotV;
        if (!MakeCurrent(logs) || !ReadPlaneTexture(uTexture_, gotU, logs) ||
            !ReadPlaneTexture(vTexture_, gotV, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t offset = y * width + x;
                if (gotU[offset] != refU[offset] || gotV[offset] != refV[offset]) {
                    logs.push_back(
                        "AVC444 GPU compositor AVC444v2 shader readback self-test failed: at=" +
                        std::to_string(x) + "," + std::to_string(y) +
                        " ref=" + std::to_string(refU[offset]) + "/" +
                        std::to_string(refV[offset]) + " gpu=" +
                        std::to_string(gotU[offset]) + "/" + std::to_string(gotV[offset]));
                    return false;
                }
            }
        }

        logs.push_back("AVC444 GPU compositor AVC444v2 mapped shader readback self-test passed "
            "bit-exact against FreeRDP general_ChromaV2ToYUV444 model");
        if (zeroCopyNativeBufferReady_ &&
            !RunNativeBufferExternalChromaV2ReadbackSelfTest(srcY, srcU, srcV, refU, refV,
                rects.data(), static_cast<uint32_t>(rects.size()), logs)) {
            zeroCopyNativeBufferReady_ = false;
            gZeroCopyNativeBufferDisabledBySelfTest = true;
            logs.push_back("AVC444 GPU compositor disabled native-buffer zero-copy path after "
                "self-test failure; mapped-plane GPU path remains available");
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

    void DetectZeroCopyNativeBufferSupport(std::vector<std::string>& logs)
    {
        const char* glExtensions = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
        const char* eglExtensions = eglQueryString(display_, EGL_EXTENSIONS);
        const bool externalEssl3Ready =
            ContainsText(glExtensions, "GL_OES_EGL_image_external_essl3");
        const bool yuvTargetReady = ContainsText(glExtensions, "GL_EXT_YUV_target");
        const bool eglNativeBufferReady =
            ContainsText(eglExtensions, "EGL_OHOS_image_native_buffer");

        eglCreateImageKHRFn_ =
            reinterpret_cast<PFNEGLCREATEIMAGEKHRPROC>(eglGetProcAddress("eglCreateImageKHR"));
        eglDestroyImageKHRFn_ =
            reinterpret_cast<PFNEGLDESTROYIMAGEKHRPROC>(eglGetProcAddress("eglDestroyImageKHR"));
        glEGLImageTargetTexture2DOESFn_ =
            reinterpret_cast<PFNGLEGLIMAGETARGETTEXTURE2DOESPROC>(
                eglGetProcAddress("glEGLImageTargetTexture2DOES"));

        zeroCopyNativeBufferReady_ = !gZeroCopyNativeBufferDisabledBySelfTest &&
            externalEssl3Ready && yuvTargetReady &&
            eglNativeBufferReady && eglCreateImageKHRFn_ != nullptr &&
            eglDestroyImageKHRFn_ != nullptr && glEGLImageTargetTexture2DOESFn_ != nullptr;
        logs.push_back("AVC444 GPU renderer zero-copy capability: externalEssl3=" +
            std::string(externalEssl3Ready ? "yes" : "no") +
            " yuvTarget=" + std::string(yuvTargetReady ? "yes" : "no") +
            " eglNativeBuffer=" + std::string(eglNativeBufferReady ? "yes" : "no") +
            " eglImageFns=" +
            std::string((eglCreateImageKHRFn_ != nullptr && eglDestroyImageKHRFn_ != nullptr &&
                glEGLImageTargetTexture2DOESFn_ != nullptr) ? "yes" : "no") +
            " disabledBySelfTest=" +
            std::string(gZeroCopyNativeBufferDisabledBySelfTest ? "yes" : "no") +
            " zeroCopy=" + std::string(zeroCopyNativeBufferReady_ ? "ready" : "disabled"));
    }

    bool NativeBufferExternalSamplingValid(const DecodedFrame& frame,
        std::vector<std::string>& logs) const
    {
        if (!zeroCopyNativeBufferReady_) {
            logs.push_back("AVC444 GPU zero-copy native buffer rejected: capability disabled");
            return false;
        }
        if (!frame.NativeBufferValid()) {
            logs.push_back("AVC444 GPU zero-copy native buffer rejected: invalid decoded frame");
            return false;
        }
        if (frame.nativeWidth < frame.alignedWidth || frame.nativeHeight < frame.height ||
            frame.nativeWidth == 0 || frame.nativeHeight == 0) {
            logs.push_back("AVC444 GPU zero-copy native buffer rejected: texture extent " +
                std::to_string(frame.nativeWidth) + "x" + std::to_string(frame.nativeHeight) +
                " is smaller than AVC444 aligned source " +
                std::to_string(frame.alignedWidth) + "x" + std::to_string(frame.height) +
                "; mapped-plane fallback will preserve FreeRDP AVC444v2 layout semantics");
            return false;
        }
        return true;
    }

    bool BindNativeBufferExternal(const DecodedFrame& frame, EGLImageKHR& image,
        std::vector<std::string>& logs)
    {
        image = EGL_NO_IMAGE_KHR;
        if (!NativeBufferExternalSamplingValid(frame, logs)) {
            return false;
        }

        const EGLint preservedAttribs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
        image = eglCreateImageKHRFn_(display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS,
            reinterpret_cast<EGLClientBuffer>(frame.nativeBuffer), preservedAttribs);
        if (image == EGL_NO_IMAGE_KHR) {
            const EGLint errorWithPreserve = eglGetError();
            const EGLint emptyAttribs[] = { EGL_NONE };
            image = eglCreateImageKHRFn_(display_, EGL_NO_CONTEXT, EGL_NATIVE_BUFFER_OHOS,
                reinterpret_cast<EGLClientBuffer>(frame.nativeBuffer), emptyAttribs);
            if (image == EGL_NO_IMAGE_KHR) {
                logs.push_back("AVC444 GPU zero-copy eglCreateImageKHR failed preserveError=" +
                    Hex32(static_cast<uint32_t>(errorWithPreserve)) +
                    " retryError=" + Hex32(static_cast<uint32_t>(eglGetError())));
                return false;
            }
        }

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, srcExternalTexture_);
        ConfigureTexture(GL_TEXTURE_EXTERNAL_OES);
        glEGLImageTargetTexture2DOESFn_(GL_TEXTURE_EXTERNAL_OES,
            static_cast<GLeglImageOES>(image));
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU zero-copy glEGLImageTargetTexture2DOES failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            DestroyEglImage(image);
            image = EGL_NO_IMAGE_KHR;
            return false;
        }
        return true;
    }

    void DestroyEglImage(EGLImageKHR& image)
    {
        if (image != EGL_NO_IMAGE_KHR && eglDestroyImageKHRFn_ != nullptr) {
            eglDestroyImageKHRFn_(display_, image);
        }
        image = EGL_NO_IMAGE_KHR;
    }

    bool RunNativeBufferExternalChromaV2ReadbackSelfTest(const std::vector<uint8_t>& srcY,
        const std::vector<uint8_t>& srcU, const std::vector<uint8_t>& srcV,
        const std::vector<uint8_t>& refU, const std::vector<uint8_t>& refV,
        const RECTANGLE_16* rects, uint32_t rectCount, std::vector<std::string>& logs)
    {
        constexpr uint32_t width = 64;
        constexpr uint32_t height = 32;
        OH_NativeBuffer_Config request {};
        request.width = static_cast<int32_t>(width);
        request.height = static_cast<int32_t>(height);
        request.format = NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP;
        request.usage = static_cast<int32_t>(NATIVEBUFFER_USAGE_CPU_READ |
            NATIVEBUFFER_USAGE_CPU_WRITE | NATIVEBUFFER_USAGE_HW_TEXTURE);
        request.stride = static_cast<int32_t>(width);

        DecodedFrame frame;
        frame.nativeBuffer = OH_NativeBuffer_Alloc(&request);
        if (frame.nativeBuffer == nullptr) {
            logs.push_back("AVC444 GPU compositor native-buffer zero-copy self-test skipped: "
                "OH_NativeBuffer_Alloc returned null");
            return false;
        }
        frame.width = width;
        frame.height = height;
        frame.alignedWidth = width;
        frame.alignedHeight = height;

        OH_NativeBuffer_Config actual {};
        OH_NativeBuffer_GetConfig(frame.nativeBuffer, &actual);
        frame.nativeFormat = actual.format;
        frame.nativeWidth = static_cast<uint32_t>(std::max(0, actual.width));
        frame.nativeHeight = static_cast<uint32_t>(std::max(0, actual.height));
        frame.nativeStride = static_cast<uint32_t>(std::max(0, actual.stride));

        void* address = nullptr;
        OH_NativeBuffer_Planes planes {};
        const int32_t mapRc = OH_NativeBuffer_MapPlanes(frame.nativeBuffer, &address, &planes);
        if (mapRc != 0 || address == nullptr || planes.planeCount < 2 ||
            planes.planes[0].rowStride < width || planes.planes[1].rowStride < width ||
            planes.planes[1].columnStride < 2) {
            logs.push_back("AVC444 GPU compositor native-buffer zero-copy self-test failed to map "
                "NV12 planes: rc=" + std::to_string(mapRc) +
                " planeCount=" + std::to_string(planes.planeCount) +
                " yStride=" + std::to_string(planes.planes[0].rowStride) +
                " uvStride=" + std::to_string(planes.planes[1].rowStride) +
                " uvColumn=" + std::to_string(planes.planes[1].columnStride));
            return false;
        }

        uint8_t* base = static_cast<uint8_t*>(address);
        for (uint32_t y = 0; y < height; ++y) {
            std::memcpy(base + planes.planes[0].offset +
                    static_cast<size_t>(y) * planes.planes[0].rowStride,
                srcY.data() + static_cast<size_t>(y) * width, width);
        }
        for (uint32_t y = 0; y < height / 2U; ++y) {
            for (uint32_t x = 0; x < width / 2U; ++x) {
                uint8_t* pixel = base + planes.planes[1].offset +
                    static_cast<size_t>(y) * planes.planes[1].rowStride +
                    static_cast<size_t>(x) * planes.planes[1].columnStride;
                pixel[0] = srcU[y * (width / 2U) + x];
                pixel[1] = srcV[y * (width / 2U) + x];
            }
        }
        OH_NativeBuffer_Unmap(frame.nativeBuffer);

        std::vector<uint8_t> baselineU(width * height, 0x80);
        std::vector<uint8_t> baselineV(width * height, 0x81);
        if (!MakeCurrent(logs) ||
            !UploadPlaneTexture(uTexture_, baselineU, logs) ||
            !UploadPlaneTexture(vTexture_, baselineV, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        if (!ApplyChromaV2NativeBuffer(frame, rects, rectCount, logs)) {
            logs.push_back("AVC444 GPU compositor native-buffer zero-copy self-test failed: "
                "external YUV shader update did not complete");
            return false;
        }

        std::vector<uint8_t> gotU;
        std::vector<uint8_t> gotV;
        if (!MakeCurrent(logs) || !ReadPlaneTexture(uTexture_, gotU, logs) ||
            !ReadPlaneTexture(vTexture_, gotV, logs)) {
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            return false;
        }
        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

        for (uint32_t y = 0; y < height; ++y) {
            for (uint32_t x = 0; x < width; ++x) {
                const uint32_t offset = y * width + x;
                if (gotU[offset] != refU[offset] || gotV[offset] != refV[offset]) {
                    logs.push_back("AVC444 GPU compositor native-buffer zero-copy self-test failed: "
                        "at=" + std::to_string(x) + "," + std::to_string(y) +
                        " ref=" + std::to_string(refU[offset]) + "/" +
                        std::to_string(refV[offset]) + " gpu=" +
                        std::to_string(gotU[offset]) + "/" + std::to_string(gotV[offset]));
                    return false;
                }
            }
        }

        logs.push_back("AVC444 GPU compositor native-buffer zero-copy AVC444v2 shader self-test "
            "passed bit-exact against FreeRDP general_ChromaV2ToYUV444 model");
        return true;
    }

    void DeleteTextures()
    {
        std::array<GLuint*, 9> textures {
            &srcYTexture_, &srcUVTexture_, &srcExternalTexture_, &yTexture_, &uTexture_,
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
        static constexpr const char* copyYExternalFragment =
            "#version 300 es\n"
            "#extension GL_OES_EGL_image_external_essl3 : require\n"
            "#extension GL_EXT_YUV_target : require\n"
            "precision mediump float;\n"
            "uniform __samplerExternal2DY2YEXT uSrcYuv;\n"
            "uniform int uNativeWidth;\n"
            "uniform int uNativeHeight;\n"
            "uniform int uSurfaceHeight;\n"
            "out vec4 fragColor;\n"
            "vec3 sampleYuv(int x, int y) {\n"
            "  x = clamp(x, 0, uNativeWidth - 1);\n"
            "  y = clamp(y, 0, uNativeHeight - 1);\n"
            "  vec2 coord = (vec2(float(x) + 0.5, float(y) + 0.5) /\n"
            "      vec2(float(uNativeWidth), float(uNativeHeight)));\n"
            "  return texture(uSrcYuv, coord).xyz;\n"
            "}\n"
            "void main() {\n"
            "  int x = int(floor(gl_FragCoord.x));\n"
            "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  fragColor = vec4(sampleYuv(x, y).x, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* lumaUvExternalFragment =
            "#version 300 es\n"
            "#extension GL_OES_EGL_image_external_essl3 : require\n"
            "#extension GL_EXT_YUV_target : require\n"
            "precision mediump float;\n"
            "uniform __samplerExternal2DY2YEXT uSrcYuv;\n"
            "uniform int uTargetPlane;\n"
            "uniform int uNativeWidth;\n"
            "uniform int uNativeHeight;\n"
            "uniform int uSurfaceHeight;\n"
            "uniform int uRectLeft;\n"
            "uniform int uRectTop;\n"
            "out vec4 fragColor;\n"
            "vec3 sampleYuv(int x, int y) {\n"
            "  x = clamp(x, 0, uNativeWidth - 1);\n"
            "  y = clamp(y, 0, uNativeHeight - 1);\n"
            "  vec2 coord = (vec2(float(x) + 0.5, float(y) + 0.5) /\n"
            "      vec2(float(uNativeWidth), float(uNativeHeight)));\n"
            "  return texture(uSrcYuv, coord).xyz;\n"
            "}\n"
            "void main() {\n"
            "  int x = int(floor(gl_FragCoord.x));\n"
            "  int y = int(floor(float(uSurfaceHeight) - gl_FragCoord.y));\n"
            "  int relX = x - uRectLeft;\n"
            "  int relY = y - uRectTop;\n"
            "  int srcX = (uRectLeft / 2 + relX / 2) * 2;\n"
            "  int srcY = (uRectTop / 2 + relY / 2) * 2;\n"
            "  vec3 yuv = sampleYuv(srcX, srcY);\n"
            "  float value = (uTargetPlane == 0) ? yuv.y : yuv.z;\n"
            "  fragColor = vec4(value, 0.0, 0.0, 1.0);\n"
            "}\n";
        static constexpr const char* chromaV2ExternalFragment =
            "#version 300 es\n"
            "#extension GL_OES_EGL_image_external_essl3 : require\n"
            "#extension GL_EXT_YUV_target : require\n"
            "precision mediump float;\n"
            "uniform highp sampler2D uPrev;\n"
            "uniform __samplerExternal2DY2YEXT uSrcYuv;\n"
            "uniform int uTargetPlane;\n"
            "uniform int uSurfaceWidth;\n"
            "uniform int uSurfaceHeight;\n"
            "uniform int uAlignedWidth;\n"
            "uniform int uNativeWidth;\n"
            "uniform int uNativeHeight;\n"
            "uniform int uRectLeft;\n"
            "uniform int uRectTop;\n"
            "out vec4 fragColor;\n"
            "vec3 sampleYuv(int x, int y) {\n"
            "  x = clamp(x, 0, uNativeWidth - 1);\n"
            "  y = clamp(y, 0, uNativeHeight - 1);\n"
            "  vec2 coord = (vec2(float(x) + 0.5, float(y) + 0.5) /\n"
            "      vec2(float(uNativeWidth), float(uNativeHeight)));\n"
            "  return texture(uSrcYuv, coord).xyz;\n"
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
            "    value = sampleYuv(srcX, y).x;\n"
            "  } else if ((relY & 1) == 1 && (relX & 3) == 0) {\n"
            "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
            "    int srcY = uRectTop / 2 + relY / 2;\n"
            "    value = sampleYuv(srcX * 2, srcY * 2).y;\n"
            "  } else if ((relY & 1) == 1 && (relX & 3) == 2) {\n"
            "    int srcX = uRectLeft / 4 + relX / 4 + ((uTargetPlane == 1) ? (uAlignedWidth / 4) : 0);\n"
            "    int srcY = uRectTop / 2 + relY / 2;\n"
            "    value = sampleYuv(srcX * 2, srcY * 2).z;\n"
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
            "float conditionalClip(float candidate, float original) {\n"
            "  float clipped = clamp(candidate, 0.0, 1.0);\n"
            "  return (abs(clipped - original) < (30.0 / 255.0)) ? original : clipped;\n"
            "}\n"
            "void main() {\n"
            "  int x = clamp(int(floor(vTexCoord.x * float(uSurfaceWidth))), 0, uSurfaceWidth - 1);\n"
            "  int y = clamp(int(floor(vTexCoord.y * float(uSurfaceHeight))), 0, uSurfaceHeight - 1);\n"
            "  float yy = fetchPlane(uY, x, y);\n"
            "  float uu = fetchPlane(uU, x, y);\n"
            "  float vv = fetchPlane(uV, x, y);\n"
            "  if ((x & 1) == 0 && (y & 1) == 0 && x + 1 < uSurfaceWidth && y + 1 < uSurfaceHeight) {\n"
            "    float candU = 4.0 * uu - fetchPlane(uU, x + 1, y) - fetchPlane(uU, x, y + 1) - fetchPlane(uU, x + 1, y + 1);\n"
            "    float candV = 4.0 * vv - fetchPlane(uV, x + 1, y) - fetchPlane(uV, x, y + 1) - fetchPlane(uV, x + 1, y + 1);\n"
            "    uu = conditionalClip(candU, uu);\n"
            "    vv = conditionalClip(candV, vv);\n"
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
        if (copyYProgram_ == 0 || lumaUvProgram_ == 0 ||
            chromaV2Program_ == 0 || presentProgram_ == 0) {
            return false;
        }

        if (zeroCopyNativeBufferReady_) {
            copyYExternalProgram_ = LinkProgram(updateVertex, copyYExternalFragment, logs);
            lumaUvExternalProgram_ = LinkProgram(updateVertex, lumaUvExternalFragment, logs);
            chromaV2ExternalProgram_ =
                LinkProgram(updateVertex, chromaV2ExternalFragment, logs);
            if (copyYExternalProgram_ == 0 || lumaUvExternalProgram_ == 0 ||
                chromaV2ExternalProgram_ == 0) {
                logs.push_back(
                    "AVC444 GPU renderer zero-copy shader compile/link failed; falling back to "
                    "mapped-plane source upload path");
                if (copyYExternalProgram_ != 0) {
                    glDeleteProgram(copyYExternalProgram_);
                    copyYExternalProgram_ = 0;
                }
                if (lumaUvExternalProgram_ != 0) {
                    glDeleteProgram(lumaUvExternalProgram_);
                    lumaUvExternalProgram_ = 0;
                }
                if (chromaV2ExternalProgram_ != 0) {
                    glDeleteProgram(chromaV2ExternalProgram_);
                    chromaV2ExternalProgram_ = 0;
                }
                zeroCopyNativeBufferReady_ = false;
            }
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
        if (zeroCopyNativeBufferReady_) {
            glGenTextures(1, &srcExternalTexture_);
        }
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
        if (zeroCopyNativeBufferReady_) {
            glBindTexture(GL_TEXTURE_EXTERNAL_OES, srcExternalTexture_);
            ConfigureTexture(GL_TEXTURE_EXTERNAL_OES);
        }
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR || framebuffer_ == 0 || srcYTexture_ == 0 || srcUVTexture_ == 0 ||
            yTexture_ == 0 || uTexture_ == 0 || vTexture_ == 0 || uScratchTexture_ == 0 ||
            vScratchTexture_ == 0) {
            logs.push_back("AVC444 GPU texture allocation failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        if (zeroCopyNativeBufferReady_ && srcExternalTexture_ == 0) {
            zeroCopyNativeBufferReady_ = false;
            logs.push_back(
                "AVC444 GPU external source texture allocation failed; mapped-plane fallback enabled");
        }
        return true;
    }

    bool UploadSource(const DecodedFrame& frame, std::vector<std::string>& logs)
    {
        if (!frame.PlanesValid()) {
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

    bool UploadPlaneTexture(GLuint texture, const std::vector<uint8_t>& data,
        std::vector<std::string>& logs)
    {
        if (data.size() < static_cast<size_t>(surfaceWidth_) * surfaceHeight_) {
            logs.push_back("AVC444 GPU self-test plane upload rejected: data too small");
            return false;
        }
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, texture);
        ConfigureTexture(GL_TEXTURE_2D);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(surfaceWidth_),
            static_cast<GLsizei>(surfaceHeight_), GL_RED, GL_UNSIGNED_BYTE, data.data());
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU self-test plane upload failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }
        return true;
    }

    bool ReadPlaneTexture(GLuint texture, std::vector<uint8_t>& topLeft,
        std::vector<std::string>& logs)
    {
        std::vector<uint8_t> bottomLeft(static_cast<size_t>(surfaceWidth_) * surfaceHeight_);
        topLeft.assign(bottomLeft.size(), 0);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
        glReadPixels(0, 0, static_cast<GLsizei>(surfaceWidth_),
            static_cast<GLsizei>(surfaceHeight_), GL_RED, GL_UNSIGNED_BYTE, bottomLeft.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        const GLenum error = glGetError();
        if (error != GL_NO_ERROR) {
            logs.push_back("AVC444 GPU self-test plane readback failed glError=" +
                Hex32(static_cast<uint32_t>(error)));
            return false;
        }

        for (uint32_t y = 0; y < surfaceHeight_; ++y) {
            const uint32_t srcY = surfaceHeight_ - 1U - y;
            std::memcpy(topLeft.data() + static_cast<size_t>(y) * surfaceWidth_,
                bottomLeft.data() + static_cast<size_t>(srcY) * surfaceWidth_, surfaceWidth_);
        }
        return true;
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
            glGetUniformLocation(program, "uRectTop"));
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
    GLuint srcExternalTexture_ = 0;
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
    GLuint copyYExternalProgram_ = 0;
    GLuint lumaUvExternalProgram_ = 0;
    GLuint chromaV2ExternalProgram_ = 0;
    GLuint presentProgram_ = 0;
    bool zeroCopyNativeBufferReady_ = false;
    PFNEGLCREATEIMAGEKHRPROC eglCreateImageKHRFn_ = nullptr;
    PFNEGLDESTROYIMAGEKHRPROC eglDestroyImageKHRFn_ = nullptr;
    PFNGLEGLIMAGETARGETTEXTURE2DOESPROC glEGLImageTargetTexture2DOESFn_ = nullptr;
    bool hasLuma_ = false;
    bool hasChroma_ = false;
};

bool RunAvc444V2GpuShaderSelfTest(std::vector<std::string>& logs)
{
    Avc444GpuRenderer renderer;
    constexpr uint32_t width = 64;
    constexpr uint32_t height = 32;
    if (!renderer.Ensure(nullptr, 0, 0, width, height, logs)) {
        logs.push_back("AVC444 GPU compositor AVC444v2 shader readback self-test failed: "
            "renderer init failed");
        return false;
    }
    const bool passed = renderer.RunChromaV2ShaderReadbackSelfTest(logs);
    renderer.Destroy();
    return passed;
}

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
    uint64_t queuedPresents = 0;
    uint64_t failures = 0;
    uint64_t ignoredUpdates = 0;
    uint64_t resyncWaits = 0;
    uint64_t endFrameCallbacks = 0;
    uint64_t endFrameSkipInactive = 0;
    uint64_t endFrameSkipNoPending = 0;
    uint64_t endFrameMismatches = 0;
    uint64_t endFramePresentAttempts = 0;
    uint64_t pendingPresentOverwrites = 0;
    bool requireFullRefresh = false;
    bool pendingPresent = false;
    bool resetDecodersBeforeNextDecode = false;
    uint32_t pendingFrameId = 0;

    void Destroy()
    {
        renderer.Destroy();
        lumaDecoder.Close();
        chromaDecoder.Close();
        sharedParameterSets.clear();
        lumaParameterSets.clear();
        chromaParameterSets.clear();
        queuedPresents = 0;
        presented = 0;
        failures = 0;
        ignoredUpdates = 0;
        resyncWaits = 0;
        endFrameCallbacks = 0;
        endFrameSkipInactive = 0;
        endFrameSkipNoPending = 0;
        endFrameMismatches = 0;
        endFramePresentAttempts = 0;
        pendingPresentOverwrites = 0;
        requireFullRefresh = false;
        pendingPresent = false;
        resetDecodersBeforeNextDecode = false;
        pendingFrameId = 0;
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
        bool rendererStateTouched = false;

        auto ignoreActiveUpdate = [&](const std::string& reason, bool resetRenderer,
                                      bool resetDecoders) {
            ++ignoredUpdates;
            requireFullRefresh = true;
            pendingPresent = false;
            pendingFrameId = 0;
            if (resetRenderer) {
                renderer.Destroy();
                logs.push_back(
                    "AVC444 GPU compositor invalidated composed state; waiting for a full LC=0 "
                    "luma/chroma refresh before presenting again");
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
            if (wasActive) {
                return ignoreActiveUpdate(reason, true, false);
            }
            if (rendererStateTouched) {
                renderer.Destroy();
                logs.push_back(
                    "AVC444 GPU compositor reset warm-up renderer state after failed update; "
                    "FreeRDP native GDI path remains authoritative");
            }
            if (activatedForThisCommand) {
                active = false;
                if (callbacks.startRenderPipeline != nullptr) {
                    callbacks.startRenderPipeline();
                }
                logs.push_back(
                    "AVC444 GPU compositor activation failed before GDI suppression was returned to the "
                    "RDPGFX bridge; preserving FreeRDP native GDI path for this command");
            }
            return false;
        };

        const bool needsLuma = command->LC == 0 || command->LC == 1;
        const bool needsChroma = command->LC == 0 || command->LC == 2;
        DecodedFrame lumaFrame;
        DecodedFrame chromaFrame;
        bool lumaUpdated = false;
        bool chromaUpdated = false;
        bool usedZeroCopy = false;
        bool usedMappedPlanes = false;

        if (resetDecodersBeforeNextDecode) {
            lumaDecoder.Close();
            chromaDecoder.Close();
            resetDecodersBeforeNextDecode = false;
            logs.push_back(
                "AVC444 GPU compositor reset hardware decoders before decode; waiting for cached "
                "SPS/PPS or fresh parameter sets to bootstrap");
        }

        if ((needsLuma || needsChroma) &&
            !renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs)) {
            if (!active) {
                logs.push_back("AVC444 GPU compositor offscreen renderer unavailable before decode; keeping GDI");
                return false;
            }
            rendererStateTouched = true;
            return fail("offscreen renderer init");
        }
        const bool zeroCopyCandidate = renderer.ZeroCopyNativeBufferReady();
        if (zeroCopyCandidate) {
            logs.push_back("AVC444 GPU compositor will try zero-copy native-buffer source sampling for frame=" +
                std::to_string(command->frameId));
        }

        if (needsLuma) {
            PreparedH264Packet packet = PrepareH264Packet(command->stream1.data,
                command->stream1.length, lumaDecoder.Started(), lumaParameterSets,
                sharedParameterSets, "luma", logs);
            if (!lumaDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (active) {
                    return ignoreActiveUpdate("luma decoder missing initial SPS/PPS", true, true);
                }
                logs.push_back("AVC444 GPU compositor waits for luma SPS/PPS before hardware decode; keeping GDI"
                    " nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!lumaDecoder.Ensure(command->width, command->height, "luma", logs)) {
                return fail("luma decoder init");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            const DecodeResult decode = zeroCopyCandidate ?
                lumaDecoder.DecodeNativeBuffer(packet.data, packet.size, pts, lumaFrame, logs) :
                lumaDecoder.Decode(packet.data, packet.size, pts, lumaFrame, logs);
            if (decode != DecodeResult::Decoded) {
                if (!active) {
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
                chromaStream->length, chromaDecoder.Started(), chromaParameterSets,
                sharedParameterSets, "chroma", logs);
            if (!chromaDecoder.Started() && !packet.hadParameterSets &&
                !packet.prependedParameterSets) {
                if (active) {
                    return ignoreActiveUpdate("chroma decoder missing initial SPS/PPS", true, true);
                }
                logs.push_back("AVC444 GPU compositor waits for chroma SPS/PPS before hardware decode; keeping GDI"
                    " nalTypes=" + packet.nalSummary);
                return false;
            }
            if (!chromaDecoder.Ensure(command->width, command->height, "chroma", logs)) {
                return fail("chroma decoder init");
            }
            const int64_t pts = static_cast<int64_t>(++streamPts);
            const DecodeResult decode = zeroCopyCandidate ?
                chromaDecoder.DecodeNativeBuffer(packet.data, packet.size, pts, chromaFrame, logs) :
                chromaDecoder.Decode(packet.data, packet.size, pts, chromaFrame, logs);
            if (decode != DecodeResult::Decoded) {
                if (!active) {
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

        if (requireFullRefresh && !IsFullAvc444Refresh(command)) {
            ++resyncWaits;
            if (resyncWaits <= 8 || (resyncWaits % 120) == 0) {
                logs.push_back(
                    "AVC444 GPU compositor decoded but ignored update while waiting for full refresh: "
                    "frame=" + std::to_string(command->frameId) +
                    " LC=" + std::to_string(command->LC) +
                    " waits=" + std::to_string(resyncWaits) +
                    " stream1First=" + RectText(command->stream1.numRegionRects == 0 ?
                        nullptr : command->stream1.regionRects) +
                    " stream2First=" + RectText(command->stream2.numRegionRects == 0 ?
                        nullptr : command->stream2.regionRects));
            }
            return active;
        }
        if (requireFullRefresh) {
            logs.push_back(
                "AVC444 GPU compositor accepted full LC=0 luma/chroma refresh for renderer resync");
            requireFullRefresh = false;
            renderer.InvalidateComposedState();
        }

        if (needsLuma) {
            bool applied = false;
            if (zeroCopyCandidate &&
                renderer.ApplyLumaNativeBuffer(lumaFrame, command->stream1.regionRects,
                    command->stream1.numRegionRects, logs)) {
                applied = true;
                usedZeroCopy = true;
            } else if (zeroCopyCandidate) {
                logs.push_back("AVC444 GPU compositor zero-copy luma update unavailable; "
                    "trying mapped-plane fallback for the same decoded output");
            }
            if (!applied) {
                if (!lumaFrame.PlanesValid() && !lumaDecoder.MapDecodedFrame(lumaFrame, logs)) {
                    rendererStateTouched = true;
                    if (!active) {
                        logs.push_back("AVC444 GPU compositor luma mapped fallback failed; keeping GDI");
                        renderer.Destroy();
                        return false;
                    }
                    return fail("luma mapped fallback");
                }
                if (!renderer.ApplyLuma(lumaFrame, command->stream1.regionRects,
                        command->stream1.numRegionRects, logs)) {
                    rendererStateTouched = true;
                    if (!active) {
                        logs.push_back("AVC444 GPU compositor luma offscreen update failed; keeping GDI");
                        renderer.Destroy();
                        return false;
                    }
                    return fail("luma shader update");
                }
                usedMappedPlanes = true;
            }
            rendererStateTouched = true;
        }

        if (needsChroma) {
            const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
                command->LC == 0 ? &command->stream2 : &command->stream1;
            bool applied = false;
            if (zeroCopyCandidate &&
                renderer.ApplyChromaV2NativeBuffer(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs)) {
                applied = true;
                usedZeroCopy = true;
            } else if (zeroCopyCandidate) {
                logs.push_back("AVC444 GPU compositor zero-copy chroma-v2 update unavailable; "
                    "trying mapped-plane fallback for the same decoded output");
            }
            if (!applied) {
                if (!chromaFrame.PlanesValid() && !chromaDecoder.MapDecodedFrame(chromaFrame, logs)) {
                    rendererStateTouched = true;
                    if (!active) {
                        logs.push_back("AVC444 GPU compositor chroma mapped fallback failed; keeping GDI");
                        renderer.Destroy();
                        return false;
                    }
                    return fail("chroma mapped fallback");
                }
                if (!renderer.ApplyChromaV2(chromaFrame, chromaStream->regionRects,
                        chromaStream->numRegionRects, logs)) {
                    rendererStateTouched = true;
                    if (!active) {
                        logs.push_back("AVC444 GPU compositor chroma offscreen update failed; keeping GDI");
                        renderer.Destroy();
                        return false;
                    }
                    return fail("chroma-v2 shader update");
                }
                usedMappedPlanes = true;
            }
            rendererStateTouched = true;
        }

        if (!renderer.ReadyToPresent()) {
            logs.push_back("AVC444 GPU compositor warmed " +
                std::string(lumaUpdated ? "luma" : "-") + "/" +
                std::string(chromaUpdated ? "chroma" : "-") +
                " state; waiting for both luma and chroma before suppressing GDI");
            if (active) {
                requireFullRefresh = true;
                ++ignoredUpdates;
                pendingPresent = false;
                pendingFrameId = 0;
                logs.push_back(
                    "AVC444 GPU compositor ignored not-ready composed state while active; "
                    "GDI remains suppressed and renderer will wait for full LC=0 refresh "
                    "ignoredUpdates=" + std::to_string(ignoredUpdates));
                return true;
            }
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
        pendingPresent = true;
        pendingFrameId = command->frameId;
        ++queuedPresents;
        const std::string route = usedZeroCopy && !usedMappedPlanes ?
            "hardware-decode+native-buffer-yuv-target-gpu-combine" :
            (usedZeroCopy ? "hardware-decode+mixed-native-buffer/mapped-plane-gpu-combine" :
                "hardware-decode+mapped-plane-gpu-combine");
        logs.push_back("AVC444 GPU compositor queued EndFrame present: frame=" +
            std::to_string(command->frameId) + " LC=" + std::to_string(command->LC) +
            " queued=" + std::to_string(queuedPresents) +
            " route=" + route);
        return true;
    }

    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame, bool active,
        std::vector<std::string>& logs)
    {
        ++endFrameCallbacks;
        const uint32_t frameId = frame == nullptr ? 0 : frame->frameId;
        const uint32_t activeFrameId = frame == nullptr ? 0 : frame->activeFrameId;
        const bool matchedFrame = frame != nullptr && frame->matchedFrame;

        if (!active || !pendingPresent) {
            if (!active) {
                ++endFrameSkipInactive;
            }
            if (!pendingPresent) {
                ++endFrameSkipNoPending;
            }
            const uint64_t skipCount = !active ? endFrameSkipInactive : endFrameSkipNoPending;
            if (ShouldLogFrequent(skipCount) || ShouldLogFrequent(endFrameCallbacks)) {
                logs.push_back("AVC444 GPU compositor EndFrame callback skipped: "
                    "endFrame=" + std::to_string(frameId) +
                    " activeFrame=" + std::to_string(activeFrameId) +
                    " matched=" + std::string(matchedFrame ? "yes" : "no") +
                    " active=" + std::string(active ? "yes" : "no") +
                    " pending=" + std::string(pendingPresent ? "yes" : "no") +
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

        if (!matchedFrame || frameId != pendingFrameId) {
            ++endFrameMismatches;
            ++ignoredUpdates;
            requireFullRefresh = true;
            pendingPresent = false;
            const uint32_t queuedFrameId = pendingFrameId;
            pendingFrameId = 0;
            renderer.Destroy();
            logs.push_back("AVC444 GPU compositor dropped pending present at EndFrame mismatch: "
                "endFrame=" + std::to_string(frameId) +
                " queuedFrame=" + std::to_string(queuedFrameId) +
                " activeFrame=" +
                std::to_string(frame == nullptr ? 0 : frame->activeFrameId) +
                " matched=" + std::string(matchedFrame ? "yes" : "no") +
                " mismatches=" + std::to_string(endFrameMismatches) +
                " callbacks=" + std::to_string(endFrameCallbacks) +
                "; waiting for full LC=0 refresh ignoredUpdates=" +
                std::to_string(ignoredUpdates) + " " + renderer.DebugState());
            return true;
        }

        ++endFramePresentAttempts;
        if (ShouldLogFrequent(endFramePresentAttempts)) {
            logs.push_back("AVC444 GPU compositor EndFrame present attempt: "
                "endFrame=" + std::to_string(frameId) +
                " pendingFrame=" + std::to_string(pendingFrameId) +
                " attempts=" + std::to_string(endFramePresentAttempts) +
                " callbacks=" + std::to_string(endFrameCallbacks) +
                " queued=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented) +
                " " + renderer.DebugState());
        }

        if (!renderer.Present(logs)) {
            ++failures;
            ++ignoredUpdates;
            requireFullRefresh = true;
            pendingPresent = false;
            pendingFrameId = 0;
            renderer.Destroy();
            logs.push_back("AVC444 GPU compositor EndFrame present failed; ignored update while "
                "preserving GPU ownership and waiting for full LC=0 refresh failures=" +
                std::to_string(failures) + " ignoredUpdates=" +
                std::to_string(ignoredUpdates) + " attempts=" +
                std::to_string(endFramePresentAttempts) + " " + renderer.DebugState());
            return true;
        }

        pendingPresent = false;
        pendingFrameId = 0;
        ++presented;
        logs.push_back("AVC444 GPU compositor presented at EndFrame: frame=" +
            std::to_string(frameId) + " presented=" + std::to_string(presented) +
            " attempts=" + std::to_string(endFramePresentAttempts) +
            " callbacks=" + std::to_string(endFrameCallbacks) +
            " queued=" + std::to_string(queuedPresents));
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
        "avc444 gpu compositor: configured experimental zero-copy compositor with mapped-plane fallback, gdi preserved until first successful present" :
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
                result.eglReady && result.avcodecHardwareReady && result.rawBufferCandidate &&
                result.avc444v2LayoutReady && result.avc444v2ShaderReady;
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

bool Avc444GpuCompositor::OnEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame)
{
    if (frame == nullptr) {
        return false;
    }

    std::vector<std::string> logs;
    bool handled = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        bool active = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return false;
            }
            active = active_;
        }

        handled = impl_ != nullptr && impl_->PresentEndFrame(frame, active, logs);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc444 gpu compositor: enabled=yes selfTest=" +
                std::string(selfTestComplete_ ? "yes" : "no") +
                " active=" + std::string(active_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " endFrame=" + std::to_string(frame->frameId) +
                " endFramePresent=" + std::string(handled ? "handled" : "none");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return handled;
}
#endif

Avc444GpuCompositor::SelfTestResult Avc444GpuCompositor::RunSelfTest(
    uint32_t width, uint32_t height)
{
    SelfTestResult result;
    std::vector<std::string> logs = ProbeEglNativeImage(result);
    std::vector<std::string> avcodecLogs = ProbeAvcCapability(width, height, result);
    logs.insert(logs.end(), avcodecLogs.begin(), avcodecLogs.end());
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    result.avc444v2LayoutReady = RunAvc444V2LayoutSelfTest(logs);
    result.avc444v2ShaderReady =
        result.avc444v2LayoutReady && RunAvc444V2GpuShaderSelfTest(logs);
    result.zeroCopyNativeBufferReady =
        result.zeroCopyNativeBufferReady && !gZeroCopyNativeBufferDisabledBySelfTest;
#endif

    result.diagnostics =
        "avc444 gpu compositor: egl=" + std::string(result.eglReady ? "yes" : "no") +
        " nativeImage=" + std::string(result.nativeImageReady ? "yes" : "no") +
        " avcodecHardware=" + std::string(result.avcodecHardwareReady ? "yes" : "no") +
        " nativeBufferFormats=" + std::string(result.nativeBufferFormatsKnown ? "known" : "unknown") +
        " rawYuvCandidate=" + std::string(result.rawBufferCandidate ? "yes" : "no") +
        " zeroCopyNativeBuffer=" + std::string(result.zeroCopyNativeBufferReady ? "yes" : "no") +
        " avc444v2Layout=" + std::string(result.avc444v2LayoutReady ? "yes" : "no") +
        " avc444v2Shader=" + std::string(result.avc444v2ShaderReady ? "yes" : "no") +
        " readyForSuppressGdi=" +
        std::string((result.eglReady && result.avcodecHardwareReady && result.rawBufferCandidate &&
            result.avc444v2LayoutReady && result.avc444v2ShaderReady) ?
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
