#include "napi/location_bridge.h"

#include "common/bridge_log.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace rdp_bridge {
namespace {

constexpr uint32_t kDefaultLocationTimeoutMs = 60000;

EventSink g_locationPermissionRequests;
std::mutex g_locationPermissionMutex;
std::condition_variable g_locationPermissionCv;
uint32_t g_nextLocationPermissionRequestId = 1;
uint32_t g_pendingLocationPermissionRequestId = 0;
bool g_locationPermissionPending = false;
bool g_locationPermissionCompleted = false;
bool g_locationPermissionGranted = false;

uint32_t NextRequestId(uint32_t& next)
{
    const uint32_t requestId = next++;
    if (next == 0) {
        next = 1;
    }
    return requestId;
}

uint32_t EffectiveTimeoutMs(uint32_t timeoutMs)
{
    return timeoutMs > 0 ? timeoutMs : kDefaultLocationTimeoutMs;
}

} // namespace

EventSink& LocationPermissionRequestSink()
{
    return g_locationPermissionRequests;
}

bool CompleteLocationPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    std::lock_guard<std::mutex> lock(g_locationPermissionMutex);
    if (!g_locationPermissionPending || g_pendingLocationPermissionRequestId != requestId) {
        return false;
    }

    g_locationPermissionGranted = granted;
    g_locationPermissionCompleted = true;
    g_locationPermissionCv.notify_all();
    return true;
}

BOOL RequestLocationPermissionForFreeRdp(void* userData, UINT32 timeoutMs)
{
    (void)userData;
    if (!g_locationPermissionRequests.IsSet()) {
        EmitHilogError("OHOS location permission request skipped: ETS callback is not registered");
        return FALSE;
    }

    uint32_t requestId = 0;
    {
        std::unique_lock<std::mutex> lock(g_locationPermissionMutex);
        if (g_locationPermissionPending) {
            const bool completed = g_locationPermissionCv.wait_for(lock,
                std::chrono::milliseconds(EffectiveTimeoutMs(timeoutMs)), []() {
                    return g_locationPermissionCompleted;
                });
            return completed && g_locationPermissionGranted ? TRUE : FALSE;
        }

        requestId = NextRequestId(g_nextLocationPermissionRequestId);
        g_pendingLocationPermissionRequestId = requestId;
        g_locationPermissionPending = true;
        g_locationPermissionCompleted = false;
        g_locationPermissionGranted = false;
    }

    EmitHilogInfo("OHOS location permission requested by RDP location channel");
    g_locationPermissionRequests.Emit(std::to_string(requestId));

    std::unique_lock<std::mutex> lock(g_locationPermissionMutex);
    const bool completed = g_locationPermissionCv.wait_for(lock,
        std::chrono::milliseconds(EffectiveTimeoutMs(timeoutMs)), [requestId]() {
            return g_locationPermissionCompleted &&
                g_pendingLocationPermissionRequestId == requestId;
        });
    const bool granted = completed && g_locationPermissionGranted;
    if (!completed) {
        EmitHilogError("OHOS location permission request timed out");
    }

    if (g_pendingLocationPermissionRequestId == requestId) {
        g_pendingLocationPermissionRequestId = 0;
        g_locationPermissionPending = false;
        g_locationPermissionCompleted = false;
        g_locationPermissionGranted = false;
    }
    return granted ? TRUE : FALSE;
}

void RegisterLocationBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log)
{
    if (api.ohosLocationSetPermissionCallback == nullptr) {
        if (log) {
            log("FreeRDP OHOS location permission callback unavailable; location redirection is disabled");
        }
        return;
    }

    const BOOL permissionOk = api.ohosLocationSetPermissionCallback(
        RequestLocationPermissionForFreeRdp, nullptr);
    if (permissionOk) {
        if (log) {
            log("FreeRDP OHOS location permission callback registered");
        }
    } else if (log) {
        log("FreeRDP OHOS location permission callback registration failed");
    }
}

} // namespace rdp_bridge
