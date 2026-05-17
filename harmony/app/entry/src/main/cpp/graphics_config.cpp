#include "graphics_config.h"

#include "string_utils.h"

#include <cstdlib>
#include <sstream>

#if defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
#include <client/OHOS/ohos_graphics.h>
#endif

namespace rdp_bridge {

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params)
{
#if defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
    const FREERDP_OHOS_GRAPHICS_CONFIG nativeConfig =
        freerdp_ohos_graphics_config_from_mode(params.graphicsMode.c_str());
    GraphicsPipelineConfig config;
    config.enabled = nativeConfig.enabled;
    config.h264 = nativeConfig.h264;
    config.mode = nativeConfig.modeName == nullptr ? "gdi" : nativeConfig.modeName;
    return config;
#else
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
#endif
}

std::vector<std::string> BuildGraphicsFallbackModes(const ConnectParams& params)
{
#if defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
    const char* nativeModes[3] = {};
    const size_t count = freerdp_ohos_graphics_fallback_modes(
        params.graphicsMode.c_str(), nativeModes, sizeof(nativeModes) / sizeof(nativeModes[0]));
    std::vector<std::string> modes;
    modes.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        modes.emplace_back(nativeModes[index] == nullptr ? "gdi" : nativeModes[index]);
    }
    return modes;
#else
    const GraphicsPipelineConfig config = ParseGraphicsPipelineConfig(params);
    if (config.mode == "rdpgfx-h264") {
        return {"rdpgfx-h264"};
    }
    if (config.mode == "rdpgfx") {
        return {"rdpgfx", "gdi"};
    }
    return {"gdi"};
#endif
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
#if defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
    return freerdp_ohos_graphics_should_retry_fallback(session.failed, attemptConnected,
        failedMode.c_str(), attemptIndex, attemptCount, session.message.c_str());
#else
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
#endif
}

} // namespace rdp_bridge
