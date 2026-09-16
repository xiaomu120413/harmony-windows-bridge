#pragma once

// Internal types shared across the avc444 GPU compositor translation units
// (avc444_gpu_compositor_utils.cpp, avc444_hardware_decoder.cpp,
// avc444_gpu_renderer.cpp, avc444_gpu_readback.cpp,
// avc444_gpu_compositor_state.cpp). These types were promoted from the
// anonymous namespace of the former avc444_gpu_compositor_internal.cpp so
// that multiple .cpp files can reference the same definitions. No rendering
// logic changed — only code location and inline/out-of-line placement.

#include "surface/avc444_gpu_compositor_internal.h"
#include "surface/avc_gpu_common.h"
#include "surface/native_rgba_copy.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_buffer/buffer_common.h>

// Opaque codec/buffer types used as pointer members (full headers included
// only in the .cpp files that call their functions).  These forward
// declarations MUST live in the global namespace so they name the same type
// as the OHOS SDK headers (which declare functions taking ::OH_AVCodec* etc.).
struct OH_AVCodec;
struct OH_AVBuffer;
struct OH_NativeBuffer;

namespace rdp_bridge {

// avc444-specific constants (values differ from avc420, kept per-compositor).
constexpr int64_t kOutputTimeoutUs = 12000;
constexpr int64_t kFollowupOutputTimeoutUs = 6000;
constexpr int64_t kOutputSyncDeadlineUs = 120000;
constexpr uint32_t kOutputSyncMaxAttempts = 32;
constexpr const char* kGpuReadbackEnv = "FREERDP_BRIDGE_GPU_READBACK";

enum class DecodeResult {
    Decoded,
    NoOutput,
    Failed,
};

struct PlaneView {
    const uint8_t* data = nullptr;
    uint32_t rowStride = 0;
    uint32_t columnStride = 0;
};

// DecodedFrame — decoded output frame holding codec/buffer/native handles with
// mapped plane views for the mapped-plane shader path.
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
    DecodedFrame(DecodedFrame&& other) noexcept;
    DecodedFrame& operator=(DecodedFrame&& other) noexcept;
    ~DecodedFrame();

    bool Valid() const
    {
        return codec != nullptr && buffer != nullptr && PlanesValid();
    }

    bool PlanesValid() const
    {
        return y.data != nullptr && uv.data != nullptr && width > 0 && height > 0 &&
            y.rowStride > 0 && uv.rowStride > 0 && uv.columnStride == 2;
    }

    void Release();

private:
    void MoveFrom(DecodedFrame& other);
};

// Shared free-function declarations (defined in avc444_gpu_compositor_utils.cpp).
bool IsGpuReadbackEnabled();
std::string CodecRoleLogPrefix(const std::string& role);
uint32_t AlignUp(uint32_t value, uint32_t align);
std::string NativeBufferFormatName(OH_NativeBuffer_Format format);
bool IsValidLcForCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command);
bool RectsValid(const RECTANGLE_16* rects, uint32_t count, uint32_t width, uint32_t height);
bool RectsCoverFullSurface(const RECTANGLE_16* rects, uint32_t count, uint32_t width,
    uint32_t height);
uint32_t RequiredChromaV1SourceYHeight(const RECTANGLE_16* rects, uint32_t count);
std::string FormatRectText(const RECTANGLE_16* rect);
std::string RectsText(const RECTANGLE_16* rects, uint32_t count);
std::string StreamText(const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO& stream);
std::string FramePlaneText(const DecodedFrame& frame);

// PreparedH264Packet — avc444 dual-cache-role variant of the H264 packet prep.
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
    const std::string& role, std::vector<std::string>& logs);

// Avc444HardwareDecoder — hardware H.264 decoder using OH_VideoDecoder sync mode
// with bounded synchronous output wait.
class Avc444HardwareDecoder {
public:
    ~Avc444HardwareDecoder();
    void Close();
    bool Ensure(uint32_t width, uint32_t height, const std::string& role,
        std::vector<std::string>& logs);
    bool Started() const;
    DecodeResult Decode(const uint8_t* data, uint32_t size, int64_t pts, DecodedFrame& frame,
        std::vector<std::string>& logs);
    bool MapDecodedFrame(DecodedFrame& frame, std::vector<std::string>& logs);

private:
    std::string LogPrefix() const;
    bool ConfigureWithPixelFormat(int32_t pixelFormat, std::vector<std::string>& logs);
    void PushEmptyInput(OH_AVBuffer* input, uint32_t inputIndex);
    void UpdateOutputDescription(std::vector<std::string>& logs, const std::string& reason);
    bool MapOutput(DecodedFrame& frame, std::vector<std::string>& logs);
    bool FinishPlaneLayout(DecodedFrame& frame, std::vector<std::string>& logs,
        const std::string& source);

