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

class Avc444GpuCompositorImpl;

struct Avc444GpuCompositorCallbacks {
    std::function<DecoderSurfaceTarget()> decoderSurfaceTarget;
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
    void SetOutputActive(bool active, const std::string& reason);

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
    bool outputActive_ = false;
    std::unique_ptr<Avc444GpuCompositorImpl> impl_;
};

Avc444GpuCompositor& SharedAvc444GpuCompositor();

} // namespace rdp_bridge
