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
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <winpr/synch.h>
#endif

#if defined(HARMONY_HAS_FREERDP_HEADERS) && defined(HARMONY_HAS_FREERDP_OHOS_CLIENT_SOURCE)
#include <client/OHOS/ohos_certificate.h>
#include <client/OHOS/ohos_clipboard.h>
#include <client/OHOS/ohos_display.h>
#include <client/OHOS/ohos_graphics.h>
#include <client/OHOS/ohos_ime.h>
#include <client/OHOS/ohos_keyboard.h>
#include <client/OHOS/ohos_pointer.h>
#include <client/OHOS/ohos_rdpgfx.h>
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

    using FreerdpNewFn = freerdp* (*)();
    using FreerdpFreeFn = void (*)(freerdp*);
    using RegisterAddinProviderFn = int (*)(FREERDP_LOAD_CHANNEL_ADDIN_ENTRY_FN, DWORD);
    using ContextNewFn = BOOL (*)(freerdp*);
    using ContextFreeFn = void (*)(freerdp*);
    using ConnectFn = BOOL (*)(freerdp*);
    using DisconnectFn = BOOL (*)(freerdp*);
    using AbortConnectContextFn = BOOL (*)(rdpContext*);
    using ShallDisconnectContextFn = BOOL (*)(const rdpContext*);
    using GetEventHandlesFn = DWORD (*)(rdpContext*, HANDLE*, DWORD);
    using CheckEventHandlesFn = BOOL (*)(rdpContext*);
    using GetLastErrorFn = UINT32 (*)(const rdpContext*);
    using GetLastErrorTextFn = const char* (*)(UINT32);
    using SettingsGetBoolFn = BOOL (*)(const rdpSettings*, FreeRDP_Settings_Keys_Bool);
    using SettingsGetUint32Fn = UINT32 (*)(const rdpSettings*, FreeRDP_Settings_Keys_UInt32);
    using SettingsSetStringFn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_String, const char*);
    using SettingsSetUint32Fn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_UInt32, UINT32);
    using SettingsSetBoolFn = BOOL (*)(rdpSettings*, FreeRDP_Settings_Keys_Bool, BOOL);
    using GdiInitFn = BOOL (*)(freerdp*, UINT32);
    using GdiFreeFn = void (*)(freerdp*);
    using GdiResizeFn = BOOL (*)(rdpGdi*, UINT32, UINT32);
    using InputSendMouseEventFn = BOOL (*)(rdpInput*, UINT16, UINT16, UINT16);
    using InputSendKeyboardEventFn = BOOL (*)(rdpInput*, UINT16, UINT8);
    using InputSendKeyboardEventExFn = BOOL (*)(rdpInput*, BOOL, BOOL, UINT32);
    using InputSendUnicodeKeyboardEventFn = BOOL (*)(rdpInput*, UINT16, UINT16);
    using InputSendFocusInEventFn = BOOL (*)(rdpInput*, UINT16);
    using ChannelsLoadStaticAddinEntryFn = PVIRTUALCHANNELENTRY (*)(LPCSTR, LPCSTR, LPCSTR, DWORD);
    using ClientLoadChannelsFn = BOOL (*)(freerdp*);
    using ClientAddStaticChannelFn = BOOL (*)(rdpSettings*, size_t, const char* const*);
    using ClientAddDynamicChannelFn = BOOL (*)(rdpSettings*, size_t, const char* const*);
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
    using OhosKeyboardMapKeyCodeToWindowsVkFn = uint32_t (*)(uint32_t);
    using OhosKeyboardKeyCodeRequiresExtendedScancodeFn = int (*)(uint32_t);
    using OhosKeyboardFormatEventFn = int (*)(const FREERDP_OHOS_KEY_EVENT*, char*, size_t);
    using OhosKeyboardStateNewFn = FREERDP_OHOS_KEYBOARD_STATE* (*)();
    using OhosKeyboardStateFreeFn = void (*)(FREERDP_OHOS_KEYBOARD_STATE*);
    using OhosKeyboardStateResetFn = void (*)(FREERDP_OHOS_KEYBOARD_STATE*);
    using OhosKeyboardStateHandleEventFn = int (*)(FREERDP_OHOS_KEYBOARD_STATE*,
        const FREERDP_OHOS_KEY_EVENT*, FREERDP_OHOS_KEY_PACKET*, size_t, size_t*);
    using OhosKeyboardStateCollectDueRepeatsFn = int (*)(FREERDP_OHOS_KEYBOARD_STATE*,
        FREERDP_OHOS_KEY_PACKET*, size_t, size_t*);
    using OhosKeyboardStateReleaseAllFn = int (*)(FREERDP_OHOS_KEYBOARD_STATE*,
        FREERDP_OHOS_KEY_PACKET*, size_t, size_t*);
    using OhosImeBuildCommittedTextPacketsFn = int (*)(const uint16_t*, size_t,
        FREERDP_OHOS_IME_PACKET*, size_t, size_t*, size_t*);
    using OhosImeFormatCommittedTextResultFn = int (*)(size_t, size_t, size_t, char*, size_t);
    using OhosPointerBuildEventFn = BOOL (*)(const FREERDP_OHOS_POINTER_VIEWPORT*,
        const FREERDP_OHOS_POINTER_EVENT*, FREERDP_OHOS_POINTER_PACKET*, char*, size_t);
    using OhosCertificatePolicyFromStringFn = UINT32 (*)(const char*);
    using OhosCertificatePolicyNameFn = const char* (*)(UINT32);
    using OhosCertificateVerifyFn = DWORD (*)(
        UINT32, const FREERDP_OHOS_CERTIFICATE_VERIFY_INFO*, char*, size_t);
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
    using OhosSessionConfigDefaultFn = FREERDP_OHOS_SESSION_CONFIG (*)();
    using OhosSessionApplySettingsFn = BOOL (*)(rdpSettings*,
        const FREERDP_OHOS_SESSION_CONFIG*, char*, size_t);
    using OhosSessionApplyConnectionSettingsFn = BOOL (*)(
        rdpSettings*, const FREERDP_OHOS_CONNECTION_CONFIG*, char*, size_t);
    using OhosSessionAddStandardChannelsFn = BOOL (*)(rdpSettings*,
        const FREERDP_OHOS_SESSION_CONFIG*, char*, size_t);
    using OhosAvcodecSetOutputSurfaceFn = BOOL (*)(void*, UINT32, UINT32, BOOL);
    using OhosAvcodecSetAvc444OutputSurfacesFn = BOOL (*)(void*, void*, UINT32, UINT32, BOOL);
    using OhosAvc444FrameCallbackFn = void (*)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, void*);
    using OhosAvcodecFallbackCallbackFn = void (*)(const char*, void*);
    using OhosAvcodecSetAvc444SurfaceRouteEnabledFn = BOOL (*)(BOOL);
    using OhosAvcodecSetAvc444FrameCallbackFn = BOOL (*)(OhosAvc444FrameCallbackFn, void*);
    using OhosAvcodecSetFallbackCallbackFn = BOOL (*)(OhosAvcodecFallbackCallbackFn, void*);
    using OhosAvcodecGetDiagnosticsFn = const char* (*)();
    using WaitForMultipleObjectsFn = DWORD (*)(DWORD, const HANDLE*, BOOL, DWORD);

    FreerdpNewFn freerdpNew = nullptr;
    FreerdpFreeFn freerdpFree = nullptr;
    RegisterAddinProviderFn registerAddinProvider = nullptr;
    ContextNewFn contextNew = nullptr;
    ContextFreeFn contextFree = nullptr;
    ConnectFn connect = nullptr;
    DisconnectFn disconnect = nullptr;
    AbortConnectContextFn abortConnectContext = nullptr;
    ShallDisconnectContextFn shallDisconnectContext = nullptr;
    GetEventHandlesFn getEventHandles = nullptr;
    CheckEventHandlesFn checkEventHandles = nullptr;
    GetLastErrorFn getLastError = nullptr;
    GetLastErrorTextFn getLastErrorName = nullptr;
    GetLastErrorTextFn getLastErrorString = nullptr;
    SettingsGetBoolFn settingsGetBool = nullptr;
    SettingsGetUint32Fn settingsGetUint32 = nullptr;
    SettingsSetStringFn settingsSetString = nullptr;
    SettingsSetUint32Fn settingsSetUint32 = nullptr;
    SettingsSetBoolFn settingsSetBool = nullptr;
    GdiInitFn gdiInit = nullptr;
    GdiFreeFn gdiFree = nullptr;
    GdiResizeFn gdiResize = nullptr;
    InputSendMouseEventFn inputSendMouseEvent = nullptr;
    InputSendKeyboardEventFn inputSendKeyboardEvent = nullptr;
    InputSendKeyboardEventExFn inputSendKeyboardEventEx = nullptr;
    InputSendUnicodeKeyboardEventFn inputSendUnicodeKeyboardEvent = nullptr;
    InputSendFocusInEventFn inputSendFocusInEvent = nullptr;
    ChannelsLoadStaticAddinEntryFn channelsLoadStaticAddinEntry = nullptr;
    ClientLoadChannelsFn clientLoadChannels = nullptr;
    ClientAddStaticChannelFn clientAddStaticChannel = nullptr;
    ClientAddDynamicChannelFn clientAddDynamicChannel = nullptr;
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
    OhosKeyboardMapKeyCodeToWindowsVkFn ohosKeyboardMapKeyCodeToWindowsVk = nullptr;
    OhosKeyboardKeyCodeRequiresExtendedScancodeFn ohosKeyboardKeyCodeRequiresExtendedScancode =
        nullptr;
    OhosKeyboardFormatEventFn ohosKeyboardFormatEvent = nullptr;
    OhosKeyboardStateNewFn ohosKeyboardStateNew = nullptr;
    OhosKeyboardStateFreeFn ohosKeyboardStateFree = nullptr;
    OhosKeyboardStateResetFn ohosKeyboardStateReset = nullptr;
    OhosKeyboardStateHandleEventFn ohosKeyboardStateHandleEvent = nullptr;
    OhosKeyboardStateCollectDueRepeatsFn ohosKeyboardStateCollectDueRepeats = nullptr;
    OhosKeyboardStateReleaseAllFn ohosKeyboardStateReleaseAll = nullptr;
    OhosImeBuildCommittedTextPacketsFn ohosImeBuildCommittedTextPackets = nullptr;
    OhosImeFormatCommittedTextResultFn ohosImeFormatCommittedTextResult = nullptr;
    OhosPointerBuildEventFn ohosPointerBuildEvent = nullptr;
    OhosCertificatePolicyFromStringFn ohosCertificatePolicyFromString = nullptr;
    OhosCertificatePolicyNameFn ohosCertificatePolicyName = nullptr;
    OhosCertificateVerifyFn ohosCertificateVerify = nullptr;
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
    OhosSessionConfigDefaultFn ohosSessionConfigDefault = nullptr;
    OhosSessionApplySettingsFn ohosSessionApplySettings = nullptr;
    OhosSessionApplyConnectionSettingsFn ohosSessionApplyConnectionSettings = nullptr;
    OhosSessionAddStandardChannelsFn ohosSessionAddStandardChannels = nullptr;
    OhosAvcodecSetOutputSurfaceFn ohosAvcodecSetOutputSurface = nullptr;
    OhosAvcodecSetAvc444OutputSurfacesFn ohosAvcodecSetAvc444OutputSurfaces = nullptr;
    OhosAvcodecSetAvc444SurfaceRouteEnabledFn ohosAvcodecSetAvc444SurfaceRouteEnabled = nullptr;
    OhosAvcodecSetAvc444FrameCallbackFn ohosAvcodecSetAvc444FrameCallback = nullptr;
    OhosAvcodecSetFallbackCallbackFn ohosAvcodecSetFallbackCallback = nullptr;
    OhosAvcodecGetDiagnosticsFn ohosAvcodecGetDiagnostics = nullptr;
    WaitForMultipleObjectsFn waitForMultipleObjects = nullptr;

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
