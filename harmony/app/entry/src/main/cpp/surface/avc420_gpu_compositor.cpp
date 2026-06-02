#include "surface/avc420_gpu_compositor.h"

#include "surface/avc444_gpu_compositor_internal.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <mutex>
#include <string>
#include <utility>

namespace rdp_bridge {
namespace {
constexpr size_t kMaxWorkerTasks = 720;
// Preserve normal EndFrame ordering; coalesce only if the worker is effectively saturated.
constexpr size_t kEndFrameCoalesceDepth = kMaxWorkerTasks;

bool ShouldLogWorkerCounter(uint64_t count)
{
    return count == 1 || (count % 600) == 0;
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

bool IsRoutineAvc420GpuLog(const std::string& message)
{
    if (message.rfind("AVC420 GPU", 0) != 0) {
        return false;
    }
    if (message.find("prewarmed") != std::string::npos) {
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
    Avc444GpuCompositorCallbacks callbacks)
{
    StopWorker();
    std::lock_guard<std::mutex> processLock(processingMutex_);
    if (impl_) {
        impl_->Destroy();
    }
    if (CurrentRenderOutputOwner() == RenderOutputOwner::Avc420Gpu) {
        ExchangeRenderOutputOwner(RenderOutputOwner::Gdi);
    }
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    log_ = std::move(log);
    callbacks_ = std::move(callbacks);
    outputActive_ = false;
    candidates_ = 0;
    lastFrameId_ = 0;
    lastBytes_ = 0;
    lastRects_ = 0;
    lastTargetWidth_ = 0;
    lastTargetHeight_ = 0;
    lastFullSurface_ = false;
    implDiagnosticsCache_.clear();
    workerQueuedPrewarms_.store(0);
    workerQueuedCommands_.store(0);
    workerQueuedEndFrames_.store(0);
    workerProcessedPrewarms_.store(0);
    workerProcessedCommands_.store(0);
    workerProcessedEndFrames_.store(0);
    workerDroppedEndFrames_.store(0);
    workerQueueOverLimit_.store(0);
    workerMaxDepth_.store(0);
    workerQueueDepth_.store(0);
    diagnostics_ = enabled ?
        "avc420 gpu compositor: configured hardware-decode mapped-plane compositor" :
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
    if (workerQueuedPrewarms != 0 || workerQueuedCommands != 0 || workerQueuedEndFrames != 0 ||
        workerProcessedPrewarms != 0 || workerProcessedCommands != 0 ||
        workerProcessedEndFrames != 0) {
        diagnostics += " | gpuWorker=depth:" + std::to_string(workerDepth) +
            ",queued:" + std::to_string(workerQueuedPrewarms) + "/" +
            std::to_string(workerQueuedCommands) + "/" +
            std::to_string(workerQueuedEndFrames) +
            ",processed:" + std::to_string(workerProcessedPrewarms) + "/" +
            std::to_string(workerProcessedCommands) + "/" +
            std::to_string(workerProcessedEndFrames) +
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

bool Avc420GpuCompositor::EnqueueSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive)
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

    uint64_t depth = 0;
    uint64_t queued = 0;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (!workerRunning_) {
            EnsureWorkerLocked();
        }
        if (!workerRunning_) {
            ++workerQueueOverLimit_;
            return false;
        }
        if (workerQueue_.size() >= kMaxWorkerTasks) {
            ++workerQueueOverLimit_;
        }
        workerQueue_.push_back(std::move(task));
        depth = workerQueue_.size();
        workerQueueDepth_.store(depth);
        workerMaxDepth_.store(std::max(workerMaxDepth_.load(), depth));
        queued = ++workerQueuedCommands_;
    }
    workerCondition_.notify_one();
    if (ShouldLogWorkerCounter(queued)) {
        Log("AVC420 GPU worker queued SurfaceCommand: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " frame=" + std::to_string(command->frameId) +
            " bytes=" + std::to_string(command->stream.length) +
            " rects=" + std::to_string(command->stream.numRegionRects));
    }
    return true;
}

bool Avc420GpuCompositor::EnqueueEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive)
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
    uint64_t dropped = 0;
    uint64_t droppedTotal = 0;
    {
        std::lock_guard<std::mutex> lock(workerMutex_);
        if (!workerRunning_) {
            EnsureWorkerLocked();
        }
        if (!workerRunning_) {
            ++workerQueueOverLimit_;
            return false;
        }
        if (workerQueue_.size() > kEndFrameCoalesceDepth) {
            for (auto it = workerQueue_.begin(); it != workerQueue_.end();) {
                if (it->type == WorkerTaskType::EndFrame) {
                    it = workerQueue_.erase(it);
                    ++dropped;
                } else {
                    ++it;
                }
            }
            if (dropped != 0) {
                droppedTotal = workerDroppedEndFrames_.fetch_add(dropped) + dropped;
            }
        }
        if (workerQueue_.size() >= kMaxWorkerTasks) {
            ++workerQueueOverLimit_;
        }
        workerQueue_.push_back(std::move(task));
        depth = workerQueue_.size();
        workerQueueDepth_.store(depth);
        workerMaxDepth_.store(std::max(workerMaxDepth_.load(), depth));
        queued = ++workerQueuedEndFrames_;
    }
    workerCondition_.notify_one();
    if (ShouldLogWorkerCounter(queued) ||
        (dropped != 0 && ShouldLogWorkerCounter(droppedTotal))) {
        Log("AVC420 GPU worker queued EndFrame: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " droppedOldEndFrames=" + std::to_string(dropped) +
            " droppedTotal=" + std::to_string(droppedTotal) +
            " frame=" + std::to_string(frame->frameId) +
            " matched=" + std::string(frame->matchedFrame ? "yes" : "no"));
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
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return;
            }
            outputActive = outputActive_ || outputActive;
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
            const uint64_t processed = ++workerProcessedCommands_;
            if (ShouldLogWorkerCounter(processed)) {
                logs.push_back("AVC420 GPU worker processed SurfaceCommand: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " frame=" + std::to_string(task.command.info.frameId));
            }
        } else {
            handled = impl_ != nullptr &&
                impl_->PresentEndFrame(&task.frame, task.callbacks, outputActive, logs);
            const uint64_t processed = ++workerProcessedEndFrames_;
            if (ShouldLogWorkerCounter(processed)) {
                logs.push_back("AVC420 GPU worker processed EndFrame: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " frame=" + std::to_string(task.frame.frameId));
            }
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive_ || outputActive;
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " worker=" + (task.type == WorkerTaskType::Prewarm ? "prewarm" :
                    (task.type == WorkerTaskType::SurfaceCommand ? "command" : "endFrame")) +
                " handled=" + std::string(handled ? "yes" : "no");
            if (impl_ != nullptr) {
                implDiagnosticsCache_ = impl_->DebugSummary();
            }
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
}

bool Avc420GpuCompositor::OnSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }

