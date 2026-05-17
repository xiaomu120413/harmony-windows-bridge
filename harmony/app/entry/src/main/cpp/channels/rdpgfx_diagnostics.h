#pragma once

#include <string>

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void SetRdpgfxRuntimeRequest(bool requested, bool h264Requested);
void SetRdpgfxBridgeAttached(bool attached);
void IncrementRdpgfxConnected();
void IncrementRdpgfxDisconnected();
void IncrementRdpgfxInitFailed();
void ResetRdpgfxDiagnosticsStats();
#endif

std::string BuildGraphicsPipelineStatsLog();

} // namespace rdp_bridge
