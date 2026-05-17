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
    using RdpsndClientGetDiagnosticsFn = const char* (*)();
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
    RdpsndClientGetDiagnosticsFn rdpsndClientGetDiagnostics = nullptr;
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
