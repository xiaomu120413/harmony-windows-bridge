#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <native_window/external_window.h>

namespace rdp_bridge {

struct ConnectParams {
    std::string host;
    std::string port;
    std::string username;
    std::string password;
    std::string resolution = "auto";
    std::string certPolicy;
    std::string graphicsMode = "rdpgfx-h264";
    std::string appFilesDir;
};

struct SurfacePaintResult {
    bool ok = false;
    bool partial = false;
    std::string message;
    std::vector<std::string> logs;
};

struct DirtyFrameStats {
    bool valid = false;
    uint32_t rectCount = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t areaPermille = 0;
};

enum class FramePixelFormat {
    Rgba,
    Bgra,
};

struct RgbaFrame {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t strideBytes = 0;
    std::string label;
    DirtyFrameStats dirty;
    uint64_t sequence = 0;
    uint64_t dirtySequenceStart = 0;
    FramePixelFormat pixelFormat = FramePixelFormat::Rgba;
};

struct RenderStatsSnapshot {
    bool running = false;
    uint64_t queued = 0;
    uint64_t rendered = 0;
    uint64_t failed = 0;
    uint64_t replaced = 0;
    uint64_t throttled = 0;
    uint64_t fullRendered = 0;
    uint64_t partialRendered = 0;
    uint32_t pending = 0;
    uint32_t lastWidth = 0;
    uint32_t lastHeight = 0;
    uint32_t lastCopyUs = 0;
    uint32_t lastRenderUs = 0;
    uint32_t avgCopyUs = 0;
    uint32_t avgRenderUs = 0;
    uint32_t fpsX100 = 0;
    DirtyFrameStats lastDirty;
    uint32_t targetFrameIntervalMs = 0;
};

struct GraphicsPipelineConfig {
    bool valid = true;
    bool enabled = false;
    bool h264 = false;
    bool avc444GpuExperimental = false;
    std::string mode = "gdi";
};

enum class LocalPointerAction {
    Move,
    ButtonDown,
    ButtonUp,
    WheelVertical,
    WheelHorizontal,
};

enum LocalPointerButton : uint32_t {
    LocalPointerButtonNone = 0,
    LocalPointerButtonLeft = 1U << 0U,
    LocalPointerButtonRight = 1U << 1U,
    LocalPointerButtonMiddle = 1U << 2U,
};

struct LocalPointerEvent {
    LocalPointerAction action = LocalPointerAction::Move;
    uint32_t buttons = LocalPointerButtonNone;
    uint32_t x = 0;
    uint32_t y = 0;
    int32_t delta = 0;
    bool allowClamp = false;
};

struct DecoderSurfaceTarget {
    OHNativeWindow* window = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct RdpSessionRunResult {
    bool available = false;
    bool connected = false;
    bool cancelled = false;
    bool failed = false;
    std::string message;
};

} // namespace rdp_bridge
