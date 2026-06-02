#pragma once

#include <chrono>
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
#include "surface/render_output_owner.h"

#include <client/OHOS/ohos_rdpgfx.h>

namespace rdp_bridge {

using Avc420GpuLogFn = std::function<void(const std::string&)>;

class Avc420GpuCompositorImpl;

struct Avc420GpuCompositorCallbacks {
    std::function<DecoderSurfaceTarget()> decoderSurfaceTarget;
    std::function<void()> startRenderPipeline;
    std::function<void()> stopRenderPipeline;
    std::function<void(const std::string&)> releaseRenderTarget;
    std::function<void(bool, const std::string&)> setOutputPolicy;
};

enum class Avc420OutputState {
    Detached,
    Active,
    TargetPaused,
    Failed,
};

class Avc420GpuCompositor {
public:
    Avc420GpuCompositor();
    ~Avc420GpuCompositor();
    Avc420GpuCompositor(const Avc420GpuCompositor&) = delete;
    Avc420GpuCompositor& operator=(const Avc420GpuCompositor&) = delete;

    void Configure(bool enabled, Avc420GpuLogFn log,
        Avc420GpuCompositorCallbacks callbacks = {});
    void Reset();
    std::string Diagnostics() const;
    void Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight);
    void SetOutputActive(bool active, const std::string& reason);
    void OnSurfaceTargetChanged(const std::string& reason);

    bool OnGdiFrame(const RgbaFrame& frame);
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
        GdiFrame,
        SurfaceCommand,
        EndFrame,
    };

    struct WorkerTask {
        WorkerTaskType type = WorkerTaskType::Prewarm;
        OwnedAvc420Command command;
        RgbaFrame gdiFrame {};
        FREERDP_OHOS_RDPGFX_FRAME_INFO frame {};
        Avc420GpuCompositorCallbacks callbacks;
        uint32_t prewarmSurfaceWidth = 0;
        uint32_t prewarmSurfaceHeight = 0;
        bool outputActive = false;
    };

    struct WorkerQueueDropCounts {
        uint64_t prewarms = 0;
        uint64_t gdiFrames = 0;
        uint64_t commands = 0;
        uint64_t endFrames = 0;
    };

    struct WorkerQueueCompaction {
        WorkerQueueDropCounts drops;
        size_t depthBefore = 0;
        size_t depthAfter = 0;
        size_t preservedCommands = 0;
        size_t preservedGdiFrames = 0;

        bool DidDrop() const
        {
            return drops.prewarms != 0 || drops.gdiFrames != 0 ||
                drops.commands != 0 || drops.endFrames != 0;
        }
    };

    void Log(const std::string& message) const;
    bool HasReadySurfaceTarget(const Avc420GpuCompositorCallbacks& callbacks) const;
    bool ShouldDetachForTargetChange(const std::string& reason) const;
    void PauseOutputForTargetUnavailable(const std::string& reason,
        const Avc420GpuCompositorCallbacks& callbacks, std::vector<std::string>& logs);
    WorkerQueueDropCounts ClearWorkerQueueLocked();
    WorkerQueueCompaction CompactNonDecoderWorkLocked();
    void AccountDroppedWorkerTasks(const WorkerQueueDropCounts& drops);
    std::string WorkerBacklogText() const;
    bool DetachOutputActive(const std::string& reason,
        const Avc420GpuCompositorCallbacks& callbacks, bool clearQueuedWork,
        RenderOutputOwnerTransitionReason transitionReason, std::vector<std::string>& logs);
    bool EnqueueGdiFrame(const RgbaFrame& frame, bool outputActive);
    bool EnqueueSurfaceCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive);
    bool EnqueueEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive);
    void EnsureWorkerLocked();
    void StopWorker();
    void WorkerLoop();
    void ProcessWorkerTask(WorkerTask task);
    void AppendPeriodicStats(std::vector<std::string>& logs, const std::string& reason);

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
    std::chrono::steady_clock::time_point nextStatsLogAt_ {};
    std::chrono::steady_clock::time_point lastStatsLogAt_ {};
    uint64_t lastStatsCandidates_ = 0;
    uint64_t lastStatsProcessedEndFrames_ = 0;
    Avc420GpuLogFn log_;
    Avc420GpuCompositorCallbacks callbacks_;
    bool outputActive_ = false;
    Avc420OutputState outputState_ = Avc420OutputState::Detached;
    std::unique_ptr<Avc420GpuCompositorImpl> impl_;
    std::atomic<uint64_t> workerQueuedPrewarms_ {0};
    std::atomic<uint64_t> workerQueuedGdiFrames_ {0};
    std::atomic<uint64_t> workerQueuedCommands_ {0};
    std::atomic<uint64_t> workerQueuedEndFrames_ {0};
    std::atomic<uint64_t> workerProcessedPrewarms_ {0};
    std::atomic<uint64_t> workerProcessedGdiFrames_ {0};
    std::atomic<uint64_t> workerProcessedCommands_ {0};
    std::atomic<uint64_t> workerProcessedEndFrames_ {0};
    std::atomic<uint64_t> workerDroppedCommands_ {0};
    std::atomic<uint64_t> workerDroppedEndFrames_ {0};
    std::atomic<uint64_t> workerQueueOverLimit_ {0};
    std::atomic<uint64_t> workerMaxDepth_ {0};
    std::atomic<uint64_t> workerQueueDepth_ {0};
};

Avc420GpuCompositor& SharedAvc420GpuCompositor();

} // namespace rdp_bridge
