#pragma once

#include <cstdint>
#include <string>
#include <vector>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/addin.h>
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <winpr/synch.h>
#endif

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
#include <client/OHOS/ohos_clipboard.h>
#include <client/OHOS/ohos_avc420_route.h>
#include <client/OHOS/ohos_display.h>
#include <client/OHOS/ohos_graphics.h>
#include <client/OHOS/ohos_ime.h>
#include <client/OHOS/ohos_input_queue.h>
#include <client/OHOS/ohos_keyboard.h>
#include <client/OHOS/ohos_pointer.h>
#include <client/OHOS/ohos_rdpgfx.h>
#include <client/OHOS/ohos_session.h>
#include <client/OHOS/ohos_session_config.h>
#endif

namespace rdp_bridge {

class FreerdpRuntimeApi;

std::string SharedLibraryDirectory();
std::string EnsureOpenSslModulesPath();

#if defined(HARMONY_HAS_FREERDP_HEADERS)
class FreerdpRuntimeApi {
public:
    // Keep FreeRDP/WinPR loaded for the process lifetime; WinPR registers TLS destructors.
    ~FreerdpRuntimeApi() = default;

    bool Load(std::string& error);

    using AbortConnectContextFn = BOOL (*)(rdpContext*);
    using GetLastErrorFn = UINT32 (*)(const rdpContext*);
    using GetLastErrorTextFn = const char* (*)(UINT32);
    using SettingsGetUint32Fn = UINT32 (*)(const rdpSettings*, FreeRDP_Settings_Keys_UInt32);
    using SettingsSetStringFn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_String, const char*);
    using SettingsSetUint32Fn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_UInt32, UINT32);
    using SettingsSetBoolFn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_Bool, BOOL);
    using GdiInitFn = BOOL (*)(freerdp*, UINT32);
    using GdiFreeFn = void (*)(freerdp*);
    using GdiResizeFn = BOOL (*)(rdpGdi*, UINT32, UINT32);
    using PubSubSubscribeFn = int (*)(wPubSub*, const char*, ...);
    using PubSubUnsubscribeFn = int (*)(wPubSub*, const char*, ...);
    using GdiGraphicsPipelineInitFn = BOOL (*)(rdpGdi*, RdpgfxClientContext*);
    using GdiGraphicsPipelineUninitFn = void (*)(rdpGdi*, RdpgfxClientContext*);
    using RdpgfxClientContextNewFn = RdpgfxClientContext* (*)(rdpContext*);
    using RdpgfxClientContextFreeFn = void (*)(RdpgfxClientContext*);
    using RdpsndOhosGetStatsFn = BOOL (*)(UINT64*, UINT64*, UINT64*, UINT64*, UINT64*, UINT64*,
        UINT64*, UINT64*, UINT32*, UINT16*, UINT16*, UINT32*);
    using RdpsndOhosGetDiagnosticsFn = const char* (*)();
    using AudinOhosGetDiagnosticsFn = const char* (*)();
    using AudinOhosPermissionRequestFn = BOOL (*)(void*, UINT32);
    using AudinOhosSetPermissionCallbackFn = BOOL (*)(AudinOhosPermissionRequestFn, void*);
    using RdpsndClientGetDiagnosticsFn = const char* (*)();
    using OhosClipboardNewFn = freerdpOhosClipboard* (*)();
    using OhosClipboardRegisterFn = BOOL (*)(freerdpOhosClipboard*, rdpContext*,
        const FREERDP_OHOS_CLIPBOARD_CONFIG*, char*, size_t);
    using OhosClipboardFreeFn = void (*)(freerdpOhosClipboard*);
    using OhosClipboardGetDiagnosticsFn = const char* (*)(freerdpOhosClipboard*);
    using OhosInputQueueNewFn = freerdpOhosInputQueue* (*)();
    using OhosInputQueueFreeFn = void (*)(freerdpOhosInputQueue*);
    using OhosInputQueueClearFn = void (*)(freerdpOhosInputQueue*);
    using OhosInputQueueResetFn = void (*)(freerdpOhosInputQueue*);
    using OhosInputQueueEnqueuePointerFn = BOOL (*)(
        freerdpOhosInputQueue*, const FREERDP_OHOS_POINTER_VIEWPORT*,
        const FREERDP_OHOS_POINTER_EVENT*, char*, size_t);
    using OhosInputQueueEnqueuePointerPacketFn = BOOL (*)(
        freerdpOhosInputQueue*, UINT16, UINT16, UINT16, char*, size_t);
    using OhosInputQueueEnqueueKeyScancodeFn = BOOL (*)(
        freerdpOhosInputQueue*, UINT32, BOOL, BOOL, char*, size_t);
    using OhosInputQueueEnqueueKeyFn = BOOL (*)(
        freerdpOhosInputQueue*, const FREERDP_OHOS_KEY_EVENT*, char*, size_t);
    using OhosInputQueueEnqueueUnicodeFn = BOOL (*)(
        freerdpOhosInputQueue*, UINT32, BOOL, char*, size_t);
    using OhosInputQueueEnqueueTextFn = BOOL (*)(
        freerdpOhosInputQueue*, const uint16_t*, size_t, char*, size_t);
    using OhosInputQueueEnqueueFocusInFn = BOOL (*)(
        freerdpOhosInputQueue*, UINT16, char*, size_t);
    using OhosInputQueueEnqueueReleaseAllKeysFn = BOOL (*)(
        freerdpOhosInputQueue*, char*, size_t);
    using OhosInputQueueDrainFn = BOOL (*)(freerdpOhosInputQueue*, rdpContext*, char*, size_t);
    using OhosInputQueueGetDiagnosticsFn = BOOL (*)(
        freerdpOhosInputQueue*, FREERDP_OHOS_INPUT_QUEUE_DIAGNOSTICS*);
    using OhosDisplayNormalizeSizeFn = void (*)(uint32_t, uint32_t, uint32_t, uint32_t*,
        uint32_t*);
    using OhosDisplaySendMonitorLayoutFn = int (*)(DispClientContext*, uint32_t, uint32_t,
        uint32_t, uint32_t*, uint32_t*, uint32_t*, char*, size_t);
    using OhosGraphicsConfigFromModeFn = FREERDP_OHOS_GRAPHICS_CONFIG (*)(const char*);
    using OhosGraphicsFallbackModesFn = size_t (*)(const char*, const char**, size_t);
    using OhosGraphicsShouldRetryFallbackFn = BOOL (*)(BOOL, BOOL, const char*, size_t, size_t,
        const char*);
    using OhosGraphicsAlignDownToMultipleFn = UINT32 (*)(UINT32, UINT32, UINT32);
    using OhosGraphicsAlignH264DesktopSizeFn = void (*)(const FREERDP_OHOS_GRAPHICS_CONFIG*,
        UINT32*, UINT32*);
    using OhosRdpgfxBridgeNewFn = freerdpOhosRdpgfxBridge* (*)();
    using OhosRdpgfxBridgeFreeFn = void (*)(freerdpOhosRdpgfxBridge*);
    using OhosRdpgfxBridgeResetFn = void (*)(freerdpOhosRdpgfxBridge*, BOOL, BOOL);
    using OhosRdpgfxBridgeSetSurfaceTargetFn = void (*)(freerdpOhosRdpgfxBridge*, UINT32, UINT32);
    using OhosRdpgfxBridgeAttachFn = BOOL (*)(freerdpOhosRdpgfxBridge*, RdpgfxClientContext*,
        const FREERDP_OHOS_RDPGFX_BRIDGE_CONFIG*, char*, size_t);
    using OhosRdpgfxBridgeDetachFn = void (*)(freerdpOhosRdpgfxBridge*, RdpgfxClientContext*);
    using OhosRdpgfxBridgeSetGdiAttachedFn = void (*)(freerdpOhosRdpgfxBridge*, BOOL);
    using OhosRdpgfxBridgeGetDiagnosticsFn = const char* (*)(freerdpOhosRdpgfxBridge*);
    using OhosRdpgfxAvc444CommandLcIsValidFn =
        BOOL (*)(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO*);
    using OhosRdpgfxRectsValidFn =
        BOOL (*)(const RECTANGLE_16*, UINT32, UINT32, UINT32);
    using OhosRdpgfxRectsCoverFullSurfaceFn =
        BOOL (*)(const RECTANGLE_16*, UINT32, UINT32, UINT32);
    using OhosRdpgfxAvc444ChromaV1RequiredYHeightFn =
        UINT32 (*)(const RECTANGLE_16*, UINT32);
    using OhosSessionConfigDefaultFn = FREERDP_OHOS_SESSION_CONFIG (*)();
    using OhosSessionPrepareOptionsFn = BOOL (*)(
        const FREERDP_OHOS_SESSION_INPUT*, FREERDP_OHOS_SESSION_PREPARED_OPTIONS*, char*,
        size_t);
    using OhosSessionNewFn = freerdpOhosSession* (*)();
    using OhosSessionFreeFn = void (*)(freerdpOhosSession*);
    using OhosSessionConnectFn = BOOL (*)(freerdpOhosSession*,
        const FREERDP_OHOS_SESSION_OPTIONS*, const FREERDP_OHOS_SESSION_CALLBACKS*, char*,
        size_t);
    using OhosSessionDisconnectFn = void (*)(freerdpOhosSession*);
    using OhosSessionGetDiagnosticsFn = const char* (*)(freerdpOhosSession*);
    using OhosAvcodecSetOutputSurfaceFn = BOOL (*)(void*, UINT32, UINT32, BOOL);
    using OhosAvcodecFallbackCallbackFn = void (*)(const char*, void*);
    using OhosAvcodecSetFallbackCallbackFn = BOOL (*)(OhosAvcodecFallbackCallbackFn, void*);
    using OhosAvcodecGetDiagnosticsFn = const char* (*)();
    using OhosAvc420RouteNewFn = freerdpOhosAvc420Route* (*)();
    using OhosAvc420RouteFreeFn = void (*)(freerdpOhosAvc420Route*);
    using OhosAvc420RouteConfigureFn = BOOL (*)(
        freerdpOhosAvc420Route*, const FREERDP_OHOS_AVC420_ROUTE_CONFIG*, char*, size_t);
    using OhosAvc420RouteResetFn = void (*)(freerdpOhosAvc420Route*);
    using OhosAvc420RouteSetOutputTargetFn = BOOL (*)(
        freerdpOhosAvc420Route*, const FREERDP_OHOS_AVC420_ROUTE_OUTPUT_TARGET*, char*, size_t);
    using OhosAvc420RouteClearOutputTargetFn = BOOL (*)(freerdpOhosAvc420Route*, char*, size_t);
    using OhosAvc420RouteBeginSurfaceFn = BOOL (*)(freerdpOhosAvc420Route*, char*, size_t);
    using OhosAvc420RouteEndSurfaceFn = void (*)(freerdpOhosAvc420Route*);
    using OhosAvc420RouteGetDiagnosticsFn = const char* (*)(freerdpOhosAvc420Route*);
    AbortConnectContextFn abortConnectContext = nullptr;
    GetLastErrorFn getLastError = nullptr;
    GetLastErrorTextFn getLastErrorName = nullptr;
    GetLastErrorTextFn getLastErrorString = nullptr;
    SettingsGetUint32Fn settingsGetUint32 = nullptr;
    SettingsSetStringFn settingsSetString = nullptr;
    SettingsSetUint32Fn settingsSetUint32 = nullptr;
    SettingsSetBoolFn settingsSetBool = nullptr;
    GdiInitFn gdiInit = nullptr;
    GdiFreeFn gdiFree = nullptr;
    GdiResizeFn gdiResize = nullptr;
    PubSubSubscribeFn pubSubSubscribe = nullptr;
    PubSubUnsubscribeFn pubSubUnsubscribe = nullptr;
    GdiGraphicsPipelineInitFn gdiGraphicsPipelineInit = nullptr;
    GdiGraphicsPipelineUninitFn gdiGraphicsPipelineUninit = nullptr;
    RdpgfxClientContextNewFn rdpgfxClientContextNew = nullptr;
    RdpgfxClientContextFreeFn rdpgfxClientContextFree = nullptr;
    RdpsndOhosGetStatsFn rdpsndOhosGetStats = nullptr;
    RdpsndOhosGetDiagnosticsFn rdpsndOhosGetDiagnostics = nullptr;
    AudinOhosGetDiagnosticsFn audinOhosGetDiagnostics = nullptr;
    AudinOhosSetPermissionCallbackFn audinOhosSetPermissionCallback = nullptr;
    RdpsndClientGetDiagnosticsFn rdpsndClientGetDiagnostics = nullptr;
    OhosClipboardNewFn ohosClipboardNew = nullptr;
    OhosClipboardRegisterFn ohosClipboardRegister = nullptr;
    OhosClipboardFreeFn ohosClipboardFree = nullptr;
    OhosClipboardGetDiagnosticsFn ohosClipboardGetDiagnostics = nullptr;
    OhosInputQueueNewFn ohosInputQueueNew = nullptr;
    OhosInputQueueFreeFn ohosInputQueueFree = nullptr;
    OhosInputQueueClearFn ohosInputQueueClear = nullptr;
    OhosInputQueueResetFn ohosInputQueueReset = nullptr;
    OhosInputQueueEnqueuePointerFn ohosInputQueueEnqueuePointer = nullptr;
    OhosInputQueueEnqueuePointerPacketFn ohosInputQueueEnqueuePointerPacket = nullptr;
    OhosInputQueueEnqueueKeyScancodeFn ohosInputQueueEnqueueKeyScancode = nullptr;
    OhosInputQueueEnqueueKeyFn ohosInputQueueEnqueueKey = nullptr;
    OhosInputQueueEnqueueUnicodeFn ohosInputQueueEnqueueUnicode = nullptr;
    OhosInputQueueEnqueueTextFn ohosInputQueueEnqueueText = nullptr;
    OhosInputQueueEnqueueFocusInFn ohosInputQueueEnqueueFocusIn = nullptr;
    OhosInputQueueEnqueueReleaseAllKeysFn ohosInputQueueEnqueueReleaseAllKeys = nullptr;
    OhosInputQueueDrainFn ohosInputQueueDrain = nullptr;
    OhosInputQueueGetDiagnosticsFn ohosInputQueueGetDiagnostics = nullptr;
    OhosDisplayNormalizeSizeFn ohosDisplayNormalizeSize = nullptr;
    OhosDisplaySendMonitorLayoutFn ohosDisplaySendMonitorLayout = nullptr;
    OhosGraphicsConfigFromModeFn ohosGraphicsConfigFromMode = nullptr;
    OhosGraphicsFallbackModesFn ohosGraphicsFallbackModes = nullptr;
    OhosGraphicsShouldRetryFallbackFn ohosGraphicsShouldRetryFallback = nullptr;
    OhosGraphicsAlignDownToMultipleFn ohosGraphicsAlignDownToMultiple = nullptr;
    OhosGraphicsAlignH264DesktopSizeFn ohosGraphicsAlignH264DesktopSize = nullptr;
    OhosRdpgfxBridgeNewFn ohosRdpgfxBridgeNew = nullptr;
    OhosRdpgfxBridgeFreeFn ohosRdpgfxBridgeFree = nullptr;
    OhosRdpgfxBridgeResetFn ohosRdpgfxBridgeReset = nullptr;
    OhosRdpgfxBridgeSetSurfaceTargetFn ohosRdpgfxBridgeSetSurfaceTarget = nullptr;
    OhosRdpgfxBridgeAttachFn ohosRdpgfxBridgeAttach = nullptr;
    OhosRdpgfxBridgeDetachFn ohosRdpgfxBridgeDetach = nullptr;
    OhosRdpgfxBridgeSetGdiAttachedFn ohosRdpgfxBridgeSetGdiAttached = nullptr;
    OhosRdpgfxBridgeGetDiagnosticsFn ohosRdpgfxBridgeGetDiagnostics = nullptr;
    OhosRdpgfxAvc444CommandLcIsValidFn ohosRdpgfxAvc444CommandLcIsValid = nullptr;
    OhosRdpgfxRectsValidFn ohosRdpgfxRectsValid = nullptr;
    OhosRdpgfxRectsCoverFullSurfaceFn ohosRdpgfxRectsCoverFullSurface = nullptr;
    OhosRdpgfxAvc444ChromaV1RequiredYHeightFn
        ohosRdpgfxAvc444ChromaV1RequiredYHeight = nullptr;
    OhosSessionConfigDefaultFn ohosSessionConfigDefault = nullptr;
    OhosSessionPrepareOptionsFn ohosSessionPrepareOptions = nullptr;
    OhosSessionNewFn ohosSessionNew = nullptr;
    OhosSessionFreeFn ohosSessionFree = nullptr;
    OhosSessionConnectFn ohosSessionConnect = nullptr;
    OhosSessionDisconnectFn ohosSessionDisconnect = nullptr;
    OhosSessionGetDiagnosticsFn ohosSessionGetDiagnostics = nullptr;
    OhosAvcodecSetOutputSurfaceFn ohosAvcodecSetOutputSurface = nullptr;
    OhosAvcodecSetFallbackCallbackFn ohosAvcodecSetFallbackCallback = nullptr;
    OhosAvcodecGetDiagnosticsFn ohosAvcodecGetDiagnostics = nullptr;
    OhosAvc420RouteNewFn ohosAvc420RouteNew = nullptr;
    OhosAvc420RouteFreeFn ohosAvc420RouteFree = nullptr;
    OhosAvc420RouteConfigureFn ohosAvc420RouteConfigure = nullptr;
    OhosAvc420RouteResetFn ohosAvc420RouteReset = nullptr;
    OhosAvc420RouteSetOutputTargetFn ohosAvc420RouteSetOutputTarget = nullptr;
    OhosAvc420RouteClearOutputTargetFn ohosAvc420RouteClearOutputTarget = nullptr;
    OhosAvc420RouteBeginSurfaceFn ohosAvc420RouteBeginSurface = nullptr;
    OhosAvc420RouteEndSurfaceFn ohosAvc420RouteEndSurface = nullptr;
    OhosAvc420RouteGetDiagnosticsFn ohosAvc420RouteGetDiagnostics = nullptr;

