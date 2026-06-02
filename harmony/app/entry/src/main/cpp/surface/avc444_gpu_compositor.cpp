#include "surface/avc444_gpu_compositor.h"

#include "surface/avc444_gpu_compositor_internal.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rdp_bridge {
namespace {
constexpr size_t kMaxWorkerTasks = 720;
constexpr size_t kEndFrameCoalesceDepth = 24;

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

bool IsRoutineAvc444GpuLog(const std::string& message)
{
    if (message.rfind("AVC444 GPU", 0) != 0) {
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

std::string RectPreviewText(const RECTANGLE_16* rects, uint32_t count)
{
    if (count == 0) {
        return "none";
    }
    return rects == nullptr ? "missing" : "present";
}
}

Avc444GpuCompositor::Avc444GpuCompositor() : impl_(std::make_unique<Avc444GpuCompositorImpl>()) {}

Avc444GpuCompositor::~Avc444GpuCompositor()
{
    StopWorker();
}

void Avc444GpuCompositor::Configure(bool enabled, Avc444GpuLogFn log,
    Avc444GpuCompositorCallbacks callbacks)
{
    StopWorker();
    std::lock_guard<std::mutex> processLock(processingMutex_);
    if (impl_) {
        impl_->Destroy();
    }
    TransitionRenderOutputOwner(
        RenderOutputOwner::Gdi, RenderOutputOwnerTransitionReason::Avc444Reset);
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    log_ = std::move(log);
    callbacks_ = std::move(callbacks);
    outputActive_ = false;
    candidates_ = 0;
    invalidLcRejects_ = 0;
    lastFrameId_ = 0;
    lastLC_ = 0;
    lastStream1Bytes_ = 0;
    lastStream2Bytes_ = 0;
    lastTargetWidth_ = 0;
    lastTargetHeight_ = 0;
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
        "avc444 gpu compositor: configured mapped-plane compositor, gdi suppression follows per-command GPU success" :
        "avc444 gpu compositor: off";
}

void Avc444GpuCompositor::Reset()
{
    Configure(false, nullptr, {});
}

std::string Avc444GpuCompositor::Diagnostics() const
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
    const uint64_t workerDroppedEndFrames = workerDroppedEndFrames_.load();
    const uint64_t workerQueueOverLimit = workerQueueOverLimit_.load();
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
            ",droppedEnd:" + std::to_string(workerDroppedEndFrames) +
            ",queueOverLimit:" + std::to_string(workerQueueOverLimit) +
            ",maxDepth:" + std::to_string(workerMaxDepth_.load());
    }
    if (lastTargetWidth != 0 || lastTargetHeight != 0) {
        diagnostics += " | callbackTarget=cached,size:" + std::to_string(lastTargetWidth) +
            "x" + std::to_string(lastTargetHeight);
    }
    return diagnostics;
}

void Avc444GpuCompositor::SetOutputActive(bool active, const std::string& reason)
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        changed = outputActive_ != active;
        outputActive_ = active;
        diagnostics_ = "avc444 gpu compositor: enabled=" +
            std::string(enabled_ ? "yes" : "no") +
            " policyActive=" + std::string(outputActive_ ? "yes" : "no") +
            " reason=" + reason;
    }
    if (changed) {
        Log("AVC444 GPU compositor observed FreeRDP output policy: active=" +
            std::string(active ? "yes" : "no") + " reason=" + reason);
    }
}

void Avc444GpuCompositor::Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight)
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
        Log("AVC444 GPU worker queued prewarm: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " surface=" + std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight));
    }
}

bool Avc444GpuCompositor::EnqueueSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
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

    auto copyStream = [](OwnedAvc444Stream& owned,
                         FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO& target,
                         const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO& source) {
        target.length = source.length;
        target.data = nullptr;
        if (source.data != nullptr && source.length > 0) {
            owned.data.assign(source.data, source.data + source.length);
            target.data = owned.data.data();
        }
        target.numRegionRects = source.numRegionRects;
        target.regionRects = nullptr;
        if (source.regionRects != nullptr && source.numRegionRects > 0) {
            owned.rects.assign(source.regionRects, source.regionRects + source.numRegionRects);
            target.regionRects = owned.rects.data();
        }
    };

    try {
        copyStream(task.command.stream1, task.command.info.stream1, command->stream1);
        copyStream(task.command.stream2, task.command.info.stream2, command->stream2);
    } catch (const std::exception& error) {
        Log("AVC444 GPU worker enqueue command failed: copy allocation error " +
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
        Log("AVC444 GPU worker queued SurfaceCommand: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " frame=" + std::to_string(command->frameId) +
            " LC=" + std::to_string(command->LC) +
            " bytes=" + std::to_string(command->stream1.length) +
            "/" + std::to_string(command->LC == 0 ? command->stream2.length : 0));
    }
    return true;
}

bool Avc444GpuCompositor::EnqueueEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
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
                workerDroppedEndFrames_.fetch_add(dropped);
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

    const uint64_t droppedTotal = workerDroppedEndFrames_.load();
    if (ShouldLogWorkerCounter(queued) ||
        (dropped != 0 && ShouldLogWorkerCounter(droppedTotal))) {
        Log("AVC444 GPU worker queued EndFrame: queued=" + std::to_string(queued) +
            " depth=" + std::to_string(depth) +
            " droppedOldEndFrames=" + std::to_string(dropped) +
            " frame=" + std::to_string(frame->frameId) +
            " matched=" + std::string(frame->matchedFrame ? "yes" : "no"));
    }
    return true;
}

void Avc444GpuCompositor::EnsureWorkerLocked()
{
    if (workerRunning_) {
        return;
    }
    workerRunning_ = true;
    worker_ = std::thread([this]() { WorkerLoop(); });
}

