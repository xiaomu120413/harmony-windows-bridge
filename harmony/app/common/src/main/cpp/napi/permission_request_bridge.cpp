#include "napi/permission_request_bridge.h"

#include "common/bridge_log.h"

#include <chrono>
#include <utility>

namespace rdp_bridge {

PermissionRequestBridge::PermissionRequestBridge(std::string label, uint32_t defaultTimeoutMs)
    : label_(std::move(label)), defaultTimeoutMs_(defaultTimeoutMs)
{
}

EventSink& PermissionRequestBridge::RequestSink()
{
    return requests_;
}

bool PermissionRequestBridge::CompleteFromUi(uint32_t requestId, bool granted)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!pending_ || pendingRequestId_ != requestId) {
        return false;
    }

    granted_ = granted;
    completed_ = true;
    cv_.notify_all();
    return true;
}

BOOL PermissionRequestBridge::Request(void* userData, UINT32 timeoutMs)
{
    (void)userData;
    if (!requests_.IsSet()) {
        BridgeLogger::Error("OHOS " + label_ + " permission request skipped: ETS callback is not registered");
        return FALSE;
    }

    uint32_t requestId = 0;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if (pending_) {
            const bool completed = cv_.wait_for(lock,
                std::chrono::milliseconds(EffectiveTimeoutMs(timeoutMs)), [this]() {
                    return completed_;
                });
            return completed && granted_ ? TRUE : FALSE;
        }

        requestId = NextRequestId();
        pendingRequestId_ = requestId;
        pending_ = true;
        completed_ = false;
        granted_ = false;
    }

    requests_.Emit(std::to_string(requestId));

    std::unique_lock<std::mutex> lock(mutex_);
    const bool completed = cv_.wait_for(lock,
        std::chrono::milliseconds(EffectiveTimeoutMs(timeoutMs)), [this, requestId]() {
            return completed_ && pendingRequestId_ == requestId;
        });
    const bool granted = completed && granted_;
    if (!completed) {
        BridgeLogger::Error("OHOS " + label_ + " permission request timed out");
    }

    if (pendingRequestId_ == requestId) {
        pendingRequestId_ = 0;
        pending_ = false;
        completed_ = false;
        granted_ = false;
    }
    return granted ? TRUE : FALSE;
}

uint32_t PermissionRequestBridge::NextRequestId()
{
    const uint32_t requestId = nextRequestId_++;
    if (nextRequestId_ == 0) {
        nextRequestId_ = 1;
    }
    return requestId;
}

uint32_t PermissionRequestBridge::EffectiveTimeoutMs(uint32_t timeoutMs) const
{
    return timeoutMs > 0 ? timeoutMs : defaultTimeoutMs_;
}

} // namespace rdp_bridge
