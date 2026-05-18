#include "freerdp_runtime.h"

#include "string_utils.h"

#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <mutex>

namespace rdp_bridge {

std::string SharedLibraryDirectory()
{
    Dl_info info = {};
    if (dladdr(reinterpret_cast<void*>(&SharedLibraryDirectory), &info) == 0 ||
        info.dli_fname == nullptr) {
        return "";
    }

    std::string path = info.dli_fname;
    size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return "";
    }
    return path.substr(0, slash);
}

std::string EnsureOpenSslModulesPath()
{
    const char* current = std::getenv("OPENSSL_MODULES");
    if (current != nullptr && current[0] != '\0') {
        return current;
    }

    std::string libraryDir = SharedLibraryDirectory();
    if (libraryDir.empty()) {
        return "";
    }

    std::string modulesPath = libraryDir + "/ossl-modules";
    setenv("OPENSSL_MODULES", modulesPath.c_str(), 0);
    return modulesPath;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool FreerdpRuntimeApi::Load(std::string& error)
{
    if (loaded_) {
        return true;
    }

    const char* libraries[] = {
        "libz.so.1",
        "libcrypto.so.3",
        "libssl.so.3",
        "libcjson.so.1",
        "libwinpr3.so",
        "libfreerdp3.so",
        "libfreerdp-client3.so",
    };

    for (const char* library : libraries) {
        void* handle = dlopen(library, RTLD_NOW | RTLD_GLOBAL);
        if (handle == nullptr) {
            const char* detail = dlerror();
            error = std::string("dlopen ") + library + " failed: " +
                (detail == nullptr ? "unknown error" : detail);
            return false;
        }
        handles_.push_back(handle);
        if (std::strcmp(library, "libwinpr3.so") == 0) {
            winprHandle_ = handle;
        }
        if (std::strcmp(library, "libfreerdp3.so") == 0) {
            freerdpHandle_ = handle;
        }
        if (std::strcmp(library, "libfreerdp-client3.so") == 0) {
            freerdpClientHandle_ = handle;
        }
    }

    loaded_ = LoadFreerdpSymbol("freerdp_new", freerdpNew, error) &&
        LoadFreerdpSymbol("freerdp_free", freerdpFree, error) &&
        LoadFreerdpSymbol("freerdp_register_addin_provider", registerAddinProvider, error) &&
        LoadFreerdpSymbol("freerdp_context_new", contextNew, error) &&
        LoadFreerdpSymbol("freerdp_context_free", contextFree, error) &&
        LoadFreerdpSymbol("freerdp_connect", connect, error) &&
        LoadFreerdpSymbol("freerdp_disconnect", disconnect, error) &&
        LoadFreerdpSymbol("freerdp_abort_connect_context", abortConnectContext, error) &&
        LoadFreerdpSymbol("freerdp_shall_disconnect_context", shallDisconnectContext, error) &&
        LoadFreerdpSymbol("freerdp_get_event_handles", getEventHandles, error) &&
        LoadFreerdpSymbol("freerdp_check_event_handles", checkEventHandles, error) &&
        LoadFreerdpSymbol("freerdp_get_last_error", getLastError, error) &&
        LoadFreerdpSymbol("freerdp_get_last_error_name", getLastErrorName, error) &&
        LoadFreerdpSymbol("freerdp_get_last_error_string", getLastErrorString, error) &&
        LoadFreerdpSymbol("freerdp_settings_get_uint32", settingsGetUint32, error) &&
        LoadFreerdpSymbol("freerdp_settings_set_string", settingsSetString, error) &&
        LoadFreerdpSymbol("freerdp_settings_set_uint32", settingsSetUint32, error) &&
        LoadFreerdpSymbol("freerdp_settings_set_bool", settingsSetBool, error) &&
        LoadFreerdpSymbol("gdi_init", gdiInit, error) &&
        LoadFreerdpSymbol("gdi_free", gdiFree, error) &&
        LoadFreerdpSymbol("gdi_resize", gdiResize, error) &&
        LoadFreerdpSymbol("freerdp_input_send_mouse_event", inputSendMouseEvent, error) &&
        LoadFreerdpSymbol("freerdp_input_send_keyboard_event", inputSendKeyboardEvent, error) &&
        LoadFreerdpSymbol("freerdp_input_send_keyboard_event_ex", inputSendKeyboardEventEx, error) &&
        LoadFreerdpSymbol("freerdp_input_send_unicode_keyboard_event", inputSendUnicodeKeyboardEvent, error) &&
        LoadFreerdpSymbol("freerdp_input_send_focus_in_event", inputSendFocusInEvent, error) &&
        LoadClientSymbol("freerdp_channels_load_static_addin_entry", channelsLoadStaticAddinEntry, error) &&
        LoadClientSymbol("freerdp_client_load_channels", clientLoadChannels, error) &&
        LoadClientSymbol("freerdp_client_add_static_channel", clientAddStaticChannel, error) &&
        LoadClientSymbol("freerdp_client_add_dynamic_channel", clientAddDynamicChannel, error) &&
        LoadWinprSymbol("PubSub_Subscribe", pubSubSubscribe, error) &&
        LoadWinprSymbol("PubSub_Unsubscribe", pubSubUnsubscribe, error) &&
        LoadWinprSymbol("WaitForMultipleObjects", waitForMultipleObjects, error);
    if (loaded_) {
        LoadOptionalFreerdpSymbol("gdi_graphics_pipeline_init", gdiGraphicsPipelineInit);
        LoadOptionalFreerdpSymbol("gdi_graphics_pipeline_uninit", gdiGraphicsPipelineUninit);
        LoadOptionalClientSymbol("rdpgfx_client_context_new", rdpgfxClientContextNew);
        LoadOptionalClientSymbol("rdpgfx_client_context_free", rdpgfxClientContextFree);
        LoadOptionalClientSymbol("freerdp_rdpsnd_ohos_get_stats", rdpsndOhosGetStats);
        LoadOptionalClientSymbol("freerdp_rdpsnd_ohos_get_diagnostics", rdpsndOhosGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_audin_ohos_get_diagnostics", audinOhosGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_audin_ohos_set_permission_callback",
            audinOhosSetPermissionCallback);
        LoadOptionalClientSymbol("freerdp_rdpsnd_client_get_diagnostics", rdpsndClientGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_new", ohosClipboardNew);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_register", ohosClipboardRegister);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_free", ohosClipboardFree);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_get_diagnostics",
            ohosClipboardGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_map_keycode_to_windows_vk",
            ohosKeyboardMapKeyCodeToWindowsVk);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_keycode_requires_extended_scancode",
            ohosKeyboardKeyCodeRequiresExtendedScancode);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_format_event", ohosKeyboardFormatEvent);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_new", ohosKeyboardStateNew);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_free", ohosKeyboardStateFree);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_reset", ohosKeyboardStateReset);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_handle_event",
            ohosKeyboardStateHandleEvent);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_collect_due_repeats",
            ohosKeyboardStateCollectDueRepeats);
        LoadOptionalClientSymbol("freerdp_ohos_keyboard_state_release_all",
            ohosKeyboardStateReleaseAll);
        LoadOptionalClientSymbol("freerdp_ohos_ime_build_committed_text_packets",
            ohosImeBuildCommittedTextPackets);
        LoadOptionalClientSymbol("freerdp_ohos_ime_format_committed_text_result",
            ohosImeFormatCommittedTextResult);
        LoadOptionalClientSymbol("freerdp_ohos_pointer_build_event", ohosPointerBuildEvent);
        LoadOptionalClientSymbol("freerdp_ohos_certificate_policy_from_string",
            ohosCertificatePolicyFromString);
        LoadOptionalClientSymbol("freerdp_ohos_certificate_policy_name",
            ohosCertificatePolicyName);
        LoadOptionalClientSymbol("freerdp_ohos_certificate_verify",
            ohosCertificateVerify);
        LoadOptionalClientSymbol("freerdp_ohos_display_normalize_size",
            ohosDisplayNormalizeSize);
        LoadOptionalClientSymbol("freerdp_ohos_display_send_monitor_layout",
            ohosDisplaySendMonitorLayout);
        LoadOptionalClientSymbol("freerdp_ohos_graphics_config_from_mode",
            ohosGraphicsConfigFromMode);
        LoadOptionalClientSymbol("freerdp_ohos_graphics_fallback_modes",
            ohosGraphicsFallbackModes);
        LoadOptionalClientSymbol("freerdp_ohos_graphics_should_retry_fallback",
            ohosGraphicsShouldRetryFallback);
        LoadOptionalClientSymbol("freerdp_ohos_graphics_align_down_to_multiple",
            ohosGraphicsAlignDownToMultiple);
        LoadOptionalClientSymbol("freerdp_ohos_graphics_align_h264_desktop_size",
            ohosGraphicsAlignH264DesktopSize);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_new",
            ohosRdpgfxBridgeNew);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_free",
            ohosRdpgfxBridgeFree);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_reset",
            ohosRdpgfxBridgeReset);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_set_surface_target",
            ohosRdpgfxBridgeSetSurfaceTarget);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_attach",
            ohosRdpgfxBridgeAttach);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_detach",
            ohosRdpgfxBridgeDetach);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_set_gdi_attached",
            ohosRdpgfxBridgeSetGdiAttached);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_get_diagnostics",
            ohosRdpgfxBridgeGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_session_config_default",
            ohosSessionConfigDefault);
        LoadOptionalClientSymbol("freerdp_ohos_session_apply_settings",
            ohosSessionApplySettings);
        LoadOptionalClientSymbol("freerdp_ohos_session_apply_connection_settings",
            ohosSessionApplyConnectionSettings);
        LoadOptionalClientSymbol("freerdp_ohos_session_add_standard_channels",
            ohosSessionAddStandardChannels);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_output_surface",
            ohosAvcodecSetOutputSurface);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_output_surfaces",
            ohosAvcodecSetAvc444OutputSurfaces);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_surface_route_enabled",
            ohosAvcodecSetAvc444SurfaceRouteEnabled);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_frame_callback",
            ohosAvcodecSetAvc444FrameCallback);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_fallback_callback",
            ohosAvcodecSetFallbackCallback);
        LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_get_diagnostics",
            ohosAvcodecGetDiagnostics);
    }
    return loaded_;
}

