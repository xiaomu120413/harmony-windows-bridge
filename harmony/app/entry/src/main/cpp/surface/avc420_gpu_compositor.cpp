#include "surface/avc420_gpu_compositor.h"

#include "surface/avc420_gpu_compositor_internal.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <initializer_list>
#include <mutex>
#include <string>
#include <utility>

namespace rdp_bridge {
namespace {
constexpr size_t kMaxWorkerTasks = 720;
constexpr size_t kEndFrameCoalesceDepth = 24;
constexpr auto kStatsLogInterval = std::chrono::seconds(2);
constexpr uint64_t kBackgroundSeedMaxAgeMs = 500;

bool ShouldLogWorkerCounter(uint64_t count)
{
    return count == 1 || (count % 600) == 0;
}

std::string RatePerSecondText(uint64_t delta, uint64_t elapsedMs)
{
    if (elapsedMs == 0) {
        return "0.00";
    }
    const uint64_t rate100 = (delta * 100000ULL) / elapsedMs;
    const uint64_t whole = rate100 / 100ULL;
    const uint64_t fraction = rate100 % 100ULL;
    return std::to_string(whole) + "." + (fraction < 10 ? "0" : "") +
        std::to_string(fraction);
}

bool ContainsAny(const std::string& value, std::initializer_list<const char*> needles)
{
    for (const char* needle : needles) {
        if (needle != nullptr && value.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool CommandCoversFullSurface(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command)
{
    if (command == nullptr || command->width == 0 || command->height == 0) {
        return false;
    }
    if (command->fullSurface) {
        return true;
    }
    const RECTANGLE_16* rects = command->stream.regionRects;
    for (uint32_t index = 0; rects != nullptr && index < command->stream.numRegionRects; ++index) {
        const RECTANGLE_16& rect = rects[index];
        if (rect.left == 0 && rect.top == 0 &&
            rect.right >= command->width && rect.bottom >= command->height) {
            return true;
        }
    }
    return false;
}

bool DirtyCoversFullFrame(const DirtyFrameStats& dirty, uint32_t width, uint32_t height)
{
    return dirty.valid && width != 0 && height != 0 &&
        dirty.x == 0 && dirty.y == 0 &&
        dirty.width >= width && dirty.height >= height;
}

DirtyFrameStats FullDirtyStats(uint32_t width, uint32_t height)
{
    DirtyFrameStats dirty;
    if (width == 0 || height == 0) {
        return dirty;
    }
    dirty.valid = true;
    dirty.rectCount = 1;
    dirty.x = 0;
    dirty.y = 0;
    dirty.width = width;
    dirty.height = height;
    dirty.areaPermille = 1000;
    return dirty;
}

const char* OutputStateName(Avc420OutputState state)
{
    switch (state) {
        case Avc420OutputState::Active:
            return "active";
        case Avc420OutputState::TargetPaused:
            return "targetPaused";
        case Avc420OutputState::Failed:
            return "failed";
        case Avc420OutputState::Detached:
        default:
            return "detached";
    }
}

bool IsRoutineAvc420GpuLog(const std::string& message)
{
    const bool isAvc420GpuLog =
        message.rfind("AVC420 GPU", 0) == 0 ||
        message.rfind("AVC420 native-buffer GPU", 0) == 0;
    if (!isAvc420GpuLog) {
        return false;
    }
    if (message.find("stats") != std::string::npos) {
        return false;
    }
    if (message.find("output policy") != std::string::npos ||
        message.find("released output") != std::string::npos) {
        return false;
    }
    if (ContainsAny(message, {
        "active update policy", "native import fallback", "fallback before takeover"
    })) {
        return false;
    }
    if (message.find("configuredFrameRate") != std::string::npos) {
        return false;
    }
    if (ContainsAny(message, {
        "failed", "rejected", "invalid", "unsupported", "timeout", "timed out",
        "unavailable", "mismatch", "overwriting", "error", "warning"
    })) {
        return false;
    }
    if (message.find("dropped") != std::string::npos &&
        message.find("droppedOldEndFrames=0") == std::string::npos) {
        return false;
    }
    return true;
}
}

Avc420GpuCompositor::Avc420GpuCompositor() : impl_(std::make_unique<Avc420GpuCompositorImpl>()) {}

Avc420GpuCompositor::~Avc420GpuCompositor()
{
    StopWorker();
}

void Avc420GpuCompositor::Configure(bool enabled, Avc420GpuLogFn log,
    Avc420GpuCompositorCallbacks callbacks)
{
    StopWorker();
    std::lock_guard<std::mutex> processLock(processingMutex_);
    if (impl_) {
        impl_->Destroy();
    }
    if (CurrentRenderOutputOwner() == RenderOutputOwner::Avc420Gpu) {
        TransitionRenderOutputOwner(
            RenderOutputOwner::Gdi,
            RenderOutputOwnerTransitionReason::Avc420ConfigureReset);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    log_ = std::move(log);
    callbacks_ = std::move(callbacks);
    outputActive_ = false;
    outputState_ = Avc420OutputState::Detached;
    candidates_ = 0;
    lastFrameId_ = 0;
    lastBytes_ = 0;
    lastRects_ = 0;
    lastTargetWidth_ = 0;
    lastTargetHeight_ = 0;
    lastFullSurface_ = false;
    backgroundSeeded_ = false;
    backgroundSeedWidth_ = 0;
    backgroundSeedHeight_ = 0;
    implDiagnosticsCache_.clear();
    nextStatsLogAt_ = {};
    lastStatsLogAt_ = {};
    lastAvc420CommandAt_ = {};
    lastStatsCandidates_ = 0;
    lastStatsProcessedEndFrames_ = 0;
    workerQueuedPrewarms_.store(0);
    workerQueuedCommands_.store(0);
    workerQueuedEndFrames_.store(0);
    workerProcessedPrewarms_.store(0);
    workerProcessedCommands_.store(0);
    workerProcessedEndFrames_.store(0);
    workerDroppedCommands_.store(0);
    workerDroppedEndFrames_.store(0);
    workerQueueOverLimit_.store(0);
    workerMaxDepth_.store(0);
    workerQueueDepth_.store(0);
    diagnostics_ = enabled ?
        "avc420 gpu compositor: configured hardware-decode native-buffer compositor" :
        "avc420 gpu compositor: off";
}

void Avc420GpuCompositor::Reset()
{
    Configure(false, nullptr, {});
}

std::string Avc420GpuCompositor::Diagnostics() const
{
    {
        std::unique_lock<std::mutex> processLock(processingMutex_, std::try_to_lock);
        if (processLock.owns_lock() && impl_ != nullptr) {
            const std::string summary = impl_->DebugSummary();
            std::lock_guard<std::mutex> lock(mutex_);
            implDiagnosticsCache_ = summary;
        }
    }

    std::string diagnostics;
    std::string implDiagnostics;
    uint32_t lastTargetWidth = 0;
    uint32_t lastTargetHeight = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics = diagnostics_;
        implDiagnostics = implDiagnosticsCache_;
        lastTargetWidth = lastTargetWidth_;
        lastTargetHeight = lastTargetHeight_;
    }
    if (!implDiagnostics.empty()) {
        diagnostics += " | ";
        diagnostics += implDiagnostics;
    }
    const uint64_t workerDepth = workerQueueDepth_.load();
    const uint64_t workerQueuedPrewarms = workerQueuedPrewarms_.load();
    const uint64_t workerQueuedCommands = workerQueuedCommands_.load();
    const uint64_t workerQueuedEndFrames = workerQueuedEndFrames_.load();
    const uint64_t workerProcessedPrewarms = workerProcessedPrewarms_.load();
    const uint64_t workerProcessedCommands = workerProcessedCommands_.load();
    const uint64_t workerProcessedEndFrames = workerProcessedEndFrames_.load();
    if (workerQueuedPrewarms != 0 || workerQueuedCommands != 0 ||
        workerQueuedEndFrames != 0 || workerProcessedPrewarms != 0 ||
        workerProcessedCommands != 0 ||
        workerProcessedEndFrames != 0) {
        diagnostics += " | gpuWorker=depth:" + std::to_string(workerDepth) +
            ",queued:" + std::to_string(workerQueuedPrewarms) + "/" +
            std::to_string(workerQueuedCommands) + "/" +
            std::to_string(workerQueuedEndFrames) +
            ",processed:" + std::to_string(workerProcessedPrewarms) + "/" +
            std::to_string(workerProcessedCommands) + "/" +
            std::to_string(workerProcessedEndFrames) +
            ",droppedCmd:" + std::to_string(workerDroppedCommands_.load()) +
            ",droppedEnd:" + std::to_string(workerDroppedEndFrames_.load()) +
            ",queueOverLimit:" + std::to_string(workerQueueOverLimit_.load()) +
            ",maxDepth:" + std::to_string(workerMaxDepth_.load());
    }
    if (lastTargetWidth != 0 || lastTargetHeight != 0) {
        diagnostics += " | callbackTarget=cached,size:" + std::to_string(lastTargetWidth) +
            "x" + std::to_string(lastTargetHeight);
    }
    return diagnostics;
}

void Avc420GpuCompositor::Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight)
{
    if (surfaceWidth == 0 || surfaceHeight == 0) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return;
        }
    }

