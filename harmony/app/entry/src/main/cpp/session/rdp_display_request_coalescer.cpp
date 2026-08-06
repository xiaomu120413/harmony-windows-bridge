#include "session/rdp_display_request_coalescer.h"

#include <utility>

namespace rdp_bridge {

RdpDisplayRequestCoalescer::RdpDisplayRequestCoalescer(std::chrono::milliseconds delay)
    : delay_(delay), worker_(&RdpDisplayRequestCoalescer::WorkerMain, this)
{
}

RdpDisplayRequestCoalescer::~RdpDisplayRequestCoalescer()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        pending_ = false;
        ++generation_;
    }
    condition_.notify_all();
    if (worker_.joinable()) {
        worker_.join();
    }
}

void RdpDisplayRequestCoalescer::SetCallback(Callback callback)
{
    std::lock_guard<std::mutex> lock(mutex_);
    callback_ = std::move(callback);
}

void RdpDisplayRequestCoalescer::Schedule(DisplayResizeRequest request)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        request_ = std::move(request);
        pending_ = true;
        ++generation_;
    }
    condition_.notify_all();
}

void RdpDisplayRequestCoalescer::Flush(DisplayResizeRequest request)
{
    Callback callback;
    DisplayResizeRequest current;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        request_ = std::move(request);
        current = request_;
        pending_ = false;
        ++generation_;
        callback = callback_;
    }
    condition_.notify_all();
    if (callback) {
        callback(current);
    }
}

void RdpDisplayRequestCoalescer::Cancel()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_ = false;
        ++generation_;
    }
    condition_.notify_all();
}

void RdpDisplayRequestCoalescer::WorkerMain()
{
    std::unique_lock<std::mutex> lock(mutex_);
    while (!stopping_) {
        condition_.wait(lock, [this]() { return stopping_ || pending_; });
        if (stopping_) {
            break;
        }

        const uint64_t generation = generation_;
        const auto deadline = std::chrono::steady_clock::now() + delay_;
        if (condition_.wait_until(lock, deadline, [this, generation]() {
            return stopping_ || !pending_ || generation_ != generation;
        })) {
            continue;
        }

        DisplayResizeRequest request = request_;
        Callback callback = callback_;
        pending_ = false;
        lock.unlock();
        if (callback) {
            callback(request);
        }
        lock.lock();
    }
}

} // namespace rdp_bridge
