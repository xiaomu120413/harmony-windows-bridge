#include "napi/microphone_permission_bridge.h"

#include "napi/permission_request_bridge.h"

namespace rdp_bridge {
namespace {

PermissionRequestBridge& MicrophonePermissionBridge()
{
    static PermissionRequestBridge bridge("microphone");
    return bridge;
}

} // namespace

EventSink& MicrophonePermissionRequestSink()
{
    return MicrophonePermissionBridge().RequestSink();
}

bool CompleteMicrophonePermissionRequestFromUi(uint32_t requestId, bool granted)
{
    return MicrophonePermissionBridge().CompleteFromUi(requestId, granted);
}

BOOL RequestMicrophonePermissionForAudin(void* userData, UINT32 timeoutMs)
{
    return MicrophonePermissionBridge().Request(userData, timeoutMs);
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

    if (!api.audinOhosSetPermissionCallback(RequestMicrophonePermissionForAudin, nullptr) && log) {
        log("FreeRDP audin OHOS permission callback registration failed");
    }
}

} // namespace rdp_bridge
