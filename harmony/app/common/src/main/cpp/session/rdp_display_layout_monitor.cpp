#include "session/rdp_display_layout_monitor.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <tuple>

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

uint32_t DesktopScaleFactor(float densityDpi)
{
    constexpr uint32_t supported[] = {100, 125, 150, 175, 200, 250, 300, 400, 500};
    const double dpi = std::isfinite(densityDpi) && densityDpi > 0.0f ? densityDpi : 160.0;
    const long requested = std::lround(dpi * 100.0 / 160.0);
    return *std::min_element(std::begin(supported), std::end(supported),
        [requested](uint32_t left, uint32_t right) {
            return std::abs(static_cast<long>(left) - requested) <
                std::abs(static_cast<long>(right) - requested);
        });
}

bool SameMonitor(const FREERDP_OHOS_MONITOR_LAYOUT& left,
    const FREERDP_OHOS_MONITOR_LAYOUT& right)
{
    return left.structSize == right.structSize && left.version == right.version &&
        left.left == right.left && left.top == right.top && left.width == right.width &&
        left.height == right.height && left.physicalWidth == right.physicalWidth &&
        left.physicalHeight == right.physicalHeight && left.orientation == right.orientation &&
        left.desktopScaleFactor == right.desktopScaleFactor &&
        left.deviceScaleFactor == right.deviceScaleFactor && left.primary == right.primary;
}

bool SameLayout(const std::vector<FREERDP_OHOS_MONITOR_LAYOUT>& left,
    const std::vector<FREERDP_OHOS_MONITOR_LAYOUT>& right)
{
    return left.size() == right.size() &&
        std::equal(left.begin(), left.end(), right.begin(), SameMonitor);
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
        hasSnapshot_ = false;
        lastLayout_.clear();
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
    hasSnapshot_ = false;
    lastLayout_.clear();
}

std::vector<FREERDP_OHOS_MONITOR_LAYOUT> RdpDisplayLayoutMonitor::Snapshot() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return lastLayout_;
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
            ToRdpOrientation(display.orientation), DesktopScaleFactor(display.densityDPI), 100,
            display.id == primaryId ? TRUE : FALSE,
        });
    }
    OH_NativeDisplayManager_DestroyAllDisplays(displays);
    OH_NativeDisplayManager_DestroyDisplay(primary);
    if (layout.empty()) {
        message = "native display layout has no alive display";
        return false;
    }
    std::sort(layout.begin(), layout.end(), [](const auto& left, const auto& right) {
        return std::tie(left.primary, left.left, left.top, left.width, left.height) >
            std::tie(right.primary, right.left, right.top, right.width, right.height);
    });

    LayoutCallback callback;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (hasSnapshot_ && lastOrientation_ == primaryOrientation &&
            SameLayout(lastLayout_, layout)) {
            message = "native display layout unchanged: count=" +
                std::to_string(layout.size()) + " source=" + source;
            return true;
        }
        hasSnapshot_ = true;
        lastOrientation_ = primaryOrientation;
        lastLayout_ = layout;
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
