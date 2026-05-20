#include "freerdp/graphics_config.h"

#include "freerdp/freerdp_runtime.h"
#include "common/string_utils.h"

#include <sstream>

namespace rdp_bridge {
namespace {

constexpr const char* kSupportedGraphicsModes = "gdi, rdpgfx, rdpgfx-h264";

std::string NormalizeGraphicsMode(const std::string& value)
{
    return ToLowerAscii(TrimAscii(value));
}

GraphicsPipelineConfig ParseGraphicsModeStrict(const std::string& value)
{
    const std::string mode = NormalizeGraphicsMode(value);
    GraphicsPipelineConfig config;
    if (mode == "gdi") {
        config.valid = true;
        config.enabled = false;
        config.h264 = false;
        config.avc444GpuExperimental = false;
        config.mode = "gdi";
        return config;
    }
    if (mode == "rdpgfx" || mode == "gfx" || mode == "on") {
        config.valid = true;
        config.enabled = true;
        config.h264 = false;
        config.avc444GpuExperimental = false;
        config.mode = "rdpgfx";
        return config;
    }
    if (mode == "rdpgfx-h264" || mode == "gfx-h264" || mode == "h264") {
        config.valid = true;
        config.enabled = true;
        config.h264 = true;
        config.avc444GpuExperimental = true;
        config.mode = "rdpgfx-h264";
        return config;
    }

    config.valid = false;
    config.enabled = false;
    config.h264 = false;
    config.avc444GpuExperimental = false;
    config.mode = mode.empty() ? "missing" : "invalid";
    return config;
}

FreerdpRuntimeApi* LoadedRuntime()
{
    auto& api = SharedFreerdpRuntimeApi();
    std::string error;
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return nullptr;
    }
    return &api;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
GraphicsPipelineConfig FromNativeGraphicsConfig(const FREERDP_OHOS_GRAPHICS_CONFIG& nativeConfig,
    const std::string& requestedMode)
{
    GraphicsPipelineConfig config;
    config.valid = nativeConfig.mode != FREERDP_OHOS_GRAPHICS_MODE_INVALID;
    config.enabled = nativeConfig.enabled;
    config.h264 = nativeConfig.h264;
    config.avc444GpuExperimental = nativeConfig.enabled && nativeConfig.h264;
    if (config.valid) {
        config.mode = nativeConfig.modeName == nullptr ? "gdi" : nativeConfig.modeName;
    } else {
        config.mode = TrimAscii(requestedMode).empty() ? "missing" : "invalid";
    }
    return config;
}
#endif

} // namespace

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params)
{
    if (FreerdpRuntimeApi* api = LoadedRuntime();
        api != nullptr && api->ohosGraphicsConfigFromMode != nullptr) {
#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
        const FREERDP_OHOS_GRAPHICS_CONFIG nativeConfig =
            api->ohosGraphicsConfigFromMode(params.graphicsMode.c_str());
        return FromNativeGraphicsConfig(nativeConfig, params.graphicsMode);
#endif
    }

    const GraphicsPipelineConfig strictConfig = ParseGraphicsModeStrict(params.graphicsMode);
    if (!strictConfig.valid) {
        return strictConfig;
    }

    GraphicsPipelineConfig config = strictConfig;
    config.avc444GpuExperimental = config.enabled && config.h264;
    return config;
}

std::string GraphicsModeValidationError(const std::string& graphicsMode)
{
    ConnectParams params;
    params.graphicsMode = graphicsMode;
    const GraphicsPipelineConfig config = ParseGraphicsPipelineConfig(params);
    if (config.valid) {
        return "";
    }

    if (TrimAscii(graphicsMode).empty()) {
        return std::string("graphicsMode is required; supported values: ") + kSupportedGraphicsModes;
    }
    return "unsupported graphicsMode '" + graphicsMode + "'; supported values: " +
        kSupportedGraphicsModes;
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

    const GraphicsPipelineConfig strictConfig = ParseGraphicsModeStrict(params.graphicsMode);
    if (!strictConfig.valid) {
        return {};
    }

    if (strictConfig.mode == "rdpgfx-h264") {
        return {"rdpgfx-h264", "gdi"};
    }
    if (strictConfig.mode == "rdpgfx") {
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

    (void)attemptConnected;

    if (!session.failed || attemptIndex + 1 >= attemptCount || failedMode == "gdi") {
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