void Avc444GpuCompositor::StopWorker()
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

void Avc444GpuCompositor::WorkerLoop()
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

void Avc444GpuCompositor::ProcessWorkerTask(WorkerTask task)
{
    auto bindCommandPointers = [](OwnedAvc444Command& command) {
        command.info.stream1.data =
            command.stream1.data.empty() ? nullptr : command.stream1.data.data();
        command.info.stream1.regionRects =
            command.stream1.rects.empty() ? nullptr : command.stream1.rects.data();
        command.info.stream2.data =
            command.stream2.data.empty() ? nullptr : command.stream2.data.data();
        command.info.stream2.regionRects =
            command.stream2.rects.empty() ? nullptr : command.stream2.rects.data();
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
                logs.push_back("AVC444 GPU worker processed prewarm: processed=" +
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
                logs.push_back("AVC444 GPU worker processed SurfaceCommand: processed=" +
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
                logs.push_back("AVC444 GPU worker processed EndFrame: processed=" +
                    std::to_string(processed) +
                    " handled=" + std::string(handled ? "yes" : "no") +
                    " depth=" + std::to_string(workerQueueDepth_.load()) +
                    " frame=" + std::to_string(task.frame.frameId));
            }
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive_ || outputActive;
            diagnostics_ = "avc444 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
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

bool Avc444GpuCompositor::OnSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }

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
        shouldLogCommand = ShouldLogWorkerCounter(candidate);

        frameOpen = command->frameOpen ? true : false;
        lcValid = Avc444GpuCompositorImpl::CommandLcIsValid(command);
        if (!lcValid) {
            invalidLcRejects_++;
        }
        lastFrameId_ = command->frameId;
        lastLC_ = command->LC;
        lastStream1Bytes_ = command->stream1.length;
        lastStream2Bytes_ = command->LC == 0 ? command->stream2.length : 0;
        lastTargetWidth_ = command->targetWidth;
        lastTargetHeight_ = command->targetHeight;
    }

    if (shouldLogCommand || !frameOpen || !lcValid) {
        const bool hasStream2 = command->LC == 0;
        const std::string stream2Text = hasStream2 ?
            "bytes:" + std::to_string(command->stream2.length) +
                ",rects:" + std::to_string(command->stream2.numRegionRects) +
                ",rectPreview:" +
                RectPreviewText(command->stream2.regionRects, command->stream2.numRegionRects) :
            "unused";
        Log("AVC444 GPU compositor candidate: index=" + std::to_string(candidate) +
            " codec=" + std::to_string(command->codecId) +
            " surface=" + std::to_string(command->surfaceId) +
            " frame=" + std::to_string(command->frameId) +
            " frameOpen=" + std::string(frameOpen ? "yes" : "no") +
            " LC=" + std::to_string(command->LC) +
            " lcValid=" + std::string(lcValid ? "yes" : "no") +
            " commandOrigin=" + std::to_string(command->left) + "," +
            std::to_string(command->top) +
            " surfaceSize=" +
            std::to_string(command->width) + "x" + std::to_string(command->height) +
            " target=" + std::to_string(command->targetWidth) + "x" +
            std::to_string(command->targetHeight) +
            " stream1=bytes:" + std::to_string(command->stream1.length) +
            ",rects:" + std::to_string(command->stream1.numRegionRects) +
            ",rectPreview:" +
            RectPreviewText(command->stream1.regionRects, command->stream1.numRegionRects) +
            " stream2=" + stream2Text);
    }

    Avc444GpuCompositorCallbacks callbacks;
    bool outputActive = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = callbacks_;
        outputActive = outputActive_;
    }

    if (!lcValid) {
        return false;
    }

    if (outputActive && EnqueueSurfaceCommand(command, callbacks, outputActive)) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc444 gpu compositor: enabled=yes active=yes candidates=" +
                std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " suppress=queued-worker";
        }
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
            const RenderOutputOwnerTransition ownerTransition = TransitionRenderOutputOwner(
                RenderOutputOwner::Avc444Gpu,
                RenderOutputOwnerTransitionReason::Avc444Takeover);
            if (ownerTransition.previous != RenderOutputOwner::Avc444Gpu) {
                if (callbacks.stopRenderPipeline != nullptr) {
                    callbacks.stopRenderPipeline();
                }
                if (callbacks.releaseRenderTarget != nullptr) {
                    callbacks.releaseRenderTarget(
                        "before AVC444 GPU compositor SurfaceCommand takeover");
                }
                logs.push_back("AVC444 GPU compositor claimed render output ownership at "
                    "SurfaceCommand before suppressing FreeRDP GDI: previousOwner=" +
                    RenderOutputOwnerName(ownerTransition.previous) +
                    " transitionReason=" +
                    RenderOutputOwnerTransitionReasonName(ownerTransition.reason) +
                    " outputOwner=avc444-gpu");
            }
            outputActive = true;
            logs.push_back("AVC444 GPU compositor is authoritative after queued update; "
                "GDI is suppressed now and present is deferred until the matching frame boundary");
        }
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive_ = outputActive;
            diagnostics_ = "avc444 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
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
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc444 gpu compositor: enabled=yes active=yes candidates=" +
                std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " endFrame=" + std::to_string(frame->frameId) +
                " endFramePresent=queued-worker";
        }
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
            diagnostics_ = "avc444 gpu compositor: enabled=yes active=" +
                std::string(outputActive_ ? "yes" : "no") +
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

void Avc444GpuCompositor::Log(const std::string& message) const
{
    if (IsRoutineAvc444GpuLog(message)) {
        return;
    }

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