private:
    template <typename Fn>
    bool LoadFreerdpSymbol(const char* name, Fn& target, std::string& error)
    {
        return LoadSymbolFrom(freerdpHandle_, "libfreerdp3.so", name, target, error);
    }

    template <typename Fn>
    bool LoadClientSymbol(const char* name, Fn& target, std::string& error)
    {
        return LoadSymbolFrom(freerdpClientHandle_, "libfreerdp-client3.so", name, target, error);
    }

    template <typename Fn>
    void LoadOptionalFreerdpSymbol(const char* name, Fn& target)
    {
        void* symbol = LoadOptionalSymbolFrom(freerdpHandle_, name);
        if (symbol != nullptr) {
            target = reinterpret_cast<Fn>(symbol);
        }
    }

    template <typename Fn>
    void LoadOptionalClientSymbol(const char* name, Fn& target)
    {
        void* symbol = LoadOptionalSymbolFrom(freerdpClientHandle_, name);
        if (symbol != nullptr) {
            target = reinterpret_cast<Fn>(symbol);
        }
    }

    template <typename Fn>
    bool LoadWinprSymbol(const char* name, Fn& target, std::string& error)
    {
        return LoadSymbolFrom(winprHandle_, "libwinpr3.so", name, target, error);
    }

    template <typename Fn>
    bool LoadSymbolFrom(void* handle, const char* library, const char* name, Fn& target, std::string& error)
    {
        void* symbol = nullptr;
        if (!LoadRawSymbolFrom(handle, library, name, symbol, error)) {
            return false;
        }
        target = reinterpret_cast<Fn>(symbol);
        return true;
    }

    bool LoadRawSymbolFrom(void* handle, const char* library, const char* name, void*& symbol, std::string& error);
    void* LoadOptionalSymbolFrom(void* handle, const char* name);

    std::vector<void*> handles_;
    void* winprHandle_ = nullptr;
    void* freerdpHandle_ = nullptr;
    void* freerdpClientHandle_ = nullptr;
    bool loaded_ = false;
};

bool SetFreerdpString(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_String key, const std::string& value, const char* name,
    std::string& error);
bool SetFreerdpUint32(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_UInt32 key, uint32_t value, const char* name, std::string& error);
bool SetFreerdpBool(FreerdpRuntimeApi& api, rdpSettings* settings,
    FreeRDP_Settings_Keys_Bool key, bool value, const char* name, std::string& error);
std::string LastErrorMessage(FreerdpRuntimeApi& api, uint32_t code);

FreerdpRuntimeApi& SharedFreerdpRuntimeApi();
bool EnsureFreerdpRuntimeLoaded(FreerdpRuntimeApi& api, std::string& error);
#endif

} // namespace rdp_bridge
