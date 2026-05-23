#include "freerdp/rdp_channel_config.h"

#include "common/string_utils.h"

#include <array>
#include <cstdint>

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
namespace {

bool BuildOhosSessionConfig(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const ConnectParams& params, FREERDP_OHOS_SESSION_CONFIG& config, std::string& error)
{
    if (api.ohosSessionConfigDefault == nullptr) {
        error = "FreeRDP OHOS session config symbol is not loaded";
        return false;
    }

    config = api.ohosSessionConfigDefault();
    config.graphicsPipeline = graphicsConfig.enabled ? TRUE : FALSE;
    config.h264 = graphicsConfig.enabled && graphicsConfig.h264 ? TRUE : FALSE;
    return true;
}

std::string OnOff(bool value)
{
    return value ? "on" : "off";
}

std::string GraphicsCapabilityDiagnostics(FreerdpRuntimeApi& api, rdpSettings* settings)
{
    if (api.settingsGetBool == nullptr || api.settingsGetUint32 == nullptr) {
        return "FreeRDP graphics capability settings verified; diagnostics unavailable";
    }

    const bool gfx = api.settingsGetBool(settings, FreeRDP_SupportGraphicsPipeline) ? true : false;
    const bool h264 = api.settingsGetBool(settings, FreeRDP_GfxH264) ? true : false;
    const bool avc444 = api.settingsGetBool(settings, FreeRDP_GfxAVC444) ? true : false;
    const bool avc444v2 = api.settingsGetBool(settings, FreeRDP_GfxAVC444v2) ? true : false;
    const bool smallCache = api.settingsGetBool(settings, FreeRDP_GfxSmallCache) ? true : false;
    const uint32_t capsFilter = api.settingsGetUint32(settings, FreeRDP_GfxCapsFilter);

    return "FreeRDP graphics capability settings verified: SupportGraphicsPipeline=" +
        OnOff(gfx) + " GfxH264=" + OnOff(h264) + " GfxAVC444=" + OnOff(avc444) +
        " GfxAVC444v2=" + OnOff(avc444v2) + " GfxSmallCache=" + OnOff(smallCache) +
        " GfxCapsFilter=" + Hex32(capsFilter) +
        " expectedAvc420Flag=0x00000010";
}

bool ValidateGraphicsCapabilitySettings(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, std::string& error)
{
    if (api.settingsGetBool == nullptr || api.settingsGetUint32 == nullptr) {
        error = "FreeRDP settings diagnostics symbols are not loaded";
        return false;
    }

    const bool gfx = api.settingsGetBool(settings, FreeRDP_SupportGraphicsPipeline) ? true : false;
    const bool h264 = api.settingsGetBool(settings, FreeRDP_GfxH264) ? true : false;
    const bool avc444 = api.settingsGetBool(settings, FreeRDP_GfxAVC444) ? true : false;
    const bool avc444v2 = api.settingsGetBool(settings, FreeRDP_GfxAVC444v2) ? true : false;
    const uint32_t capsFilter = api.settingsGetUint32(settings, FreeRDP_GfxCapsFilter);

    if (gfx != graphicsConfig.enabled) {
        error = "FreeRDP graphics pipeline setting mismatch after OHOS helper";
        return false;
    }
    if (graphicsConfig.h264 && !h264) {
        error = "rdpgfx-h264 requested but FreeRDP_GfxH264 is off after OHOS helper";
        return false;
    }
    if (graphicsConfig.h264 && capsFilter != 0) {
        error = "rdpgfx-h264 requested but GfxCapsFilter is filtering capabilities";
        return false;
    }
    return true;
}

} // namespace

bool EnableFreerdpClientChannels(FreerdpRuntimeApi& api, freerdp* instance,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (instance == nullptr) {
        error = "FreeRDP instance unavailable for channel setup";
        return false;
    }
    if (api.registerAddinProvider == nullptr || api.channelsLoadStaticAddinEntry == nullptr ||
        api.clientLoadChannels == nullptr) {
        error = "FreeRDP client channel symbols are not loaded";
        return false;
    }

    int rc = api.registerAddinProvider(api.channelsLoadStaticAddinEntry, 0);
    if (rc != 0) {
        error = "freerdp_register_addin_provider failed: " + std::to_string(rc);
        return false;
    }

    instance->LoadChannels = api.clientLoadChannels;
    log("FreeRDP client channel loader registered");
    return true;
}

bool ConfigureEnhancedRdpSettings(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const ConnectParams& params,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (api.ohosSessionApplySettings == nullptr) {
        error = "FreeRDP OHOS session settings helper is not loaded";
        return false;
    }

    FREERDP_OHOS_SESSION_CONFIG config = {};
    if (!BuildOhosSessionConfig(api, graphicsConfig, params, config, error)) {
        return false;
    }

    std::array<char, 256> detail {};
    if (!api.ohosSessionApplySettings(settings, &config, detail.data(), detail.size())) {
        error = detail[0] == '\0' ? "FreeRDP OHOS session settings helper failed" : detail.data();
        return false;
    }
    if (!ValidateGraphicsCapabilitySettings(api, settings, graphicsConfig, error)) {
        return false;
    }

    log(detail[0] == '\0' ? "OHOS FreeRDP settings applied" : detail.data());
    log(GraphicsCapabilityDiagnostics(api, settings));
    if (graphicsConfig.enabled) {
        const bool h264Requested = graphicsConfig.enabled && graphicsConfig.h264;
        log("FreeRDP graphics pipeline requested: mode=" + graphicsConfig.mode +
            " h264=" + std::string(h264Requested ? "surface-avc420-preferred" : "off") +
            " avc444=" +
            std::string(graphicsConfig.avc444GpuCompositor ? "gpu-auto+native-fallback" :
                (h264Requested ? "freerdp-native-yuv444" : "off")) +
            " capsFilter=" + Hex32(0) +
            " requestedCodec=" + std::string(h264Requested ? "avc420+avc444" : "none") +
            " rfx=off nscodec=" + std::string(h264Requested ? "on" : "off") +
            " smallCache=on progressive=off fastPath=on frameMarker=on frameAck=2");
    } else {
        log("FreeRDP graphics pipeline disabled at runtime; using stable software GDI frame rendering");
    }
    log("FreeRDP rdpdr base settings remain enabled; drive/printer runtime toggles are disabled by the OHOS helper; smartcard is not compiled in the delivery profile");
    return true;
}

bool ConfigureOhosStandardChannels(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const ConnectParams& params,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (api.ohosSessionAddStandardChannels == nullptr) {
        error = "FreeRDP OHOS standard channel helper is not loaded";
        return false;
    }

    FREERDP_OHOS_SESSION_CONFIG config = {};
    if (!BuildOhosSessionConfig(api, graphicsConfig, params, config, error)) {
        return false;
    }

    std::array<char, 256> detail {};
    if (!api.ohosSessionAddStandardChannels(settings, &config, detail.data(), detail.size())) {
        error = detail[0] == '\0' ? "FreeRDP OHOS standard channel helper failed" : detail.data();
        return false;
    }

    log(detail[0] == '\0' ? "OHOS FreeRDP standard channels added" : detail.data());
    log("FreeRDP clipboard/display/graphics/audio channel parameters are owned by client/OHOS session helper; clipboard and microphone permissions are requested on use");
    return true;
}
#endif

} // namespace rdp_bridge
