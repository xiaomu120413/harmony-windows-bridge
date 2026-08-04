#include "freerdp/freerdp_runtime.h"

#include "common/string_utils.h"

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

    loaded_ = LoadFreerdpSymbol("freerdp_abort_connect_context", abortConnectContext, error) &&
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
        LoadWinprSymbol("PubSub_Subscribe", pubSubSubscribe, error) &&
        LoadWinprSymbol("PubSub_Unsubscribe", pubSubUnsubscribe, error);
    if (loaded_) {
        LoadOptionalFreerdpSymbol("gdi_graphics_pipeline_init", gdiGraphicsPipelineInit);
        LoadOptionalFreerdpSymbol("gdi_graphics_pipeline_uninit", gdiGraphicsPipelineUninit);
        LoadOptionalFreerdpSymbol("graphics_register_pointer", graphicsRegisterPointer);
        LoadOptionalFreerdpSymbol("freerdp_image_copy_from_pointer_data", imageCopyFromPointerData);
        LoadOptionalClientSymbol("rdpgfx_client_context_new", rdpgfxClientContextNew);
        LoadOptionalClientSymbol("rdpgfx_client_context_free", rdpgfxClientContextFree);
        LoadOptionalClientSymbol("freerdp_rdpsnd_ohos_get_stats", rdpsndOhosGetStats);
        LoadOptionalClientSymbol("freerdp_rdpsnd_ohos_get_diagnostics", rdpsndOhosGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_audin_ohos_get_diagnostics", audinOhosGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_audin_ohos_set_permission_callback",
            audinOhosSetPermissionCallback);
        LoadOptionalClientSymbol("freerdp_rdpecam_ohos_set_permission_callback",
            rdpecamOhosSetPermissionCallback);
        LoadOptionalClientSymbol("freerdp_ohos_location_set_permission_callback",
            ohosLocationSetPermissionCallback);
        LoadOptionalClientSymbol("freerdp_rdpsnd_client_get_diagnostics", rdpsndClientGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_new", ohosClipboardNew);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_register", ohosClipboardRegister);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_free", ohosClipboardFree);
        LoadOptionalClientSymbol("freerdp_ohos_clipboard_get_diagnostics",
            ohosClipboardGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_new", ohosInputQueueNew);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_free", ohosInputQueueFree);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_clear", ohosInputQueueClear);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_reset", ohosInputQueueReset);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_pointer",
            ohosInputQueueEnqueuePointer);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_pointer_packet",
            ohosInputQueueEnqueuePointerPacket);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_key_scancode",
            ohosInputQueueEnqueueKeyScancode);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_key",
            ohosInputQueueEnqueueKey);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_unicode",
            ohosInputQueueEnqueueUnicode);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_text",
            ohosInputQueueEnqueueText);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_focus_in",
            ohosInputQueueEnqueueFocusIn);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_enqueue_release_all_keys",
            ohosInputQueueEnqueueReleaseAllKeys);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_drain", ohosInputQueueDrain);
        LoadOptionalClientSymbol("freerdp_ohos_input_queue_get_diagnostics",
            ohosInputQueueGetDiagnostics);
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
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_set_avc420_gpu_output_active",
            ohosRdpgfxBridgeSetAvc420OutputActive);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_attach",
            ohosRdpgfxBridgeAttach);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_detach",
            ohosRdpgfxBridgeDetach);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_set_gdi_attached",
            ohosRdpgfxBridgeSetGdiAttached);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_bridge_get_diagnostics",
            ohosRdpgfxBridgeGetDiagnostics);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_avc444_command_lc_is_valid",
            ohosRdpgfxAvc444CommandLcIsValid);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_rects_valid",
            ohosRdpgfxRectsValid);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_rects_cover_full_surface",
            ohosRdpgfxRectsCoverFullSurface);
        LoadOptionalClientSymbol("freerdp_ohos_rdpgfx_avc444_chroma_v1_required_y_height",
            ohosRdpgfxAvc444ChromaV1RequiredYHeight);
        LoadOptionalClientSymbol("freerdp_ohos_session_config_default",
            ohosSessionConfigDefault);
        LoadOptionalClientSymbol("freerdp_ohos_session_prepare_options",
            ohosSessionPrepareOptions);
        LoadOptionalClientSymbol("freerdp_ohos_session_new",
            ohosSessionNew);
        LoadOptionalClientSymbol("freerdp_ohos_session_free",
            ohosSessionFree);
        LoadOptionalClientSymbol("freerdp_ohos_session_connect",
            ohosSessionConnect);
        LoadOptionalClientSymbol("freerdp_ohos_session_disconnect",
            ohosSessionDisconnect);
        LoadOptionalClientSymbol("freerdp_ohos_session_attach_display_control",
            ohosSessionAttachDisplayControl);
        LoadOptionalClientSymbol("freerdp_ohos_session_detach_display_control",
            ohosSessionDetachDisplayControl);
        LoadOptionalClientSymbol("freerdp_ohos_session_resize_ex",
            ohosSessionResizeEx);
        LoadOptionalClientSymbol("freerdp_ohos_session_get_diagnostics",
            ohosSessionGetDiagnostics);
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

} // namespace rdp_bridge
