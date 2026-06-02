#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/bridge_types.h"
#include "surface/avc444_gpu_compositor.h"

#include <client/OHOS/ohos_rdpgfx.h>

namespace rdp_bridge {

using Avc420GpuLogFn = std::function<void(const std::string&)>;

class Avc420GpuCompositorImpl;

class Avc420GpuCompositor {
public:
    Avc420GpuCompositor();
    ~Avc420GpuCompositor();
    Avc420GpuCompositor(const Avc420GpuCompositor&) = delete;
    Avc420GpuCompositor& operator=(const Avc420GpuCompositor&) = delete;

    void Configure(bool enabled, Avc420GpuLogFn log,
        Avc444GpuCompositorCallbacks callbacks = {});
    void Reset();
    std::string Diagnostics() const;
    void Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight);

    bool OnSurfaceCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command);
    bool OnEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame);

private:
    struct OwnedAvc420Command {
        FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO info {};
        std::vector<uint8_t> data;
        std::vector<RECTANGLE_16> rects;
    };

    enum class WorkerTaskType {
        Prewarm,
        SurfaceCommand,
        EndFrame,
    };

    struct WorkerTask {
        WorkerTaskType type = WorkerTaskType::Prewarm;
        OwnedAvc420Command command;
        FREERDP_OHOS_RDPGFX_FRAME_INFO frame {};
        Avc444GpuCompositorCallbacks callbacks;
        uint32_t prewarmSurfaceWidth = 0;
        uint32_t prewarmSurfaceHeight = 0;
        bool outputActive = false;
    };

    void Log(const std::string& message) const;
    bool EnqueueSurfaceCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
        const Avc444GpuCompositorCallbacks& callbacks, bool outputActive);
    bool EnqueueEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc444GpuCompositorCallbacks& callbacks, bool outputActive);
    void EnsureWorkerLocked();
    void StopWorker();
    void WorkerLoop();
    void ProcessWorkerTask(WorkerTask task);

    mutable std::mutex processingMutex_;
    mutable std::mutex mutex_;
    std::mutex workerMutex_;
    std::condition_variable workerCondition_;
    std::deque<WorkerTask> workerQueue_;
    std::thread worker_;
    bool workerRunning_ = false;
    bool enabled_ = false;
    uint64_t candidates_ = 0;
    uint32_t lastFrameId_ = 0;
    uint32_t lastBytes_ = 0;
    uint32_t lastRects_ = 0;
    uint32_t lastTargetWidth_ = 0;
    uint32_t lastTargetHeight_ = 0;
    bool lastFullSurface_ = false;
    std::string diagnostics_;
    mutable std::string implDiagnosticsCache_;
    Avc420GpuLogFn log_;
    Avc444GpuCompositorCallbacks callbacks_;
    bool outputActive_ = false;
    std::unique_ptr<Avc420GpuCompositorImpl> impl_;
    std::atomic<uint64_t> workerQueuedPrewarms_ {0};
    std::atomic<uint64_t> workerQueuedCommands_ {0};
    std::atomic<uint64_t> workerQueuedEndFrames_ {0};
    std::atomic<uint64_t> workerProcessedPrewarms_ {0};
    std::atomic<uint64_t> workerProcessedCommands_ {0};
    std::atomic<uint64_t> workerProcessedEndFrames_ {0};
    std::atomic<uint64_t> workerDroppedEndFrames_ {0};
    std::atomic<uint64_t> workerQueueOverLimit_ {0};
    std::atomic<uint64_t> workerMaxDepth_ {0};
    std::atomic<uint64_t> workerQueueDepth_ {0};
};

Avc420GpuCompositor& SharedAvc420GpuCompositor();

} // namespace rdp_bridge
