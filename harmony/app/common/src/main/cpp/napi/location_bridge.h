#pragma once

#include "freerdp/freerdp_runtime.h"
#include "napi/napi_event_sink.h"

#include <cstdint>
#include <functional>
#include <string>

namespace rdp_bridge {

EventSink& LocationPermissionRequestSink();

bool CompleteLocationPermissionRequestFromUi(uint32_t requestId, bool granted);

BOOL RequestLocationPermissionForFreeRdp(void* userData, UINT32 timeoutMs);
void RegisterLocationBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log);

} // namespace rdp_bridge
