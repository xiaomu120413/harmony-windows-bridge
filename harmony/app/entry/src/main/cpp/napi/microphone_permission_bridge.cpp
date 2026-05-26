#include "napi/microphone_permission_bridge.h"

#include "common/bridge_log.h"

#include <chrono>
#include <condition_variable>
#include <mutex>

namespace rdp_bridge {
namespace {

constexpr uint32_t kDefaultMicrophonePermissionTimeoutMs = 60000;

EventSink g_microphonePermissionRequests;
std::mutex g_microphonePermissionMutex;
std::condition_variable g_microphonePermissionCv;
uint32_t g_nextMicrophonePermissionRequestId = 1;
uint32_t g_pendingMicrophonePermissionRequestId = 0;
bool g_microphonePermissionPending = false;
bool g_microphonePermissionCompleted = false;
bool g_microphonePermissionGranted = false;

} // namespace

EventSink& MicrophonePermissionRequestSink()
{
    return g_microphonePermissionRequests;
}

bool CompleteMicrophonePermissionRequestFromUi(uint32_t requestId, bool granted)
{
    std::lock_guard<std::mutex> lock(g_microphonePermissionMutex);
    if (!g_microphonePermissionPending ||
        g_pendingMicrophonePermissionRequestId != requestId) {
        return false;
    }

    g_microphonePermissionGranted = granted;
    g_microphonePermissionCompleted = true;
    g_microphonePermissionCv.notify_all();
    return true;
}

BOOL RequestMicrophonePermissionForAudin(void* userData, UINT32 timeoutMs)
{
    (void)userData;
    if (!g_microphonePermissionRequests.IsSet()) {
        EmitHilogError("OHOS microphone permission request skipped: ETS callback is not registered");
        return FALSE;
    }

    uint32_t requestId = 0;
    {
        std::unique_lock<std::mutex> lock(g_microphonePermissionMutex);
        if (g_microphonePermissionPending) {
            const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultMicrophonePermissionTimeoutMs;
            const bool completed = g_microphonePermissionCv.wait_for(lock,
                std::chrono::milliseconds(waitMs), []() {
                    return g_microphonePermissionCompleted;
                });
            return completed && g_microphonePermissionGranted ? TRUE : FALSE;
        }

        requestId = g_nextMicrophonePermissionRequestId++;
        if (g_nextMicrophonePermissionRequestId == 0) {
            g_nextMicrophonePermissionRequestId = 1;
        }
        g_pendingMicrophonePermissionRequestId = requestId;
        g_microphonePermissionPending = true;
        g_microphonePermissionCompleted = false;
        g_microphonePermissionGranted = false;
    }

    EmitHilogInfo("OHOS microphone permission requested by remote audin channel");
    g_microphonePermissionRequests.Emit(std::to_string(requestId));

    const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultMicrophonePermissionTimeoutMs;
    std::unique_lock<std::mutex> lock(g_microphonePermissionMutex);
    const bool completed = g_microphonePermissionCv.wait_for(lock,
        std::chrono::milliseconds(waitMs), [requestId]() {
            return g_microphonePermissionCompleted &&
                g_pendingMicrophonePermissionRequestId == requestId;
        });
    const bool granted = completed && g_microphonePermissionGranted;
    if (!completed) {
        EmitHilogError("OHOS microphone permission request timed out");
    }

    if (g_pendingMicrophonePermissionRequestId == requestId) {
        g_pendingMicrophonePermissionRequestId = 0;
        g_microphonePermissionPending = false;
        g_microphonePermissionCompleted = false;
        g_microphonePermissionGranted = false;
    }
    return granted ? TRUE : FALSE;
}

void RegisterMicrophonePermissionBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log)
{
    if (api.audinOhosSetPermissionCallback == nullptr) {
        if (log) {
            log("FreeRDP audin OHOS permission callback unavailable; microphone permission will be enforced by OHAudio");
        }
        return;
    }

    if (api.audinOhosSetPermissionCallback(RequestMicrophonePermissionForAudin, nullptr)) {
        if (log) {
            log("FreeRDP audin OHOS permission callback registered");
        }
    } else if (log) {
        log("FreeRDP audin OHOS permission callback registration failed");
    }
}

} // namespace rdp_bridge
