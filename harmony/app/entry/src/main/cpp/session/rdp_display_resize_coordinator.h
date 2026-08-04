#pragma once

#include "session/rdp_display_resize_types.h"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>

namespace rdp_bridge {

enum class DisplayResizeWaitState {
    Idle,
    WaitingForTarget,
    Fallback,
};

struct DisplayResizeCoordinatorSnapshot {
    DisplayResizeWaitState state = DisplayResizeWaitState::Idle;
    uint64_t targetGeneration = 0;
    uint32_t targetWidth = 0;
    uint32_t targetHeight = 0;
    uint32_t skippedFrameCount = 0;
    std::string reason;
};

class RdpDisplayResizeCoordinator {
public:
    using TimeoutCallback = std::function<void(uint64_t, const std::string&)>;

    explicit RdpDisplayResizeCoordinator(
        std::chrono::milliseconds sentTimeout = std::chrono::milliseconds(2000));
    ~RdpDisplayResizeCoordinator();

    RdpDisplayResizeCoordinator(const RdpDisplayResizeCoordinator&) = delete;
    RdpDisplayResizeCoordinator& operator=(const RdpDisplayResizeCoordinator&) = delete;

    void SetTimeoutCallback(TimeoutCallback callback);
    void Reset(const std::string& reason);
    void ApplyResult(const DisplayResizeResult& result, const std::string& reason);
    bool ShouldQueueFrame(uint32_t width, uint32_t height, const std::string& label,
        std::string& message);
    bool IsFallbackGeneration(uint64_t generation) const;
    DisplayResizeCoordinatorSnapshot Snapshot() const;

private:
    static constexpr uint32_t kFrameAlignmentTolerance = 16;

    void BeginSent(uint32_t width, uint32_t height, const std::string& reason);
    void UseFallback(const std::string& reason);
    bool FrameMatchesTarget(uint32_t width, uint32_t height) const;
    void WorkerMain();

    const std::chrono::milliseconds sentTimeout_;
    mutable std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_ = false;
    DisplayResizeWaitState state_ = DisplayResizeWaitState::Idle;
    uint64_t targetGeneration_ = 0;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    uint32_t skippedFrameCount_ = 0;
    std::string reason_;
    std::chrono::steady_clock::time_point deadline_;
    TimeoutCallback timeoutCallback_;
    std::thread worker_;
};

} // namespace rdp_bridge