    WorkerTask task;
    task.type = WorkerTaskType::Prewarm;
    task.prewarmSurfaceWidth = surfaceWidth;
    task.prewarmSurfaceHeight = surfaceHeight;

    uint64_t depth = 0;
    uint64_t queued = 0;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (!workerRunning_) {
            EnsureWorkerLocked();
        }
        if (!workerRunning_) {
            ++workerQueueOverLimit_;
            return;
        }
        if (workerQueue_.size() >= kMaxWorkerTasks) {
            ++workerQueueOverLimit_;
            return;
        }
        workerQueue_.push_back(std::move(task));
        depth = workerQueue_.size();
        workerQueueDepth_.store(depth);
        workerMaxDepth_.store(std::max(workerMaxDepth_.load(), depth));
        queued = ++workerQueuedPrewarms_;
    }
    workerCondition_.notify_one();
    if (ShouldLogWorkerCounter(queued)) {
        Log("AVC420 GPU worker queued prewarm: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " surface=" + std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight));
    }
}

void Avc420GpuCompositor::SetOutputActive(bool active, const std::string& reason)
{
    std::vector<std::string> logs;
    Avc420GpuCompositorCallbacks callbacks;
    bool changed = false;
    bool startRenderPipeline = false;
    RenderOutputOwnerTransition ownerTransition;
    WorkerQueueDropCounts drops;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return;
            }
            changed = outputActive_ != active;
            outputActive_ = active;
            outputState_ = active ? Avc420OutputState::Active : Avc420OutputState::Detached;
            if (!active) {
                backgroundSeeded_ = false;
                backgroundSeedWidth_ = 0;
                backgroundSeedHeight_ = 0;
            }
            callbacks = callbacks_;
        }
        if (!active) {
            if (impl_ != nullptr) {
                impl_->Destroy();
            }
            ownerTransition = TransitionRenderOutputOwner(
                RenderOutputOwner::Gdi,
                RenderOutputOwnerTransitionReason::Avc420FreeRdpPolicyRelease);
            {
                std::lock_guard<std::mutex> lock(workerMutex_);
                drops = ClearWorkerQueueLocked();
            }
            AccountDroppedWorkerTasks(drops);
            startRenderPipeline = ownerTransition.previous == RenderOutputOwner::Avc420Gpu;
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " state=" + OutputStateName(outputState_) +
                " policyState=" + reason;
            if (impl_ != nullptr) {
                implDiagnosticsCache_ = impl_->DebugSummary();
            }
        }
    }
    if (!active && startRenderPipeline && callbacks.startRenderPipeline != nullptr) {
        callbacks.startRenderPipeline();
    }
    if (changed || !active) {
        std::string line = "AVC420 GPU compositor observed FreeRDP output policy: active=" +
            std::string(active ? "yes" : "no") + " reason=" + reason;
        if (!active) {
            line += " previousOwner=" + RenderOutputOwnerName(ownerTransition.previous) +
                " transitionReason=" +
                RenderOutputOwnerTransitionReasonName(ownerTransition.reason);
        } else {
            line += " outputOwner=" + CurrentRenderOutputOwnerName();
        }
        logs.push_back(std::move(line));
    }
    if (drops.prewarms != 0 || drops.commands != 0 || drops.endFrames != 0) {
        logs.push_back("AVC420 GPU worker cleared queued work after output policy release: "
            "droppedPrewarms=" + std::to_string(drops.prewarms) +
            " droppedCommands=" + std::to_string(drops.commands) +
            " droppedEndFrames=" + std::to_string(drops.endFrames));
    }
    for (const std::string& line : logs) {
        Log(line);
    }
}

bool Avc420GpuCompositor::HasReadySurfaceTarget(
    const Avc420GpuCompositorCallbacks& callbacks) const
{
    if (callbacks.decoderSurfaceTarget == nullptr) {
        return false;
    }
    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    return target.window != nullptr && target.width != 0 && target.height != 0;
}

bool Avc420GpuCompositor::ShouldDetachForTargetChange(const std::string& reason) const
{
    return ContainsAny(reason, {
        "surface destroyed",
        "rdpgfx bridge detached",
        "rdpgfx diagnostics hook restore",
        "AVC surface output reset",
    });
}

