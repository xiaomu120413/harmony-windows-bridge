#pragma once

#include "napi/native_api.h"
#include "napi/napi_event_sink.h"
#include "session/rdp_session_core.h"
#include "surface/surface_bridge.h"

#include <cstdint>
#include <string>

namespace rdp_bridge {

SessionEventHub& BridgeEvents();
RdpSession& BridgeSession();
bool BindImeHostWindow(uint32_t windowId, std::string& message);

void InitializeNativeBridgeContext();
bool RegisterNativeXComponent(napi_env env, napi_value exports);

} // namespace rdp_bridge
