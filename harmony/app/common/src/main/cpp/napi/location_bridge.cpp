#include "napi/location_bridge.h"

#include "napi/permission_request_bridge.h"

namespace rdp_bridge {
namespace {

PermissionRequestBridge& LocationPermissionBridge()
{
    static PermissionRequestBridge bridge("location");
    return bridge;
}

} // namespace

EventSink& LocationPermissionRequestSink()
{
    return LocationPermissionBridge().RequestSink();
}

bool CompleteLocationPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    return LocationPermissionBridge().CompleteFromUi(requestId, granted);
}

BOOL RequestLocationPermissionForFreeRdp(void* userData, UINT32 timeoutMs)
{
    return LocationPermissionBridge().Request(userData, timeoutMs);
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