void Avc420GpuCompositor::PauseOutputForTargetUnavailable(
    const std::string& reason, const Avc420GpuCompositorCallbacks& callbacks,
    std::vector<std::string>& logs)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_ || !outputActive_) {
            return;
        }
        outputState_ = Avc420OutputState::TargetPaused;
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes state=targetPaused "
            "targetUnavailable=" + reason;
    }

    if (impl_ != nullptr) {
        impl_->OnSurfaceTargetChanged(reason, callbacks, true, logs);
    }
    logs.push_back("AVC420 GPU compositor entered TargetPaused after target unavailable: "
        "reason=" + reason +
        " outputOwner=" + CurrentRenderOutputOwnerName() +
        " policyActive=yes retainDecoder=yes retainComposite=yes queuedWorkPreserved=yes");
}

Avc420GpuCompositor::WorkerQueueDropCounts
Avc420GpuCompositor::ClearWorkerQueueLocked()
{
    WorkerQueueDropCounts drops;
    for (const WorkerTask& task : workerQueue_) {
        switch (task.type) {
            case WorkerTaskType::Prewarm:
                ++drops.prewarms;
                break;
            case WorkerTaskType::SurfaceCommand:
                ++drops.commands;
                break;
            case WorkerTaskType::EndFrame:
                ++drops.endFrames;
                break;
        }
    }
    workerQueue_.clear();
    workerQueueDepth_.store(0);
    return drops;
}

Avc420GpuCompositor::WorkerQueueCompaction
Avc420GpuCompositor::CompactWorkerBacklogLocked()
{
    WorkerQueueCompaction compaction;
    compaction.depthBefore = workerQueue_.size();

    size_t keepEndFrameIndex = workerQueue_.size();
    for (size_t index = workerQueue_.size(); index > 0; --index) {
        const size_t taskIndex = index - 1;
        const WorkerTask& task = workerQueue_[taskIndex];
        if (task.type == WorkerTaskType::EndFrame && task.frame.matchedFrame) {
            keepEndFrameIndex = taskIndex;
            break;
        }
    }
    if (keepEndFrameIndex == workerQueue_.size()) {
        for (size_t index = workerQueue_.size(); index > 0; --index) {
            const size_t taskIndex = index - 1;
            if (workerQueue_[taskIndex].type == WorkerTaskType::EndFrame) {
                keepEndFrameIndex = taskIndex;
                break;
            }
        }
    }

    std::deque<WorkerTask> compacted;
    for (size_t index = 0; index < workerQueue_.size(); ++index) {
        WorkerTask& task = workerQueue_[index];
        if (task.type == WorkerTaskType::Prewarm) {
            ++compaction.drops.prewarms;
            continue;
        }
        if (task.type == WorkerTaskType::EndFrame && index != keepEndFrameIndex) {
            ++compaction.drops.endFrames;
            continue;
        }
        if (task.type == WorkerTaskType::SurfaceCommand) {
            ++compaction.preservedCommands;
        } else if (task.type == WorkerTaskType::EndFrame) {
            ++compaction.preservedEndFrames;
            compaction.preservedEndFrameId = task.frame.frameId;
            compaction.preservedEndFrameActiveId = task.frame.activeFrameId;
            compaction.preservedEndFrameMatched = task.frame.matchedFrame ? true : false;
        }
        compacted.push_back(std::move(task));
    }

    workerQueue_.swap(compacted);
    compaction.depthAfter = workerQueue_.size();
    workerQueueDepth_.store(workerQueue_.size());
    return compaction;
}

void Avc420GpuCompositor::AccountDroppedWorkerTasks(const WorkerQueueDropCounts& drops)
{
    if (drops.commands != 0) {
        workerDroppedCommands_.fetch_add(drops.commands);
    }
    if (drops.endFrames != 0) {
        workerDroppedEndFrames_.fetch_add(drops.endFrames);
    }
}

std::string Avc420GpuCompositor::WorkerBacklogText() const
{
    const auto pending = [](uint64_t queued, uint64_t processed) {
        return queued >= processed ? queued - processed : 0;
    };
    const uint64_t queuedCommands = workerQueuedCommands_.load();
    const uint64_t processedCommands = workerProcessedCommands_.load();
    const uint64_t queuedEndFrames = workerQueuedEndFrames_.load();
    const uint64_t processedEndFrames = workerProcessedEndFrames_.load();
    return "queueDepth=" + std::to_string(workerQueueDepth_.load()) +
        " commandBacklog=" + std::to_string(pending(queuedCommands, processedCommands)) +
        " presentBacklog=" + std::to_string(pending(queuedEndFrames, processedEndFrames)) +
        " queuedCommands=" + std::to_string(queuedCommands) +
        " processedCommands=" + std::to_string(processedCommands) +
        " queuedEndFrames=" + std::to_string(queuedEndFrames) +
        " processedEndFrames=" + std::to_string(processedEndFrames);
}

bool Avc420GpuCompositor::DetachOutputActive(
    const std::string& reason, const Avc420GpuCompositorCallbacks& callbacks,
    bool clearQueuedWork, RenderOutputOwnerTransitionReason transitionReason,
    std::vector<std::string>& logs)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_ || !outputActive_) {
            return false;
        }
        outputActive_ = false;
        outputState_ = transitionReason == RenderOutputOwnerTransitionReason::Avc420FatalFallback ?
            Avc420OutputState::Failed : Avc420OutputState::Detached;
        backgroundSeeded_ = false;
        backgroundSeedWidth_ = 0;
        backgroundSeedHeight_ = 0;
    }

    if (impl_ != nullptr) {
        impl_->Destroy();
    }

    const RenderOutputOwnerTransition ownerTransition = TransitionRenderOutputOwner(
        RenderOutputOwner::Gdi, transitionReason);
    WorkerQueueDropCounts drops;
    if (clearQueuedWork) {
        std::lock_guard<std::mutex> lock(workerMutex_);
        drops = ClearWorkerQueueLocked();
    }
    AccountDroppedWorkerTasks(drops);
    if (callbacks.setOutputPolicy != nullptr) {
        callbacks.setOutputPolicy(false, "AVC420 GPU detached: " + reason);
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=no state=" +
            std::string(OutputStateName(outputState_)) + " detached=" +
            reason;
        if (impl_ != nullptr) {
            implDiagnosticsCache_ = impl_->DebugSummary();
        }
    }

    logs.push_back("AVC420 GPU compositor detached output ownership: "
        "reason=" + reason +
        " previousOwner=" + RenderOutputOwnerName(ownerTransition.previous) +
        " transitionReason=" +
        RenderOutputOwnerTransitionReasonName(ownerTransition.reason) +
        " outputOwner=gdi; decoder/composite resources released and FreeRDP fallback is allowed");
    if (drops.prewarms != 0 || drops.commands != 0 || drops.endFrames != 0) {
        logs.push_back("AVC420 GPU worker cleared queued work after output detach: "
            "droppedPrewarms=" + std::to_string(drops.prewarms) +
            " droppedCommands=" + std::to_string(drops.commands) +
            " droppedEndFrames=" + std::to_string(drops.endFrames));
    }
    return true;
}

