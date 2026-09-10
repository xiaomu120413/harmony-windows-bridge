#pragma once

#include "napi/napi_event_sink.h"
#include "session/rdp_session_core.h"
#include "surface/surface_bridge.h"
#include "session/session_display_settings.h"

#include <cstdint>
#include <string>

namespace rdp_bridge {

SessionEventHub& BridgeEvents();
RdpSession& BridgeSession();
bool BindImeHostWindow(uint32_t windowId, std::string& message);
bool SetSessionDisplayResolution(bool fixed, uint32_t width, uint32_t height, std::string& message);
bool RefreshSessionDisplay(std::string& message);

void InitializeNativeBridgeContext();
bool RegisterNativeXComponentInstance(OH_NativeXComponent* component);

} // namespace rdp_bridge
