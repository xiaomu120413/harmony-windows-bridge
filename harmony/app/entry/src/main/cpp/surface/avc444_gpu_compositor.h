#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>

#include "common/bridge_types.h"

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <client/OHOS/ohos_rdpgfx.h>
#endif

namespace rdp_bridge {

using Avc444GpuLogFn = std::function<void(const std::string&)>;

struct Avc444GpuCompositorCallbacks {
    std::function<DecoderSurfaceTarget()> decoderSurfaceTarget;
    std::function<void()> stopRenderPipeline;
    std::function<void()> startRenderPipeline;
    std::function<void(const std::string&)> releaseRenderTarget;
};

class Avc444GpuCompositor {
public:
    Avc444GpuCompositor();
    ~Avc444GpuCompositor();
    Avc444GpuCompositor(const Avc444GpuCompositor&) = delete;
    Avc444GpuCompositor& operator=(const Avc444GpuCompositor&) = delete;

    void Configure(bool enabled, Avc444GpuLogFn log,
        Avc444GpuCompositorCallbacks callbacks = {});
    void Reset();
    std::string Diagnostics() const;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    bool OnSurfaceCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command);
    bool OnEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame);
#endif

private:
    void Log(const std::string& message) const;

    mutable std::mutex processingMutex_;
    mutable std::mutex mutex_;
    bool enabled_ = false;
    uint64_t candidates_ = 0;
    uint64_t invalidLcRejects_ = 0;
    uint32_t lastFrameId_ = 0;
    uint32_t lastLC_ = 0;
    uint32_t lastStream1Bytes_ = 0;
    uint32_t lastStream2Bytes_ = 0;
    uint32_t lastTargetWidth_ = 0;
    uint32_t lastTargetHeight_ = 0;
    std::string diagnostics_;
    Avc444GpuLogFn log_;
    Avc444GpuCompositorCallbacks callbacks_;
    bool active_ = false;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

Avc444GpuCompositor& SharedAvc444GpuCompositor();

} // namespace rdp_bridge