bool Avc420GpuCompositor::SeedBackgroundBeforeTakeover(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks& callbacks, std::vector<std::string>& logs)
{
    if (command == nullptr) {
        return false;
    }
    if (CommandCoversFullSurface(command)) {
        std::lock_guard<std::mutex> lock(mutex_);
        backgroundSeeded_ = true;
        backgroundSeedWidth_ = command->width;
        backgroundSeedHeight_ = command->height;
        return true;
    }
    if (callbacks.snapshotGdiFrame == nullptr) {
        logs.push_back("AVC420 GPU compositor cannot bootstrap partial SurfaceCommand: "
            "trusted GDI snapshot callback is not configured");
        return false;
    }

    RgbaFrame background;
    if (!callbacks.snapshotGdiFrame(
            background, true, "avc420 bootstrap gdi background", kBackgroundSeedMaxAgeMs)) {
        logs.push_back("AVC420 GPU compositor waits for trusted full GDI background before "
            "partial SurfaceCommand takeover: frame=" + std::to_string(command->frameId) +
            " surface=" + std::to_string(command->width) + "x" +
            std::to_string(command->height) +
            " maxAgeMs=" + std::to_string(kBackgroundSeedMaxAgeMs));
        return false;
    }
    if (background.width != command->width || background.height != command->height) {
        logs.push_back("AVC420 GPU compositor rejected GDI background seed with mismatched size: "
            "background=" + std::to_string(background.width) + "x" +
            std::to_string(background.height) +
            " command=" + std::to_string(command->width) + "x" +
            std::to_string(command->height));
        return false;
    }
    if (impl_ == nullptr || !impl_->ProcessGdiFrame(background, true, logs)) {
        logs.push_back("AVC420 GPU compositor failed to seed retained background before "
            "partial SurfaceCommand takeover: frame=" + std::to_string(command->frameId));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        backgroundSeeded_ = true;
        backgroundSeedWidth_ = background.width;
        backgroundSeedHeight_ = background.height;
    }
    logs.push_back("AVC420 GPU compositor seeded retained background before partial takeover: "
        "frame=" + std::to_string(command->frameId) +
        " background=" + std::to_string(background.width) + "x" +
        std::to_string(background.height));
    return true;
}

bool Avc420GpuCompositor::ClaimOutputAfterTakeover(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks& callbacks, std::vector<std::string>& logs)
{
    const RenderOutputOwnerTransition ownerTransition = TransitionRenderOutputOwner(
        RenderOutputOwner::Avc420Gpu,
        RenderOutputOwnerTransitionReason::Avc420Takeover);
    if (ownerTransition.previous != RenderOutputOwner::Avc420Gpu) {
        if (callbacks.stopRenderPipeline != nullptr) {
            callbacks.stopRenderPipeline();
        }
        if (callbacks.releaseRenderTarget != nullptr) {
            callbacks.releaseRenderTarget(
                "before AVC420 GPU compositor SurfaceCommand takeover");
        }
        logs.push_back("AVC420 GPU compositor claimed render output ownership at "
            "SurfaceCommand before suppressing FreeRDP GDI: previousOwner=" +
            RenderOutputOwnerName(ownerTransition.previous) +
            " transitionReason=" +
            RenderOutputOwnerTransitionReasonName(ownerTransition.reason) +
            " outputOwner=avc420-gpu");
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        backgroundSeeded_ = command != nullptr && CommandCoversFullSurface(command) ?
            true : backgroundSeeded_;
        if (backgroundSeeded_ && command != nullptr) {
            backgroundSeedWidth_ = command->width;
            backgroundSeedHeight_ = command->height;
        }
    }
    logs.push_back("AVC420 GPU compositor is authoritative after queued update; "
        "GDI is suppressed now and present is deferred until the matching frame boundary");
    return true;
}

void Avc420GpuCompositor::OnSurfaceTargetChanged(const std::string& reason)
{
    std::vector<std::string> logs;
    Avc420GpuCompositorCallbacks callbacks;
    bool enabled = false;
    bool outputActive = false;
    bool startRenderPipeline = false;
    bool detached = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            enabled = enabled_;
            outputActive = outputActive_;
            callbacks = callbacks_;
        }
        if (enabled && impl_) {
            const bool targetReady = HasReadySurfaceTarget(callbacks);
            if (outputActive && ShouldDetachForTargetChange(reason)) {
                detached = DetachOutputActive(reason, callbacks, true,
                    RenderOutputOwnerTransitionReason::SurfaceDestroyed, logs);
                startRenderPipeline = detached;
            } else if (outputActive && !targetReady) {
                PauseOutputForTargetUnavailable(reason, callbacks, logs);
            } else {
                impl_->OnSurfaceTargetChanged(reason, callbacks, outputActive, logs);
                if (outputActive && targetReady) {
                    std::lock_guard<std::mutex> lock(mutex_);
                    outputState_ = Avc420OutputState::Active;
                    logs.push_back("AVC420 GPU compositor target ready: reason=" + reason +
                        " state=active outputOwner=" + CurrentRenderOutputOwnerName() +
                        " retainDecoder=yes retainComposite=yes");
                }
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc420 gpu compositor: enabled=" +
                std::string(enabled_ ? "yes" : "no") +
                " active=" + std::string(outputActive_ ? "yes" : "no") +
                " state=" + OutputStateName(outputState_) +
                " targetChange=" + reason;
            if (impl_) {
                implDiagnosticsCache_ = impl_->DebugSummary();
            }
        }
    }
    if (startRenderPipeline && callbacks.startRenderPipeline != nullptr) {
        callbacks.startRenderPipeline();
    }

    if (enabled || outputActive) {
        for (const std::string& line : logs) {
            Log(line);
        }
    }
}

