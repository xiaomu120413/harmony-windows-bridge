#pragma once

#include "napi/native_api.h"
#include "napi/napi_event_sink.h"
#include "session/rdp_session_core.h"
#include "surface/surface_bridge.h"

namespace rdp_bridge {

SessionEventHub& BridgeEvents();
RdpSession& BridgeSession();

void InitializeNativeBridgeContext();
bool RegisterNativeXComponent(napi_env env, napi_value exports);

} // namespace rdp_bridge
