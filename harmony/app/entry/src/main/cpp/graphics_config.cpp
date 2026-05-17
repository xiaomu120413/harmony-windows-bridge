#include "graphics_config.h"

#include "string_utils.h"

#include <cstdlib>
#include <sstream>

namespace rdp_bridge {

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params)
{
    GraphicsPipelineConfig config;
    std::string mode = ToLowerAscii(TrimAscii(params.graphicsMode));
    if (mode.empty()) {
        const char* envMode = std::getenv("FREERDP_OHOS_GRAPHICS");
        if (envMode != nullptr) {
            mode = ToLowerAscii(TrimAscii(envMode));
        }
    }

    if (mode == "rdpgfx" || mode == "gfx" || mode == "on") {
        config.enabled = true;
        config.mode = "rdpgfx";
    } else if (mode == "rdpgfx-h264" || mode == "gfx-h264" || mode == "h264") {
        config.enabled = true;
        config.h264 = true;
        config.mode = "rdpgfx-h264";
    } else {
        config.enabled = false;
        config.h264 = false;
        config.mode = "gdi";
    }
    return config;
}

std::vector<std::string> BuildGraphicsFallbackModes(const ConnectParams& params)
{
    const GraphicsPipelineConfig config = ParseGraphicsPipelineConfig(params);
    if (config.mode == "rdpgfx-h264") {
        return {"rdpgfx-h264"};
    }
    if (config.mode == "rdpgfx") {
        return {"rdpgfx", "gdi"};
    }
    return {"gdi"};
}

std::string JoinGraphicsModes(const std::vector<std::string>& modes)
{
    std::ostringstream stream;
    for (size_t i = 0; i < modes.size(); ++i) {
        if (i > 0) {
            stream << " -> ";
        }
        stream << modes[i];
    }
    return stream.str();
}

bool ShouldRetryGraphicsFallback(const RdpSessionRunResult& session, bool attemptConnected,
    const std::string& failedMode, size_t attemptIndex, size_t attemptCount)
{
    if (!session.failed || attemptConnected || attemptIndex + 1 >= attemptCount ||
        failedMode == "gdi") {
        return false;
    }

    const std::string message = ToLowerAscii(session.message);
    return message.find("graphics") != std::string::npos ||
        message.find("rdpgfx") != std::string::npos ||
        message.find("gfx") != std::string::npos ||
        message.find("dynamic channel") != std::string::npos ||
        message.find("h264") != std::string::npos ||
        message.find("surface") != std::string::npos;
}

} // namespace rdp_bridge