    OH_AVCodec* decoder_ = nullptr;
    bool started_ = false;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    int32_t pixelFormat_ = 0;
    uint32_t outputStride_ = 0;
    uint32_t outputSliceHeight_ = 0;
    int32_t outputPixelFormat_ = 0;
    uint64_t pushed_ = 0;
    uint64_t outputs_ = 0;
    uint64_t noOutput_ = 0;
    std::string role_;
};

// Avc444GpuRenderer — EGL/GLES3 renderer that uploads mapped Y/UV planes to
// separate R8/RG8 textures and composites them via 5 shader programs
// (copyY, lumaUv, chromaV1, chromaV2, present).
class Avc444GpuRenderer {
public:
    ~Avc444GpuRenderer();
    void Destroy();
    bool Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
        uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs);
    bool ReadyToPresent() const;
    std::string DebugState() const;
    bool HasWindowTarget() const;
    void InvalidateComposedState();
    bool ApplyLuma(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs);
    bool ApplyChromaV1(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs);
    bool ApplyChromaV2(const DecodedFrame& frame, const RECTANGLE_16* rects, uint32_t rectCount,
        std::vector<std::string>& logs);
    bool Present(std::vector<std::string>& logs, bool logSuccess);

private:
    struct PixelSample {
        const char* name = "";
        uint32_t x = 0;
        uint32_t y = 0;
        std::array<uint8_t, 4> rgba {};
    };

    static std::string PixelText(const PixelSample& sample);
    std::string SampleFramebuffer(const RenderViewport& viewport) const;
    bool MakeCurrent(std::vector<std::string>& logs);
    void DeleteTextures();
    bool CreatePrograms(std::vector<std::string>& logs);
    bool CreateTextures(uint32_t surfaceWidth, uint32_t surfaceHeight,
        std::vector<std::string>& logs);
    bool UploadSource(const DecodedFrame& frame, std::vector<std::string>& logs);
    void DrawRectsToTexture(GLuint texture, const RECTANGLE_16* rects, uint32_t rectCount,
        GLint rectLeftLocation = -1, GLint rectTopLocation = -1,
        GLint rectRightLocation = -1, GLint rectBottomLocation = -1);
    void CopyPlaneTexture(GLuint source, GLuint target);
    void PingPongChromaPlane(bool uPlane, const RECTANGLE_16* rects, uint32_t rectCount,
        GLuint program);
    static void ConfigureTexture(GLenum target);

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
    std::string lastFramebufferSample_ = "readback:disabled";
};

// Avc444GpuCompositorImpl::State — pimpl state holding decoder, renderer, and
// all compositor orchestration logic.
struct Avc444GpuCompositorImpl::State {
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
    uint64_t prewarms = 0;
    uint64_t prewarmFailures = 0;
    bool pendingPresent = false;
    bool resetDecodersBeforeNextDecode = false;
    uint32_t pendingFrameId = 0;
    uint32_t pendingSurfaceWidth = 0;
    uint32_t pendingSurfaceHeight = 0;
    uint64_t processedCommands = 0;
    uint64_t lastSampledProcessStartUs = 0;
    TimingBucket commandTiming;
    TimingBucket commandIntervalTiming;
    TimingBucket offscreenEnsureTiming;
    TimingBucket lumaDecodeTiming;
    TimingBucket chromaDecodeTiming;
    TimingBucket lumaApplyTiming;
    TimingBucket chromaApplyTiming;
    TimingBucket windowEnsureTiming;
    TimingBucket presentTiming;

    void Destroy();
    bool Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs);
    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
        const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool PresentQueuedUpdate(const std::string& trigger, uint32_t frameId,
        uint32_t activeFrameId, bool matchedFrame,
        const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    std::string DebugSummary() const;
};

} // namespace rdp_bridge
