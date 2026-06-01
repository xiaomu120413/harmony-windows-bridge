#include "napi/camera_permission_bridge.h"

#include "napi/permission_request_bridge.h"

namespace rdp_bridge {
namespace {

PermissionRequestBridge& CameraPermissionBridge()
{
    static PermissionRequestBridge bridge("camera");
    return bridge;
}

} // namespace

EventSink& CameraPermissionRequestSink()
{
    return CameraPermissionBridge().RequestSink();
}

bool CompleteCameraPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    return CameraPermissionBridge().CompleteFromUi(requestId, granted);
}

BOOL RequestCameraPermissionForRdpecam(void* userData, UINT32 timeoutMs)
{
    return CameraPermissionBridge().Request(userData, timeoutMs);
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