bool Avc420GpuCompositor::EnqueueSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive)
{
    if (command == nullptr) {
        return false;
    }

    WorkerTask task;
    task.type = WorkerTaskType::SurfaceCommand;
    task.callbacks = callbacks;
    task.outputActive = outputActive;
    task.command.info = *command;
    task.command.info.stream.data = nullptr;
    task.command.info.stream.regionRects = nullptr;

    uint64_t depth = 0;
    uint64_t queued = 0;
    WorkerQueueCompaction compaction;

    try {
        if (command->stream.data != nullptr && command->stream.length > 0) {
            task.command.data.assign(
                command->stream.data, command->stream.data + command->stream.length);
            task.command.info.stream.data = task.command.data.data();
        }
        if (command->stream.regionRects != nullptr && command->stream.numRegionRects > 0) {
            task.command.rects.assign(command->stream.regionRects,
                command->stream.regionRects + command->stream.numRegionRects);
            task.command.info.stream.regionRects = task.command.rects.data();
        }
    } catch (const std::exception& error) {
        Log("AVC420 GPU worker enqueue command failed: copy allocation error " +
            std::string(error.what()));
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (!workerRunning_) {
            EnsureWorkerLocked();
        }
        if (!workerRunning_) {
            ++workerQueueOverLimit_;
            return false;
        }
        if (workerQueue_.size() >= kEndFrameCoalesceDepth) {
            compaction = CompactWorkerBacklogLocked();
        }
        if (workerQueue_.size() >= kMaxWorkerTasks) {
            ++workerQueueOverLimit_;
            return false;
        }
        workerQueue_.push_back(std::move(task));
        depth = workerQueue_.size();
        workerQueueDepth_.store(depth);
        workerMaxDepth_.store(std::max(workerMaxDepth_.load(), depth));
        queued = ++workerQueuedCommands_;
    }
    if (compaction.DidDrop()) {
        AccountDroppedWorkerTasks(compaction.drops);
    }
    workerCondition_.notify_one();
    if (ShouldLogWorkerCounter(queued)) {
        Log("AVC420 GPU worker queued SurfaceCommand: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " frame=" + std::to_string(command->frameId) +
            " bytes=" + std::to_string(command->stream.length) +
            " rects=" + std::to_string(command->stream.numRegionRects) +
            " " + WorkerBacklogText());
    }
    if (compaction.DidDrop()) {
        Log("AVC420 GPU worker compacted backlog before SurfaceCommand: "
            "droppedPrewarms=" + std::to_string(compaction.drops.prewarms) +
            " droppedCommands=" + std::to_string(compaction.drops.commands) +
            " droppedEndFrames=" + std::to_string(compaction.drops.endFrames) +
            " preservedCommands=" + std::to_string(compaction.preservedCommands) +
            " preservedEndFrames=" + std::to_string(compaction.preservedEndFrames) +
            " preservedEndFrame=" + std::to_string(compaction.preservedEndFrameId) +
            " preservedActiveFrame=" + std::to_string(compaction.preservedEndFrameActiveId) +
            " preservedMatched=" +
            std::string(compaction.preservedEndFrameMatched ? "yes" : "no") +
            " depthBeforeCompact=" + std::to_string(compaction.depthBefore) +
            " depthAfterCompact=" + std::to_string(compaction.depthAfter) +
            " frame=" + std::to_string(command->frameId) +
            " depth=" + std::to_string(depth) +
            " " + WorkerBacklogText());
    }
    return true;
}

bool Avc420GpuCompositor::EnqueueEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive)
{
    if (frame == nullptr) {
        return false;
    }

    WorkerTask task;
    task.type = WorkerTaskType::EndFrame;
    task.frame = *frame;
    task.callbacks = callbacks;
    task.outputActive = outputActive;

    uint64_t depth = 0;
    uint64_t queued = 0;
    WorkerQueueCompaction compaction;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (!workerRunning_) {
            EnsureWorkerLocked();
        }
        if (!workerRunning_) {
            ++workerQueueOverLimit_;
            return false;
        }
        if (workerQueue_.size() >= kEndFrameCoalesceDepth) {
            compaction = CompactWorkerBacklogLocked();
        }
        if (workerQueue_.size() >= kMaxWorkerTasks) {
            ++workerQueueOverLimit_;
            return false;
        }
        workerQueue_.push_back(std::move(task));
        depth = workerQueue_.size();
        workerQueueDepth_.store(depth);
        workerMaxDepth_.store(std::max(workerMaxDepth_.load(), depth));
        queued = ++workerQueuedEndFrames_;
    }
    if (compaction.DidDrop()) {
        AccountDroppedWorkerTasks(compaction.drops);
    }
    workerCondition_.notify_one();
    const bool logCompaction = compaction.DidDrop() &&
        (ShouldLogWorkerCounter(queued) ||
            (compaction.drops.endFrames != 0 &&
                ShouldLogWorkerCounter(workerDroppedEndFrames_.load())));
    if (ShouldLogWorkerCounter(queued) || logCompaction) {
        Log("AVC420 GPU worker queued EndFrame: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " droppedPrewarms=" + std::to_string(compaction.drops.prewarms) +
            " droppedCommands=" + std::to_string(compaction.drops.commands) +
            " droppedOldEndFrames=" + std::to_string(compaction.drops.endFrames) +
            " droppedEndFrameTotal=" + std::to_string(workerDroppedEndFrames_.load()) +
            " preservedCommands=" + std::to_string(compaction.preservedCommands) +
            " preservedEndFrames=" + std::to_string(compaction.preservedEndFrames) +
            " preservedEndFrame=" + std::to_string(compaction.preservedEndFrameId) +
            " preservedActiveFrame=" + std::to_string(compaction.preservedEndFrameActiveId) +
            " preservedMatched=" +
            std::string(compaction.preservedEndFrameMatched ? "yes" : "no") +
            " depthBeforeCompact=" + std::to_string(compaction.depthBefore) +
            " depthAfterCompact=" + std::to_string(compaction.depthAfter) +
            " frame=" + std::to_string(frame->frameId) +
            " matched=" + std::string(frame->matchedFrame ? "yes" : "no") +
            " " + WorkerBacklogText());
    }
    return true;
}

void Avc420GpuCompositor::EnsureWorkerLocked()
{
    if (workerRunning_) {
        return;
    }
    workerRunning_ = true;
    worker_ = std::thread([this]() { WorkerLoop(); });
}

void Avc420GpuCompositor::StopWorker()
{
    std::thread worker;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        workerRunning_ = false;
        workerQueue_.clear();
        workerQueueDepth_.store(0);
        worker = std::move(worker_);
    }
    workerCondition_.notify_all();
    if (worker.joinable()) {
        worker.join();
    }
}

void Avc420GpuCompositor::WorkerLoop()
{
    for (;;) {
        WorkerTask task;
        {
            std::unique_lock<std::mutex> lock(workerMutex_);
            workerCondition_.wait(lock, [this]() {
                return !workerRunning_ || !workerQueue_.empty();
            });
            if (!workerRunning_ && workerQueue_.empty()) {
                workerQueueDepth_.store(0);
                return;
            }
            task = std::move(workerQueue_.front());
            workerQueue_.pop_front();
            workerQueueDepth_.store(workerQueue_.size());
        }
        ProcessWorkerTask(std::move(task));
    }
}

