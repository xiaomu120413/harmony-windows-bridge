#pragma once

#include "common/bridge_types.h"

#include <cstddef>
#include <string>
#include <vector>

namespace rdp_bridge {

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params);
std::string GraphicsModeValidationError(const std::string& graphicsMode);
std::vector<std::string> BuildGraphicsFallbackModes(const ConnectParams& params);
std::string JoinGraphicsModes(const std::vector<std::string>& modes);
bool ShouldRetryGraphicsFallback(const RdpSessionRunResult& session, bool attemptConnected,
    const std::string& failedMode, size_t attemptIndex, size_t attemptCount);

} // namespace rdp_bridge
