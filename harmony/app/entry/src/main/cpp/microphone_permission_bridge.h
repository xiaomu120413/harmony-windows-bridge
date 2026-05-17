#pragma once

#include "freerdp_runtime.h"
#include "napi_event_sink.h"

#include <cstdint>
#include <functional>
#include <string>

namespace rdp_bridge {

EventSink& MicrophonePermissionRequestSink();
bool CompleteMicrophonePermissionRequestFromUi(uint32_t requestId, bool granted);

#if defined(HARMONY_HAS_FREERDP_HEADERS)
BOOL RequestMicrophonePermissionForAudin(void* userData, UINT32 timeoutMs);
void RegisterMicrophonePermissionBridge(FreerdpRuntimeApi& api,
    const std::function<void(const std::string&)>& log);
#endif

} // namespace rdp_bridge
