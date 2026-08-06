#include "surface/latest_frame_renderer.h"

#include "common/bridge_log.h"
#include "common/frame_utils.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <iomanip>
#include <sstream>
#include <thread>
#include <utility>

namespace rdp_bridge {

struct LatestFrameRenderer::Impl {
    void SetCallbacks(RenderFrameFn renderFrame, LogFn log)
    {
        renderFrame_ = std::move(renderFrame);
        log_ = std::move(log);
    }

    void Start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }

        ResetStatsLocked();
        running_ = true;
        worker_ = std::thread([this]() { WorkerLoop(); });
    }

    void Stop()
    {
        std::thread worker;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && !worker_.joinable()) {
                hasPending_ = false;
                return;
            }
            running_ = false;
            hasPending_ = false;
            worker = std::move(worker_);
        }

        condition_.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool DropPending(const std::string& reason, std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!hasPending_) {
            message = "no pending render frame";
            return false;
        }

        hasPending_ = false;
        ++replacedCount_;
        message = "pending render frame dropped after " + reason;
        return true;
    }

    bool Enqueue(const RgbaFrame& frame, std::string& message, bool forceRender)
    {
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            message = "invalid frame";
            return false;
        }

        PendingFrame next;
        next.width = frame.width;
        next.height = frame.height;
        next.strideBytes = sourceStride;
        next.label = frame.label.empty() ? "freerdp gdi queued" : frame.label + " queued";
        next.data = frame.data;
        next.dirty = frame.dirty;
        next.forceRender = forceRender;
        next.sequence = 0;
        next.dirtySequenceStart = 0;
        const uint32_t copyUs = 0;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                message = "render thread stopped";
                return false;
            }
            next.sequence = ++frameSequence_;
            next.dirtySequenceStart = next.sequence;
            if (hasPending_) {
                next.forceRender = next.forceRender || pending_.forceRender;
                if (pending_.width == next.width && pending_.height == next.height) {
                    next.dirty = MergeDirtyStats(pending_.dirty, next.dirty, next.width, next.height);
                    if (pending_.dirtySequenceStart > 0) {
                        next.dirtySequenceStart = pending_.dirtySequenceStart;
                    }
                }
                ++replacedCount_;
            }
            lastCopyUs_ = copyUs;
            totalCopyUs_ += copyUs;
            lastDirty_ = next.dirty;
            ++queuedCount_;
            message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                " latest-gdi " + DescribeDirtyStats(frame.dirty);
            pending_ = std::move(next);
            hasPending_ = true;
        }

        condition_.notify_one();
        return true;
    }

    RenderStatsSnapshot Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RenderStatsSnapshot snapshot;
        snapshot.running = running_;
        snapshot.queued = queuedCount_;
        snapshot.rendered = renderedCount_;
        snapshot.failed = failedCount_;
        snapshot.replaced = replacedCount_;
        snapshot.throttled = throttledCount_;
        snapshot.fullRendered = fullRenderedCount_;
        snapshot.partialRendered = partialRenderedCount_;
        snapshot.pending = hasPending_ ? 1U : 0U;
        snapshot.lastWidth = lastRenderedWidth_;
        snapshot.lastHeight = lastRenderedHeight_;
        snapshot.lastCopyUs = lastCopyUs_;
        snapshot.lastRenderUs = lastRenderUs_;
        snapshot.avgCopyUs = queuedCount_ == 0 ? 0 :
            static_cast<uint32_t>(totalCopyUs_ / queuedCount_);
        snapshot.avgRenderUs = renderedCount_ == 0 ? 0 :
            static_cast<uint32_t>(totalRenderUs_ / renderedCount_);
        snapshot.lastDirty = lastDirty_;
        snapshot.targetFrameIntervalMs = lastTargetFrameIntervalMs_;
        const uint64_t elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - statsStartedAt_).count();
        if (elapsedMs > 0) {
            snapshot.fpsX100 = static_cast<uint32_t>((renderedCount_ * 100000ULL) / elapsedMs);
        }
        return snapshot;
    }

    std::string BuildStatsLog()
    {
        const RenderStatsSnapshot stats = Snapshot();
        std::ostringstream out;
        out << "render "
            << (stats.running ? "running" : "stopped")
            << " queued=" << stats.queued
            << " rendered=" << stats.rendered
            << " failed=" << stats.failed
            << " replaced=" << stats.replaced
            << " paced=" << stats.throttled
            << " full=" << stats.fullRendered
            << " partial=" << stats.partialRendered
            << " paceMs=" << stats.targetFrameIntervalMs
            << " pending=" << stats.pending
            << " fps=" << (stats.fpsX100 / 100) << "."
            << std::setw(2) << std::setfill('0') << (stats.fpsX100 % 100)
            << std::setfill(' ')
            << " copyMs=" << (stats.lastCopyUs / 1000) << "."
            << std::setw(3) << std::setfill('0') << (stats.lastCopyUs % 1000)
            << std::setfill(' ')
            << " avgCopyMs=" << (stats.avgCopyUs / 1000) << "."
            << std::setw(3) << std::setfill('0') << (stats.avgCopyUs % 1000)
            << std::setfill(' ')
            << " renderMs=" << (stats.lastRenderUs / 1000) << "."
            << std::setw(3) << std::setfill('0') << (stats.lastRenderUs % 1000)
            << std::setfill(' ')
            << " avgRenderMs=" << (stats.avgRenderUs / 1000) << "."
            << std::setw(3) << std::setfill('0') << (stats.avgRenderUs % 1000)
            << std::setfill(' ')
            << " last=" << stats.lastWidth << "x" << stats.lastHeight
            << " " << DescribeDirtyStats(stats.lastDirty);
        return out.str();
    }

