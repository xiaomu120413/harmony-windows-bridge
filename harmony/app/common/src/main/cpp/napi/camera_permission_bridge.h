#pragma once

#include "freerdp/freerdp_runtime.h"
#include "napi/napi_event_sink.h"

#include <cstdint>
#include <functional>
#include <string>

namespace rdp_bridge {

EventSink& CameraPermissionRequestSink();
bool CompleteCameraPermissionRequestFromUi(uint32_t requestId, bool granted);

BOOL RequestCameraPermissionForRdpecam(void* userData, UINT32 timeoutMs);
void RegisterCameraPermissionBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log);

} // namespace rdp_bridge
