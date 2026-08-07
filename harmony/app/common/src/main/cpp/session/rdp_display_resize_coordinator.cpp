#include "session/rdp_display_resize_coordinator.h"

#include <utility>

namespace rdp_bridge {

RdpDisplayResizeCoordinator::RdpDisplayResizeCoordinator(std::chrono::milliseconds sentTimeout)
    : sentTimeout_(sentTimeout), worker_(&RdpDisplayResizeCoordinator::WorkerMain, this)
{
}

RdpDisplayResizeCoordinator::~RdpDisplayResizeCoordinator()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        ++targetGeneration_;
        state_ = DisplayResizeWaitState::Idle;
    }
    condition_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void RdpDisplayResizeCoordinator::SetTimeoutCallback(TimeoutCallback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    timeoutCallback_ = std::move(callback);
}

void RdpDisplayResizeCoordinator::Reset(const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++targetGeneration_;
        state_ = DisplayResizeWaitState::Idle;
        targetWidth_ = 0;
        targetHeight_ = 0;
        skippedFrameCount_ = 0;
        reason_ = reason;
    }
    condition_.notify_all();
}

void RdpDisplayResizeCoordinator::ApplyResult(const DisplayResizeResult& result,
    const std::string& reason)
{
    if (result.status == DisplayResizeStatus::Unchanged) {
        return;
    }
    if (result.status == DisplayResizeStatus::Sent) {
        const uint32_t width = result.sentWidth > 0 ? result.sentWidth : result.normalizedWidth;
        const uint32_t height = result.sentHeight > 0 ? result.sentHeight : result.normalizedHeight;
        if (width > 0 && height > 0) {
            BeginSent(width, height, reason);
            return;
        }
    }

    UseFallback(reason + " status=" + DisplayResizeStatusName(result.status));
}

void RdpDisplayResizeCoordinator::BeginSent(uint32_t width, uint32_t height,
    const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++targetGeneration_;
        state_ = DisplayResizeWaitState::WaitingForTarget;
        targetWidth_ = width;
        targetHeight_ = height;
        skippedFrameCount_ = 0;
        reason_ = reason;
        deadline_ = std::chrono::steady_clock::now() + sentTimeout_;
    }
    condition_.notify_all();
}

void RdpDisplayResizeCoordinator::UseFallback(const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++targetGeneration_;
        state_ = DisplayResizeWaitState::Fallback;
        targetWidth_ = 0;
        targetHeight_ = 0;
        skippedFrameCount_ = 0;
        reason_ = reason;
    }
    condition_.notify_all();
}

bool RdpDisplayResizeCoordinator::ShouldQueueFrame(uint32_t width, uint32_t height,
    const std::string& label, std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (state_ != DisplayResizeWaitState::WaitingForTarget) {
        return true;
    }

    if (FrameMatchesTarget(width, height)) {
        state_ = DisplayResizeWaitState::Idle;
        skippedFrameCount_ = 0;
        message = "resize target accepted by frame: generation=" +
            std::to_string(targetGeneration_) + " target=" + std::to_string(targetWidth_) +
            "x" + std::to_string(targetHeight_) + " frame=" + std::to_string(width) + "x" +
            std::to_string(height) + " label=" + label;
        condition_.notify_all();
        return true;
    }

    ++skippedFrameCount_;
    message = "resize waiting: generation=" + std::to_string(targetGeneration_) + " target=" +
        std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) + " skippedFrame=" +
        std::to_string(width) + "x" + std::to_string(height) + " label=" + label + " skipped=" +
        std::to_string(skippedFrameCount_);
    return false;
}

bool RdpDisplayResizeCoordinator::IsFallbackGeneration(uint64_t generation) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_ == DisplayResizeWaitState::Fallback && targetGeneration_ == generation;
}

DisplayResizeCoordinatorSnapshot RdpDisplayResizeCoordinator::Snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return {state_, targetGeneration_, targetWidth_, targetHeight_, skippedFrameCount_, reason_};
}

bool RdpDisplayResizeCoordinator::FrameMatchesTarget(uint32_t width, uint32_t height) const
{
    if (width == 0 || height == 0) {
        return false;
    }
    const uint32_t widthDelta = width > targetWidth_ ? width - targetWidth_ : targetWidth_ - width;
    const uint32_t heightDelta = height > targetHeight_ ? height - targetHeight_ :
        targetHeight_ - height;
    return widthDelta < kFrameAlignmentTolerance && heightDelta < kFrameAlignmentTolerance;
}

void RdpDisplayResizeCoordinator::WorkerMain()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopping_) {
        condition_.wait(lock, [this]() {
            return stopping_ || state_ == DisplayResizeWaitState::WaitingForTarget;
        });
        if (stopping_) {
            break;
        }

        const uint64_t generation = targetGeneration_;
        const auto deadline = deadline_;
        const bool interrupted = condition_.wait_until(lock, deadline, [this, generation]() {
            return stopping_ || state_ != DisplayResizeWaitState::WaitingForTarget ||
                targetGeneration_ != generation;
        });
        if (interrupted) {
            continue;
        }

        state_ = DisplayResizeWaitState::Fallback;
        reason_ = "resize sent timeout after " + std::to_string(sentTimeout_.count()) + "ms";
        const std::string timeoutReason = reason_;
        const TimeoutCallback callback = timeoutCallback_;
        lock.unlock();
        if (callback) {
            callback(generation, timeoutReason);
        }
        lock.lock();
    }
}

} // namespace rdp_bridge