    bool shouldLogCommand = false;
    bool frameOpen = false;
    uint64_t candidate = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return false;
        }
        candidate = ++candidates_;
        shouldLogCommand = ShouldLogWorkerCounter(candidate);
        frameOpen = command->frameOpen ? true : false;
        lastFrameId_ = command->frameId;
        lastBytes_ = command->stream.length;
        lastRects_ = command->stream.numRegionRects;
        lastTargetWidth_ = command->targetWidth;
        lastTargetHeight_ = command->targetHeight;
        lastFullSurface_ = command->fullSurface ? true : false;
    }

    if (shouldLogCommand || !frameOpen) {
        const RECTANGLE_16* firstRect =
            command->stream.numRegionRects == 0 ? nullptr : command->stream.regionRects;
        Log("AVC420 GPU compositor candidate: index=" + std::to_string(candidate) +
            " surface=" + std::to_string(command->surfaceId) +
            " frame=" + std::to_string(command->frameId) +
            " frameOpen=" + std::string(frameOpen ? "yes" : "no") +
            " commandOrigin=" + std::to_string(command->left) + "," +
            std::to_string(command->top) +
            " surfaceSize=" + std::to_string(command->width) + "x" +
            std::to_string(command->height) +
            " target=" + std::to_string(command->targetWidth) + "x" +
            std::to_string(command->targetHeight) +
            " fullSurface=" + std::string(command->fullSurface ? "yes" : "no") +
            " stream=bytes:" + std::to_string(command->stream.length) +
            ",rects:" + std::to_string(command->stream.numRegionRects) +
            ",first:" + Avc420GpuCompositorImpl::RectText(firstRect));
    }

    Avc444GpuCompositorCallbacks callbacks;
    bool outputActive = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = callbacks_;
        outputActive = outputActive_;
    }

    if (outputActive && EnqueueSurfaceCommand(command, callbacks, outputActive)) {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes candidates=" +
            std::to_string(candidates_) +
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
        consumed = impl_ != nullptr &&
            impl_->ProcessCommand(command, callbacks, outputActive, logs);
        if (consumed && !outputActive) {
            const RenderOutputOwner previousOwner =
                ExchangeRenderOutputOwner(RenderOutputOwner::Avc420Gpu);
            if (previousOwner != RenderOutputOwner::Avc420Gpu) {
                if (callbacks.stopRenderPipeline != nullptr) {
                    callbacks.stopRenderPipeline();
                }
                if (callbacks.releaseRenderTarget != nullptr) {
                    callbacks.releaseRenderTarget(
                        "before AVC420 GPU compositor SurfaceCommand takeover");
                }
                logs.push_back("AVC420 GPU compositor claimed render output ownership at "
                    "SurfaceCommand before suppressing FreeRDP GDI: previousOwner=" +
                    RenderOutputOwnerName(previousOwner) + " outputOwner=avc420-gpu");
            }
            outputActive = true;
            logs.push_back("AVC420 GPU compositor is authoritative after queued update; "
                "GDI is suppressed now and present is deferred until the matching frame boundary");
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive;
            diagnostics_ = "avc420 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
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
    Avc444GpuCompositorCallbacks callbacks;
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

    if (outputActive && EnqueueEndFrame(frame, callbacks, outputActive)) {
        std::lock_guard<std::mutex> lock(mutex_);
        diagnostics_ = "avc420 gpu compositor: enabled=yes active=yes candidates=" +
            std::to_string(candidates_) +
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
