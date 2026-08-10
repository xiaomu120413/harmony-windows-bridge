#pragma once

#include "napi/napi_event_sink.h"
#include "session/rdp_session_core.h"
#include "surface/surface_bridge.h"

#include <cstdint>
#include <string>

namespace rdp_bridge {

SessionEventHub& BridgeEvents();
RdpSession& BridgeSession();
bool BindImeHostWindow(uint32_t windowId, std::string& message);
std::string BuildNativeDiagnostics();

void InitializeNativeBridgeContext();
bool RegisterNativeXComponentInstance(OH_NativeXComponent* component);

} // namespace rdp_bridge
