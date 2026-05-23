#pragma once

#include "freerdp/freerdp_runtime.h"
#include "napi/napi_event_sink.h"

#include <cstdint>

namespace rdp_bridge {

EventSink& ClipboardPermissionRequestSink();
bool CompleteClipboardPermissionRequestFromUi(uint32_t requestId, bool granted);

#if defined(HARMONY_HAS_FREERDP_HEADERS)
BOOL RequestClipboardPermissionForPasteboard(void* userData, UINT32 timeoutMs);
#endif

} // namespace rdp_bridge
