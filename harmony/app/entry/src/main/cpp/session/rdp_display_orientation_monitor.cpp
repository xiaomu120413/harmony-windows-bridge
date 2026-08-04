#include "session/rdp_display_orientation_monitor.h"

#include <atomic>
#include <limits>
#include <utility>

#include <freerdp/settings_types.h>
#include <window_manager/oh_display_manager.h>

namespace rdp_bridge {
namespace {

std::atomic<RdpDisplayOrientationMonitor*> g_activeMonitor{nullptr};

bool ToRdpOrientation(NativeDisplayManager_Orientation orientation, uint32_t& rdpOrientation)
{
    switch (orientation) {
        case DISPLAY_MANAGER_LANDSCAPE:
            rdpOrientation = ORIENTATION_LANDSCAPE;
            return true;
        case DISPLAY_MANAGER_PORTRAIT:
            rdpOrientation = ORIENTATION_PORTRAIT;
            return true;
        case DISPLAY_MANAGER_LANDSCAPE_INVERTED:
            rdpOrientation = ORIENTATION_LANDSCAPE_FLIPPED;
            return true;
        case DISPLAY_MANAGER_PORTRAIT_INVERTED:
            rdpOrientation = ORIENTATION_PORTRAIT_FLIPPED;
            return true;
        case DISPLAY_MANAGER_UNKNOWN:
        default:
            return false;
    }
}

} // namespace

RdpDisplayOrientationMonitor::RdpDisplayOrientationMonitor() = default;

RdpDisplayOrientationMonitor::~RdpDisplayOrientationMonitor()
{
    Stop();
}

bool RdpDisplayOrientationMonitor::Start(OrientationCallback orientationCallback,
    LogCallback logCallback, std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    orientationCallback_ = std::move(orientationCallback);
    logCallback_ = std::move(logCallback);
    if (started_) {
        message = "native display orientation monitor already started";
        return true;
    }

    uint64_t defaultDisplayId = 0;
    const NativeDisplayManager_ErrorCode defaultResult =
        OH_NativeDisplayManager_GetDefaultDisplayId(&defaultDisplayId);
    if (defaultResult != DISPLAY_MANAGER_OK ||
        defaultDisplayId > std::numeric_limits<uint32_t>::max()) {
        message = "native default display id unavailable: rc=" +
            std::to_string(static_cast<int32_t>(defaultResult));
        return false;
    }
    activeDisplayId_ = static_cast<uint32_t>(defaultDisplayId);
    g_activeMonitor.store(this);
    const NativeDisplayManager_ErrorCode registerResult =
        OH_NativeDisplayManager_RegisterDisplayChangeListener(OnDisplayChanged, &listenerIndex_);
    if (registerResult != DISPLAY_MANAGER_OK) {
        g_activeMonitor.store(nullptr);
        message = "native display listener registration failed: rc=" +
            std::to_string(static_cast<int32_t>(registerResult));
        return false;
    }
    started_ = true;
    message = "native display orientation monitor started";
    return true;
}

void RdpDisplayOrientationMonitor::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) {
        return;
    }
    (void)OH_NativeDisplayManager_UnregisterDisplayChangeListener(listenerIndex_);
    if (g_activeMonitor.load() == this) {
        g_activeMonitor.store(nullptr);
    }
    started_ = false;
    orientationCallback_ = nullptr;
    logCallback_ = nullptr;
}

bool RdpDisplayOrientationMonitor::SetActiveDisplayId(uint32_t displayId, std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activeDisplayId_ = displayId;
    }
    return Refresh("host_window", message);
}

void RdpDisplayOrientationMonitor::OnDisplayChanged(uint64_t displayId)
{
    RdpDisplayOrientationMonitor* monitor = g_activeMonitor.load();
    if (monitor != nullptr) {
        monitor->HandleDisplayChanged(displayId);
    }
}

void RdpDisplayOrientationMonitor::HandleDisplayChanged(uint64_t displayId)
{
    uint32_t activeDisplayId = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        activeDisplayId = activeDisplayId_;
    }
    if (displayId != activeDisplayId) {
        return;
    }
    std::string message;
    if (!Refresh("native_display_change", message)) {
        LogCallback log;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            log = logCallback_;
        }
        if (log != nullptr) {
            log(message);
        }
    }
}

bool RdpDisplayOrientationMonitor::Refresh(const std::string& source, std::string& message)
{
    uint32_t displayId = 0;
    OrientationCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        displayId = activeDisplayId_;
        callback = orientationCallback_;
    }

    NativeDisplayManager_DisplayInfo* displayInfo = nullptr;
    const NativeDisplayManager_ErrorCode result =
        OH_NativeDisplayManager_CreateDisplayById(displayId, &displayInfo);
    if (result != DISPLAY_MANAGER_OK || displayInfo == nullptr) {
        message = "native display query failed: displayId=" + std::to_string(displayId) +
            " rc=" + std::to_string(static_cast<int32_t>(result));
        return false;
    }

    uint32_t rdpOrientation = ORIENTATION_LANDSCAPE;
    const NativeDisplayManager_Orientation nativeOrientation = displayInfo->orientation;
    const bool valid = ToRdpOrientation(nativeOrientation, rdpOrientation);
    OH_NativeDisplayManager_DestroyDisplay(displayInfo);
    if (!valid) {
        message = "native display orientation unknown: displayId=" + std::to_string(displayId);
        return false;
    }

    message = "native display orientation updated: displayId=" + std::to_string(displayId) +
        " nativeOrientation=" + std::to_string(static_cast<int32_t>(nativeOrientation)) +
        " rdpOrientation=" + std::to_string(rdpOrientation);
    if (callback != nullptr) {
        callback(displayId, rdpOrientation, source);
    }
    return true;
}

} // namespace rdp_bridge
