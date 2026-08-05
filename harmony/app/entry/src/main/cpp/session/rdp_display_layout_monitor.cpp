#include "session/rdp_display_layout_monitor.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>

#include <freerdp/settings_types.h>
#include <window_manager/oh_display_manager.h>

namespace rdp_bridge {
namespace {

std::atomic<RdpDisplayLayoutMonitor*> g_activeLayoutMonitor{nullptr};

uint32_t ToRdpOrientation(NativeDisplayManager_Orientation orientation)
{
    switch (orientation) {
        case DISPLAY_MANAGER_PORTRAIT:
            return ORIENTATION_PORTRAIT;
        case DISPLAY_MANAGER_LANDSCAPE_INVERTED:
            return ORIENTATION_LANDSCAPE_FLIPPED;
        case DISPLAY_MANAGER_PORTRAIT_INVERTED:
            return ORIENTATION_PORTRAIT_FLIPPED;
        case DISPLAY_MANAGER_LANDSCAPE:
        case DISPLAY_MANAGER_UNKNOWN:
        default:
            return ORIENTATION_LANDSCAPE;
    }
}

uint32_t PhysicalMillimeters(int32_t pixels, float dpi)
{
    const double effectiveDpi = std::isfinite(dpi) && dpi > 0.0f ? dpi : 160.0;
    const long value = std::lround(static_cast<double>(std::max(pixels, 1)) * 25.4 /
        effectiveDpi);
    return static_cast<uint32_t>(std::clamp(value, 10L, 10000L));
}

} // namespace

RdpDisplayLayoutMonitor::~RdpDisplayLayoutMonitor()
{
    Stop();
}

bool RdpDisplayLayoutMonitor::Start(LayoutCallback layoutCallback, LogCallback logCallback,
    std::string& message)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        layoutCallback_ = std::move(layoutCallback);
        logCallback_ = std::move(logCallback);
        if (started_) {
            message = "native display layout monitor already started";
            return true;
        }
        g_activeLayoutMonitor.store(this);
        const bool changeRegistered = OH_NativeDisplayManager_RegisterDisplayChangeListener(
            OnDisplayChanged, &changeListener_) == DISPLAY_MANAGER_OK;
        const bool addRegistered = changeRegistered &&
            OH_NativeDisplayManager_RegisterDisplayAddListener(OnDisplayAdded,
                &addListener_) == DISPLAY_MANAGER_OK;
        const bool removeRegistered = addRegistered &&
            OH_NativeDisplayManager_RegisterDisplayRemoveListener(OnDisplayRemoved,
                &removeListener_) == DISPLAY_MANAGER_OK;
        if (!removeRegistered) {
            if (changeRegistered) {
                (void)OH_NativeDisplayManager_UnregisterDisplayChangeListener(changeListener_);
            }
            if (addRegistered) {
                (void)OH_NativeDisplayManager_UnregisterDisplayAddListener(addListener_);
            }
            g_activeLayoutMonitor.store(nullptr);
            message = "native display layout listener registration failed";
            return false;
        }
        started_ = true;
    }
    std::string refreshMessage;
    const bool refreshed = Refresh("native_display_initial", refreshMessage);
    if (!refreshMessage.empty() && logCallback_ != nullptr) {
        logCallback_(refreshMessage);
    }
    message = refreshed ? "native display layout monitor started" : refreshMessage;
    return refreshed;
}

void RdpDisplayLayoutMonitor::Stop()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!started_) {
        return;
    }
    (void)OH_NativeDisplayManager_UnregisterDisplayChangeListener(changeListener_);
    (void)OH_NativeDisplayManager_UnregisterDisplayAddListener(addListener_);
    (void)OH_NativeDisplayManager_UnregisterDisplayRemoveListener(removeListener_);
    if (g_activeLayoutMonitor.load() == this) {
        g_activeLayoutMonitor.store(nullptr);
    }
    started_ = false;
}