void Avc420GpuCompositor::ProcessWorkerTask(WorkerTask task)
{
    auto bindCommandPointers = [](OwnedAvc420Command& command) {
        command.info.stream.data = command.data.empty() ? nullptr : command.data.data();
        command.info.stream.regionRects = command.rects.empty() ? nullptr : command.rects.data();
    };

    std::vector<std::string> logs;
    bool handled = false;
    bool outputActive = task.outputActive;
    bool outputActiveBeforeTask = false;
    bool releaseActiveOutput = false;
    RenderOutputOwnerTransition ownerTransition;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return;
            }
            outputActive = outputActive_ || outputActive;
            outputActiveBeforeTask = outputActive;
        }
        if (task.type == WorkerTaskType::Prewarm) {
            handled = impl_ != nullptr &&
                impl_->Prewarm(task.prewarmSurfaceWidth, task.prewarmSurfaceHeight, logs);
            const uint64_t processed = ++workerProcessedPrewarms_;
            if (ShouldLogWorkerCounter(processed) || !handled) {
                logs.push_back("AVC420 GPU worker processed prewarm: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " surface=" + std::to_string(task.prewarmSurfaceWidth) + "x" +
                    std::to_string(task.prewarmSurfaceHeight));
            }
        } else if (task.type == WorkerTaskType::SurfaceCommand) {
            bindCommandPointers(task.command);
            handled = impl_ != nullptr &&
                impl_->ProcessCommand(&task.command.info, task.callbacks, outputActive, logs);
            if (handled && CommandCoversFullSurface(&task.command.info)) {
                std::lock_guard<std::mutex> lock(mutex_);
                backgroundSeeded_ = true;
                backgroundSeedWidth_ = task.command.info.width;
                backgroundSeedHeight_ = task.command.info.height;
            }
            const uint64_t processed = ++workerProcessedCommands_;
            if (ShouldLogWorkerCounter(processed)) {
                logs.push_back("AVC420 GPU worker processed SurfaceCommand: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " frame=" + std::to_string(task.command.info.frameId));
            }
        } else {
            if (outputActive && !HasReadySurfaceTarget(task.callbacks)) {
                PauseOutputForTargetUnavailable(
                    "AVC420 worker EndFrame target unavailable", task.callbacks, logs);
                handled = true;
            } else {
                handled = impl_ != nullptr &&
                    impl_->PresentEndFrame(&task.frame, task.callbacks, outputActive, logs);
            }
            const uint64_t processed = ++workerProcessedEndFrames_;
            if (ShouldLogWorkerCounter(processed)) {
                logs.push_back("AVC420 GPU worker processed EndFrame: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " frame=" + std::to_string(task.frame.frameId));
            }
        }
        if (task.type == WorkerTaskType::SurfaceCommand && outputActiveBeforeTask && !handled) {
            if (impl_ != nullptr) {
                impl_->Destroy();
            }
            ownerTransition = TransitionRenderOutputOwner(
                RenderOutputOwner::Gdi,
                RenderOutputOwnerTransitionReason::Avc420FatalFallback);
            outputActive = false;
            releaseActiveOutput = true;
            logs.push_back("AVC420 GPU worker released output ownership after active command "
                "failure: previousOwner=" + RenderOutputOwnerName(ownerTransition.previous) +
                " transitionReason=" +
                RenderOutputOwnerTransitionReasonName(ownerTransition.reason) +
                " outputOwner=gdi; queued stale AVC420 work will be ignored until the stream "
                "becomes decodable again");
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = releaseActiveOutput ? false : (outputActive_ || outputActive);
            if (releaseActiveOutput) {
                outputState_ = Avc420OutputState::Failed;
            } else if (outputActive_) {
                outputState_ = outputState_ == Avc420OutputState::TargetPaused ?
                    Avc420OutputState::TargetPaused : Avc420OutputState::Active;
            } else {
                outputState_ = Avc420OutputState::Detached;
            }
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " state=" + OutputStateName(outputState_) +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " worker=" + (task.type == WorkerTaskType::Prewarm ? "prewarm" :
                    (task.type == WorkerTaskType::SurfaceCommand ? "command" : "endFrame")) +
                " handled=" + std::string(handled ? "yes" : "no");
            if (impl_ != nullptr) {
                implDiagnosticsCache_ = impl_->DebugSummary();
            }
        }
        AppendPeriodicStats(logs, task.type == WorkerTaskType::Prewarm ? "prewarm" :
            (task.type == WorkerTaskType::SurfaceCommand ? "command" : "endFrame"));
    }
    if (releaseActiveOutput) {
        WorkerQueueDropCounts drops;
        {
            std::lock_guard<std::mutex> lock(workerMutex_);
            drops = ClearWorkerQueueLocked();
        }
        AccountDroppedWorkerTasks(drops);
        if (drops.prewarms != 0 || drops.commands != 0 || drops.endFrames != 0) {
            logs.push_back("AVC420 GPU worker cleared stale queued work after fail-open: "
                "droppedPrewarms=" + std::to_string(drops.prewarms) +
                " droppedCommands=" + std::to_string(drops.commands) +
                " droppedEndFrames=" + std::to_string(drops.endFrames));
        }
    }
    if (releaseActiveOutput && task.callbacks.setOutputPolicy != nullptr) {
        task.callbacks.setOutputPolicy(false, "AVC420 GPU active command failure");
    }
    if (releaseActiveOutput && task.callbacks.startRenderPipeline != nullptr) {
        task.callbacks.startRenderPipeline();
    }
    for (const std::string& line : logs) {
        Log(line);
    }
}

