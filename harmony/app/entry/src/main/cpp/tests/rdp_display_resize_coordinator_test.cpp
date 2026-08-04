#include "session/rdp_display_resize_coordinator.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

namespace rdp_bridge {

DisplayResizeResult Sent(uint32_t width, uint32_t height)
{
    DisplayResizeResult result;
    result.status = DisplayResizeStatus::Sent;
    result.normalizedWidth = width;
    result.normalizedHeight = height;
    result.sentWidth = width;
    result.sentHeight = height;
    return result;
}

} // namespace rdp_bridge

int main()
{
    using namespace rdp_bridge;
    using namespace std::chrono_literals;

    RdpDisplayResizeCoordinator coordinator(30ms);
    std::mutex callbackMutex;
    std::condition_variable callbackCondition;
    std::atomic_uint32_t timeoutCount {0};
    coordinator.SetTimeoutCallback([&](uint64_t, const std::string&) {
        ++timeoutCount;
        callbackCondition.notify_all();
    });

    DisplayResizeResult deferred;
    deferred.status = DisplayResizeStatus::Deferred;
    coordinator.ApplyResult(deferred, "deferred");
    std::string message;
    assert(coordinator.ShouldQueueFrame(1280, 720, "fallback", message));
    assert(coordinator.Snapshot().state == DisplayResizeWaitState::Fallback);

    coordinator.ApplyResult(Sent(1920, 1080), "sent-match");
    assert(!coordinator.ShouldQueueFrame(1280, 720, "old", message));
    assert(coordinator.ShouldQueueFrame(1920, 1080, "target", message));
    assert(coordinator.Snapshot().state == DisplayResizeWaitState::Idle);

    coordinator.ApplyResult(Sent(1600, 900), "sent-timeout");
    {
        std::unique_lock<std::mutex> lock(callbackMutex);
        callbackCondition.wait_for(lock, 500ms, [&]() {
            return timeoutCount.load() == 1;
        });
    }
    assert(timeoutCount.load() == 1);
    assert(coordinator.Snapshot().state == DisplayResizeWaitState::Fallback);
    assert(coordinator.ShouldQueueFrame(1280, 720, "timeout-fallback", message));

    coordinator.ApplyResult(Sent(1024, 768), "old-generation");
    std::this_thread::sleep_for(10ms);
    coordinator.ApplyResult(Sent(1366, 768), "new-generation");
    assert(!coordinator.ShouldQueueFrame(1024, 768, "stale-target", message));
    assert(coordinator.ShouldQueueFrame(1366, 768, "latest-target", message));
    std::this_thread::sleep_for(50ms);
    assert(timeoutCount.load() == 1);

    coordinator.ApplyResult(Sent(1440, 900), "disconnect-reset");
    coordinator.Reset("disconnected");
    std::this_thread::sleep_for(50ms);
    assert(timeoutCount.load() == 1);
    assert(coordinator.Snapshot().state == DisplayResizeWaitState::Idle);
    return 0;
}
