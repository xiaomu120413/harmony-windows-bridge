#include "rdp_channel_config.h"

#include "string_utils.h"

#include <cstdint>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/channels/cliprdr.h>
#include <freerdp/channels/disp.h>
#include <freerdp/settings_keys.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
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
    const GraphicsPipelineConfig& graphicsConfig,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    const bool h264Requested = graphicsConfig.enabled && graphicsConfig.h264;
    const bool avc444Requested = h264Requested;
    const bool gfxSmallCache = true;
    const uint32_t gfxCapsFilter = 0;
    const uint32_t frameAcknowledge = 2;

    if (!SetFreerdpBool(api, settings, FreeRDP_SupportDynamicChannels, true, "SupportDynamicChannels", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportDisplayControl, true, "SupportDisplayControl", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_DynamicResolutionUpdate, true, "DynamicResolutionUpdate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportGraphicsPipeline, graphicsConfig.enabled,
            "SupportGraphicsPipeline", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxH264, h264Requested,
            "GfxH264", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444, avc444Requested,
            "GfxAVC444", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444v2, avc444Requested,
            "GfxAVC444v2", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_GfxCapsFilter, gfxCapsFilter, "GfxCapsFilter", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RemoteFxCodec, false, "RemoteFxCodec", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_NSCodec, h264Requested, "NSCodec", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxProgressive, false, "GfxProgressive", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxProgressiveV2, false, "GfxProgressiveV2", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxSmallCache, gfxSmallCache, "GfxSmallCache", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_FastPathOutput, true, "FastPathOutput", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_FrameMarkerCommandEnabled, true,
            "FrameMarkerCommandEnabled", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_FrameAcknowledge, frameAcknowledge,
            "FrameAcknowledge", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectClipboard, true, "RedirectClipboard", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ClipboardFeatureMask,
            CLIPRDR_FLAG_LOCAL_TO_REMOTE | CLIPRDR_FLAG_REMOTE_TO_LOCAL,
            "ClipboardFeatureMask", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_DeviceRedirection, true, "DeviceRedirection", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AudioPlayback, false, "AudioPlayback", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AudioCapture, false, "AudioCapture", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RemoteConsoleAudio, false, "RemoteConsoleAudio", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectDrives, false, "RedirectDrives", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectPrinters, false, "RedirectPrinters", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectSmartCards, false, "RedirectSmartCards", error)) {
        return false;
    }

    log("FreeRDP enhanced runtime libraries packaged; clipboard text redirection enabled");
    log("FreeRDP display-control dynamic resolution enabled");
    if (graphicsConfig.enabled) {
        log("FreeRDP graphics pipeline requested: mode=" + graphicsConfig.mode +
            " h264=" + std::string(h264Requested ? "surface-avc420-preferred" : "off") +
            " avc444=" + std::string(avc444Requested ? "on" : "off") +
            " capsFilter=" + Hex32(gfxCapsFilter) +
            " requestedCodec=" + std::string(h264Requested ? "avc420-avc444-diagnostic" : "none") +
            " rfx=off nscodec=" + std::string(h264Requested ? "on" : "off") +
            " smallCache=" + std::string(gfxSmallCache ? "on" : "off") +
            " progressive=off fastPath=on frameMarker=on frameAck=" +
            std::to_string(frameAcknowledge));
    } else {
        log("FreeRDP graphics pipeline disabled at runtime; using stable software GDI frame rendering");
    }
    log("FreeRDP audio playback is requested through explicit rdpsnd sys:ohos channels");
    log("FreeRDP rdpdr base channel enabled; drive/printer/smartcard runtime toggles remain disabled by default");
    return true;
}

bool ConfigureDisplayControlChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (api.clientAddDynamicChannel == nullptr) {
        error = "FreeRDP display-control channel helper is not loaded";
        return false;
    }

    const char* params[] = {DISP_CHANNEL_NAME};
    if (!api.clientAddDynamicChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set disp dynamic channel failed";
        return false;
    }

    log("FreeRDP display-control requested: dynamic disp channel");
    return true;
}

bool ConfigureClipboardChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (api.clientAddStaticChannel == nullptr) {
        error = "FreeRDP static channel helper is not loaded";
        return false;
    }

    const char* params[] = {"cliprdr"};
    if (!api.clientAddStaticChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set cliprdr static channel failed";
        return false;
    }

    log("FreeRDP clipboard requested: static cliprdr text only");
    return true;
}

bool ConfigureAudioPlaybackChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const std::function<void(const std::string&)>& log, std::string& error)
{
    if (api.clientAddStaticChannel == nullptr || api.clientAddDynamicChannel == nullptr) {
        error = "FreeRDP audio channel helpers are not loaded";
        return false;
    }

    const char* params[] = {
        "rdpsnd",
        "sys:ohos",
        "format:1",
        "rate:44100",
        "channel:2",
        "latency:100",
        "quality:high"
    };
    if (!api.clientAddStaticChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set rdpsnd static channel failed";
        return false;
    }
    if (!api.clientAddDynamicChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set rdpsnd dynamic channel failed";
        return false;
    }
    if (!SetFreerdpBool(api, settings, FreeRDP_AudioPlayback, true, "AudioPlayback", error)) {
        return false;
    }

    log("FreeRDP audio playback requested with static and dynamic rdpsnd sys:ohos PCM 44.1kHz stereo latency 100ms");
    log("FreeRDP AudioPlayback enabled so the logon Info Packet does not request no-audio playback");
    log("FreeRDP microphone capture remains disabled");
    return true;
}
#endif

} // namespace rdp_bridge