void Avc420GpuCompositor::AppendPeriodicStats(
    std::vector<std::string>& logs, const std::string& reason)
{
    const auto now = std::chrono::steady_clock::now();
    bool shouldLog = false;
    bool outputActive = false;
    uint64_t candidates = 0;
    uint32_t lastFrameId = 0;
    uint32_t lastBytes = 0;
    uint32_t lastRects = 0;
    uint32_t lastTargetWidth = 0;
    uint32_t lastTargetHeight = 0;
    bool lastFullSurface = false;
    bool backgroundSeeded = false;
    uint32_t backgroundSeedWidth = 0;
    uint32_t backgroundSeedHeight = 0;
    uint64_t avc420IdleMs = 0;
    std::string inputFps = "0.00";
    std::string endFrameFps = "0.00";
    uint64_t elapsedMs = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (nextStatsLogAt_ == std::chrono::steady_clock::time_point{} ||
            now >= nextStatsLogAt_) {
            nextStatsLogAt_ = now + kStatsLogInterval;
            shouldLog = true;
            outputActive = outputActive_;
            candidates = candidates_;
            lastFrameId = lastFrameId_;
            lastBytes = lastBytes_;
            lastRects = lastRects_;
            lastTargetWidth = lastTargetWidth_;
            lastTargetHeight = lastTargetHeight_;
            lastFullSurface = lastFullSurface_;
            backgroundSeeded = backgroundSeeded_;
            backgroundSeedWidth = backgroundSeedWidth_;
            backgroundSeedHeight = backgroundSeedHeight_;
            if (lastAvc420CommandAt_ != std::chrono::steady_clock::time_point{}) {
                avc420IdleMs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - lastAvc420CommandAt_).count());
            }
            if (lastStatsLogAt_ != std::chrono::steady_clock::time_point{}) {
                elapsedMs = static_cast<uint64_t>(
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        now - lastStatsLogAt_).count());
                inputFps = RatePerSecondText(candidates_ - lastStatsCandidates_, elapsedMs);
                endFrameFps = RatePerSecondText(
                    workerProcessedEndFrames_.load() - lastStatsProcessedEndFrames_, elapsedMs);
            }
            lastStatsLogAt_ = now;
            lastStatsCandidates_ = candidates_;
            lastStatsProcessedEndFrames_ = workerProcessedEndFrames_.load();
        }
    }
    if (!shouldLog) {
        return;
    }

    const std::string implStats = impl_ == nullptr ? "decoded=0 presented=0 mismatch=0" :
        impl_->StatsSummary();
    logs.push_back("AVC420 GPU stats: reason=" + reason +
        " active=" + std::string(outputActive ? "yes" : "no") +
        " candidates=" + std::to_string(candidates) +
        " lastFrame=" + std::to_string(lastFrameId) +
        " lastTarget=" + std::to_string(lastTargetWidth) + "x" +
        std::to_string(lastTargetHeight) +
        " lastBytes=" + std::to_string(lastBytes) +
        " lastRects=" + std::to_string(lastRects) +
        " lastFullSurface=" + std::string(lastFullSurface ? "yes" : "no") +
        " backgroundSeeded=" + std::string(backgroundSeeded ? "yes" : "no") +
        " backgroundSeedSize=" + std::to_string(backgroundSeedWidth) + "x" +
        std::to_string(backgroundSeedHeight) +
        " avc420IdleMs=" + std::to_string(avc420IdleMs) +
        " statsWindowMs=" + std::to_string(elapsedMs) +
        " inputFps=" + inputFps +
        " endFrameFps=" + endFrameFps +
        " queuedCommands=" + std::to_string(workerQueuedCommands_.load()) +
        " processedCommands=" + std::to_string(workerProcessedCommands_.load()) +
        " queuedEndFrames=" + std::to_string(workerQueuedEndFrames_.load()) +
        " processedEndFrames=" + std::to_string(workerProcessedEndFrames_.load()) +
        " droppedCommands=" + std::to_string(workerDroppedCommands_.load()) +
        " droppedEndFrames=" + std::to_string(workerDroppedEndFrames_.load()) +
        " queueDepth=" + std::to_string(workerQueueDepth_.load()) +
        " maxDepth=" + std::to_string(workerMaxDepth_.load()) +
        " queueOverLimit=" + std::to_string(workerQueueOverLimit_.load()) +
        " " + implStats);
}

bool Avc420GpuCompositor::OnGdiFrame(const RgbaFrame& frame)
{
    const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
        static_cast<int32_t>(frame.width * 4U);
    if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
        sourceStride < static_cast<int32_t>(frame.width * 4U)) {
        return false;
    }

    std::vector<std::string> logs;
    bool handled = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);

        Avc420GpuCompositorCallbacks callbacks;
        bool outputActive = false;
        bool backgroundSeeded = false;
        uint32_t backgroundSeedWidth = 0;
        uint32_t backgroundSeedHeight = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_ || !outputActive_) {
                return false;
            }
            callbacks = callbacks_;
            outputActive = outputActive_;
            backgroundSeeded = backgroundSeeded_;
            backgroundSeedWidth = backgroundSeedWidth_;
            backgroundSeedHeight = backgroundSeedHeight_;
        }

        const bool sizeMatches = backgroundSeeded &&
            backgroundSeedWidth == frame.width && backgroundSeedHeight == frame.height;
        const bool needsFullSeed = !sizeMatches;
        if (needsFullSeed && !DirtyCoversFullFrame(frame.dirty, frame.width, frame.height)) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (backgroundSeeded_ &&
                (backgroundSeedWidth_ != frame.width || backgroundSeedHeight_ != frame.height)) {
                backgroundSeeded_ = false;
                backgroundSeedWidth_ = 0;
                backgroundSeedHeight_ = 0;
            }
            return false;
        }

        RgbaFrame background = frame;
        background.strideBytes = sourceStride;
        if (background.label.empty()) {
            background.label = needsFullSeed ? "freerdp gdi background seed" :
                "freerdp gdi background";
        }
        if (needsFullSeed) {
            background.dirty = FullDirtyStats(background.width, background.height);
            background.dirtySequenceStart = background.sequence;
        }

        handled = impl_ != nullptr && impl_->ProcessGdiFrame(background, outputActive, logs);
        if (handled && impl_ != nullptr) {
            (void)impl_->PresentGdiBackgroundNow(background.label, callbacks, outputActive, logs);
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (handled) {
                backgroundSeeded_ = true;
                backgroundSeedWidth_ = background.width;
                backgroundSeedHeight_ = background.height;
            }
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes state=" +
                std::string(OutputStateName(outputState_)) +
                " gdiBackground=" + std::string(handled ? "composited" : "skipped");
            if (impl_ != nullptr) {
                implDiagnosticsCache_ = impl_->DebugSummary();
            }
        }
    }

    for (const std::string& line : logs) {
        Log(line);
    }
    return handled;
}