bool FreerdpRuntimeApi::LoadRawSymbolFrom(
    void* handle, const char* library, const char* name, void*& symbol, std::string& error)
{
    if (handle == nullptr) {
        error = std::string(library) + " handle is not loaded";
        return false;
    }

    dlerror();
    symbol = dlsym(handle, name);
    const char* detail = dlerror();
    if (detail != nullptr || symbol == nullptr) {
        error = std::string("dlsym ") + library + "!" + name + " failed: " +
            (detail == nullptr ? "symbol not found" : detail);
        return false;
    }
    return true;
}

void* FreerdpRuntimeApi::LoadOptionalSymbolFrom(void* handle, const char* name)
{
    if (handle == nullptr) {
        return nullptr;
    }

    dlerror();
    void* symbol = dlsym(handle, name);
    if (dlerror() == nullptr && symbol != nullptr) {
        return symbol;
    }
    return nullptr;
}

bool SetFreerdpString(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_String key, const std::string& value, const char* name,
    std::string& error)
{
    if (api.settingsSetString(settings, key, value.c_str())) {
        return true;
    }
    error = std::string("set ") + name + " failed";
    return false;
}

bool SetFreerdpUint32(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_UInt32 key, uint32_t value, const char* name, std::string& error)
{
    if (api.settingsSetUint32(settings, key, value)) {
        return true;
    }
    error = std::string("set ") + name + " failed";
    return false;
}

bool SetFreerdpBool(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_Bool key, bool value, const char* name, std::string& error)
{
    if (api.settingsSetBool(settings, key, value ? TRUE : FALSE)) {
        return true;
    }
    error = std::string("set ") + name + " failed";
    return false;
}

std::string LastErrorMessage(FreerdpRuntimeApi& api, uint32_t code)
{
    const char* name = api.getLastErrorName == nullptr ? nullptr : api.getLastErrorName(code);
    const char* text = api.getLastErrorString == nullptr ? nullptr : api.getLastErrorString(code);

    std::string result = name == nullptr ? "UNKNOWN" : name;
    result += " [";
    result += Hex32(code);
    result += "]";
    if (text != nullptr && text[0] != '\0') {
        result += " ";
        result += text;
    }
    return result;
}

FreerdpRuntimeApi& SharedFreerdpRuntimeApi()
{
    static FreerdpRuntimeApi api;
    return api;
}

bool EnsureFreerdpRuntimeLoaded(FreerdpRuntimeApi& api, std::string& error)
{
    static std::mutex loadMutex;
    std::lock_guard<std::mutex> lock(loadMutex);
    return api.Load(error);
}
#endif

} // namespace rdp_bridge