void RdpDisplayLayoutMonitor::OnDisplayChanged(uint64_t displayId)
{
    if (auto* monitor = g_activeLayoutMonitor.load(); monitor != nullptr) {
        monitor->HandleChange("native_display_change", displayId);
    }
}

void RdpDisplayLayoutMonitor::OnDisplayAdded(uint64_t displayId)
{
    if (auto* monitor = g_activeLayoutMonitor.load(); monitor != nullptr) {
        monitor->HandleChange("native_display_add", displayId);
    }
}

void RdpDisplayLayoutMonitor::OnDisplayRemoved(uint64_t displayId)
{
    if (auto* monitor = g_activeLayoutMonitor.load(); monitor != nullptr) {
        monitor->HandleChange("native_display_remove", displayId);
    }
}

void RdpDisplayLayoutMonitor::HandleChange(const std::string& source, uint64_t displayId)
{
    std::string message;
    if (!Refresh(source, message)) {
        LogCallback log;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            log = logCallback_;
        }
        if (log != nullptr) {
            log(message + " displayId=" + std::to_string(displayId));
        }
    }
}

bool RdpDisplayLayoutMonitor::Refresh(const std::string& source, std::string& message)
{
    NativeDisplayManager_DisplayInfo* primary = nullptr;
    NativeDisplayManager_DisplaysInfo* displays = nullptr;
    if (OH_NativeDisplayManager_CreatePrimaryDisplay(&primary) != DISPLAY_MANAGER_OK ||
        primary == nullptr ||
        OH_NativeDisplayManager_CreateAllDisplays(&displays) != DISPLAY_MANAGER_OK ||
        displays == nullptr) {
        if (primary != nullptr) {
            OH_NativeDisplayManager_DestroyDisplay(primary);
        }
        message = "native display layout query failed";
        return false;
    }

    const uint32_t primaryId = primary->id;
    const uint32_t primaryOrientation = ToRdpOrientation(primary->orientation);
    std::vector<FREERDP_OHOS_MONITOR_LAYOUT> layout;
    layout.reserve(std::min(displays->displaysLength, FREERDP_OHOS_MAX_MONITORS));
    for (uint32_t index = 0; index < displays->displaysLength &&
        layout.size() < FREERDP_OHOS_MAX_MONITORS; ++index) {
        const NativeDisplayManager_DisplayInfo& display = displays->displaysInfo[index];
        if (!display.isAlive || display.width <= 0 || display.height <= 0) {
            continue;
        }
        int32_t left = 0;
        int32_t top = 0;
        if (OH_NativeDisplayManager_GetDisplayPosition(display.id, &left, &top) !=
            DISPLAY_MANAGER_OK && display.id != primaryId) {
            continue;
        }
        layout.push_back({
            sizeof(FREERDP_OHOS_MONITOR_LAYOUT), FREERDP_OHOS_MONITOR_LAYOUT_VERSION,
            left, top, static_cast<uint32_t>(display.width),
            static_cast<uint32_t>(display.height),
            PhysicalMillimeters(display.width, display.xDPI > 0.0f ? display.xDPI : display.densityDPI),
            PhysicalMillimeters(display.height, display.yDPI > 0.0f ? display.yDPI : display.densityDPI),
            ToRdpOrientation(display.orientation), 100, 100,
            display.id == primaryId ? TRUE : FALSE,
        });
    }
    OH_NativeDisplayManager_DestroyAllDisplays(displays);
    OH_NativeDisplayManager_DestroyDisplay(primary);
    if (layout.empty()) {
        message = "native display layout has no alive display";
        return false;
    }
    std::stable_sort(layout.begin(), layout.end(), [](const auto& left, const auto& right) {
        return left.primary > right.primary;
    });

    LayoutCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callback = layoutCallback_;
    }
    if (callback != nullptr) {
        callback(primaryOrientation, layout, source);
    }
    message = "native display layout updated: count=" + std::to_string(layout.size()) +
        " primaryId=" + std::to_string(primaryId);
    return true;
}

} // namespace rdp_bridge
