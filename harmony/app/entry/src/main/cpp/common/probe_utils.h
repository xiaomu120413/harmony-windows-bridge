#pragma once

#include <string>

namespace rdp_bridge {

struct FreerdpProbeResult {
    bool linked = false;
    std::string json;
    std::string error;
    std::string freerdpVersion = "not-linked";
    std::string winprVersion = "not-linked";
    std::string opensslVersion = "not-linked";
};

std::string CurrentAbi();
FreerdpProbeResult LoadFreerdpProbe();

} // namespace rdp_bridge
