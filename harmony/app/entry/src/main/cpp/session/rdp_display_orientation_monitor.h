#pragma once

#include <cstdint>
#include <functional>
#include <mutex>
#include <string>

namespace rdp_bridge {

class RdpDisplayOrientationMonitor {
public:
    using OrientationCallback = std::function<void(uint32_t, uint32_t, const std::string&)>;
    using LogCallback = std::function<void(const std::string&)>;

    RdpDisplayOrientationMonitor();
    ~RdpDisplayOrientationMonitor();

    RdpDisplayOrientationMonitor(const RdpDisplayOrientationMonitor&) = delete;
    RdpDisplayOrientationMonitor& operator=(const RdpDisplayOrientationMonitor&) = delete;

    bool Start(OrientationCallback orientationCallback, LogCallback logCallback,
        std::string& message);
    void Stop();
    bool SetActiveDisplayId(uint32_t displayId, std::string& message);

private:
    static void OnDisplayChanged(uint64_t displayId);
    void HandleDisplayChanged(uint64_t displayId);
    bool Refresh(const std::string& source, std::string& message);

    std::mutex mutex_;
    uint32_t activeDisplayId_ = 0;
    uint32_t listenerIndex_ = 0;
    bool started_ = false;
    OrientationCallback orientationCallback_;
    LogCallback logCallback_;
};

} // namespace rdp_bridge
