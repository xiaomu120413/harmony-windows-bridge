#pragma once

#include <cstdint>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/channels/rdpgfx.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
const char* RdpgfxCodecName(uint32_t codecId);
void SetRdpgfxRuntimeRequest(bool requested, bool h264Requested);
void SetRdpgfxBridgeAttached(bool attached);
void IncrementRdpgfxConnected();
void IncrementRdpgfxDisconnected();
void IncrementRdpgfxInitFailed();
void RecordRdpgfxStartFrame();
void RecordRdpgfxEndFrame();
void ResetRdpgfxDiagnosticsStats();
void RecordRdpgfxCapsConfirm(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm);
void RecordRdpgfxCapsConfirmValues(uint32_t version, uint32_t flags, const char* source);
void RecordRdpgfxSurfaceCommand(const RDPGFX_SURFACE_COMMAND& command);
#endif

std::string BuildGraphicsPipelineStatsLog();

} // namespace rdp_bridge
