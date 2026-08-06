#pragma once

#include "client/OHOS/ohos_display.h"

#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace rdp_bridge {

class RdpDisplayLayoutMonitor {
public:
    using LayoutCallback = std::function<void(uint32_t,
        const std::vector<FREERDP_OHOS_MONITOR_LAYOUT>&, const std::string&)>;
    using LogCallback = std::function<void(const std::string&)>;

    ~RdpDisplayLayoutMonitor();
    bool Start(LayoutCallback layoutCallback, LogCallback logCallback, std::string& message);
    void Stop();

private:
    static void OnDisplayChanged(uint64_t displayId);
    static void OnDisplayAdded(uint64_t displayId);
    static void OnDisplayRemoved(uint64_t displayId);
    void HandleChange(const std::string& source, uint64_t displayId);
    bool Refresh(const std::string& source, std::string& message);

    std::mutex mutex_;
    uint32_t changeListener_ = 0;
    uint32_t addListener_ = 0;
    uint32_t removeListener_ = 0;
    bool started_ = false;
    bool hasSnapshot_ = false;
    uint32_t lastOrientation_ = 0;
    std::vector<FREERDP_OHOS_MONITOR_LAYOUT> lastLayout_;
    LayoutCallback layoutCallback_;
    LogCallback logCallback_;
};

} // namespace rdp_bridge
