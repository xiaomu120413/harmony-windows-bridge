#include "graphics_config.h"

#include "freerdp_runtime.h"
#include "string_utils.h"

#include <cstdlib>
#include <sstream>

namespace rdp_bridge {
namespace {

FreerdpRuntimeApi* LoadedRuntime()
{
    auto& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return nullptr;
    }
    return &api;
}

} // namespace

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params)
{
    if (FreerdpRuntimeApi* api = LoadedRuntime();
        api != nullptr && api->ohosGraphicsConfigFromMode != nullptr) {
    const FREERDP_OHOS_GRAPHICS_CONFIG nativeConfig =
            api->ohosGraphicsConfigFromMode(params.graphicsMode.c_str());
    GraphicsPipelineConfig config;
    config.enabled = nativeConfig.enabled;
    config.h264 = nativeConfig.h264;
    config.mode = nativeConfig.modeName == nullptr ? "gdi" : nativeConfig.modeName;
    return config;
    }

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
    if (FreerdpRuntimeApi* api = LoadedRuntime();
        api != nullptr && api->ohosGraphicsFallbackModes != nullptr) {
    const char* nativeModes[3] = {};
        const size_t count = api->ohosGraphicsFallbackModes(
        params.graphicsMode.c_str(), nativeModes, sizeof(nativeModes) / sizeof(nativeModes[0]));
    std::vector<std::string> modes;
    modes.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        modes.emplace_back(nativeModes[index] == nullptr ? "gdi" : nativeModes[index]);
    }
    return modes;
    }

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
    if (FreerdpRuntimeApi* api = LoadedRuntime();
        api != nullptr && api->ohosGraphicsShouldRetryFallback != nullptr) {
        return api->ohosGraphicsShouldRetryFallback(session.failed, attemptConnected,
            failedMode.c_str(), attemptIndex, attemptCount, session.message.c_str());
    }

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