bool Avc420GpuCompositor::OnSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }

    bool shouldLogCommand = false;
    uint64_t candidate = 0;
    const auto commandReceivedAt = std::chrono::steady_clock::now();
    const bool fullCommand = CommandCoversFullSurface(command);
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return false;
        }
        candidate = ++candidates_;
        shouldLogCommand = ShouldLogWorkerCounter(candidate);
        lastFrameId_ = command->frameId;
        lastBytes_ = command->stream.length;
        lastRects_ = command->stream.numRegionRects;
        lastTargetWidth_ = command->targetWidth;
        lastTargetHeight_ = command->targetHeight;
        lastFullSurface_ = command->fullSurface ? true : false;
        lastAvc420CommandAt_ = commandReceivedAt;
        if (!fullCommand && backgroundSeeded_ &&
            (backgroundSeedWidth_ != command->width ||
                backgroundSeedHeight_ != command->height)) {
            backgroundSeeded_ = false;
            backgroundSeedWidth_ = 0;
            backgroundSeedHeight_ = 0;
        }
    }

    Avc420GpuCompositorCallbacks callbacks;
    bool outputActive = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = callbacks_;
        outputActive = outputActive_;
    }

    if (!HasReadySurfaceTarget(callbacks)) {
        std::vector<std::string> logs;
        bool targetReady = false;
        {
            std::lock_guard<std::mutex> processLock(processingMutex_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                outputActive = outputActive_;
                callbacks = callbacks_;
            }
            targetReady = HasReadySurfaceTarget(callbacks);
            if (outputActive && !targetReady) {
                PauseOutputForTargetUnavailable(
                    "AVC420 SurfaceCommand target unavailable", callbacks, logs);
            }
        }
        for (const std::string& line : logs) {
            Log(line);
        }
        if (!targetReady && !outputActive) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                diagnostics_ = "avc420 gpu compositor: enabled=yes active=no candidates=" +
                    std::to_string(candidates_) +
                    " lastFrame=" + std::to_string(lastFrameId_) +
                    " suppress=no targetUnavailable=yes";
            }
            if (shouldLogCommand) {
                Log("AVC420 GPU compositor skipped SurfaceCommand takeover because target is "
                    "unavailable; preserving FreeRDP native GDI path: frame=" +
                    std::to_string(command->frameId));
            }
            return false;
        }
    }

    bool backgroundReadyForCommand = fullCommand;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        backgroundReadyForCommand = backgroundReadyForCommand ||
            (backgroundSeeded_ &&
                backgroundSeedWidth_ == command->width &&
                backgroundSeedHeight_ == command->height);
    }
    if (outputActive && !backgroundReadyForCommand) {
        std::vector<std::string> logs;
        bool detached = false;
        {
            std::lock_guard<std::mutex> processLock(processingMutex_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                outputActive = outputActive_;
                callbacks = callbacks_;
            }
            if (outputActive) {
                detached = DetachOutputActive(
                    "AVC420 bootstrap background unavailable before partial SurfaceCommand",
                    callbacks, true,
                    RenderOutputOwnerTransitionReason::Avc420BootstrapFallback, logs);
            }
        }
        if (detached && callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        logs.push_back("AVC420 GPU compositor preserved FreeRDP native GDI for partial "
            "SurfaceCommand because retained background is not trusted: frame=" +
            std::to_string(command->frameId) +
            " surface=" + std::to_string(command->width) + "x" +
            std::to_string(command->height));
        for (const std::string& line : logs) {
            Log(line);
        }
        return false;
    }

    if (outputActive && EnqueueSurfaceCommand(command, callbacks, outputActive)) {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes candidates=" +
            std::to_string(candidates_) +
            " state=" + OutputStateName(outputState_) +
            " lastFrame=" + std::to_string(lastFrameId_) +
            " suppress=queued-worker";
        return true;
    }

    std::vector<std::string> logs;
    bool consumed = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive = outputActive_;
        }
        if (!outputActive && !SeedBackgroundBeforeTakeover(command, callbacks, logs)) {
            {
                std::lock_guard<std::mutex> lock(mutex_);
                diagnostics_ = "avc420 gpu compositor: enabled=yes active=no candidates=" +
                    std::to_string(candidates_) +
                    " lastFrame=" + std::to_string(lastFrameId_) +
                    " suppress=no backgroundSeed=no";
            }
            consumed = false;
        } else {
            consumed = impl_ != nullptr &&
                impl_->ProcessCommand(command, callbacks, outputActive, logs);
        }
        if (consumed && !outputActive) {
            outputActive = ClaimOutputAfterTakeover(command, callbacks, logs);
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive;
            if (outputActive_) {
                outputState_ = Avc420OutputState::Active;
            }
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " state=" + OutputStateName(outputState_) +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " bytes=" + std::to_string(lastBytes_) +
                " rects=" + std::to_string(lastRects_) +
                " full=" + std::string(lastFullSurface_ ? "yes" : "no") +
                " suppress=" + std::string(consumed ? "this-command" : "no");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return consumed;
}

bool Avc420GpuCompositor::OnEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame)
{
    if (frame == nullptr) {
        return false;
    }

    std::vector<std::string> logs;
    Avc420GpuCompositorCallbacks callbacks;
    bool handled = false;
    bool outputActive = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return false;
        }
        outputActive = outputActive_;
        callbacks = callbacks_;
    }

    if (!HasReadySurfaceTarget(callbacks)) {
        bool targetReady = false;
        {
            std::lock_guard<std::mutex> processLock(processingMutex_);
            {
                std::lock_guard<std::mutex> lock(mutex_);
                outputActive = outputActive_;
                callbacks = callbacks_;
            }
            targetReady = HasReadySurfaceTarget(callbacks);
            if (outputActive && !targetReady) {
                PauseOutputForTargetUnavailable(
                    "AVC420 EndFrame target unavailable", callbacks, logs);
            }
        }
        for (const std::string& line : logs) {
            Log(line);
        }
        if (!targetReady) {
            if (outputActive) {
                std::lock_guard<std::mutex> lock(mutex_);
                diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes state=" +
                    std::string(OutputStateName(outputState_)) +
                    " candidates=" + std::to_string(candidates_) +
                    " lastFrame=" + std::to_string(lastFrameId_) +
                    " endFrame=" + std::to_string(frame->frameId) +
                    " endFramePresent=targetPaused";
                return true;
            }
            {
                std::lock_guard<std::mutex> lock(mutex_);
                diagnostics_ = "avc420 gpu compositor: enabled=yes active=no candidates=" +
                    std::to_string(candidates_) +
                    " lastFrame=" + std::to_string(lastFrameId_) +
                    " endFrame=" + std::to_string(frame->frameId) +
                    " endFramePresent=targetUnavailable";
            }
            return false;
        }
    }

    if (outputActive && EnqueueEndFrame(frame, callbacks, outputActive)) {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes candidates=" +
            std::to_string(candidates_) +
            " state=" + OutputStateName(outputState_) +
            " lastFrame=" + std::to_string(lastFrameId_) +
            " endFrame=" + std::to_string(frame->frameId) +
            " endFramePresent=queued-worker";
        return true;
    }

    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return false;
            }
            outputActive = outputActive_;
            callbacks = callbacks_;
        }
        handled = impl_ != nullptr &&
            impl_->PresentEndFrame(frame, callbacks, outputActive, logs);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive;
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " endFrame=" + std::to_string(frame->frameId) +
                " endFramePresent=" + std::string(handled ? "handled" : "none");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return handled;
}

void Avc420GpuCompositor::Log(const std::string& message) const
{
    if (IsRoutineAvc420GpuLog(message)) {
        return;
    }
    Avc420GpuLogFn log;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        log = log_;
    }
    if (log != nullptr) {
        log(message);
    }
}

Avc420GpuCompositor& SharedAvc420GpuCompositor()
{
    static Avc420GpuCompositor compositor;
    return compositor;
}

} // namespace rdp_bridge
