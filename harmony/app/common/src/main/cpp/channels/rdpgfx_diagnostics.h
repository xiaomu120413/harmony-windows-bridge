#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

struct RdpgfxFrameProgress {
    bool available = false;
    bool bridgeAttached = false;
    uint64_t startFrames = 0;
    uint64_t endFrames = 0;
    uint64_t surfaceCommands = 0;
};

void SetRdpgfxRuntimeRequest(bool requested, bool h264Requested);
void SetRdpgfxBridgeAttached(bool attached);
void IncrementRdpgfxConnected();
void IncrementRdpgfxDisconnected();
void IncrementRdpgfxInitFailed();
void ResetRdpgfxDiagnosticsStats();

RdpgfxFrameProgress SnapshotRdpgfxFrameProgress();
std::string BuildGraphicsPipelineStatsLog();

} // namespace rdp_bridge
