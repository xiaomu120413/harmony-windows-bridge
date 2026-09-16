#pragma once

// Internal types shared across the avc420 GPU compositor translation units
// (avc420_gpu_compositor_utils.cpp, avc420_hardware_decoder.cpp,
// avc420_native_buffer_renderer.cpp, avc420_gpu_compositor_state.cpp,
// avc420_gpu_compositor_impl.cpp). These types were promoted from the
// anonymous namespace of the former avc420_gpu_compositor_internal.cpp so
// that multiple .cpp files can reference the same definitions. No rendering
// logic changed — only code location and inline/out-of-line placement.

#include "surface/avc420_gpu_compositor_internal.h"
#include "surface/avc_gpu_common.h"
#include "surface/gpu_rgba_renderer.h"

#include <cstdint>
#include <string>
#include <vector>

#include <EGL/egl.h>
#include <EGL/eglext.h>
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

// avc420-specific constants (values differ from avc444, kept per-compositor).
constexpr int64_t kOutputTimeoutUs = 8000;
constexpr int64_t kFollowupOutputTimeoutUs = 4000;
constexpr int64_t kOutputSyncDeadlineUs = 32000;
constexpr uint32_t kOutputSyncMaxAttempts = 8;
constexpr uint64_t kActiveFailureFallbackThreshold = 3U;
constexpr uint64_t kActiveResetIgnoreFallbackThreshold = 6U;
constexpr double kAvc420DecoderFrameRate = 60.0;

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

#ifndef EGL_NATIVE_BUFFER_OHOS
#define EGL_NATIVE_BUFFER_OHOS 0x34E1
#endif

enum class DecodeResult {
    Decoded,
    NoOutput,
    Failed,
};

enum class ActiveAvc420UpdatePolicy {
    PreserveOwner,
    ResetDecoderAndPreserveOwner,
    ReleaseOwner,
};

// NativeDecodedFrame — decoded output frame holding codec/buffer/native handles.
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

    NativeDecodedFrame();
    NativeDecodedFrame(const NativeDecodedFrame&) = delete;
    NativeDecodedFrame& operator=(const NativeDecodedFrame&) = delete;
    NativeDecodedFrame(NativeDecodedFrame&& other) noexcept;
    NativeDecodedFrame& operator=(NativeDecodedFrame&& other) noexcept;
    ~NativeDecodedFrame();

    bool Valid() const
    {
        return codec != nullptr && buffer != nullptr && nativeBuffer != nullptr &&
            hasOutputIndex && width > 0 && height > 0;
    }

    void Release();

private:
    void MoveFrom(NativeDecodedFrame& other);
};

// Shared free-function declarations (defined in avc420_gpu_compositor_utils.cpp).
const char* ActiveAvc420UpdatePolicyName(ActiveAvc420UpdatePolicy policy);
RenderViewport FitAvc420PresentViewport(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t sourceWidth, uint32_t sourceHeight, bool& snapped);
std::string FormatFixed(double value, int precision);
std::string FormatMs(uint64_t valueUs);
int64_t MakeDecoderPts(uint32_t frameId, uint64_t sequence);
std::string NativeBufferFormatName(OH_NativeBuffer_Format format);
std::string NativeFrameText(const NativeDecodedFrame& frame);

// Avc420HardwareDecoder — hardware H.264 decoder using OH_VideoDecoder sync mode.
class Avc420HardwareDecoder {
public:
    ~Avc420HardwareDecoder();
    void Close();
    bool Started() const;
    bool Ensure(uint32_t width, uint32_t height, std::vector<std::string>& logs);
    DecodeResult Decode(const uint8_t* data, uint32_t size, int64_t pts,
        NativeDecodedFrame& frame, std::vector<std::string>& logs);

private:
    bool ConfigureWithPixelFormat(int32_t pixelFormat, std::vector<std::string>& logs);
    void PushEmptyInput(OH_AVBuffer* input, uint32_t inputIndex);
    void UpdateOutputDescription(std::vector<std::string>& logs, const std::string& reason);
    bool AttachNativeOutput(NativeDecodedFrame& frame, std::vector<std::string>& logs);

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

// Avc420NativeBufferRenderer — EGL/GLES renderer that imports OH_NativeBuffer
// via EGLImage into GL_TEXTURE_EXTERNAL_OES and composites to a retained FBO.
class Avc420NativeBufferRenderer {
public:
    ~Avc420NativeBufferRenderer();
    void Destroy();
    bool Ensure(OHNativeWindow* window, uint32_t targetWidth, uint32_t targetHeight,
        uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs);
    bool CompositeFrame(const NativeDecodedFrame& frame, const RECTANGLE_16* rects,
        uint32_t rectCount, std::vector<std::string>& logs, bool logSuccess);
    bool CompositeRgbaFrame(const RgbaFrame& frame, std::vector<std::string>& logs,
        bool logSuccess);
    bool PresentComposite(std::vector<std::string>& logs, bool logSuccess);
    void DetachWindowSurface(const std::string& reason, std::vector<std::string>& logs);
    std::string DebugState() const;

private:
    struct ImportedTexture {
        OHNativeWindowBuffer* windowBuffer = nullptr;
        EGLImageKHR image = EGL_NO_IMAGE_KHR;
        GLuint texture = 0;
    };

    EGLSurface CreatePbufferSurface(std::vector<std::string>& logs);
    EGLSurface CreateWindowSurface(OHNativeWindow* window, std::vector<std::string>& logs);
    void DestroyWindowSurface();
    void DeleteCompositeSurface();
    bool EnsureCompositeSurface(std::vector<std::string>& logs);
    void DrawExternalRectToComposite(const RECTANGLE_16& rect,
        GLfloat nativeWidth, GLfloat nativeHeight);
    bool EnsureInitialized(std::vector<std::string>& logs);
    bool CreateProgram(std::vector<std::string>& logs);
    static void ConfigureTexture(GLenum target);
    bool MakeCurrent(EGLSurface surface, const char* label, std::vector<std::string>& logs);
    bool MakePbufferCurrent(std::vector<std::string>& logs);
    bool MakeWindowCurrent(std::vector<std::string>& logs);
    bool ImportFrame(const NativeDecodedFrame& frame, ImportedTexture& imported,
        std::vector<std::string>& logs);
    void ReleaseImport(ImportedTexture& imported);

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

// Avc420GpuCompositorImpl::State — pimpl state holding decoder, renderer, and
// all compositor orchestration logic.
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

    void Destroy();
    void RecordCommandGap(uint64_t nowUs);
    void RecordEndFrameGap(uint64_t nowUs);
    void RecordPresentGap(uint64_t nowUs);
    void ClearPendingPresent();
    void OnSurfaceTargetChanged(const std::string& reason,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs);
    bool ProcessGdiFrame(const RgbaFrame& frame, bool outputActive,
        std::vector<std::string>& logs);
    bool PresentGdiBackgroundNow(const std::string& trigger,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
        const Avc420GpuCompositorCallbacks&, bool outputActive, std::vector<std::string>& logs);
    bool PresentQueuedUpdate(const std::string& trigger, uint32_t frameId,
        uint32_t activeFrameId, bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks,
        bool outputActive, std::vector<std::string>& logs);
    bool PresentGdiBackgroundAtEndFrame(uint32_t frameId, uint32_t activeFrameId,
        bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    std::string DebugSummary() const;
    std::string StatsSummary();
};

} // namespace rdp_bridge
