#pragma once

#include "napi/native_api.h"
#include "napi/napi_event_sink.h"
#include "session/rdp_session_core.h"
#include "surface/surface_bridge.h"

#include <string>

namespace rdp_bridge {

SessionEventHub& BridgeEvents();
RdpSession& BridgeSession();
SurfaceSnapshot BridgeSurfaceSnapshot();
std::string BridgeRenderStatsLog();

void InitializeNativeBridgeContext();
bool RegisterNativeXComponent(napi_env env, napi_value exports);
bool NotifyBridgeSurfaceLayout(uint32_t width, uint32_t height, std::string& message);

} // namespace rdp_bridge
