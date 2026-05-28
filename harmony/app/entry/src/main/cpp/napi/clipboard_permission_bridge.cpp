#include "napi/clipboard_permission_bridge.h"

#include "common/bridge_log.h"

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>

namespace rdp_bridge {
namespace {

constexpr uint32_t kDefaultClipboardPermissionTimeoutMs = 60000;

EventSink g_clipboardPermissionRequests;
std::mutex g_clipboardPermissionMutex;
std::condition_variable g_clipboardPermissionCv;
uint32_t g_nextClipboardPermissionRequestId = 1;
uint32_t g_pendingClipboardPermissionRequestId = 0;
bool g_clipboardPermissionPending = false;
bool g_clipboardPermissionCompleted = false;
bool g_clipboardPermissionGranted = false;

} // namespace

EventSink& ClipboardPermissionRequestSink()
{
    return g_clipboardPermissionRequests;
}

bool CompleteClipboardPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    std::lock_guard<std::mutex> lock(g_clipboardPermissionMutex);
    if (!g_clipboardPermissionPending ||
        g_pendingClipboardPermissionRequestId != requestId) {
        return false;
    }

    g_clipboardPermissionGranted = granted;
    g_clipboardPermissionCompleted = true;
    g_clipboardPermissionCv.notify_all();
    return true;
}

BOOL RequestClipboardPermissionForPasteboard(void* userData, UINT32 timeoutMs)
{
    (void)userData;
    if (!g_clipboardPermissionRequests.IsSet()) {
        BridgeLogger::Error("OHOS clipboard permission request skipped: ETS callback is not registered");
        return FALSE;
    }

    uint32_t requestId = 0;
    {
        std::unique_lock<std::mutex> lock(g_clipboardPermissionMutex);
        if (g_clipboardPermissionPending) {
            const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultClipboardPermissionTimeoutMs;
            const bool completed = g_clipboardPermissionCv.wait_for(lock,
                std::chrono::milliseconds(waitMs), []() {
                    return g_clipboardPermissionCompleted;
                });
            return completed && g_clipboardPermissionGranted ? TRUE : FALSE;
        }

        requestId = g_nextClipboardPermissionRequestId++;
        if (g_nextClipboardPermissionRequestId == 0) {
            g_nextClipboardPermissionRequestId = 1;
        }
        g_pendingClipboardPermissionRequestId = requestId;
        g_clipboardPermissionPending = true;
        g_clipboardPermissionCompleted = false;
        g_clipboardPermissionGranted = false;
    }

    g_clipboardPermissionRequests.Emit(std::to_string(requestId));

    const uint32_t waitMs = timeoutMs > 0 ? timeoutMs : kDefaultClipboardPermissionTimeoutMs;
    std::unique_lock<std::mutex> lock(g_clipboardPermissionMutex);
    const bool completed = g_clipboardPermissionCv.wait_for(lock,
        std::chrono::milliseconds(waitMs), [requestId]() {
            return g_clipboardPermissionCompleted &&
                g_pendingClipboardPermissionRequestId == requestId;
        });
    const bool granted = completed && g_clipboardPermissionGranted;
    if (!completed) {
        BridgeLogger::Error("OHOS clipboard permission request timed out");
    }

    if (g_pendingClipboardPermissionRequestId == requestId) {
        g_pendingClipboardPermissionRequestId = 0;
        g_clipboardPermissionPending = false;
        g_clipboardPermissionCompleted = false;
        g_clipboardPermissionGranted = false;
    }
    return granted ? TRUE : FALSE;
}

} // namespace rdp_bridge