private:
    struct PendingFrame {
        const uint8_t* data = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        int32_t strideBytes = 0;
        std::string label;
        bool forceRender = false;
        DirtyFrameStats dirty;
        uint64_t sequence = 0;
        uint64_t dirtySequenceStart = 0;
    };

    static constexpr uint32_t kTargetFrameIntervalMs = 16;
    static constexpr uint32_t kLargeDirtyFrameIntervalMs = 16;
    static constexpr uint32_t kLargeDirtyAreaPermille = 700;

    static uint32_t ResolveTargetFrameIntervalMs(const PendingFrame& frame, bool sizeChanged)
    {
        if (frame.forceRender || sizeChanged) {
            return 0;
        }
        if (frame.dirty.valid && frame.dirty.areaPermille >= kLargeDirtyAreaPermille) {
            return kLargeDirtyFrameIntervalMs;
        }
        return kTargetFrameIntervalMs;
    }

    void ResetStatsLocked()
    {
        hasPending_ = false;
        queuedCount_ = 0;
        renderedCount_ = 0;
        failedCount_ = 0;
        replacedCount_ = 0;
        throttledCount_ = 0;
        fullRenderedCount_ = 0;
        partialRenderedCount_ = 0;
        totalCopyUs_ = 0;
        totalRenderUs_ = 0;
        lastCopyUs_ = 0;
        lastRenderUs_ = 0;
        lastDirty_ = DirtyFrameStats{};
        lastRenderedWidth_ = 0;
        lastRenderedHeight_ = 0;
        frameSequence_ = 0;
        lastTargetFrameIntervalMs_ = 0;
        lastRenderFinishedAt_ = std::chrono::steady_clock::time_point{};
        statsStartedAt_ = std::chrono::steady_clock::now();
    }

    SurfacePaintResult RenderFrame(const RgbaFrame& frame)
    {
        if (!renderFrame_) {
            return {false, false, "render callback is not configured", {}};
        }
        return renderFrame_(frame);
    }

    void EmitLog(const std::string& line)
    {
        if (log_) {
            log_(line);
        } else {
            BridgeLogger::Debug(line);
        }
    }

    void WorkerLoop()
    {
        for (;;) {
            PendingFrame frame;
            uint32_t targetFrameIntervalMs = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() { return !running_ || hasPending_; });
                for (;;) {
                    if (!running_ && !hasPending_) {
                        return;
                    }
                    if (!hasPending_) {
                        condition_.wait(lock, [this]() { return !running_ || hasPending_; });
                        continue;
                    }

                    const bool forceRender = pending_.forceRender;
                    const bool sizeChanged = pending_.width != lastRenderedWidth_ ||
                        pending_.height != lastRenderedHeight_;
                    targetFrameIntervalMs = ResolveTargetFrameIntervalMs(pending_, sizeChanged);
                    lastTargetFrameIntervalMs_ = targetFrameIntervalMs;
                    if (!forceRender && !sizeChanged && targetFrameIntervalMs > 0 && renderedCount_ > 0 &&
                        lastRenderFinishedAt_ != std::chrono::steady_clock::time_point{}) {
                        const auto nextRenderAt = lastRenderFinishedAt_ +
                            std::chrono::milliseconds(targetFrameIntervalMs);
                        const auto now = std::chrono::steady_clock::now();
                        if (now < nextRenderAt) {
                            ++throttledCount_;
                            condition_.wait_until(lock, nextRenderAt);
                            continue;
                        }
                    }

                    frame = std::move(pending_);
                    hasPending_ = false;
                    break;
                }
            }

            RgbaFrame view = {
                frame.data,
                frame.width,
                frame.height,
                frame.strideBytes,
                frame.label,
                frame.dirty,
                frame.sequence,
                frame.dirtySequenceStart,
            };
            const bool forcedRender = frame.forceRender;
            const auto renderStart = std::chrono::steady_clock::now();
            SurfacePaintResult paint = RenderFrame(view);
            const auto renderEnd = std::chrono::steady_clock::now();
            const uint32_t renderUs = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart).count());

            uint64_t rendered = 0;
            uint64_t failed = 0;
            uint64_t partialRendered = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                lastRenderUs_ = renderUs;
                lastRenderedWidth_ = frame.width;
                lastRenderedHeight_ = frame.height;
                lastRenderFinishedAt_ = renderEnd;
                if (paint.ok) {
                    ++renderedCount_;
                    if (paint.partial) {
                        ++partialRenderedCount_;
                    } else {
                        ++fullRenderedCount_;
                    }
                    totalRenderUs_ += renderUs;
                } else {
                    ++failedCount_;
                }
                rendered = renderedCount_;
                failed = failedCount_;
                partialRendered = partialRenderedCount_;
            }

            if (paint.ok) {
                if (forcedRender || rendered == 1 || rendered % 600 == 0 ||
                    (paint.partial && (partialRendered == 1 || partialRendered % 600 == 0))) {
                    EmitLog("Render thread painted frame " + std::to_string(rendered) +
                        " render=" + std::to_string(renderUs / 1000.0) + "ms pace=" +
                        std::to_string(targetFrameIntervalMs) + "ms " + paint.message);
                }
            } else if (failed == 1 || failed % 300 == 0) {
                EmitLog("Render thread paint failed: " + paint.message);
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_;
    RenderFrameFn renderFrame_;
    LogFn log_;
    PendingFrame pending_;
    bool running_ = false;
    bool hasPending_ = false;
    uint64_t queuedCount_ = 0;
    uint64_t renderedCount_ = 0;
    uint64_t failedCount_ = 0;
    uint64_t replacedCount_ = 0;
    uint64_t throttledCount_ = 0;
    uint64_t fullRenderedCount_ = 0;
    uint64_t partialRenderedCount_ = 0;
    uint64_t totalCopyUs_ = 0;
    uint64_t totalRenderUs_ = 0;
    uint32_t lastCopyUs_ = 0;
    uint32_t lastRenderUs_ = 0;
    DirtyFrameStats lastDirty_;
    uint32_t lastRenderedWidth_ = 0;
    uint32_t lastRenderedHeight_ = 0;
    uint64_t frameSequence_ = 0;
    uint32_t lastTargetFrameIntervalMs_ = 0;
    std::chrono::steady_clock::time_point lastRenderFinishedAt_;
    std::chrono::steady_clock::time_point statsStartedAt_ = std::chrono::steady_clock::now();
};

LatestFrameRenderer::LatestFrameRenderer() : impl_(std::make_unique<Impl>()) {}

LatestFrameRenderer::~LatestFrameRenderer()
{
    Stop();
}

void LatestFrameRenderer::SetCallbacks(RenderFrameFn renderFrame, LogFn log)
{
    impl_->SetCallbacks(std::move(renderFrame), std::move(log));
}

void LatestFrameRenderer::Start()
{
    impl_->Start();
}

void LatestFrameRenderer::Stop()
{
    impl_->Stop();
}

bool LatestFrameRenderer::DropPending(const std::string& reason, std::string& message)
{
    return impl_->DropPending(reason, message);
}

bool LatestFrameRenderer::Enqueue(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    return impl_->Enqueue(frame, message, forceRender);
}

RenderStatsSnapshot LatestFrameRenderer::Snapshot()
{
    return impl_->Snapshot();
}

std::string LatestFrameRenderer::BuildStatsLog()
{
    return impl_->BuildStatsLog();
}

} // namespace rdp_bridge
