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

class RdpDisplayRequestCoalescer {
public:
    using Callback = std::function<void(const DisplayResizeRequest&)>;

    explicit RdpDisplayRequestCoalescer(
        std::chrono::milliseconds delay = std::chrono::milliseconds(200));
    ~RdpDisplayRequestCoalescer();

    RdpDisplayRequestCoalescer(const RdpDisplayRequestCoalescer&) = delete;
    RdpDisplayRequestCoalescer& operator=(const RdpDisplayRequestCoalescer&) = delete;

    void SetCallback(Callback callback);
    void Schedule(DisplayResizeRequest request);
    void Flush(DisplayResizeRequest request);
    void Cancel();

private:
    void WorkerMain();

    const std::chrono::milliseconds delay_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool stopping_ = false;
    bool pending_ = false;
    uint64_t generation_ = 0;
    DisplayResizeRequest request_;
    Callback callback_;
    std::thread worker_;
};

} // namespace rdp_bridge
