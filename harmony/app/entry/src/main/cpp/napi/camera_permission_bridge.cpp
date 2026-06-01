#include "napi/camera_permission_bridge.h"

#include "common/bridge_log.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace rdp_bridge {
namespace {

constexpr uint32_t kDefaultCameraPermissionTimeoutMs = 60000;

EventSink g_cameraPermissionRequests;
std::mutex g_cameraPermissionMutex;
std::condition_variable g_cameraPermissionCv;
uint32_t g_nextCameraPermissionRequestId = 1;
uint32_t g_pendingCameraPermissionRequestId = 0;
bool g_cameraPermissionPending = false;
bool g_cameraPermissionCompleted = false;
bool g_cameraPermissionGranted = false;

} // namespace

EventSink& CameraPermissionRequestSink()
{
    return g_cameraPermissionRequests;
}

bool CompleteCameraPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    std::lock_guard<std::mutex> lock(g_cameraPermissionMutex);
    if (!g_cameraPermissionPending || g_pendingCameraPermissionRequestId != requestId) {
        return false;
    }

    g_cameraPermissionGranted = granted;
    g_cameraPermissionCompleted = true;
    g_cameraPermissionCv.notify_all();
    return true;
}

BOOL RequestCameraPermissionForRdpecam(void* userData, UINT32 timeoutMs)
{
    (void)userData;
    if (!g_cameraPermissionRequests.IsSet()) {
        BridgeLogger::Error("OHOS camera permission request skipped: ETS callback is not registered");
        return FALSE;
    }

    uint32_t requestId = 0;
    {
        std::unique_lock<std::mutex> lock(g_cameraPermissionMutex);
        if (g_cameraPermissionPending) {
            const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultCameraPermissionTimeoutMs;
            const bool completed = g_cameraPermissionCv.wait_for(lock,
                std::chrono::milliseconds(waitMs), []() {
                    return g_cameraPermissionCompleted;
                });
            return completed && g_cameraPermissionGranted ? TRUE : FALSE;
        }

        requestId = g_nextCameraPermissionRequestId++;
        if (g_nextCameraPermissionRequestId == 0) {
            g_nextCameraPermissionRequestId = 1;
        }
        g_pendingCameraPermissionRequestId = requestId;
        g_cameraPermissionPending = true;
        g_cameraPermissionCompleted = false;
        g_cameraPermissionGranted = false;
    }

    g_cameraPermissionRequests.Emit(std::to_string(requestId));

    const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultCameraPermissionTimeoutMs;
    std::unique_lock<std::mutex> lock(g_cameraPermissionMutex);
    const bool completed = g_cameraPermissionCv.wait_for(lock,
        std::chrono::milliseconds(waitMs), [requestId]() {
            return g_cameraPermissionCompleted &&
                g_pendingCameraPermissionRequestId == requestId;
        });
    const bool granted = completed && g_cameraPermissionGranted;
    if (!completed) {
        BridgeLogger::Error("OHOS camera permission request timed out");
    }

    if (g_pendingCameraPermissionRequestId == requestId) {
        g_pendingCameraPermissionRequestId = 0;
        g_cameraPermissionPending = false;
        g_cameraPermissionCompleted = false;
        g_cameraPermissionGranted = false;
    }
    return granted ? TRUE : FALSE;
}

void RegisterCameraPermissionBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log)
{
    if (api.rdpecamOhosSetPermissionCallback == nullptr) {
        if (log) {
            log("FreeRDP rdpecam OHOS permission callback unavailable; camera permission will be enforced by OHOS camera APIs");
        }
        return;
    }

    if (!api.rdpecamOhosSetPermissionCallback(RequestCameraPermissionForRdpecam, nullptr) && log) {
        log("FreeRDP rdpecam OHOS permission callback registration failed");
    }
}

} // namespace rdp_bridge
