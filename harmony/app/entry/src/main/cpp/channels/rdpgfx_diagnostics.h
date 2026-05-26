#pragma once

#include <string>

namespace rdp_bridge {

void SetRdpgfxRuntimeRequest(bool requested, bool h264Requested);
void SetRdpgfxBridgeAttached(bool attached);
void IncrementRdpgfxConnected();
void IncrementRdpgfxDisconnected();
void IncrementRdpgfxInitFailed();
void ResetRdpgfxDiagnosticsStats();

std::string BuildGraphicsPipelineStatsLog();

} // namespace rdp_bridge
