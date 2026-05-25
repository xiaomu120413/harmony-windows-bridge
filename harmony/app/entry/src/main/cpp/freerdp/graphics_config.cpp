#include "freerdp/graphics_config.h"

#include "common/string_utils.h"
#include "freerdp/freerdp_runtime.h"

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

GraphicsPipelineConfig InvalidGraphicsConfig(const std::string& mode)
{
    GraphicsPipelineConfig config;
    config.valid = false;
    config.enabled = false;
    config.h264 = false;
    config.avc444GpuCompositor = false;
    config.mode = mode;
    return config;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
GraphicsPipelineConfig FromNativeGraphicsConfig(const FREERDP_OHOS_GRAPHICS_CONFIG& nativeConfig,
    const std::string& requestedMode)
{
    GraphicsPipelineConfig config;
    config.valid = nativeConfig.mode != FREERDP_OHOS_GRAPHICS_MODE_INVALID;
    config.enabled = nativeConfig.enabled;
    config.h264 = nativeConfig.h264;
    config.avc444GpuCompositor = nativeConfig.enabled && nativeConfig.h264;
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
    FreerdpRuntimeApi* api = LoadedRuntime();
    if (api == nullptr) {
        return InvalidGraphicsConfig("runtime-unavailable");
    }
    if (api->ohosGraphicsConfigFromMode == nullptr) {
        return InvalidGraphicsConfig("helper-missing");
    }

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
    const FREERDP_OHOS_GRAPHICS_CONFIG nativeConfig =
        api->ohosGraphicsConfigFromMode(params.graphicsMode.c_str());
    return FromNativeGraphicsConfig(nativeConfig, params.graphicsMode);
#else
    (void)params;
    return InvalidGraphicsConfig("headers-unavailable");
#endif
}

std::string GraphicsModeValidationError(const std::string& graphicsMode)
{
    ConnectParams params;
    params.graphicsMode = graphicsMode;
    const GraphicsPipelineConfig config = ParseGraphicsPipelineConfig(params);
    if (config.valid) {
        return "";
    }

    if (config.mode == "runtime-unavailable") {
        return "FreeRDP runtime is unavailable for OHOS graphics mode parsing";
    }
    if (config.mode == "helper-missing") {
        return "FreeRDP OHOS graphics helper symbols are not loaded";
    }
    if (TrimAscii(graphicsMode).empty()) {
        return "graphicsMode is required; FreeRDP OHOS graphics helper owns supported values";
    }
    return "unsupported graphicsMode '" + graphicsMode + "' from FreeRDP OHOS graphics helper";
}

std::vector<std::string> BuildGraphicsFallbackModes(const ConnectParams& params)
{
    FreerdpRuntimeApi* api = LoadedRuntime();
    if (api == nullptr || api->ohosGraphicsFallbackModes == nullptr) {
        return {};
    }

    const char* nativeModes[3] = {};
    const size_t count = api->ohosGraphicsFallbackModes(
        params.graphicsMode.c_str(), nativeModes, sizeof(nativeModes) / sizeof(nativeModes[0]));
    std::vector<std::string> modes;
    modes.reserve(count);
    for (size_t index = 0; index < count; ++index) {
        if (nativeModes[index] != nullptr && nativeModes[index][0] != '\0') {
            modes.emplace_back(nativeModes[index]);
        }
    }
    return modes;
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
    FreerdpRuntimeApi* api = LoadedRuntime();
    if (api == nullptr || api->ohosGraphicsShouldRetryFallback == nullptr) {
        return false;
    }

    return api->ohosGraphicsShouldRetryFallback(session.failed, attemptConnected,
        failedMode.c_str(), attemptIndex, attemptCount, session.message.c_str());
}

} // namespace rdp_bridge
