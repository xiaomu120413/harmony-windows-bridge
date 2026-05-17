#include "napi/native_api.h"
#include "bridge_log.h"
#include "napi_utils.h"
#include "net_utils.h"
#include "probe_utils.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <new>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <database/pasteboard/oh_pasteboard.h>
#include <database/pasteboard/oh_pasteboard_err_code.h>
#include <database/udmf/udmf.h>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/uds.h>
#include <hilog/log.h>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/addin.h>
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/rdpgfx.h>
#include <freerdp/codec/color.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gfx.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <freerdp/update.h>
#include <winpr/clipboard.h>
#include <winpr/synch.h>
#endif

namespace {

using namespace rdp_bridge;

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

struct ConnectParams {
    std::string host;
    std::string port;
    std::string username;
    std::string password;
    std::string resolution;
    std::string certPolicy;
    std::string graphicsMode;
    std::string appFilesDir;
};

struct CallbackData {
    std::string value;
};

struct SurfacePaintResult {
    bool ok = false;
    bool partial = false;
    std::string message;
    std::vector<std::string> logs;
};

struct DirtyFrameStats {
    bool valid = false;
    uint32_t rectCount = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t areaPermille = 0;
};

struct RgbaFrame {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t strideBytes = 0;
    std::string label;
    DirtyFrameStats dirty;
    uint64_t sequence = 0;
    uint64_t dirtySequenceStart = 0;
};

struct RenderStatsSnapshot {
    bool running = false;
    uint64_t queued = 0;
    uint64_t rendered = 0;
    uint64_t failed = 0;
    uint64_t replaced = 0;
    uint64_t throttled = 0;
    uint64_t fullRendered = 0;
    uint64_t partialRendered = 0;
    uint32_t pending = 0;
    uint32_t lastWidth = 0;
    uint32_t lastHeight = 0;
    uint32_t lastCopyUs = 0;
    uint32_t lastRenderUs = 0;
    uint32_t avgCopyUs = 0;
    uint32_t avgRenderUs = 0;
    uint32_t fpsX100 = 0;
    DirtyFrameStats lastDirty;
    uint32_t targetFrameIntervalMs = 0;
};

struct GraphicsPipelineConfig {
    bool enabled = false;
    bool h264 = false;
    std::string mode = "gdi";
};

struct DecoderSurfaceTarget {
    OHNativeWindow* window = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
};

struct Avc444SurfaceTargets {
    OHNativeWindow* lumaWindow = nullptr;
    OHNativeWindow* chromaWindow = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t lumaTexture = 0;
    uint32_t chromaTexture = 0;
    uint64_t lumaSurfaceId = 0;
    uint64_t chromaSurfaceId = 0;
};

std::atomic_bool g_rdpgfxRuntimeRequested{false};
std::atomic_bool g_rdpgfxH264Requested{false};
std::atomic_bool g_rdpgfxBridgeAttached{false};
std::atomic_uint32_t g_rdpgfxConnectedCount{0};
std::atomic_uint32_t g_rdpgfxDisconnectedCount{0};
std::atomic_uint32_t g_rdpgfxInitFailedCount{0};
std::atomic_uint64_t g_rdpgfxStartFrameCount{0};
std::atomic_uint64_t g_rdpgfxEndFrameCount{0};
std::atomic_uint64_t g_rdpgfxSurfaceCommandCount{0};
std::atomic_uint64_t g_rdpgfxCodecUncompressedCount{0};
std::atomic_uint64_t g_rdpgfxCodecCavideoCount{0};
std::atomic_uint64_t g_rdpgfxCodecClearCodecCount{0};
std::atomic_uint64_t g_rdpgfxCodecPlanarCount{0};
std::atomic_uint64_t g_rdpgfxCodecProgressiveCount{0};
std::atomic_uint64_t g_rdpgfxCodecAvc420Count{0};
std::atomic_uint64_t g_rdpgfxCodecAlphaCount{0};
std::atomic_uint64_t g_rdpgfxCodecAvc444Count{0};
std::atomic_uint64_t g_rdpgfxCodecAvc444v2Count{0};
std::atomic_uint64_t g_rdpgfxCodecUnknownCount{0};
std::atomic_uint32_t g_rdpgfxLastCodecId{0};
std::atomic_uint32_t g_rdpgfxLastSurfaceId{0};
std::atomic_uint32_t g_rdpgfxLastCommandWidth{0};
std::atomic_uint32_t g_rdpgfxLastCommandHeight{0};
std::atomic_bool g_avc420SurfaceOutputEnabled{false};
std::atomic_uint64_t g_avc444SurfaceFrameCallbackCount{0};

std::string DescribeDirtyStats(const DirtyFrameStats& dirty)
{
    if (!dirty.valid) {
        return "dirty=none";
    }

    std::ostringstream out;
    out << "dirtyRects=" << dirty.rectCount
        << " dirtyBox=" << dirty.x << "," << dirty.y << " "
        << dirty.width << "x" << dirty.height
        << " dirtyArea=" << (dirty.areaPermille / 10) << "."
        << (dirty.areaPermille % 10) << "%";
    return out.str();
}

DirtyFrameStats MergeDirtyStats(const DirtyFrameStats& first, const DirtyFrameStats& second,
    uint32_t frameWidth, uint32_t frameHeight)
{
    if (!first.valid) {
        return second;
    }
    if (!second.valid) {
        return first;
    }
    if (frameWidth == 0 || frameHeight == 0) {
        return DirtyFrameStats{};
    }

    DirtyFrameStats merged;
    const uint64_t firstRight = static_cast<uint64_t>(first.x) + first.width;
    const uint64_t firstBottom = static_cast<uint64_t>(first.y) + first.height;
    const uint64_t secondRight = static_cast<uint64_t>(second.x) + second.width;
    const uint64_t secondBottom = static_cast<uint64_t>(second.y) + second.height;
    const uint32_t right = static_cast<uint32_t>(
        std::min<uint64_t>(frameWidth, std::max(firstRight, secondRight)));
    const uint32_t bottom = static_cast<uint32_t>(
        std::min<uint64_t>(frameHeight, std::max(firstBottom, secondBottom)));

    merged.valid = true;
    merged.rectCount = first.rectCount + second.rectCount;
    if (merged.rectCount < first.rectCount) {
        merged.rectCount = UINT32_MAX;
    }
    merged.x = std::min(first.x, second.x);
    merged.y = std::min(first.y, second.y);
    merged.width = right > merged.x ? right - merged.x : 0;
    merged.height = bottom > merged.y ? bottom - merged.y : 0;
    const uint64_t frameArea = static_cast<uint64_t>(frameWidth) * frameHeight;
    const uint64_t dirtyArea = static_cast<uint64_t>(merged.width) * merged.height;
    merged.areaPermille = frameArea == 0 ? 0 :
        static_cast<uint32_t>((std::min(dirtyArea, frameArea) * 1000U + frameArea / 2U) / frameArea);
    return merged;
}

class FreerdpRuntimeApi;

void EmitNativeLog(const std::string& line);
SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame);
bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender = false);
void RequestSurfaceRepaint(const std::string& reason);
void RequestRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& reason);
void StartRenderPipeline();
void StopRenderPipeline();
DecoderSurfaceTarget SnapshotDecoderSurfaceTarget();
bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const std::function<void(const std::string&)>& log);
void OnAvc444SurfaceFrameDecoded(uint32_t surfaceId, uint32_t width, uint32_t height,
    uint32_t op, uint32_t codecId, void*);
void UpdateAvc420SurfaceOutputIfActive(const std::string& reason);
std::string BuildRenderStatsLog();
std::string BuildGraphicsPipelineStatsLog();
std::string BuildOHAudioStatsLog();

struct RdpSessionRunResult {
    bool available = false;
    bool connected = false;
    bool cancelled = false;
    bool failed = false;
    std::string message;
};

bool ParseUInt32(const std::string& value, uint32_t& result)
{
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    result = static_cast<uint32_t>(parsed);
    return true;
}

bool ParsePort(const std::string& value, uint32_t& port)
{
    uint32_t parsed = 0;
    if (!ParseUInt32(value, parsed) || parsed == 0 || parsed > 65535) {
        return false;
    }
    port = parsed;
    return true;
}

void ParseResolutionOrDefault(const std::string& value, uint32_t& width, uint32_t& height)
{
    width = 1280;
    height = 720;

    size_t separator = value.find('x');
    if (separator == std::string::npos) {
        separator = value.find('X');
    }
    if (separator == std::string::npos) {
        return;
    }

    uint32_t parsedWidth = 0;
    uint32_t parsedHeight = 0;
    if (!ParseUInt32(value.substr(0, separator), parsedWidth) ||
        !ParseUInt32(value.substr(separator + 1), parsedHeight) ||
        parsedWidth < 320 || parsedHeight < 240) {
        return;
    }

    width = parsedWidth;
    height = parsedHeight;
}

struct UserParts {
    std::string domain;
    std::string username;
};

std::string TrimAscii(const std::string& value);

UserParts SplitDomainUsername(const std::string& value)
{
    size_t separator = value.find('\\');
    size_t separatorLength = 1;
    const size_t ideographicSeparator = value.find("\xE3\x80\x81");
    if (separator == std::string::npos ||
        (ideographicSeparator != std::string::npos && ideographicSeparator < separator)) {
        separator = ideographicSeparator;
        separatorLength = 3;
    }

    if (separator == std::string::npos || separator == 0 || separator + separatorLength >= value.size()) {
        return {"", TrimAscii(value)};
    }

    std::string domain = TrimAscii(value.substr(0, separator));
    std::string username = TrimAscii(value.substr(separator + separatorLength));
    if (domain.empty() || username.empty()) {
        return {"", TrimAscii(value)};
    }
    return {domain, username};
}

std::string Hex32(uint32_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return stream.str();
}

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
class FreerdpRuntimeApi {
public:
    // Keep FreeRDP/WinPR loaded for the process lifetime; WinPR registers TLS destructors.
    ~FreerdpRuntimeApi() = default;

    bool Load(std::string& error)
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
            LoadFreerdpSymbol("freerdp_input_send_keyboard_event_ex", inputSendKeyboardEventEx, error) &&
            LoadFreerdpSymbol("freerdp_input_send_unicode_keyboard_event", inputSendUnicodeKeyboardEvent, error) &&
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
            LoadOptionalClientSymbol("freerdp_rdpsnd_client_get_diagnostics", rdpsndClientGetDiagnostics);
            LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_output_surface",
                ohosAvcodecSetOutputSurface);
            LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_output_surfaces",
                ohosAvcodecSetAvc444OutputSurfaces);
            LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_surface_route_enabled",
                ohosAvcodecSetAvc444SurfaceRouteEnabled);
            LoadOptionalFreerdpSymbol("freerdp_ohos_avcodec_set_avc444_frame_callback",
                ohosAvcodecSetAvc444FrameCallback);
        }
        return loaded_;
    }

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
    using OhosAvcodecSetAvc444SurfaceRouteEnabledFn = BOOL (*)(BOOL);
    using OhosAvcodecSetAvc444FrameCallbackFn = BOOL (*)(OhosAvc444FrameCallbackFn, void*);
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
        if (freerdpHandle_ == nullptr) {
            return;
        }

        dlerror();
        void* symbol = dlsym(freerdpHandle_, name);
        if (dlerror() == nullptr && symbol != nullptr) {
            target = reinterpret_cast<Fn>(symbol);
        }
    }

    template <typename Fn>
    void LoadOptionalClientSymbol(const char* name, Fn& target)
    {
        if (freerdpClientHandle_ == nullptr) {
            return;
        }

        dlerror();
        void* symbol = dlsym(freerdpClientHandle_, name);
        if (dlerror() == nullptr && symbol != nullptr) {
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
        if (handle == nullptr) {
            error = std::string(library) + " handle is not loaded";
            return false;
        }

        dlerror();
        void* symbol = dlsym(handle, name);
        const char* detail = dlerror();
        if (detail != nullptr || symbol == nullptr) {
            error = std::string("dlsym ") + library + "!" + name + " failed: " +
                (detail == nullptr ? "symbol not found" : detail);
            return false;
        }

        target = reinterpret_cast<Fn>(symbol);
        return true;
    }

    std::vector<void*> handles_;
    void* winprHandle_ = nullptr;
    void* freerdpHandle_ = nullptr;
    void* freerdpClientHandle_ = nullptr;
    bool loaded_ = false;
};

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

enum class CertificatePolicy {
    Tofu,
    Strict,
    Ignore,
};

const char* CertificatePolicyName(CertificatePolicy policy)
{
    switch (policy) {
        case CertificatePolicy::Strict:
            return "strict";
        case CertificatePolicy::Ignore:
            return "ignore";
        case CertificatePolicy::Tofu:
        default:
            return "tofu";
    }
}

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string TrimAscii(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string TrimTrailingSlashes(std::string value)
{
    while (value.size() > 1 && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

std::string JoinPath(const std::string& base, const std::string& child)
{
    if (base.empty()) {
        return child;
    }
    if (child.empty()) {
        return base;
    }
    if (base.back() == '/') {
        return base + child;
    }
    return base + "/" + child;
}

bool EnsureDirectory(const std::string& path, std::string& error)
{
    if (path.empty()) {
        error = "empty directory path";
        return false;
    }
    std::string current;
    size_t index = 0;
    if (path[0] == '/') {
        current = "/";
        index = 1;
    }
    while (index <= path.size()) {
        const size_t next = path.find('/', index);
        const std::string part = path.substr(index, next == std::string::npos ? std::string::npos : next - index);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '/') {
                current += "/";
            }
            current += part;
            if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) {
                error = "mkdir " + current + " failed: " + SystemErrorMessage(errno);
                return false;
            }
        }
        if (next == std::string::npos) {
            break;
        }
        index = next + 1;
    }
    return true;
}

bool ConfigureFreerdpStoragePaths(FreerdpRuntimeApi& api, rdpSettings* settings,
    const ConnectParams& params, const std::function<void(const std::string&)>& log, std::string& error)
{
    std::string filesDir = TrimTrailingSlashes(TrimAscii(params.appFilesDir));
    if (filesDir.empty()) {
        error = "appFilesDir is required for FreeRDP certificate storage";
        return false;
    }

    const std::string configPath = JoinPath(filesDir, "freerdp");
    if (!EnsureDirectory(configPath, error) ||
        !EnsureDirectory(JoinPath(configPath, "certs"), error) ||
        !EnsureDirectory(JoinPath(configPath, "server"), error)) {
        return false;
    }

    setenv("HOME", filesDir.c_str(), 1);
    setenv("XDG_CONFIG_HOME", filesDir.c_str(), 1);
    if (!SetFreerdpString(api, settings, FreeRDP_HomePath, filesDir, "HomePath", error) ||
        !SetFreerdpString(api, settings, FreeRDP_ConfigPath, configPath, "ConfigPath", error)) {
        return false;
    }
    log("FreeRDP storage path configured: " + configPath);
    return true;
}

CertificatePolicy ParseCertificatePolicy(const std::string& value)
{
    const std::string normalized = ToLowerAscii(TrimAscii(value));
    if (normalized == "strict" || normalized == "verify" || normalized == "valid-ca") {
        return CertificatePolicy::Strict;
    }
    if (normalized == "ignore" || normalized == "accept" || normalized == "insecure") {
        return CertificatePolicy::Ignore;
    }
    if (normalized == "deny" || normalized == "reject") {
        return CertificatePolicy::Strict;
    }
    return CertificatePolicy::Tofu;
}

GraphicsPipelineConfig ParseGraphicsPipelineConfig(const ConnectParams& params)
{
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

std::string SafeCString(const char* value)
{
    return value == nullptr ? "" : value;
}

std::mutex g_certificatePolicyMutex;
std::unordered_map<freerdp*, CertificatePolicy> g_certificatePolicies;

void RegisterCertificatePolicy(freerdp* instance, CertificatePolicy policy)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    if (instance == nullptr) {
        return;
    }
    g_certificatePolicies[instance] = policy;
}

void UnregisterCertificatePolicy(freerdp* instance)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    g_certificatePolicies.erase(instance);
}

CertificatePolicy LookupCertificatePolicy(freerdp* instance)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    auto it = g_certificatePolicies.find(instance);
    if (it == g_certificatePolicies.end()) {
        return CertificatePolicy::Tofu;
    }
    return it->second;
}

DWORD HarmonyVerifyCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    DWORD)
{
    CertificatePolicy policy = LookupCertificatePolicy(instance);
    const std::string target = SafeCString(host) + ":" + std::to_string(port);
    if (policy == CertificatePolicy::Ignore) {
        EmitNativeLog("Certificate accepted for current session by ignore policy: " + target);
        return 2;
    }
    if (policy == CertificatePolicy::Tofu) {
        EmitNativeLog("Certificate accepted by TOFU policy and requested for FreeRDP store: " + target +
            " cn=" + SafeCString(commonName));
        return 1;
    }

    EmitNativeLog("Certificate rejected by strict policy: " + target +
        " cn=" + SafeCString(commonName) + " issuer=" + SafeCString(issuer));
    if (fingerprint != nullptr && fingerprint[0] != '\0') {
        EmitNativeLog("Rejected certificate fingerprint/pem is available in native callback");
    }
    return 0;
}

DWORD HarmonyVerifyChangedCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    const char* oldSubject, const char* oldIssuer, const char* oldFingerprint, DWORD)
{
    CertificatePolicy policy = LookupCertificatePolicy(instance);
    const std::string target = SafeCString(host) + ":" + std::to_string(port);
    if (policy == CertificatePolicy::Ignore) {
        EmitNativeLog("Changed certificate accepted for current session by ignore policy: " + target);
        return 2;
    }

    EmitNativeLog("Changed certificate rejected by " + std::string(CertificatePolicyName(policy)) +
        " policy: " + target + " cn=" + SafeCString(commonName));
    if ((subject != nullptr && subject[0] != '\0') || (oldSubject != nullptr && oldSubject[0] != '\0')) {
        EmitNativeLog("Certificate subject changed from [" + SafeCString(oldSubject) + "] to [" +
            SafeCString(subject) + "]");
    }
    if ((issuer != nullptr && issuer[0] != '\0') || (oldIssuer != nullptr && oldIssuer[0] != '\0')) {
        EmitNativeLog("Certificate issuer changed from [" + SafeCString(oldIssuer) + "] to [" +
            SafeCString(issuer) + "]");
    }
    if ((fingerprint != nullptr && fingerprint[0] != '\0') ||
        (oldFingerprint != nullptr && oldFingerprint[0] != '\0')) {
        EmitNativeLog("Changed certificate fingerprint/pem is available in native callback");
    }
    return 0;
}

std::atomic_uint32_t g_freerdpRenderedFrameCount{0};
std::atomic_uint32_t g_freerdpRenderSkipCount{0};
std::atomic_uint32_t g_rdpDesktopWidth{0};
std::atomic_uint32_t g_rdpDesktopHeight{0};

void SetRdpDesktopSize(uint32_t width, uint32_t height)
{
    g_rdpDesktopWidth.store(width);
    g_rdpDesktopHeight.store(height);
}

void ClearRdpDesktopSize()
{
    SetRdpDesktopSize(0, 0);
}

DirtyFrameStats CaptureGdiDirtyStats(const rdpGdi* gdi)
{
    DirtyFrameStats stats;
    if (gdi == nullptr || gdi->width <= 0 || gdi->height <= 0 ||
        gdi->primary == nullptr || gdi->primary->hdc == nullptr ||
        gdi->primary->hdc->hwnd == nullptr) {
        return stats;
    }

    HGDI_WND hwnd = gdi->primary->hdc->hwnd;
    const uint32_t frameWidth = static_cast<uint32_t>(gdi->width);
    const uint32_t frameHeight = static_cast<uint32_t>(gdi->height);
    const uint64_t frameArea = static_cast<uint64_t>(frameWidth) * frameHeight;
    if (frameArea == 0) {
        return stats;
    }

    uint32_t minX = frameWidth;
    uint32_t minY = frameHeight;
    uint32_t maxX = 0;
    uint32_t maxY = 0;
    uint64_t dirtyArea = 0;

    auto addRegion = [&](const GDI_RGN* region) {
        if (region == nullptr || region->null || region->w <= 0 || region->h <= 0) {
            return;
        }

        const int64_t left = std::max<int64_t>(0, region->x);
        const int64_t top = std::max<int64_t>(0, region->y);
        const int64_t right = std::min<int64_t>(frameWidth, static_cast<int64_t>(region->x) + region->w);
        const int64_t bottom = std::min<int64_t>(frameHeight, static_cast<int64_t>(region->y) + region->h);
        if (right <= left || bottom <= top) {
            return;
        }

        const uint32_t clampedLeft = static_cast<uint32_t>(left);
        const uint32_t clampedTop = static_cast<uint32_t>(top);
        const uint32_t clampedRight = static_cast<uint32_t>(right);
        const uint32_t clampedBottom = static_cast<uint32_t>(bottom);
        minX = std::min(minX, clampedLeft);
        minY = std::min(minY, clampedTop);
        maxX = std::max(maxX, clampedRight);
        maxY = std::max(maxY, clampedBottom);
        dirtyArea += static_cast<uint64_t>(clampedRight - clampedLeft) *
            static_cast<uint64_t>(clampedBottom - clampedTop);
        ++stats.rectCount;
    };

    if (hwnd->ninvalid > 0 && hwnd->cinvalid != nullptr) {
        for (INT32 i = 0; i < hwnd->ninvalid; ++i) {
            addRegion(&hwnd->cinvalid[i]);
        }
    } else {
        addRegion(hwnd->invalid);
    }

    if (stats.rectCount == 0) {
        return stats;
    }

    stats.valid = true;
    stats.x = minX;
    stats.y = minY;
    stats.width = maxX > minX ? maxX - minX : 0;
    stats.height = maxY > minY ? maxY - minY : 0;
    const uint64_t cappedArea = std::min(dirtyArea, frameArea);
    stats.areaPermille = static_cast<uint32_t>((cappedArea * 1000U + frameArea / 2U) / frameArea);
    return stats;
}

BOOL HarmonyBeginPaint(rdpContext* context)
{
    if (context == nullptr || context->gdi == nullptr || context->gdi->primary == nullptr ||
        context->gdi->primary->hdc == nullptr || context->gdi->primary->hdc->hwnd == nullptr ||
        context->gdi->primary->hdc->hwnd->invalid == nullptr) {
        return TRUE;
    }

    context->gdi->primary->hdc->hwnd->invalid->null = TRUE;
    return TRUE;
}

BOOL HarmonyEndPaint(rdpContext* context)
{
    if (context == nullptr || context->gdi == nullptr) {
        return TRUE;
    }
    if (g_avc420SurfaceOutputEnabled.load()) {
        return TRUE;
    }

    rdpGdi* gdi = context->gdi;
    if (gdi->suppressOutput || gdi->primary_buffer == nullptr || gdi->width <= 0 ||
        gdi->height <= 0 || gdi->stride == 0) {
        return TRUE;
    }

    if (gdi->primary != nullptr && gdi->primary->hdc != nullptr &&
        gdi->primary->hdc->hwnd != nullptr) {
        HGDI_WND hwnd = gdi->primary->hdc->hwnd;
        if (hwnd->invalid != nullptr && hwnd->invalid->null) {
            return TRUE;
        }
    }

    RgbaFrame frame = {
        gdi->primary_buffer,
        static_cast<uint32_t>(gdi->width),
        static_cast<uint32_t>(gdi->height),
        static_cast<int32_t>(gdi->stride),
        "freerdp gdi",
        CaptureGdiDirtyStats(gdi),
    };
    const uint32_t frameCount = ++g_freerdpRenderedFrameCount;
    std::string queueMessage;
    if (!QueueSurfaceRgbaFrame(frame, queueMessage)) {
        const uint32_t skipCount = ++g_freerdpRenderSkipCount;
        if (skipCount <= 3 || skipCount % 120 == 0) {
            EmitNativeLog("FreeRDP GDI frame queue skipped: " + queueMessage);
        }
    } else {
        g_freerdpRenderSkipCount.store(0);
        if (frameCount <= 3 || frameCount % 60 == 0) {
            EmitNativeLog("FreeRDP GDI frame queued: " + queueMessage);
        }
    }

    if (gdi->primary != nullptr && gdi->primary->hdc != nullptr &&
        gdi->primary->hdc->hwnd != nullptr) {
        HGDI_WND hwnd = gdi->primary->hdc->hwnd;
        if (hwnd->invalid != nullptr) {
            hwnd->invalid->null = TRUE;
        }
        hwnd->ninvalid = 0;
    }
    return TRUE;
}

BOOL HarmonyDesktopResize(rdpContext* context)
{
    if (context == nullptr || context->settings == nullptr || context->gdi == nullptr) {
        return FALSE;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    const uint32_t width = api.settingsGetUint32(context->settings, FreeRDP_DesktopWidth);
    const uint32_t height = api.settingsGetUint32(context->settings, FreeRDP_DesktopHeight);
    StopRenderPipeline();
    if (width == 0 || height == 0 || !api.gdiResize(context->gdi, width, height)) {
        if (!g_avc420SurfaceOutputEnabled.load()) {
            StartRenderPipeline();
        }
        EmitNativeLog("FreeRDP desktop resize failed");
        return FALSE;
    }

    SetRdpDesktopSize(width, height);
    if (!g_avc420SurfaceOutputEnabled.load()) {
        StartRenderPipeline();
    }
    EmitNativeLog("FreeRDP desktop resized: " + std::to_string(width) + "x" + std::to_string(height));
    return TRUE;
}

BOOL HarmonyPostConnect(freerdp* instance)
{
    if (instance == nullptr || instance->context == nullptr || instance->context->update == nullptr) {
        return FALSE;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiInit == nullptr || !api.gdiInit(instance, PIXEL_FORMAT_RGBA32)) {
        EmitNativeLog("FreeRDP gdi_init failed");
        return FALSE;
    }

    rdpUpdate* update = instance->context->update;
    update->BeginPaint = HarmonyBeginPaint;
    update->EndPaint = HarmonyEndPaint;
    update->DesktopResize = HarmonyDesktopResize;
    if (g_avc420SurfaceOutputEnabled.load()) {
        StopRenderPipeline();
    } else {
        StartRenderPipeline();
    }
    g_freerdpRenderedFrameCount.store(0);
    g_freerdpRenderSkipCount.store(0);
    if (instance->context->settings != nullptr) {
        const uint32_t width = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopWidth);
        const uint32_t height = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopHeight);
        if (width > 0 && height > 0) {
            SetRdpDesktopSize(width, height);
            EmitNativeLog("FreeRDP desktop size: " + std::to_string(width) + "x" + std::to_string(height));
        }
    }
    EmitNativeLog("FreeRDP GDI callbacks registered");
    return TRUE;
}

void HarmonyPostDisconnect(freerdp* instance)
{
    StopRenderPipeline();
    if (instance == nullptr || instance->context == nullptr || instance->context->gdi == nullptr) {
        ClearRdpDesktopSize();
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiFree != nullptr) {
        api.gdiFree(instance);
        EmitNativeLog("FreeRDP GDI resources released");
    }
    ClearRdpDesktopSize();
}

const char* RdpgfxCodecName(uint32_t codecId)
{
    switch (codecId) {
        case RDPGFX_CODECID_UNCOMPRESSED:
            return "UNCOMPRESSED";
        case RDPGFX_CODECID_CAVIDEO:
            return "CAVIDEO";
        case RDPGFX_CODECID_CLEARCODEC:
            return "CLEARCODEC";
        case RDPGFX_CODECID_PLANAR:
            return "PLANAR";
        case RDPGFX_CODECID_CAPROGRESSIVE:
            return "CAPROGRESSIVE";
        case RDPGFX_CODECID_CAPROGRESSIVE_V2:
            return "CAPROGRESSIVE_V2";
        case RDPGFX_CODECID_AVC420:
            return "AVC420";
        case RDPGFX_CODECID_ALPHA:
            return "ALPHA";
        case RDPGFX_CODECID_AVC444:
            return "AVC444";
        case RDPGFX_CODECID_AVC444v2:
            return "AVC444v2";
        default:
            return "UNKNOWN";
    }
}

void ResetRdpgfxDiagnosticsStats()
{
    g_rdpgfxConnectedCount.store(0);
    g_rdpgfxDisconnectedCount.store(0);
    g_rdpgfxInitFailedCount.store(0);
    g_rdpgfxStartFrameCount.store(0);
    g_rdpgfxEndFrameCount.store(0);
    g_rdpgfxSurfaceCommandCount.store(0);
    g_rdpgfxCodecUncompressedCount.store(0);
    g_rdpgfxCodecCavideoCount.store(0);
    g_rdpgfxCodecClearCodecCount.store(0);
    g_rdpgfxCodecPlanarCount.store(0);
    g_rdpgfxCodecProgressiveCount.store(0);
    g_rdpgfxCodecAvc420Count.store(0);
    g_rdpgfxCodecAlphaCount.store(0);
    g_rdpgfxCodecAvc444Count.store(0);
    g_rdpgfxCodecAvc444v2Count.store(0);
    g_rdpgfxCodecUnknownCount.store(0);
    g_rdpgfxLastCodecId.store(0);
    g_rdpgfxLastSurfaceId.store(0);
    g_rdpgfxLastCommandWidth.store(0);
    g_rdpgfxLastCommandHeight.store(0);
}

void RecordRdpgfxSurfaceCommand(const RDPGFX_SURFACE_COMMAND& command)
{
    const uint64_t total = g_rdpgfxSurfaceCommandCount.fetch_add(1) + 1;
    g_rdpgfxLastCodecId.store(command.codecId);
    g_rdpgfxLastSurfaceId.store(command.surfaceId);
    g_rdpgfxLastCommandWidth.store(command.width);
    g_rdpgfxLastCommandHeight.store(command.height);

    switch (command.codecId) {
        case RDPGFX_CODECID_UNCOMPRESSED:
            g_rdpgfxCodecUncompressedCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CAVIDEO:
            g_rdpgfxCodecCavideoCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CLEARCODEC:
            g_rdpgfxCodecClearCodecCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_PLANAR:
            g_rdpgfxCodecPlanarCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CAPROGRESSIVE:
        case RDPGFX_CODECID_CAPROGRESSIVE_V2:
            g_rdpgfxCodecProgressiveCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC420:
            g_rdpgfxCodecAvc420Count.fetch_add(1);
            break;
        case RDPGFX_CODECID_ALPHA:
            g_rdpgfxCodecAlphaCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC444:
            g_rdpgfxCodecAvc444Count.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC444v2:
            g_rdpgfxCodecAvc444v2Count.fetch_add(1);
            break;
        default:
            g_rdpgfxCodecUnknownCount.fetch_add(1);
            break;
    }

    if (total <= 5 || total % 120 == 0) {
        EmitHilogInfo("rdpgfx surface command: total=" + std::to_string(total) +
            " codec=" + RdpgfxCodecName(command.codecId) +
            "(" + std::to_string(command.codecId) + ")" +
            " surface=" + std::to_string(command.surfaceId) +
            " rect=" + std::to_string(command.left) + "," + std::to_string(command.top) +
            " " + std::to_string(command.width) + "x" + std::to_string(command.height) +
            " counts=clear:" + std::to_string(g_rdpgfxCodecClearCodecCount.load()) +
            ",progressive:" + std::to_string(g_rdpgfxCodecProgressiveCount.load()) +
            ",avc420:" + std::to_string(g_rdpgfxCodecAvc420Count.load()) +
            ",avc444:" + std::to_string(g_rdpgfxCodecAvc444Count.load()) +
            ",raw:" + std::to_string(g_rdpgfxCodecUncompressedCount.load()) +
            ",unknown:" + std::to_string(g_rdpgfxCodecUnknownCount.load()));
    }
}

struct RdpgfxDiagnosticsHookState {
    pcRdpgfxStartFrame startFrame = nullptr;
    pcRdpgfxEndFrame endFrame = nullptr;
    pcRdpgfxSurfaceCommand surfaceCommand = nullptr;
    pcRdpgfxCapsConfirm capsConfirm = nullptr;
};

std::mutex g_rdpgfxHooksMutex;
std::unordered_map<RdpgfxClientContext*, RdpgfxDiagnosticsHookState> g_rdpgfxHooks;

bool RdpgfxCapsConfirmAvc420(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    return capsConfirm != nullptr && capsConfirm->capsSet != nullptr &&
        capsConfirm->capsSet->version == RDPGFX_CAPVERSION_81 &&
        (capsConfirm->capsSet->flags & RDPGFX_CAPS_FLAG_AVC420_ENABLED) != 0;
}

bool RdpgfxCapsConfirmAvc444(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    if (capsConfirm == nullptr || capsConfirm->capsSet == nullptr) {
        return false;
    }

    const uint32_t version = capsConfirm->capsSet->version;
    const uint32_t flags = capsConfirm->capsSet->flags;
    return version == RDPGFX_CAPVERSION_101 ||
        (version >= RDPGFX_CAPVERSION_10 && (flags & RDPGFX_CAPS_FLAG_AVC_DISABLED) == 0);
}

std::string RdpgfxCapsConfirmSummary(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    if (capsConfirm == nullptr || capsConfirm->capsSet == nullptr) {
        return "capsConfirm=null";
    }

    const RDPGFX_CAPSET* capsSet = capsConfirm->capsSet;
    return "version=" + Hex32(capsSet->version) + " flags=" + Hex32(capsSet->flags);
}

void SwitchAvc420SurfaceToSoftwareFallback(const std::string& reason)
{
    if (!g_avc420SurfaceOutputEnabled.exchange(false)) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }

    StartRenderPipeline();
    EmitNativeLog("AVC420 surface output disabled; using FreeRDP buffer/GLES fallback: " + reason);
}

UINT HarmonyRdpgfxStartFrame(RdpgfxClientContext* context, const RDPGFX_START_FRAME_PDU* startFrame)
{
    g_rdpgfxStartFrameCount.fetch_add(1);
    pcRdpgfxStartFrame original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.startFrame;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, startFrame);
}

UINT HarmonyRdpgfxEndFrame(RdpgfxClientContext* context, const RDPGFX_END_FRAME_PDU* endFrame)
{
    g_rdpgfxEndFrameCount.fetch_add(1);
    pcRdpgfxEndFrame original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.endFrame;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, endFrame);
}

UINT HarmonyRdpgfxCapsConfirm(RdpgfxClientContext* context, const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    if (g_avc420SurfaceOutputEnabled.load()) {
        const std::string summary = RdpgfxCapsConfirmSummary(capsConfirm);
        if (RdpgfxCapsConfirmAvc420(capsConfirm)) {
            StopRenderPipeline();
            EmitNativeLog("RDPGFX negotiated AVC420 surface mode: " + summary);
        } else if (RdpgfxCapsConfirmAvc444(capsConfirm)) {
            SwitchAvc420SurfaceToSoftwareFallback("server selected AVC444 buffer mode " + summary);
        } else {
            SwitchAvc420SurfaceToSoftwareFallback("server selected non-AVC graphics mode " + summary);
        }
    }

    pcRdpgfxCapsConfirm original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.capsConfirm;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, capsConfirm);
}

UINT HarmonyRdpgfxSurfaceCommand(RdpgfxClientContext* context, const RDPGFX_SURFACE_COMMAND* command)
{
    if (command != nullptr) {
        RecordRdpgfxSurfaceCommand(*command);
        if (g_avc420SurfaceOutputEnabled.load() && command->codecId != RDPGFX_CODECID_AVC420) {
            SwitchAvc420SurfaceToSoftwareFallback("first non-AVC420 surface command codec=" +
                std::string(RdpgfxCodecName(command->codecId)) +
                "(" + std::to_string(command->codecId) + ")");
        }
    }

    pcRdpgfxSurfaceCommand original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.surfaceCommand;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, command);
}

void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
    if (g_rdpgfxHooks.find(gfx) != g_rdpgfxHooks.end()) {
        return;
    }

    RdpgfxDiagnosticsHookState state;
    state.startFrame = gfx->StartFrame;
    state.endFrame = gfx->EndFrame;
    state.surfaceCommand = gfx->SurfaceCommand;
    state.capsConfirm = gfx->CapsConfirm;
    g_rdpgfxHooks[gfx] = state;
    if (state.startFrame != nullptr) {
        gfx->StartFrame = HarmonyRdpgfxStartFrame;
    }
    if (state.endFrame != nullptr) {
        gfx->EndFrame = HarmonyRdpgfxEndFrame;
    }
    if (state.surfaceCommand != nullptr) {
        gfx->SurfaceCommand = HarmonyRdpgfxSurfaceCommand;
    }
    if (state.capsConfirm != nullptr) {
        gfx->CapsConfirm = HarmonyRdpgfxCapsConfirm;
    }
}

void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
    auto iter = g_rdpgfxHooks.find(gfx);
    if (iter == g_rdpgfxHooks.end()) {
        return;
    }

    gfx->StartFrame = iter->second.startFrame;
    gfx->EndFrame = iter->second.endFrame;
    gfx->SurfaceCommand = iter->second.surfaceCommand;
    gfx->CapsConfirm = iter->second.capsConfirm;
    g_rdpgfxHooks.erase(iter);
}

using FreerdpSetActiveFn = std::function<void(FreerdpRuntimeApi*, freerdp*, rdpContext*)>;
using FreerdpClearActiveFn = std::function<void(freerdp*)>;
using FreerdpLogFn = std::function<void(const std::string&)>;
using FreerdpConnectedFn = std::function<void()>;
using FreerdpInputPumpFn = std::function<void(FreerdpRuntimeApi*, rdpContext*)>;

bool EnableFreerdpClientChannels(FreerdpRuntimeApi& api, freerdp* instance,
    const FreerdpLogFn& log, std::string& error)
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
    const FreerdpLogFn& log, std::string& error)
{
    const bool h264Requested = graphicsConfig.enabled && graphicsConfig.h264;
    const bool avc444FallbackEnabled = h264Requested;
    const uint32_t gfxCapsFilter = 0;

    if (!SetFreerdpBool(api, settings, FreeRDP_SupportDynamicChannels, true, "SupportDynamicChannels", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportDisplayControl, true, "SupportDisplayControl", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_DynamicResolutionUpdate, true, "DynamicResolutionUpdate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportGraphicsPipeline, graphicsConfig.enabled,
            "SupportGraphicsPipeline", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxH264, h264Requested,
            "GfxH264", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444, avc444FallbackEnabled,
            "GfxAVC444", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444v2, avc444FallbackEnabled,
            "GfxAVC444v2", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_GfxCapsFilter, gfxCapsFilter, "GfxCapsFilter", error) ||
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
            " avc444=" + std::string(avc444FallbackEnabled ? "buffer-fallback" : "off") +
            " capsFilter=" + Hex32(gfxCapsFilter) +
            " fallback=" + std::string(avc444FallbackEnabled ? "avc444-buffer" : "disabled"));
    } else {
        log("FreeRDP graphics pipeline disabled at runtime; using stable software GDI frame rendering");
    }
    log("FreeRDP audio playback is requested through explicit rdpsnd sys:ohos channels");
    log("FreeRDP rdpdr base channel enabled; drive/printer/smartcard runtime toggles remain disabled by default");
    return true;
}

bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const FreerdpLogFn& log, std::string& error)
{
    if (!graphicsConfig.enabled || !graphicsConfig.h264) {
        g_avc420SurfaceOutputEnabled.store(false);
        if (api.ohosAvcodecSetOutputSurface != nullptr) {
            api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
            api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        return true;
    }

    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        error = "OHOS AVCodec surface output symbol is not loaded";
        return false;
    }

    const DecoderSurfaceTarget target = SnapshotDecoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        error = "AVC420 surface output requires a ready XComponent NativeWindow";
        return false;
    }

    if (!api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE)) {
        error = "OHOS AVCodec surface output setup failed";
        return false;
    }

    g_avc420SurfaceOutputEnabled.store(true);
    RegisterAvc444DecodeSurfaces(api, target.width, target.height, log);
    StopRenderPipeline();
    log("OHOS AVCodec output surface configured: XComponent NativeWindow " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " mode=avc420-surface-preferred fallback=avc444-buffer");
    return true;
}

bool ConfigureGraphicsPipelineChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const FreerdpLogFn& log, std::string& error)
{
    g_rdpgfxRuntimeRequested.store(graphicsConfig.enabled);
    g_rdpgfxH264Requested.store(graphicsConfig.enabled && graphicsConfig.h264);
    g_rdpgfxBridgeAttached.store(false);
    ResetRdpgfxDiagnosticsStats();

    if (!graphicsConfig.enabled) {
        log("FreeRDP rdpgfx dynamic channel not requested: graphicsMode=gdi");
        log(BuildGraphicsPipelineStatsLog());
        return true;
    }

    if (api.clientAddDynamicChannel == nullptr) {
        error = "FreeRDP rdpgfx dynamic channel helper is not loaded";
        return false;
    }
    if (api.gdiGraphicsPipelineInit == nullptr || api.gdiGraphicsPipelineUninit == nullptr) {
        error = "FreeRDP GDI graphics pipeline symbols are not loaded";
        return false;
    }

    const char* params[] = {RDPGFX_CHANNEL_NAME};
    if (!api.clientAddDynamicChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set rdpgfx dynamic channel failed";
        return false;
    }

    log("FreeRDP rdpgfx requested: dynamic channel + GDI graphics pipeline bridge");
    log(BuildGraphicsPipelineStatsLog());
    return true;
}

bool ConfigureDisplayControlChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const FreerdpLogFn& log, std::string& error)
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
    const FreerdpLogFn& log, std::string& error)
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
    const FreerdpLogFn& log, std::string& error)
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

std::string Utf16LeClipboardToUtf8(const BYTE* data, UINT32 size)
{
    std::string text;
    if (data == nullptr || size < 2) {
        return text;
    }

    auto appendCodePoint = [&text](uint32_t cp) {
        if (cp <= 0x7F) {
            text.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            text.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            text.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            text.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };

    const UINT32 units = size / 2U;
    for (UINT32 index = 0; index < units; ++index) {
        uint16_t unit = static_cast<uint16_t>(data[index * 2U]) |
            (static_cast<uint16_t>(data[index * 2U + 1U]) << 8U);
        if (unit == 0) {
            break;
        }

        if (unit >= 0xD800 && unit <= 0xDBFF && index + 1U < units) {
            uint16_t next = static_cast<uint16_t>(data[(index + 1U) * 2U]) |
                (static_cast<uint16_t>(data[(index + 1U) * 2U + 1U]) << 8U);
            if (next >= 0xDC00 && next <= 0xDFFF) {
                const uint32_t cp = 0x10000U +
                    (((static_cast<uint32_t>(unit) - 0xD800U) << 10U) |
                        (static_cast<uint32_t>(next) - 0xDC00U));
                appendCodePoint(cp);
                ++index;
                continue;
            }
        }

        appendCodePoint(unit);
    }
    return text;
}

bool ReadUtf8CodePoint(const std::string& text, size_t& offset, uint32_t& cp)
{
    if (offset >= text.size()) {
        return false;
    }

    const uint8_t first = static_cast<uint8_t>(text[offset++]);
    if (first < 0x80) {
        cp = first;
        return true;
    }

    uint32_t value = 0;
    size_t trailing = 0;
    if ((first & 0xE0U) == 0xC0U) {
        value = first & 0x1FU;
        trailing = 1;
    } else if ((first & 0xF0U) == 0xE0U) {
        value = first & 0x0FU;
        trailing = 2;
    } else if ((first & 0xF8U) == 0xF0U) {
        value = first & 0x07U;
        trailing = 3;
    } else {
        cp = 0xFFFD;
        return true;
    }

    if (offset + trailing > text.size()) {
        cp = 0xFFFD;
        offset = text.size();
        return true;
    }

    for (size_t index = 0; index < trailing; ++index) {
        const uint8_t next = static_cast<uint8_t>(text[offset++]);
        if ((next & 0xC0U) != 0x80U) {
            cp = 0xFFFD;
            return true;
        }
        value = (value << 6U) | (next & 0x3FU);
    }

    cp = value;
    return true;
}

std::vector<BYTE> Utf8ToUtf16LeClipboard(const std::string& text)
{
    std::vector<BYTE> output;
    output.reserve(text.size() * 2U + 2U);

    auto appendUnit = [&output](uint16_t unit) {
        output.push_back(static_cast<BYTE>(unit & 0xFFU));
        output.push_back(static_cast<BYTE>((unit >> 8U) & 0xFFU));
    };

    size_t offset = 0;
    uint32_t cp = 0;
    while (ReadUtf8CodePoint(text, offset, cp)) {
        if (cp > 0x10FFFFU) {
            cp = 0xFFFD;
        }

        if (cp <= 0xFFFFU) {
            appendUnit(static_cast<uint16_t>(cp));
        } else {
            cp -= 0x10000U;
            appendUnit(static_cast<uint16_t>(0xD800U | (cp >> 10U)));
            appendUnit(static_cast<uint16_t>(0xDC00U | (cp & 0x3FFU)));
        }
    }

    appendUnit(0);
    return output;
}

class HarmonyClipboardBridge {
public:
    ~HarmonyClipboardBridge()
    {
        Uninitialize();
    }

    bool Initialize(rdpContext* context, FreerdpRuntimeApi& api, const FreerdpLogFn& log,
        std::string& error)
    {
        if (context == nullptr || context->pubSub == nullptr) {
            error = "FreeRDP pubSub is unavailable for clipboard";
            return false;
        }
        if (api.pubSubSubscribe == nullptr || api.pubSubUnsubscribe == nullptr) {
            error = "WinPR PubSub symbols are not loaded for clipboard";
            return false;
        }

        context_ = context;
        api_ = &api;
        log_ = log;

        {
            std::lock_guard<std::mutex> lock(RegistryMutex());
            Registry()[context_] = this;
        }

        int rc = api_->pubSubSubscribe(context_->pubSub, "ChannelConnected", OnChannelConnected);
        if (rc < 0) {
            RemoveFromRegistry();
            error = "subscribe ChannelConnected for clipboard failed: " + std::to_string(rc);
            return false;
        }
        subscribedConnected_ = true;

        rc = api_->pubSubSubscribe(context_->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        if (rc < 0) {
            Uninitialize();
            error = "subscribe ChannelDisconnected for clipboard failed: " + std::to_string(rc);
            return false;
        }
        subscribedDisconnected_ = true;
        Log("cliprdr bridge subscribed to FreeRDP channel events");

        pasteboard_ = OH_Pasteboard_Create();
        if (pasteboard_ == nullptr) {
            Log("HarmonyOS Pasteboard create failed; cliprdr will advertise no local text");
            return true;
        }
        Log("HarmonyOS Pasteboard created for cliprdr text bridge");

        observer_ = OH_PasteboardObserver_Create();
        if (observer_ == nullptr) {
            Log("HarmonyOS Pasteboard observer create failed; local clipboard changes require reconnect");
            return true;
        }

        rc = OH_PasteboardObserver_SetData(observer_, this, OnPasteboardChanged, OnPasteboardFinalize);
        if (rc != ERR_OK) {
            Log("HarmonyOS Pasteboard observer setup failed: " + std::to_string(rc));
            return true;
        }

        rc = OH_Pasteboard_Subscribe(pasteboard_, NOTIFY_LOCAL_DATA_CHANGE, observer_);
        if (rc == ERR_OK) {
            pasteboardSubscribed_ = true;
            Log("HarmonyOS Pasteboard observer subscribed");
        } else {
            Log("HarmonyOS Pasteboard subscribe warning: " + std::to_string(rc));
        }

        return true;
    }

    void Uninitialize()
    {
        if (cliprdr_ != nullptr) {
            DetachCliprdr(cliprdr_);
        }

        if (pasteboard_ != nullptr && observer_ != nullptr && pasteboardSubscribed_) {
            (void)OH_Pasteboard_Unsubscribe(pasteboard_, NOTIFY_LOCAL_DATA_CHANGE, observer_);
            pasteboardSubscribed_ = false;
        }
        if (observer_ != nullptr) {
            (void)OH_PasteboardObserver_Destroy(observer_);
            observer_ = nullptr;
        }
        if (pasteboard_ != nullptr) {
            OH_Pasteboard_Destroy(pasteboard_);
            pasteboard_ = nullptr;
        }

        if (api_ != nullptr && context_ != nullptr && context_->pubSub != nullptr) {
            if (subscribedConnected_) {
                (void)api_->pubSubUnsubscribe(context_->pubSub, "ChannelConnected", OnChannelConnected);
                subscribedConnected_ = false;
            }
            if (subscribedDisconnected_) {
                (void)api_->pubSubUnsubscribe(context_->pubSub, "ChannelDisconnected", OnChannelDisconnected);
                subscribedDisconnected_ = false;
            }
        }

        RemoveFromRegistry();
        context_ = nullptr;
        api_ = nullptr;
        log_ = nullptr;
    }

private:
    static std::mutex& RegistryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<rdpContext*, HarmonyClipboardBridge*>& Registry()
    {
        static std::unordered_map<rdpContext*, HarmonyClipboardBridge*> registry;
        return registry;
    }

    static HarmonyClipboardBridge* FromContext(void* context)
    {
        auto* rdpCtx = static_cast<::rdpContext*>(context);
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(rdpCtx);
        return iter == Registry().end() ? nullptr : iter->second;
    }

    static HarmonyClipboardBridge* FromCliprdr(CliprdrClientContext* cliprdr)
    {
        return cliprdr == nullptr ? nullptr : static_cast<HarmonyClipboardBridge*>(cliprdr->custom);
    }

    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event)
    {
        HarmonyClipboardBridge* bridge = FromContext(context);
        if (bridge == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }
        bridge->Log("FreeRDP channel connected: " + std::string(event->name));
        if (std::strcmp(event->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
            bridge->AttachCliprdr(static_cast<CliprdrClientContext*>(event->pInterface));
        }
    }

    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
    {
        HarmonyClipboardBridge* bridge = FromContext(context);
        if (bridge == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }
        bridge->Log("FreeRDP channel disconnected: " + std::string(event->name));
        if (std::strcmp(event->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
            bridge->DetachCliprdr(static_cast<CliprdrClientContext*>(event->pInterface));
        }
    }

    static void OnPasteboardChanged(void* context, Pasteboard_NotifyType type)
    {
        auto* bridge = static_cast<HarmonyClipboardBridge*>(context);
        if (bridge == nullptr || type != NOTIFY_LOCAL_DATA_CHANGE) {
            return;
        }
        bridge->HandleLocalPasteboardChanged();
    }

    static void OnPasteboardFinalize(void*)
    {
    }

    static UINT CliprdrMonitorReady(CliprdrClientContext* cliprdr,
        const CLIPRDR_MONITOR_READY* monitorReady)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || monitorReady == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        UINT rc = bridge->SendClientCapabilities();
        if (rc != CHANNEL_RC_OK) {
            return rc;
        }
        bridge->Log("cliprdr monitor ready");
        return bridge->SendLocalFormatList("monitor ready");
    }

    static UINT CliprdrServerCapabilities(CliprdrClientContext* cliprdr,
        const CLIPRDR_CAPABILITIES* capabilities)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || capabilities == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        bridge->Log("cliprdr server capabilities received");
        return CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFormatList(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_LIST* formatList)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || formatList == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatList(*formatList);
    }

    static UINT CliprdrServerFormatListResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_LIST_RESPONSE* response)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || response == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        bridge->Log("cliprdr server accepted local format list");
        return CHANNEL_RC_OK;
    }

    static UINT CliprdrServerLockClipboardData(CliprdrClientContext* cliprdr,
        const CLIPRDR_LOCK_CLIPBOARD_DATA* lockClipboardData)
    {
        return (cliprdr == nullptr || lockClipboardData == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerUnlockClipboardData(CliprdrClientContext* cliprdr,
        const CLIPRDR_UNLOCK_CLIPBOARD_DATA* unlockClipboardData)
    {
        return (cliprdr == nullptr || unlockClipboardData == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFormatDataRequest(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_DATA_REQUEST* request)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || request == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatDataRequest(*request);
    }

    static UINT CliprdrServerFormatDataResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FORMAT_DATA_RESPONSE* response)
    {
        HarmonyClipboardBridge* bridge = FromCliprdr(cliprdr);
        if (bridge == nullptr || response == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }
        return bridge->HandleServerFormatDataResponse(*response);
    }

    static UINT CliprdrServerFileContentsRequest(CliprdrClientContext* cliprdr,
        const CLIPRDR_FILE_CONTENTS_REQUEST* request)
    {
        return (cliprdr == nullptr || request == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    static UINT CliprdrServerFileContentsResponse(CliprdrClientContext* cliprdr,
        const CLIPRDR_FILE_CONTENTS_RESPONSE* response)
    {
        return (cliprdr == nullptr || response == nullptr) ? ERROR_INVALID_PARAMETER :
            CHANNEL_RC_OK;
    }

    void AttachCliprdr(CliprdrClientContext* cliprdr)
    {
        if (cliprdr == nullptr) {
            Log("cliprdr connected without client context");
            return;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        cliprdr_ = cliprdr;
        cliprdr_->custom = this;
        cliprdr_->MonitorReady = CliprdrMonitorReady;
        cliprdr_->ServerCapabilities = CliprdrServerCapabilities;
        cliprdr_->ServerFormatList = CliprdrServerFormatList;
        cliprdr_->ServerFormatListResponse = CliprdrServerFormatListResponse;
        cliprdr_->ServerLockClipboardData = CliprdrServerLockClipboardData;
        cliprdr_->ServerUnlockClipboardData = CliprdrServerUnlockClipboardData;
        cliprdr_->ServerFormatDataRequest = CliprdrServerFormatDataRequest;
        cliprdr_->ServerFormatDataResponse = CliprdrServerFormatDataResponse;
        cliprdr_->ServerFileContentsRequest = CliprdrServerFileContentsRequest;
        cliprdr_->ServerFileContentsResponse = CliprdrServerFileContentsResponse;
        Log("cliprdr connected to HarmonyOS Pasteboard text bridge");
    }

    void DetachCliprdr(CliprdrClientContext* cliprdr)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (cliprdr_ == nullptr || cliprdr_ != cliprdr) {
            return;
        }

        cliprdr_->custom = nullptr;
        cliprdr_ = nullptr;
        serverFormats_.clear();
        requestedFormatId_ = 0;
        Log("cliprdr disconnected from HarmonyOS Pasteboard text bridge");
    }

    UINT SendClientCapabilities()
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientCapabilities == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_CAPABILITIES capabilities = {};
        CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet = {};
        capabilities.cCapabilitiesSets = 1;
        capabilities.capabilitySets = reinterpret_cast<CLIPRDR_CAPABILITY_SET*>(&generalCapabilitySet);
        generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
        generalCapabilitySet.capabilitySetLength = 12;
        generalCapabilitySet.version = CB_CAPS_VERSION_2;
        generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;
        return cliprdr->ClientCapabilities(cliprdr, &capabilities);
    }

    UINT SendClientFormatListResponse(bool accepted)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatListResponse == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_FORMAT_LIST_RESPONSE response = {};
        response.common.msgType = CB_FORMAT_LIST_RESPONSE;
        response.common.msgFlags = accepted ? CB_RESPONSE_OK : CB_RESPONSE_FAIL;
        response.common.dataLen = 0;
        const UINT rc = cliprdr->ClientFormatListResponse(cliprdr, &response);
        if (rc == CHANNEL_RC_OK) {
            Log(std::string("cliprdr server format list ") + (accepted ? "accepted" : "rejected"));
        }
        return rc;
    }

    UINT SendLocalFormatList(const char* reason)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatList == nullptr) {
            return CHANNEL_RC_OK;
        }

        std::string text;
        std::string error;
        const bool hasText = ReadLocalPlainText(text, error);
        if (!hasText && !error.empty()) {
            Log("HarmonyOS Pasteboard read warning: " + error);
        }

        CLIPRDR_FORMAT format = {};
        format.formatId = CF_UNICODETEXT;
        CLIPRDR_FORMAT_LIST formatList = {};
        formatList.common.msgType = CB_FORMAT_LIST;
        formatList.common.msgFlags = 0;
        formatList.numFormats = hasText ? 1U : 0U;
        formatList.formats = hasText ? &format : nullptr;

        UINT rc = cliprdr->ClientFormatList(cliprdr, &formatList);
        if (rc == CHANNEL_RC_OK) {
            Log(std::string("cliprdr local format list sent: ") +
                (hasText ? "CF_UNICODETEXT" : "empty") + " reason=" + SafeCString(reason));
        }
        return rc;
    }

    UINT HandleServerFormatList(const CLIPRDR_FORMAT_LIST& formatList)
    {
        UINT32 requested = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            serverFormats_.clear();
            for (UINT32 index = 0; index < formatList.numFormats; ++index) {
                serverFormats_.push_back(formatList.formats[index].formatId);
                if (formatList.formats[index].formatId == CF_UNICODETEXT) {
                    requested = CF_UNICODETEXT;
                } else if (requested == 0 && formatList.formats[index].formatId == CF_TEXT) {
                    requested = CF_TEXT;
                } else if (requested == 0 && formatList.formats[index].formatId == CF_OEMTEXT) {
                    requested = CF_OEMTEXT;
                }
            }
            requestedFormatId_ = requested;
        }

        Log("cliprdr server format list received: " + std::to_string(formatList.numFormats));
        UINT rc = SendClientFormatListResponse(true);
        if (rc != CHANNEL_RC_OK) {
            Log("cliprdr server format list response failed: " + std::to_string(rc));
            return rc;
        }
        if (requested == 0) {
            Log("cliprdr server format list has no supported text format");
            return CHANNEL_RC_OK;
        }

        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatDataRequest == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        CLIPRDR_FORMAT_DATA_REQUEST request = {};
        request.common.msgType = CB_FORMAT_DATA_REQUEST;
        request.requestedFormatId = requested;
        Log("cliprdr remote text request sent: format=" + std::to_string(requested));
        return cliprdr->ClientFormatDataRequest(cliprdr, &request);
    }

    UINT HandleServerFormatDataRequest(const CLIPRDR_FORMAT_DATA_REQUEST& request)
    {
        CliprdrClientContext* cliprdr = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            cliprdr = cliprdr_;
        }
        if (cliprdr == nullptr || cliprdr->ClientFormatDataResponse == nullptr) {
            return ERROR_INVALID_PARAMETER;
        }

        std::string text;
        std::string error;
        const bool ok = ReadLocalPlainText(text, error);
        std::vector<BYTE> data;
        if (ok && request.requestedFormatId == CF_UNICODETEXT) {
            data = Utf8ToUtf16LeClipboard(text);
        } else if (ok && (request.requestedFormatId == CF_TEXT || request.requestedFormatId == CF_OEMTEXT)) {
            data.assign(text.begin(), text.end());
            data.push_back(0);
        }

        CLIPRDR_FORMAT_DATA_RESPONSE response = {};
        response.common.msgType = CB_FORMAT_DATA_RESPONSE;
        response.common.msgFlags = data.empty() ? CB_RESPONSE_FAIL : CB_RESPONSE_OK;
        response.common.dataLen = static_cast<UINT32>(data.size());
        response.requestedFormatData = data.empty() ? nullptr : data.data();

        if (data.empty()) {
            Log("cliprdr local text request failed: " + error);
        } else {
            Log("cliprdr local text response sent: " + std::to_string(data.size()) + " bytes");
        }
        return cliprdr->ClientFormatDataResponse(cliprdr, &response);
    }

    UINT HandleServerFormatDataResponse(const CLIPRDR_FORMAT_DATA_RESPONSE& response)
    {
        if ((response.common.msgFlags & CB_RESPONSE_FAIL) != 0 || response.requestedFormatData == nullptr) {
            Log("cliprdr remote text response failed: flags=" + std::to_string(response.common.msgFlags) +
                " length=" + std::to_string(response.common.dataLen));
            return CHANNEL_RC_OK;
        }

        UINT32 requested = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            requested = requestedFormatId_;
        }

        std::string text;
        if (requested == CF_UNICODETEXT) {
            text = Utf16LeClipboardToUtf8(response.requestedFormatData, response.common.dataLen);
        } else if (requested == CF_TEXT || requested == CF_OEMTEXT) {
            const auto* bytes = reinterpret_cast<const char*>(response.requestedFormatData);
            const size_t length = strnlen(bytes, response.common.dataLen);
            text.assign(bytes, length);
        }

        if (text.empty()) {
            Log("cliprdr remote text response was empty");
            return CHANNEL_RC_OK;
        }

        std::string error;
        if (!WriteLocalPlainText(text, error)) {
            Log("HarmonyOS Pasteboard write failed: " + error);
            return ERROR_INTERNAL_ERROR;
        }

        Log("cliprdr remote text copied to HarmonyOS Pasteboard: " +
            std::to_string(text.size()) + " bytes utf8");
        return CHANNEL_RC_OK;
    }

    void HandleLocalPasteboardChanged()
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ignoreLocalPasteboardChanges_ > 0) {
                --ignoreLocalPasteboardChanges_;
                return;
            }
        }
        (void)SendLocalFormatList("pasteboard changed");
    }

    bool ReadLocalPlainText(std::string& text, std::string& error)
    {
        text.clear();
        if (pasteboard_ == nullptr) {
            error = "pasteboard unavailable";
            return false;
        }

        int status = ERR_OK;
        OH_UdmfData* data = OH_Pasteboard_GetData(pasteboard_, &status);
        if (status != ERR_OK || data == nullptr) {
            error = "OH_Pasteboard_GetData status=" + std::to_string(status);
            return false;
        }

        OH_UdsPlainText* primaryPlainText = OH_UdsPlainText_Create();
        if (primaryPlainText != nullptr) {
            const int primaryRc = OH_UdmfData_GetPrimaryPlainText(data, primaryPlainText);
            if (primaryRc == UDMF_E_OK) {
                const char* content = OH_UdsPlainText_GetContent(primaryPlainText);
                if (content != nullptr) {
                    text = content;
                }
            }
            OH_UdsPlainText_Destroy(primaryPlainText);
            if (!text.empty()) {
                OH_UdmfData_Destroy(data);
                return true;
            }
        }

        const int recordCount = OH_UdmfData_GetRecordCount(data);
        for (int index = 0; index < recordCount; ++index) {
            OH_UdmfRecord* record = OH_UdmfData_GetRecord(data, static_cast<unsigned int>(index));
            if (record == nullptr) {
                continue;
            }
            OH_UdsPlainText* plainText = OH_UdsPlainText_Create();
            if (plainText == nullptr) {
                continue;
            }
            const int rc = OH_UdmfRecord_GetPlainText(record, plainText);
            if (rc == UDMF_E_OK) {
                const char* content = OH_UdsPlainText_GetContent(plainText);
                if (content != nullptr) {
                    text = content;
                }
            }
            OH_UdsPlainText_Destroy(plainText);
            if (!text.empty()) {
                break;
            }
        }

        OH_UdmfData_Destroy(data);
        if (text.empty()) {
            error = "pasteboard has no plain text record";
            return false;
        }
        return true;
    }

    bool WriteLocalPlainText(const std::string& text, std::string& error)
    {
        if (pasteboard_ == nullptr) {
            error = "pasteboard unavailable";
            return false;
        }

        OH_UdsPlainText* plainText = OH_UdsPlainText_Create();
        OH_UdmfRecord* record = OH_UdmfRecord_Create();
        OH_UdmfData* data = OH_UdmfData_Create();
        if (plainText == nullptr || record == nullptr || data == nullptr) {
            error = "UDMF allocation failed";
            if (plainText != nullptr) {
                OH_UdsPlainText_Destroy(plainText);
            }
            if (record != nullptr) {
                OH_UdmfRecord_Destroy(record);
            }
            if (data != nullptr) {
                OH_UdmfData_Destroy(data);
            }
            return false;
        }

        int rc = OH_UdsPlainText_SetContent(plainText, text.c_str());
        if (rc == UDMF_E_OK) {
            rc = OH_UdmfRecord_AddPlainText(record, plainText);
        }
        if (rc == UDMF_E_OK) {
            rc = OH_UdmfData_AddRecord(data, record);
        }
        if (rc != UDMF_E_OK) {
            error = "UDMF plain text setup failed: " + std::to_string(rc);
            OH_UdsPlainText_Destroy(plainText);
            OH_UdmfRecord_Destroy(record);
            OH_UdmfData_Destroy(data);
            return false;
        }

        {
            std::lock_guard<std::mutex> lock(mutex_);
            ++ignoreLocalPasteboardChanges_;
        }
        rc = OH_Pasteboard_SetData(pasteboard_, data);
        OH_UdsPlainText_Destroy(plainText);
        OH_UdmfRecord_Destroy(record);
        OH_UdmfData_Destroy(data);
        if (rc != ERR_OK) {
            std::lock_guard<std::mutex> lock(mutex_);
            if (ignoreLocalPasteboardChanges_ > 0) {
                --ignoreLocalPasteboardChanges_;
            }
            error = "OH_Pasteboard_SetData status=" + std::to_string(rc);
            return false;
        }
        return true;
    }

    void RemoveFromRegistry()
    {
        if (context_ == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(context_);
        if (iter != Registry().end() && iter->second == this) {
            Registry().erase(iter);
        }
    }

    void Log(const std::string& line)
    {
        if (log_) {
            log_(line);
        } else {
            EmitNativeLog(line);
        }
    }

    std::mutex mutex_;
    rdpContext* context_ = nullptr;
    FreerdpRuntimeApi* api_ = nullptr;
    FreerdpLogFn log_;
    CliprdrClientContext* cliprdr_ = nullptr;
    OH_Pasteboard* pasteboard_ = nullptr;
    OH_PasteboardObserver* observer_ = nullptr;
    bool pasteboardSubscribed_ = false;
    bool subscribedConnected_ = false;
    bool subscribedDisconnected_ = false;
    std::vector<UINT32> serverFormats_;
    UINT32 requestedFormatId_ = 0;
    uint32_t ignoreLocalPasteboardChanges_ = 0;
};

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const FreerdpSetActiveFn& setActive, const FreerdpClearActiveFn& clearActive,
    const FreerdpLogFn& log, const FreerdpConnectedFn& onConnected,
    const FreerdpInputPumpFn& pumpInput)
{
    RdpSessionRunResult result;
    result.available = true;

    std::string modulesPath = EnsureOpenSslModulesPath();
    if (!modulesPath.empty()) {
        log("OPENSSL_MODULES=" + modulesPath);
    }

    uint32_t port = 0;
    if (!ParsePort(params.port, port)) {
        result.message = "invalid RDP port: " + params.port;
        result.failed = true;
        return result;
    }

    uint32_t width = 1280;
    uint32_t height = 720;
    ParseResolutionOrDefault(params.resolution, width, height);

    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        result.available = false;
        result.message = error;
        result.failed = true;
        return result;
    }
    log("FreeRDP runtime symbols loaded");

    freerdp* instance = api.freerdpNew();
    if (instance == nullptr) {
        result.message = "freerdp_new failed";
        result.failed = true;
        return result;
    }

    bool contextCreated = false;
    HarmonyClipboardBridge clipboardBridge;
    auto cleanup = [&]() {
        g_avc420SurfaceOutputEnabled.store(false);
        if (api.ohosAvcodecSetOutputSurface != nullptr) {
            api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
            api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        clipboardBridge.Uninitialize();
        ClearRdpDesktopSize();
        UnregisterCertificatePolicy(instance);
        if (contextCreated && instance->context != nullptr) {
            api.abortConnectContext(instance->context);
            api.disconnect(instance);
            StopRenderPipeline();
            clearActive(instance);
            api.contextFree(instance);
        } else {
            clearActive(instance);
        }
        api.freerdpFree(instance);
    };

    if (!api.contextNew(instance)) {
        result.message = "freerdp_context_new failed";
        result.failed = true;
        cleanup();
        return result;
    }
    contextCreated = true;
    if (!EnableFreerdpClientChannels(api, instance, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }
    setActive(&api, instance, instance->context);

    rdpSettings* settings = instance->context == nullptr ? nullptr : instance->context->settings;
    if (settings == nullptr) {
        result.message = "FreeRDP settings unavailable";
        result.failed = true;
        cleanup();
        return result;
    }

    UserParts user = SplitDomainUsername(params.username);
    const CertificatePolicy certificatePolicy = ParseCertificatePolicy(params.certPolicy);
    const bool ignoreCertificate = certificatePolicy == CertificatePolicy::Ignore;
    const GraphicsPipelineConfig graphicsConfig = ParseGraphicsPipelineConfig(params);

    if (!SetFreerdpString(api, settings, FreeRDP_ServerHostname, params.host, "ServerHostname", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ServerPort, port, "ServerPort", error) ||
        !SetFreerdpString(api, settings, FreeRDP_Username, user.username, "Username", error) ||
        !SetFreerdpString(api, settings, FreeRDP_Password, params.password, "Password", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_DesktopWidth, width, "DesktopWidth", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_DesktopHeight, height, "DesktopHeight", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ColorDepth, 32, "ColorDepth", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_TcpConnectTimeout, 5000, "TcpConnectTimeout", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_OsMajorType, OSMAJORTYPE_UNIX, "OsMajorType", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_OsMinorType, OSMINORTYPE_NATIVE_WAYLAND, "OsMinorType", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AuthenticationOnly, false, "AuthenticationOnly", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_Authentication, true, "Authentication", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SoftwareGdi, true, "SoftwareGdi", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_NegotiateSecurityLayer, true, "NegotiateSecurityLayer", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_CertificateCallbackPreferPEM, true, "CertificateCallbackPreferPEM", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_IgnoreCertificate, ignoreCertificate, "IgnoreCertificate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AutoAcceptCertificate, false, "AutoAcceptCertificate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AutoDenyCertificate, false, "AutoDenyCertificate", error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureFreerdpStoragePaths(api, settings, params, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureEnhancedRdpSettings(api, settings, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureGraphicsPipelineChannel(api, settings, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureClipboardChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureDisplayControlChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!clipboardBridge.Initialize(instance->context, api, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureAudioPlaybackChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!user.domain.empty() &&
        !SetFreerdpString(api, settings, FreeRDP_Domain, user.domain, "Domain", error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    instance->PostConnect = HarmonyPostConnect;
    instance->PostDisconnect = HarmonyPostDisconnect;
    instance->VerifyCertificateEx = HarmonyVerifyCertificateEx;
    instance->VerifyChangedCertificateEx = HarmonyVerifyChangedCertificateEx;
    RegisterCertificatePolicy(instance, certificatePolicy);
    ClearRdpDesktopSize();

    log("FreeRDP target configured");
    log("FreeRDP mode=PersistentSession");
    log("FreeRDP GDI renderer configured");
    log(std::string("FreeRDP certificate policy=") + CertificatePolicyName(certificatePolicy));
    if (!user.domain.empty()) {
        log("FreeRDP domain parsed from username");
    }

    BOOL rc = api.connect(instance);
    uint32_t lastError = instance->context == nullptr ? UINT32_MAX : api.getLastError(instance->context);
    log(std::string("freerdp_connect returned ") + (rc ? "true" : "false"));

    if (!running.load()) {
        result.cancelled = true;
        result.message = "FreeRDP connect cancelled";
        cleanup();
        return result;
    }

    if (!rc) {
        result.failed = true;
        result.message = "FreeRDP connect failed: " + LastErrorMessage(api, lastError);
        cleanup();
        return result;
    }

    result.connected = true;
    result.message = "FreeRDP session connected";
    onConnected();
    log("FreeRDP event loop started");
    auto nextAudioDiagnosticsLog = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (running.load() && !api.shallDisconnectContext(instance->context)) {
        pumpInput(&api, instance->context);
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextAudioDiagnosticsLog) {
            EmitHilogInfo("FreeRDP audio diagnostics: " + BuildOHAudioStatsLog());
            nextAudioDiagnosticsLog = now + std::chrono::seconds(10);
        }

        HANDLE handles[MAXIMUM_WAIT_OBJECTS] = {};
        DWORD count = api.getEventHandles(instance->context, handles, MAXIMUM_WAIT_OBJECTS);
        if (count == 0) {
            uint32_t errorCode = api.getLastError(instance->context);
            result.failed = true;
            result.message = "freerdp_get_event_handles failed: " + LastErrorMessage(api, errorCode);
            break;
        }

        DWORD waitStatus = api.waitForMultipleObjects(count, handles, FALSE, 25);
        if (!running.load()) {
            result.cancelled = true;
            result.message = "FreeRDP session cancelled";
            break;
        }

        if (waitStatus == WAIT_TIMEOUT) {
            continue;
        }

        if (waitStatus == WAIT_FAILED) {
            result.failed = true;
            result.message = "WaitForMultipleObjects failed: " + Hex32(static_cast<uint32_t>(waitStatus));
            break;
        }

        if (!api.checkEventHandles(instance->context)) {
            uint32_t errorCode = api.getLastError(instance->context);
            if (errorCode == FREERDP_ERROR_SUCCESS) {
                result.message = "FreeRDP event loop stopped without error";
            } else {
                result.failed = true;
                result.message = "FreeRDP event loop failed: " + LastErrorMessage(api, errorCode);
            }
            break;
        }

        pumpInput(&api, instance->context);
    }

    if (!result.cancelled && !result.failed && result.message == "FreeRDP session connected") {
        result.message = "FreeRDP session ended";
    }

    cleanup();
    return result;
}
#else
RdpSessionRunResult RunFreerdpSessionUnavailable()
{
    RdpSessionRunResult result;
    result.available = false;
    result.message = "FreeRDP headers not found at build time";
    result.failed = true;
    return result;
}
#endif

void CallStringCallback(napi_env env, napi_value jsCallback, void* context, void* data)
{
    std::unique_ptr<CallbackData> callbackData(static_cast<CallbackData*>(data));
    if (env == nullptr || jsCallback == nullptr || callbackData == nullptr) {
        return;
    }

    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    napi_value value = nullptr;
    napi_create_string_utf8(env, callbackData->value.c_str(), callbackData->value.size(), &value);
    napi_value argv[1] = {value};
    napi_call_function(env, undefined, jsCallback, 1, argv, nullptr);
}

class EventSink {
public:
    ~EventSink()
    {
        Reset();
    }

    bool Set(napi_env env, napi_value callback, const char* name, bool mirrorToHilog = false)
    {
        napi_valuetype type = napi_undefined;
        napi_typeof(env, callback, &type);
        if (type != napi_function) {
            return false;
        }

        napi_value resourceName = nullptr;
        napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &resourceName);

        napi_threadsafe_function next = nullptr;
        napi_status status = napi_create_threadsafe_function(
            env,
            callback,
            nullptr,
            resourceName,
            0,
            1,
            nullptr,
            nullptr,
            nullptr,
            CallStringCallback,
            &next);
        if (status != napi_ok || next == nullptr) {
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        if (function_ != nullptr) {
            napi_release_threadsafe_function(function_, napi_tsfn_abort);
        }
        function_ = next;
        mirrorToHilog_.store(mirrorToHilog);
        return true;
    }

    void Emit(const std::string& value)
    {
        if (mirrorToHilog_.load()) {
            EmitHilogInfo(value);
        }

        napi_threadsafe_function current = nullptr;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            current = function_;
            if (current == nullptr) {
                return;
            }
            napi_acquire_threadsafe_function(current);
        }

        auto data = new CallbackData{value};
        napi_status status = napi_call_threadsafe_function(current, data, napi_tsfn_nonblocking);
        if (status != napi_ok) {
            delete data;
        }
        napi_release_threadsafe_function(current, napi_tsfn_release);
    }

    void Reset()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (function_ != nullptr) {
            napi_release_threadsafe_function(function_, napi_tsfn_abort);
            function_ = nullptr;
        }
        mirrorToHilog_.store(false);
    }

private:
    std::mutex mutex_;
    napi_threadsafe_function function_ = nullptr;
    std::atomic_bool mirrorToHilog_{false};
};

struct SessionEventHub {
    EventSink state;
    EventSink log;
    EventSink error;
};

SessionEventHub g_events;

void EmitNativeLog(const std::string& line)
{
    g_events.log.Emit(line);
}

struct SurfaceSnapshot {
    bool registered = false;
    bool ready = false;
    std::string id;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t viewportX = 0;
    uint32_t viewportY = 0;
    uint32_t viewportWidth = 0;
    uint32_t viewportHeight = 0;
    uint32_t createdCount = 0;
    uint32_t changedCount = 0;
    uint32_t destroyedCount = 0;
    uint32_t touchCount = 0;
    uint32_t paintCount = 0;
    std::string lastPaintMessage;
};

std::string ReadXComponentId(OH_NativeXComponent* component)
{
    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t size = sizeof(id);
    int32_t rc = OH_NativeXComponent_GetXComponentId(component, id, &size);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS || size == 0) {
        return "unknown";
    }
    return std::string(id);
}

class SurfaceBridge {
public:
    void Register(OH_NativeXComponent* component, bool ok)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        component_ = component;
        registered_ = ok;
        id_ = component == nullptr ? "" : ReadXComponentId(component);
    }

    void OnSurfaceCreated(OH_NativeXComponent* component, void* window)
    {
        uint64_t width = 0;
        uint64_t height = 0;
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = window;
            registered_ = true;
            ready_ = window != nullptr;
            id_ = ReadXComponentId(component);
            width_ = static_cast<uint32_t>(width);
            height_ = static_cast<uint32_t>(height);
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++createdCount_;
            snapshot = SnapshotLocked();
        }

        g_events.log.Emit("XComponent surface created: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height));
    }

    void OnSurfaceChanged(OH_NativeXComponent* component, void* window)
    {
        uint64_t width = 0;
        uint64_t height = 0;
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = window;
            ready_ = window != nullptr;
            id_ = ReadXComponentId(component);
            width_ = static_cast<uint32_t>(width);
            height_ = static_cast<uint32_t>(height);
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        g_events.log.Emit("XComponent surface changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height));
    }

    bool OnSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
    {
        if (width == 0 || height == 0) {
            message = "XComponent layout size is invalid";
            return false;
        }

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (width_ == width && height_ == height) {
                message = "XComponent layout unchanged: " + std::to_string(width) + "x" +
                    std::to_string(height);
                return false;
            }

            width_ = width;
            height_ = height;
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        message = "XComponent layout changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height);
        return true;
    }

    void OnSurfaceDestroyed(OH_NativeXComponent* component, void*)
    {
        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = nullptr;
            ready_ = false;
            id_ = ReadXComponentId(component);
            width_ = 0;
            height_ = 0;
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++destroyedCount_;
            snapshot = SnapshotLocked();
        }

        g_events.log.Emit("XComponent surface destroyed: " + snapshot.id);
    }

    void OnTouchEvent()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++touchCount_;
    }

    SurfacePaintResult RenderRgbaFrame(const RgbaFrame& frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return RenderRgbaFrameLocked(frame);
    }

    SurfaceSnapshot Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return SnapshotLocked();
    }

    DecoderSurfaceTarget DecoderSurface()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            return {};
        }
        return {static_cast<OHNativeWindow*>(window_), width_, height_};
    }

    bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
        std::string& error)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || window_ == nullptr) {
            error = "XComponent surface is not ready";
            return false;
        }
        return avc444Surfaces_.Ensure(width, height, targets, error);
    }

private:
    struct RenderViewport {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    struct DirtyHistoryEntry {
        uint64_t fromSequence = 0;
        uint64_t toSequence = 0;
        DirtyFrameStats dirty;
    };

    class GpuRgbaRenderer {
    public:
        ~GpuRgbaRenderer()
        {
            Destroy();
        }

        void Destroy()
        {
            if (display_ != EGL_NO_DISPLAY) {
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                if (context_ != EGL_NO_CONTEXT) {
                    if (texture_ != 0) {
                        eglMakeCurrent(display_, surface_, surface_, context_);
                        glDeleteTextures(1, &texture_);
                        texture_ = 0;
                        if (program_ != 0) {
                            glDeleteProgram(program_);
                            program_ = 0;
                        }
                        eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    }
                    eglDestroyContext(display_, context_);
                }
                if (surface_ != EGL_NO_SURFACE) {
                    eglDestroySurface(display_, surface_);
                }
                eglTerminate(display_);
            }

            display_ = EGL_NO_DISPLAY;
            context_ = EGL_NO_CONTEXT;
            surface_ = EGL_NO_SURFACE;
            config_ = nullptr;
            window_ = nullptr;
            width_ = 0;
            height_ = 0;
            textureWidth_ = 0;
            textureHeight_ = 0;
            positionAttrib_ = -1;
            texCoordAttrib_ = -1;
            textureUniform_ = -1;
        }

        bool Render(OHNativeWindow* nativeWindow, uint32_t targetWidth, uint32_t targetHeight,
            const RgbaFrame& frame, int32_t sourceStride, const RenderViewport& viewport,
            SurfacePaintResult& result)
        {
            if (!EnsureReady(nativeWindow, targetWidth, targetHeight, result)) {
                return false;
            }
            if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
                result.logs.push_back("EGL make current failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }

            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glClearColor(0.0F, 0.0F, 0.0F, 1.0F);
            glClear(GL_COLOR_BUFFER_BIT);

            if (!UploadTexture(frame, sourceStride, result)) {
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                return false;
            }

            glUseProgram(program_);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture_);
            glUniform1i(textureUniform_, 0);

            const GLfloat vertices[] = {
                -1.0F, -1.0F, 0.0F, 1.0F,
                 1.0F, -1.0F, 1.0F, 1.0F,
                -1.0F,  1.0F, 0.0F, 0.0F,
                 1.0F,  1.0F, 1.0F, 0.0F,
            };
            glVertexAttribPointer(static_cast<GLuint>(positionAttrib_), 2, GL_FLOAT, GL_FALSE,
                4 * sizeof(GLfloat), vertices);
            glEnableVertexAttribArray(static_cast<GLuint>(positionAttrib_));
            glVertexAttribPointer(static_cast<GLuint>(texCoordAttrib_), 2, GL_FLOAT, GL_FALSE,
                4 * sizeof(GLfloat), vertices + 2);
            glEnableVertexAttribArray(static_cast<GLuint>(texCoordAttrib_));

            const GLint viewportY = static_cast<GLint>(targetHeight - viewport.y - viewport.height);
            glViewport(static_cast<GLint>(viewport.x), viewportY, static_cast<GLsizei>(viewport.width),
                static_cast<GLsizei>(viewport.height));
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            const GLenum glError = glGetError();
            if (glError != GL_NO_ERROR) {
                result.logs.push_back("GLES draw failed: " + Hex32(static_cast<uint32_t>(glError)));
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                return false;
            }
            if (!eglSwapBuffers(display_, surface_)) {
                result.logs.push_back("EGL swap buffers failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

            result.ok = true;
            result.partial = false;
            const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
            result.message = "GLES texture rendered: " + frameLabel + " " +
                std::to_string(viewport.width) + "x" + std::to_string(viewport.height) +
                " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y) +
                " source=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                " upload=" + (uploadMode_.empty() ?
                    (usingStagingBuffer_ ? "staged" : "direct") : uploadMode_);
            result.logs.push_back(result.message);
            return true;
        }

    private:
        bool EnsureReady(OHNativeWindow* nativeWindow, uint32_t targetWidth, uint32_t targetHeight,
            SurfacePaintResult& result)
        {
            if (nativeWindow == nullptr || targetWidth == 0 || targetHeight == 0) {
                result.logs.push_back("GLES target NativeWindow is invalid");
                return false;
            }
            if (display_ != EGL_NO_DISPLAY && window_ == nativeWindow &&
                width_ == targetWidth && height_ == targetHeight) {
                return true;
            }

            Destroy();
            display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (display_ == EGL_NO_DISPLAY) {
                result.logs.push_back("EGL get display failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                return false;
            }

            if (!eglInitialize(display_, nullptr, nullptr)) {
                result.logs.push_back("EGL initialize failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }
            if (!eglBindAPI(EGL_OPENGL_ES_API)) {
                result.logs.push_back("EGL bind GLES API failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }

            const EGLint configAttribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_NONE,
            };
            EGLint configCount = 0;
            if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) || configCount <= 0) {
                result.logs.push_back("EGL choose config failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }

            surface_ = eglCreateWindowSurface(display_, config_,
                reinterpret_cast<EGLNativeWindowType>(nativeWindow), nullptr);
            if (surface_ == EGL_NO_SURFACE) {
                result.logs.push_back("EGL create window surface failed: " +
                    Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }

            const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
            context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
            if (context_ == EGL_NO_CONTEXT) {
                result.logs.push_back("EGL create context failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }

            if (!eglMakeCurrent(display_, surface_, surface_, context_)) {
                result.logs.push_back("EGL make current failed: " + Hex32(static_cast<uint32_t>(eglGetError())));
                Destroy();
                return false;
            }
            if (!EnsureProgram(result)) {
                eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                Destroy();
                return false;
            }

            glGenTextures(1, &texture_);
            glBindTexture(GL_TEXTURE_2D, texture_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

            const GLenum glError = glGetError();
            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (glError != GL_NO_ERROR) {
                result.logs.push_back("GLES texture setup failed: " + Hex32(static_cast<uint32_t>(glError)));
                Destroy();
                return false;
            }

            window_ = nativeWindow;
            width_ = targetWidth;
            height_ = targetHeight;
            result.logs.push_back("GLES renderer initialized: " + std::to_string(width_) + "x" +
                std::to_string(height_));
            return true;
        }

        bool EnsureProgram(SurfacePaintResult& result)
        {
            if (program_ != 0) {
                return true;
            }

            static constexpr const char* vertexShaderSource =
                "attribute vec2 aPosition;\n"
                "attribute vec2 aTexCoord;\n"
                "varying vec2 vTexCoord;\n"
                "void main() {\n"
                "  gl_Position = vec4(aPosition, 0.0, 1.0);\n"
                "  vTexCoord = aTexCoord;\n"
                "}\n";
            static constexpr const char* fragmentShaderSource =
                "precision mediump float;\n"
                "varying vec2 vTexCoord;\n"
                "uniform sampler2D uTexture;\n"
                "void main() {\n"
                "  gl_FragColor = texture2D(uTexture, vTexCoord);\n"
                "}\n";

            const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource, result);
            if (vertexShader == 0) {
                return false;
            }
            const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource, result);
            if (fragmentShader == 0) {
                glDeleteShader(vertexShader);
                return false;
            }

            program_ = glCreateProgram();
            glAttachShader(program_, vertexShader);
            glAttachShader(program_, fragmentShader);
            glLinkProgram(program_);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            GLint linked = GL_FALSE;
            glGetProgramiv(program_, GL_LINK_STATUS, &linked);
            if (linked != GL_TRUE) {
                result.logs.push_back("GLES shader link failed: " + ReadProgramInfoLog(program_));
                glDeleteProgram(program_);
                program_ = 0;
                return false;
            }

            positionAttrib_ = glGetAttribLocation(program_, "aPosition");
            texCoordAttrib_ = glGetAttribLocation(program_, "aTexCoord");
            textureUniform_ = glGetUniformLocation(program_, "uTexture");
            if (positionAttrib_ < 0 || texCoordAttrib_ < 0 || textureUniform_ < 0) {
                result.logs.push_back("GLES shader bindings missing");
                glDeleteProgram(program_);
                program_ = 0;
                return false;
            }
            return true;
        }

        static GLuint CompileShader(GLenum type, const char* source, SurfacePaintResult& result)
        {
            const GLuint shader = glCreateShader(type);
            if (shader == 0) {
                result.logs.push_back("GLES create shader failed: " + Hex32(static_cast<uint32_t>(glGetError())));
                return 0;
            }
            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);
            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_TRUE) {
                return shader;
            }

            result.logs.push_back("GLES shader compile failed type=" + std::to_string(type) +
                " glError=" + Hex32(static_cast<uint32_t>(glGetError())) +
                " log=" + ReadShaderInfoLog(shader));
            glDeleteShader(shader);
            return 0;
        }

        static std::string ReadShaderInfoLog(GLuint shader)
        {
            GLint length = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
            if (length <= 1) {
                return "no info log";
            }
            std::string log(static_cast<size_t>(length), '\0');
            GLsizei written = 0;
            glGetShaderInfoLog(shader, length, &written, log.data());
            log.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
            return log;
        }

        static std::string ReadProgramInfoLog(GLuint program)
        {
            GLint length = 0;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
            if (length <= 1) {
                return "no info log";
            }
            std::string log(static_cast<size_t>(length), '\0');
            GLsizei written = 0;
            glGetProgramInfoLog(program, length, &written, log.data());
            log.resize(static_cast<size_t>(std::max<GLsizei>(0, written)));
            return log;
        }

        bool UploadTexture(const RgbaFrame& frame, int32_t sourceStride, SurfacePaintResult& result)
        {
            const size_t tightRowBytes = static_cast<size_t>(frame.width) * 4U;
            const uint8_t* upload = nullptr;
            usingStagingBuffer_ = false;
            uploadMode_ = "full-direct";

            glBindTexture(GL_TEXTURE_2D, texture_);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            const bool textureSizeChanged = textureWidth_ != frame.width || textureHeight_ != frame.height;
            DirtyFrameStats dirty = frame.dirty;
            const bool uploadDirty = !textureSizeChanged && CanUploadDirty(frame, dirty);
            if (!uploadDirty) {
                upload = PrepareFullUpload(frame, sourceStride, tightRowBytes);
            }
            if (textureSizeChanged) {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, static_cast<GLsizei>(frame.width),
                    static_cast<GLsizei>(frame.height), 0, GL_RGBA, GL_UNSIGNED_BYTE, upload);
                textureWidth_ = frame.width;
                textureHeight_ = frame.height;
            } else if (uploadDirty) {
                UploadDirtyTexture(frame, sourceStride, dirty);
            } else {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, static_cast<GLsizei>(frame.width),
                    static_cast<GLsizei>(frame.height), GL_RGBA, GL_UNSIGNED_BYTE, upload);
            }

            const GLenum glError = glGetError();
            if (glError == GL_NO_ERROR) {
                return true;
            }
            result.logs.push_back("GLES texture upload failed: " + Hex32(static_cast<uint32_t>(glError)));
            return false;
        }

        const uint8_t* PrepareFullUpload(const RgbaFrame& frame, int32_t sourceStride, size_t tightRowBytes)
        {
            if (sourceStride == static_cast<int32_t>(tightRowBytes)) {
                usingStagingBuffer_ = false;
                uploadMode_ = "full-direct";
                return frame.data;
            }

            const size_t required = tightRowBytes * frame.height;
            uploadBuffer_.resize(required);
            for (uint32_t y = 0; y < frame.height; ++y) {
                std::memcpy(uploadBuffer_.data() + tightRowBytes * y,
                    frame.data + static_cast<int64_t>(sourceStride) * y, tightRowBytes);
            }
            usingStagingBuffer_ = true;
            uploadMode_ = "full-staged";
            return uploadBuffer_.data();
        }

        bool CanUploadDirty(const RgbaFrame& frame, DirtyFrameStats& dirty) const
        {
            constexpr uint32_t kMaxDirtyUploadAreaPermille = 850;
            if (!dirty.valid || dirty.width == 0 || dirty.height == 0 ||
                dirty.areaPermille > kMaxDirtyUploadAreaPermille) {
                return false;
            }
            if (dirty.x >= frame.width || dirty.y >= frame.height) {
                return false;
            }
            if (dirty.x + dirty.width > frame.width) {
                dirty.width = frame.width - dirty.x;
            }
            if (dirty.y + dirty.height > frame.height) {
                dirty.height = frame.height - dirty.y;
            }
            return dirty.width > 0 && dirty.height > 0;
        }

        void UploadDirtyTexture(const RgbaFrame& frame, int32_t sourceStride, const DirtyFrameStats& dirty)
        {
            const size_t dirtyRowBytes = static_cast<size_t>(dirty.width) * 4U;
            const uint8_t* upload = nullptr;
            if (dirty.x == 0 && dirty.width == frame.width &&
                sourceStride == static_cast<int32_t>(dirtyRowBytes)) {
                upload = frame.data + static_cast<size_t>(dirty.y) * dirtyRowBytes;
                usingStagingBuffer_ = false;
                uploadMode_ = "dirty-direct";
            } else {
                const size_t required = dirtyRowBytes * dirty.height;
                uploadBuffer_.resize(required);
                for (uint32_t y = 0; y < dirty.height; ++y) {
                    const uint8_t* src = frame.data +
                        static_cast<int64_t>(dirty.y + y) * sourceStride +
                        static_cast<size_t>(dirty.x) * 4U;
                    std::memcpy(uploadBuffer_.data() + dirtyRowBytes * y, src, dirtyRowBytes);
                }
                upload = uploadBuffer_.data();
                usingStagingBuffer_ = true;
                uploadMode_ = "dirty-staged";
            }

            glTexSubImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(dirty.x),
                static_cast<GLint>(dirty.y), static_cast<GLsizei>(dirty.width),
                static_cast<GLsizei>(dirty.height), GL_RGBA, GL_UNSIGNED_BYTE, upload);
        }

        EGLDisplay display_ = EGL_NO_DISPLAY;
        EGLConfig config_ = nullptr;
        EGLContext context_ = EGL_NO_CONTEXT;
        EGLSurface surface_ = EGL_NO_SURFACE;
        OHNativeWindow* window_ = nullptr;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        GLuint program_ = 0;
        GLuint texture_ = 0;
        uint32_t textureWidth_ = 0;
        uint32_t textureHeight_ = 0;
        GLint positionAttrib_ = -1;
        GLint texCoordAttrib_ = -1;
        GLint textureUniform_ = -1;
        bool usingStagingBuffer_ = false;
        std::string uploadMode_;
        std::vector<uint8_t> uploadBuffer_;
    };

    class GpuAvc444SurfacePool {
    public:
        ~GpuAvc444SurfacePool()
        {
            Destroy();
        }

        void Destroy()
        {
            if (display_ != EGL_NO_DISPLAY) {
                if (context_ != EGL_NO_CONTEXT && pbufferSurface_ != EGL_NO_SURFACE) {
                    eglMakeCurrent(display_, pbufferSurface_, pbufferSurface_, context_);
                    DestroyDecodeSurface(luma_);
                    DestroyDecodeSurface(chroma_);
                    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                } else {
                    DestroyDecodeSurface(luma_);
                    DestroyDecodeSurface(chroma_);
                }

                if (context_ != EGL_NO_CONTEXT) {
                    eglDestroyContext(display_, context_);
                }
                if (pbufferSurface_ != EGL_NO_SURFACE) {
                    eglDestroySurface(display_, pbufferSurface_);
                }
                eglTerminate(display_);
            }

            display_ = EGL_NO_DISPLAY;
            config_ = nullptr;
            context_ = EGL_NO_CONTEXT;
            pbufferSurface_ = EGL_NO_SURFACE;
            width_ = 0;
            height_ = 0;
        }

        bool Ensure(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets, std::string& error)
        {
            if (width == 0 || height == 0) {
                error = "AVC444 decode surface size is invalid";
                return false;
            }
            if (!EnsureContext(error)) {
                return false;
            }
            if (!eglMakeCurrent(display_, pbufferSurface_, pbufferSurface_, context_)) {
                error = "AVC444 pbuffer make current failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }

            if (width_ != width || height_ != height || luma_.window == nullptr || chroma_.window == nullptr) {
                DestroyDecodeSurface(luma_);
                DestroyDecodeSurface(chroma_);
                width_ = 0;
                height_ = 0;
                if (!CreateDecodeSurface("luma", luma_, error) ||
                    !CreateDecodeSurface("chroma", chroma_, error)) {
                    eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
                    Destroy();
                    return false;
                }
                width_ = width;
                height_ = height;
            }

            eglMakeCurrent(display_, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            targets.lumaWindow = luma_.window;
            targets.chromaWindow = chroma_.window;
            targets.width = width_;
            targets.height = height_;
            targets.lumaTexture = luma_.texture;
            targets.chromaTexture = chroma_.texture;
            targets.lumaSurfaceId = luma_.surfaceId;
            targets.chromaSurfaceId = chroma_.surfaceId;
            return true;
        }

    private:
        struct DecodeSurface {
            GLuint texture = 0;
            OH_NativeImage* image = nullptr;
            OHNativeWindow* window = nullptr;
            uint64_t surfaceId = 0;
        };

        bool EnsureContext(std::string& error)
        {
            if (display_ != EGL_NO_DISPLAY && context_ != EGL_NO_CONTEXT &&
                pbufferSurface_ != EGL_NO_SURFACE) {
                return true;
            }

            Destroy();
            display_ = eglGetDisplay(EGL_DEFAULT_DISPLAY);
            if (display_ == EGL_NO_DISPLAY) {
                error = "AVC444 EGL get display failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                return false;
            }
            if (!eglInitialize(display_, nullptr, nullptr)) {
                error = "AVC444 EGL initialize failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }
            if (!eglBindAPI(EGL_OPENGL_ES_API)) {
                error = "AVC444 EGL bind GLES API failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }

            const EGLint configAttribs[] = {
                EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                EGL_SURFACE_TYPE, EGL_PBUFFER_BIT | EGL_WINDOW_BIT,
                EGL_RED_SIZE, 8,
                EGL_GREEN_SIZE, 8,
                EGL_BLUE_SIZE, 8,
                EGL_ALPHA_SIZE, 8,
                EGL_NONE,
            };
            EGLint configCount = 0;
            if (!eglChooseConfig(display_, configAttribs, &config_, 1, &configCount) || configCount <= 0) {
                error = "AVC444 EGL choose config failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }

            const EGLint pbufferAttribs[] = {EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE};
            pbufferSurface_ = eglCreatePbufferSurface(display_, config_, pbufferAttribs);
            if (pbufferSurface_ == EGL_NO_SURFACE) {
                error = "AVC444 EGL create pbuffer failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }

            const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
            context_ = eglCreateContext(display_, config_, EGL_NO_CONTEXT, contextAttribs);
            if (context_ == EGL_NO_CONTEXT) {
                error = "AVC444 EGL create context failed: " + Hex32(static_cast<uint32_t>(eglGetError()));
                Destroy();
                return false;
            }
            return true;
        }

        bool CreateDecodeSurface(const char* name, DecodeSurface& surface, std::string& error)
        {
            glGenTextures(1, &surface.texture);
            if (surface.texture == 0) {
                error = std::string("AVC444 ") + name + " texture allocation failed";
                return false;
            }

            glBindTexture(GL_TEXTURE_EXTERNAL_OES, surface.texture);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            const GLenum glError = glGetError();
            if (glError != GL_NO_ERROR) {
                error = std::string("AVC444 ") + name +
                    " external texture setup failed: " + Hex32(static_cast<uint32_t>(glError));
                DestroyDecodeSurface(surface);
                return false;
            }

            surface.image = OH_NativeImage_Create(surface.texture, GL_TEXTURE_EXTERNAL_OES);
            if (surface.image == nullptr) {
                error = std::string("AVC444 ") + name + " NativeImage create failed";
                DestroyDecodeSurface(surface);
                return false;
            }

            surface.window = OH_NativeImage_AcquireNativeWindow(surface.image);
            if (surface.window == nullptr) {
                error = std::string("AVC444 ") + name + " NativeImage window acquire failed";
                DestroyDecodeSurface(surface);
                return false;
            }

            (void)OH_NativeImage_GetSurfaceId(surface.image, &surface.surfaceId);
            return true;
        }

        static void DestroyDecodeSurface(DecodeSurface& surface)
        {
            if (surface.image != nullptr) {
                OH_NativeImage_Destroy(&surface.image);
            }
            if (surface.texture != 0) {
                GLuint texture = surface.texture;
                glDeleteTextures(1, &texture);
            }
            surface.texture = 0;
            surface.window = nullptr;
            surface.surfaceId = 0;
        }

        EGLDisplay display_ = EGL_NO_DISPLAY;
        EGLConfig config_ = nullptr;
        EGLContext context_ = EGL_NO_CONTEXT;
        EGLSurface pbufferSurface_ = EGL_NO_SURFACE;
        uint32_t width_ = 0;
        uint32_t height_ = 0;
        DecodeSurface luma_;
        DecodeSurface chroma_;
    };

    static constexpr size_t kDirtyHistoryLimit = 240;

    SurfacePaintResult RenderRgbaFrameLocked(const RgbaFrame& frame)
    {
        SurfacePaintResult result;
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            result.message = "XComponent surface is not ready for render";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0) {
            result.message = "RGBA frame is empty";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            result.message = "RGBA frame stride is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        constexpr uint32_t maxTargetSize = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
        if (width_ > maxTargetSize || height_ > maxTargetSize) {
            result.message = "NativeWindow target geometry exceeds int32 range";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        const RenderViewport viewport = FitFrameIntoTarget(width_, height_, frame.width, frame.height);
        if (viewport.width == 0 || viewport.height == 0) {
            result.message = "render viewport is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        if (gpuRenderer_.Render(nativeWindow, width_, height_, frame, sourceStride, viewport, result)) {
            ++paintCount_;
            viewportX_ = viewport.x;
            viewportY_ = viewport.y;
            viewportWidth_ = viewport.width;
            viewportHeight_ = viewport.height;
            lastPaintMessage_ = result.message;
            return result;
        }

        if (!gpuFallbackLogged_ && !result.logs.empty()) {
            EmitNativeLog("GLES render fallback to CPU: " + result.logs.back());
            gpuFallbackLogged_ = true;
        }
        return RenderRgbaFrameCpuLocked(frame);
    }

    SurfacePaintResult RenderRgbaFrameCpuLocked(const RgbaFrame& frame)
    {
        SurfacePaintResult result;
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            result.message = "XComponent surface is not ready for render";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0) {
            result.message = "RGBA frame is empty";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            result.message = "RGBA frame stride is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        constexpr uint32_t maxTargetSize = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
        if (width_ > maxTargetSize || height_ > maxTargetSize) {
            result.message = "NativeWindow target geometry exceeds int32 range";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t targetWidth = static_cast<int32_t>(width_);
        const int32_t targetHeight = static_cast<int32_t>(height_);

        if (!ConfigureNativeWindowLocked(nativeWindow, targetWidth, targetHeight, result)) {
            return result;
        }

        OHNativeWindowBuffer* buffer = nullptr;
        int fenceFd = -1;
        int32_t rc = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &buffer, &fenceFd);
        if (rc != 0 || buffer == nullptr) {
            CloseFence(fenceFd);
            result.message = "NativeWindow request buffer failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        std::string fenceError;
        if (!WaitFenceAndClose(fenceFd, fenceError)) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow fence wait failed: " + fenceError;
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        fenceFd = -1;

        BufferHandle* handle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
        if (handle == nullptr || handle->virAddr == nullptr) {
            result.logs.push_back("NativeWindow BufferHandle has no direct CPU address; using NativeBuffer map");
        }
        if (handle == nullptr) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer handle is null";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        if (!IsSupportedFourByteFormat(handle->format)) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer format is not supported: " + std::to_string(handle->format);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        const uint32_t bufferWidth = handle->width > 0 ? static_cast<uint32_t>(handle->width) : width_;
        const uint32_t bufferHeight = handle->height > 0 ? static_cast<uint32_t>(handle->height) : height_;
        const uint32_t targetAreaWidth = std::min(width_, bufferWidth);
        const uint32_t targetAreaHeight = std::min(height_, bufferHeight);
        const int32_t rowBytes = ResolveRowBytes(*handle, targetAreaWidth, targetAreaHeight);
        if (targetAreaWidth == 0 || targetAreaHeight == 0 || rowBytes <= 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer geometry is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const RenderViewport bufferViewport = FitFrameIntoTarget(
            targetAreaWidth, targetAreaHeight, frame.width, frame.height);
        if (bufferViewport.width == 0 || bufferViewport.height == 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "render viewport is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        OH_NativeBuffer* nativeBuffer = nullptr;
        void* mappedAddress = handle->virAddr;
        int32_t mappedRowBytes = rowBytes;
        bool mappedNativeBuffer = false;
        if (mappedAddress == nullptr) {
            rc = OH_NativeBuffer_FromNativeWindowBuffer(buffer, &nativeBuffer);
            if (rc != 0 || nativeBuffer == nullptr) {
                OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
                result.message = "NativeBuffer conversion failed: " + std::to_string(rc);
                result.logs.push_back(result.message);
                lastPaintMessage_ = result.message;
                return result;
            }

            rc = OH_NativeBuffer_Map(nativeBuffer, &mappedAddress);
            if (rc != 0 || mappedAddress == nullptr) {
                OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
                result.message = "NativeBuffer map failed: " + std::to_string(rc);
                result.logs.push_back(result.message);
                lastPaintMessage_ = result.message;
                return result;
            }
            mappedNativeBuffer = true;

            OH_NativeBuffer_Config config = {};
            OH_NativeBuffer_GetConfig(nativeBuffer, &config);
            if (config.stride >= static_cast<int32_t>(targetAreaWidth * 4U)) {
                mappedRowBytes = config.stride;
            }
        }

        if (mappedRowBytes < static_cast<int32_t>(targetAreaWidth * 4U)) {
            if (mappedNativeBuffer) {
                OH_NativeBuffer_Unmap(nativeBuffer);
            }
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer row stride is invalid: " + std::to_string(mappedRowBytes);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        BufferHandle mappedHandle = *handle;
        mappedHandle.virAddr = mappedAddress;
        const uintptr_t bufferKey = reinterpret_cast<uintptr_t>(buffer);
        DirtyFrameStats partialDirty;
        const bool canUsePartialDirty = CanUsePartialDirtyLocked(bufferKey, frame,
            targetAreaWidth, targetAreaHeight, bufferViewport, partialDirty);
        if (canUsePartialDirty) {
            CopyRgbaRectToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride,
                partialDirty.x, partialDirty.y, partialDirty.width, partialDirty.height);
        } else {
            FillNativeLetterbox(mappedHandle, mappedRowBytes, targetAreaWidth, targetAreaHeight,
                bufferViewport, 0, 0, 0, 0xFF);
            CopyScaledRgbaToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride,
                frame.width, frame.height, bufferViewport);
        }
        if (mappedNativeBuffer) {
            OH_NativeBuffer_Unmap(nativeBuffer);
        }

        Region::Rect dirtyRect = canUsePartialDirty ?
            Region::Rect{static_cast<int32_t>(partialDirty.x), static_cast<int32_t>(partialDirty.y),
                partialDirty.width, partialDirty.height} :
            Region::Rect{0, 0, targetAreaWidth, targetAreaHeight};
        Region dirtyRegion = {&dirtyRect, 1};
        rc = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, buffer, -1, dirtyRegion);
        if (rc != 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow flush buffer failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        ++paintCount_;
        RecordBufferFrameLocked(bufferKey, frame);
        result.partial = canUsePartialDirty;
        viewportX_ = bufferViewport.x;
        viewportY_ = bufferViewport.y;
        viewportWidth_ = bufferViewport.width;
        viewportHeight_ = bufferViewport.height;
        result.ok = true;
        const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
        result.message = "NativeWindow RGBA frame rendered: " + frameLabel + " " +
            std::to_string(bufferViewport.width) + "x" + std::to_string(bufferViewport.height) +
            " bufferViewport=" + std::to_string(bufferViewport.x) + "," +
            std::to_string(bufferViewport.y) + " displayViewport=" +
            std::to_string(viewportX_) + "," + std::to_string(viewportY_) + " " +
            std::to_string(viewportWidth_) + "x" + std::to_string(viewportHeight_) +
            (canUsePartialDirty ? " mode=dirty-bbox " + DescribeDirtyStats(partialDirty) : " mode=full");
        result.logs.push_back(result.message);
        result.logs.push_back("RGBA source=" + std::to_string(frame.width) + "x" +
            std::to_string(frame.height) + " stride=" + std::to_string(sourceStride));
        result.logs.push_back("NativeWindow format=" + std::to_string(handle->format) +
            " stride=" + std::to_string(handle->stride) +
            " rowBytes=" + std::to_string(mappedRowBytes) +
            " directVirAddr=" + std::string(mappedNativeBuffer ? "false" : "true"));
        lastPaintMessage_ = result.message;
        return result;
    }

    void ClearViewportLocked()
    {
        viewportX_ = 0;
        viewportY_ = 0;
        viewportWidth_ = 0;
        viewportHeight_ = 0;
    }

    void ClearNativeWindowConfigLocked()
    {
        gpuRenderer_.Destroy();
        avc444Surfaces_.Destroy();
        gpuFallbackLogged_ = false;
        configuredWindow_ = nullptr;
        configuredWidth_ = 0;
        configuredHeight_ = 0;
        configuredFormat_ = 0;
        configuredUsage_ = 0;
        bufferFrameSequences_.clear();
        dirtyHistory_.clear();
        dirtyHistoryWidth_ = 0;
        dirtyHistoryHeight_ = 0;
    }

    bool CanUsePartialDirtyLocked(uintptr_t bufferKey, const RgbaFrame& frame,
        uint32_t targetAreaWidth, uint32_t targetAreaHeight, const RenderViewport& viewport,
        DirtyFrameStats& dirty) const
    {
        constexpr uint32_t kMaxPartialDirtyAreaPermille = 650;
        if (bufferKey == 0 || frame.sequence == 0 || frame.dirtySequenceStart == 0 || !frame.dirty.valid) {
            return false;
        }
        if (frame.width != targetAreaWidth || frame.height != targetAreaHeight ||
            viewport.x != 0 || viewport.y != 0 ||
            viewport.width != frame.width || viewport.height != frame.height) {
            return false;
        }
        if (dirtyHistoryWidth_ != frame.width || dirtyHistoryHeight_ != frame.height) {
            return false;
        }

        const auto bufferIt = bufferFrameSequences_.find(bufferKey);
        if (bufferIt == bufferFrameSequences_.end() || bufferIt->second >= frame.sequence) {
            return false;
        }

        const uint64_t lastBufferSequence = bufferIt->second;
        uint64_t expectedSequence = lastBufferSequence + 1U;
        DirtyFrameStats accumulated;
        for (const DirtyHistoryEntry& entry : dirtyHistory_) {
            if (entry.toSequence <= lastBufferSequence) {
                continue;
            }
            if (entry.fromSequence > expectedSequence) {
                return false;
            }
            accumulated = MergeDirtyStats(accumulated, entry.dirty, frame.width, frame.height);
            expectedSequence = entry.toSequence + 1U;
        }

        if (frame.dirtySequenceStart > expectedSequence) {
            return false;
        }
        accumulated = MergeDirtyStats(accumulated, frame.dirty, frame.width, frame.height);
        if (!accumulated.valid || accumulated.width == 0 || accumulated.height == 0) {
            return false;
        }
        if (accumulated.x + accumulated.width > targetAreaWidth ||
            accumulated.y + accumulated.height > targetAreaHeight) {
            return false;
        }
        if (accumulated.areaPermille > kMaxPartialDirtyAreaPermille) {
            return false;
        }

        dirty = accumulated;
        return true;
    }

    void RecordBufferFrameLocked(uintptr_t bufferKey, const RgbaFrame& frame)
    {
        if (frame.sequence == 0) {
            return;
        }
        if (dirtyHistoryWidth_ != frame.width || dirtyHistoryHeight_ != frame.height) {
            dirtyHistory_.clear();
            bufferFrameSequences_.clear();
            dirtyHistoryWidth_ = frame.width;
            dirtyHistoryHeight_ = frame.height;
        }

        DirtyHistoryEntry entry;
        entry.fromSequence = frame.dirtySequenceStart == 0 ? frame.sequence : frame.dirtySequenceStart;
        entry.toSequence = frame.sequence;
        entry.dirty = frame.dirty;
        dirtyHistory_.push_back(entry);
        while (dirtyHistory_.size() > kDirtyHistoryLimit) {
            dirtyHistory_.pop_front();
        }
        if (bufferKey != 0) {
            bufferFrameSequences_[bufferKey] = frame.sequence;
        }
    }

    bool ConfigureNativeWindowLocked(OHNativeWindow* nativeWindow, int32_t targetWidth,
        int32_t targetHeight, SurfacePaintResult& result)
    {
        if (configuredWindow_ != nativeWindow) {
            configuredWindow_ = nativeWindow;
            configuredWidth_ = 0;
            configuredHeight_ = 0;
            configuredFormat_ = 0;
            configuredUsage_ = 0;
        }

        if (configuredWidth_ != targetWidth || configuredHeight_ != targetHeight) {
            const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(
                nativeWindow, SET_BUFFER_GEOMETRY, targetWidth, targetHeight);
            if (rc != 0) {
                result.message = "NativeWindow SET_BUFFER_GEOMETRY failed: " + std::to_string(rc);
                result.logs.push_back(result.message);
                lastPaintMessage_ = result.message;
                return false;
            }
            configuredWidth_ = targetWidth;
            configuredHeight_ = targetHeight;
        }

        constexpr int32_t format = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
        if (configuredFormat_ != format) {
            const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, format);
            if (rc != 0) {
                result.logs.push_back("NativeWindow SET_FORMAT warning: " + std::to_string(rc));
            } else {
                configuredFormat_ = format;
            }
        }

        constexpr uint64_t usage = NATIVEBUFFER_USAGE_CPU_WRITE | NATIVEBUFFER_USAGE_MEM_DMA |
            NATIVEBUFFER_USAGE_HW_TEXTURE;
        if (configuredUsage_ != usage) {
            const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_USAGE, usage);
            if (rc != 0) {
                result.logs.push_back("NativeWindow SET_USAGE warning: " + std::to_string(rc));
            } else {
                configuredUsage_ = usage;
            }
        }

        return true;
    }

    static void CloseFence(int fenceFd)
    {
        if (fenceFd >= 0) {
            ::close(fenceFd);
        }
    }

    static bool WaitFenceAndClose(int fenceFd, std::string& error)
    {
        if (fenceFd < 0) {
            return true;
        }

        pollfd fence = {fenceFd, POLLIN, 0};
        int rc = ::poll(&fence, 1, 3000);
        int savedErrno = errno;
        CloseFence(fenceFd);
        if (rc > 0) {
            return true;
        }
        if (rc == 0) {
            error = "timeout";
            return false;
        }
        error = SystemErrorMessage(savedErrno);
        return false;
    }

    static bool IsSupportedFourByteFormat(int32_t format)
    {
        return format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888;
    }

    static int32_t ResolveRowBytes(const BufferHandle& handle, uint32_t drawWidth, uint32_t drawHeight)
    {
        if (drawWidth == 0 || drawHeight == 0) {
            return 0;
        }

        const int64_t tightRowBytes = static_cast<int64_t>(drawWidth) * 4;
        const int64_t stride = handle.stride > 0 ? handle.stride : handle.width;
        const int64_t pixelStrideRowBytes = stride * 4;
        const int64_t byteStrideRowBytes = stride;
        const int64_t size = handle.size;

        if (byteStrideRowBytes >= tightRowBytes && (size <= 0 || byteStrideRowBytes * drawHeight <= size)) {
            return static_cast<int32_t>(byteStrideRowBytes);
        }
        if (pixelStrideRowBytes >= tightRowBytes && (size <= 0 || pixelStrideRowBytes * drawHeight <= size)) {
            return static_cast<int32_t>(pixelStrideRowBytes);
        }
        if (size <= 0 || tightRowBytes * drawHeight <= size) {
            return static_cast<int32_t>(tightRowBytes);
        }
        return 0;
    }

    static RenderViewport FitFrameIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
        uint32_t sourceWidth, uint32_t sourceHeight)
    {
        RenderViewport viewport;
        if (targetWidth == 0 || targetHeight == 0 || sourceWidth == 0 || sourceHeight == 0) {
            return viewport;
        }

        const uint64_t targetBySourceHeight = static_cast<uint64_t>(targetWidth) * sourceHeight;
        const uint64_t targetHeightBySourceWidth = static_cast<uint64_t>(targetHeight) * sourceWidth;
        if (targetBySourceHeight <= targetHeightBySourceWidth) {
            viewport.width = targetWidth;
            viewport.height = static_cast<uint32_t>(
                std::max<uint64_t>(1, targetBySourceHeight / sourceWidth));
        } else {
            viewport.height = targetHeight;
            viewport.width = static_cast<uint32_t>(
                std::max<uint64_t>(1, targetHeightBySourceWidth / sourceHeight));
        }

        viewport.width = std::min(viewport.width, targetWidth);
        viewport.height = std::min(viewport.height, targetHeight);
        viewport.x = (targetWidth - viewport.width) / 2U;
        viewport.y = (targetHeight - viewport.height) / 2U;
        return viewport;
    }

    static void WriteRgbaPixel(uint8_t* pixel, uint8_t r, uint8_t g, uint8_t b)
    {
        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
        pixel[3] = 0xFF;
    }

    static void CopyRgbaPixelToNative(uint8_t* pixel, int32_t format, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        if (format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 || format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888) {
            pixel[0] = b;
            pixel[1] = g;
            pixel[2] = r;
            pixel[3] = a;
            return;
        }

        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
        pixel[3] = a;
    }

    static void FillNativeRect(const BufferHandle& handle, int32_t rowBytes, uint32_t x,
        uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        if (width == 0 || height == 0) {
            return;
        }

        auto* target = static_cast<uint8_t*>(handle.virAddr);
        for (uint32_t row = 0; row < height; ++row) {
            uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (y + row) +
                static_cast<int64_t>(x) * 4;
            for (uint32_t column = 0; column < width; ++column) {
                CopyRgbaPixelToNative(targetRow + column * 4, handle.format, r, g, b, a);
            }
        }
    }

    static void FillNativeLetterbox(const BufferHandle& handle, int32_t rowBytes,
        uint32_t width, uint32_t height, const RenderViewport& viewport,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a)
    {
        FillNativeRect(handle, rowBytes, 0, 0, width, viewport.y, r, g, b, a);
        const uint32_t bottomY = viewport.y + viewport.height;
        if (bottomY < height) {
            FillNativeRect(handle, rowBytes, 0, bottomY, width, height - bottomY, r, g, b, a);
        }
        FillNativeRect(handle, rowBytes, 0, viewport.y, viewport.x, viewport.height, r, g, b, a);
        const uint32_t rightX = viewport.x + viewport.width;
        if (rightX < width) {
            FillNativeRect(handle, rowBytes, rightX, viewport.y, width - rightX,
                viewport.height, r, g, b, a);
        }
    }

    static void CopyRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t width, uint32_t height)
    {
        auto* target = static_cast<uint8_t*>(handle.virAddr);
        if (handle.format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
            handle.format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888) {
            const size_t bytesPerRow = static_cast<size_t>(width) * 4U;
            for (uint32_t y = 0; y < height; ++y) {
                std::memcpy(target + static_cast<int64_t>(rowBytes) * y,
                    source + static_cast<int64_t>(sourceStride) * y, bytesPerRow);
            }
            return;
        }

        for (uint32_t y = 0; y < height; ++y) {
            uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * y;
            const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * y;
            for (uint32_t x = 0; x < width; ++x) {
                const uint8_t* sourcePixel = sourceRow + x * 4;
                CopyRgbaPixelToNative(targetRow + x * 4, handle.format, sourcePixel[0],
                    sourcePixel[1], sourcePixel[2], sourcePixel[3]);
            }
        }
    }

    static void CopyRgbaRectToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0) {
            return;
        }

        auto* target = static_cast<uint8_t*>(handle.virAddr);
        if (handle.format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
            handle.format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888) {
            const size_t bytesPerRow = static_cast<size_t>(width) * 4U;
            for (uint32_t row = 0; row < height; ++row) {
                std::memcpy(target + static_cast<int64_t>(rowBytes) * (y + row) +
                    static_cast<int64_t>(x) * 4,
                    source + static_cast<int64_t>(sourceStride) * (y + row) +
                    static_cast<int64_t>(x) * 4,
                    bytesPerRow);
            }
            return;
        }

        for (uint32_t row = 0; row < height; ++row) {
            uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (y + row) +
                static_cast<int64_t>(x) * 4;
            const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * (y + row) +
                static_cast<int64_t>(x) * 4;
            for (uint32_t column = 0; column < width; ++column) {
                const uint8_t* sourcePixel = sourceRow + column * 4;
                CopyRgbaPixelToNative(targetRow + column * 4, handle.format, sourcePixel[0],
                    sourcePixel[1], sourcePixel[2], sourcePixel[3]);
            }
        }
    }

    static void CopyScaledRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t sourceWidth, uint32_t sourceHeight, const RenderViewport& viewport)
    {
        auto* target = static_cast<uint8_t*>(handle.virAddr);
        if (sourceWidth == viewport.width && sourceHeight == viewport.height &&
            viewport.x == 0 && viewport.y == 0) {
            CopyRgbaToNative(handle, rowBytes, source, sourceStride, sourceWidth, sourceHeight);
            return;
        }

        for (uint32_t y = 0; y < viewport.height; ++y) {
            const uint32_t sourceY = static_cast<uint32_t>(
                (static_cast<uint64_t>(y) * sourceHeight) / viewport.height);
            uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (viewport.y + y) +
                static_cast<int64_t>(viewport.x) * 4;
            const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * sourceY;
            for (uint32_t x = 0; x < viewport.width; ++x) {
                const uint32_t sourceX = static_cast<uint32_t>(
                    (static_cast<uint64_t>(x) * sourceWidth) / viewport.width);
                const uint8_t* sourcePixel = sourceRow + sourceX * 4;
                CopyRgbaPixelToNative(targetRow + x * 4, handle.format, sourcePixel[0],
                    sourcePixel[1], sourcePixel[2], sourcePixel[3]);
            }
        }
    }

    SurfaceSnapshot SnapshotLocked() const
    {
        return SurfaceSnapshot{
            registered_,
            ready_,
            id_.empty() ? "unknown" : id_,
            width_,
            height_,
            viewportX_,
            viewportY_,
            viewportWidth_,
            viewportHeight_,
            createdCount_,
            changedCount_,
            destroyedCount_,
            touchCount_,
            paintCount_,
            lastPaintMessage_,
        };
    }

    std::mutex mutex_;
    OH_NativeXComponent* component_ = nullptr;
    void* window_ = nullptr;
    bool registered_ = false;
    bool ready_ = false;
    std::string id_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t viewportX_ = 0;
    uint32_t viewportY_ = 0;
    uint32_t viewportWidth_ = 0;
    uint32_t viewportHeight_ = 0;
    uint32_t createdCount_ = 0;
    uint32_t changedCount_ = 0;
    uint32_t destroyedCount_ = 0;
    uint32_t touchCount_ = 0;
    uint32_t paintCount_ = 0;
    void* configuredWindow_ = nullptr;
    int32_t configuredWidth_ = 0;
    int32_t configuredHeight_ = 0;
    int32_t configuredFormat_ = 0;
    uint64_t configuredUsage_ = 0;
    GpuRgbaRenderer gpuRenderer_;
    GpuAvc444SurfacePool avc444Surfaces_;
    bool gpuFallbackLogged_ = false;
    std::unordered_map<uintptr_t, uint64_t> bufferFrameSequences_;
    std::deque<DirtyHistoryEntry> dirtyHistory_;
    uint32_t dirtyHistoryWidth_ = 0;
    uint32_t dirtyHistoryHeight_ = 0;
    std::string lastPaintMessage_;
};

SurfaceBridge g_surface;

DecoderSurfaceTarget SnapshotDecoderSurfaceTarget()
{
    return g_surface.DecoderSurface();
}

bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
    std::string& error)
{
    return g_surface.EnsureAvc444SurfaceTargets(width, height, targets, error);
}

void OnAvc444SurfaceFrameDecoded(uint32_t surfaceId, uint32_t width, uint32_t height,
    uint32_t op, uint32_t codecId, void*)
{
    const uint64_t count = ++g_avc444SurfaceFrameCallbackCount;
    if (count <= 3 || (count % 120) == 0) {
        EmitNativeLog("OHOS AVC444 surface frame callback: count=" + std::to_string(count) +
            " surfaceId=" + std::to_string(surfaceId) +
            " size=" + std::to_string(width) + "x" + std::to_string(height) +
            " op=" + std::to_string(op) +
            " codec=" + Hex32(codecId));
    }
}

bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const FreerdpLogFn& log)
{
    if (api.ohosAvcodecSetAvc444OutputSurfaces == nullptr) {
        log("OHOS AVC444 NativeImage surface registration skipped: FreeRDP symbol unavailable");
        return false;
    }

    Avc444SurfaceTargets targets;
    std::string error;
    if (!EnsureAvc444SurfaceTargets(width, height, targets, error)) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        log("OHOS AVC444 NativeImage surface registration failed: " + error);
        return false;
    }

    api.ohosAvcodecSetAvc444OutputSurfaces(
        targets.lumaWindow, targets.chromaWindow, targets.width, targets.height, TRUE);
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(OnAvc444SurfaceFrameDecoded, nullptr);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    log("OHOS AVC444 NativeImage decode surfaces registered: " +
        std::to_string(targets.width) + "x" + std::to_string(targets.height) +
        " lumaTex=" + std::to_string(targets.lumaTexture) +
        " chromaTex=" + std::to_string(targets.chromaTexture) +
        " lumaSurface=" + std::to_string(targets.lumaSurfaceId) +
        " chromaSurface=" + std::to_string(targets.chromaSurfaceId) +
        " route=disabled-until-compositor");
    return true;
}

void UpdateAvc420SurfaceOutputIfActive(const std::string& reason)
{
    if (!g_avc420SurfaceOutputEnabled.load()) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        EmitNativeLog("AVC420 surface update skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return;
    }

    const DecoderSurfaceTarget target = SnapshotDecoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
            api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        EmitNativeLog("AVC420 surface output disabled after " + reason + ": XComponent surface unavailable");
        return;
    }

    api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE);
    RegisterAvc444DecodeSurfaces(api, target.width, target.height, EmitNativeLog);
    EmitNativeLog("AVC420 surface output updated after " + reason + ": " +
        std::to_string(target.width) + "x" + std::to_string(target.height));
}

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

class LatestFrameRenderer {
public:
    void Start()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (running_) {
            return;
        }

        ResetStatsLocked();
        running_ = true;
        worker_ = std::thread([this]() { WorkerLoop(); });
    }

    void Stop()
    {
        std::thread worker;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && !worker_.joinable()) {
                hasPending_ = false;
                return;
            }
            running_ = false;
            hasPending_ = false;
            worker = std::move(worker_);
        }

        condition_.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool Enqueue(const RgbaFrame& frame, std::string& message, bool forceRender)
    {
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            message = "invalid frame";
            return false;
        }

        PendingFrame next;
        next.width = frame.width;
        next.height = frame.height;
        next.strideBytes = sourceStride;
        next.label = frame.label.empty() ? "freerdp gdi queued" : frame.label + " queued";
        next.data = frame.data;
        next.dirty = frame.dirty;
        next.forceRender = forceRender;
        next.sequence = 0;
        next.dirtySequenceStart = 0;
        const uint32_t copyUs = 0;

        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_) {
                message = "render thread stopped";
                return false;
            }
            next.sequence = ++frameSequence_;
            next.dirtySequenceStart = next.sequence;
            if (hasPending_) {
                next.forceRender = next.forceRender || pending_.forceRender;
                if (pending_.width == next.width && pending_.height == next.height) {
                    next.dirty = MergeDirtyStats(pending_.dirty, next.dirty, next.width, next.height);
                    if (pending_.dirtySequenceStart > 0) {
                        next.dirtySequenceStart = pending_.dirtySequenceStart;
                    }
                }
                ++replacedCount_;
            }
            lastCopyUs_ = copyUs;
            totalCopyUs_ += copyUs;
            lastDirty_ = next.dirty;
            ++queuedCount_;
            message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                " latest-gdi " + DescribeDirtyStats(frame.dirty);
            pending_ = std::move(next);
            hasPending_ = true;
        }

        condition_.notify_one();
        return true;
    }

    RenderStatsSnapshot Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RenderStatsSnapshot snapshot;
        snapshot.running = running_;
        snapshot.queued = queuedCount_;
        snapshot.rendered = renderedCount_;
        snapshot.failed = failedCount_;
        snapshot.replaced = replacedCount_;
        snapshot.throttled = throttledCount_;
        snapshot.fullRendered = fullRenderedCount_;
        snapshot.partialRendered = partialRenderedCount_;
        snapshot.pending = hasPending_ ? 1U : 0U;
        snapshot.lastWidth = lastRenderedWidth_;
        snapshot.lastHeight = lastRenderedHeight_;
        snapshot.lastCopyUs = lastCopyUs_;
        snapshot.lastRenderUs = lastRenderUs_;
        snapshot.avgCopyUs = queuedCount_ == 0 ? 0 :
            static_cast<uint32_t>(totalCopyUs_ / queuedCount_);
        snapshot.avgRenderUs = renderedCount_ == 0 ? 0 :
            static_cast<uint32_t>(totalRenderUs_ / renderedCount_);
        snapshot.lastDirty = lastDirty_;
        snapshot.targetFrameIntervalMs = lastTargetFrameIntervalMs_;
        const uint64_t elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - statsStartedAt_).count();
        if (elapsedMs > 0) {
            snapshot.fpsX100 = static_cast<uint32_t>((renderedCount_ * 100000ULL) / elapsedMs);
        }
        return snapshot;
    }

private:
    struct PendingFrame {
        const uint8_t* data = nullptr;
        uint32_t width = 0;
        uint32_t height = 0;
        int32_t strideBytes = 0;
        std::string label;
        bool forceRender = false;
        DirtyFrameStats dirty;
        uint64_t sequence = 0;
        uint64_t dirtySequenceStart = 0;
    };

    static constexpr uint32_t kTargetFrameIntervalMs = 16;
    static constexpr uint32_t kLargeDirtyFrameIntervalMs = 33;
    static constexpr uint32_t kLargeDirtyAreaPermille = 700;

    static uint32_t ResolveTargetFrameIntervalMs(const PendingFrame& frame, bool sizeChanged)
    {
        if (frame.forceRender || sizeChanged) {
            return 0;
        }
        if (frame.dirty.valid && frame.dirty.areaPermille >= kLargeDirtyAreaPermille) {
            return kLargeDirtyFrameIntervalMs;
        }
        return kTargetFrameIntervalMs;
    }

    void ResetStatsLocked()
    {
        hasPending_ = false;
        queuedCount_ = 0;
        renderedCount_ = 0;
        failedCount_ = 0;
        replacedCount_ = 0;
        throttledCount_ = 0;
        fullRenderedCount_ = 0;
        partialRenderedCount_ = 0;
        totalCopyUs_ = 0;
        totalRenderUs_ = 0;
        lastCopyUs_ = 0;
        lastRenderUs_ = 0;
        lastDirty_ = DirtyFrameStats{};
        lastRenderedWidth_ = 0;
        lastRenderedHeight_ = 0;
        frameSequence_ = 0;
        lastTargetFrameIntervalMs_ = 0;
        lastRenderFinishedAt_ = std::chrono::steady_clock::time_point{};
        statsStartedAt_ = std::chrono::steady_clock::now();
    }

    void WorkerLoop()
    {
        for (;;) {
            PendingFrame frame;
            uint32_t targetFrameIntervalMs = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() { return !running_ || hasPending_; });
                for (;;) {
                    if (!running_ && !hasPending_) {
                        return;
                    }
                    if (!hasPending_) {
                        condition_.wait(lock, [this]() { return !running_ || hasPending_; });
                        continue;
                    }

                    const bool forceRender = pending_.forceRender;
                    const bool sizeChanged = pending_.width != lastRenderedWidth_ ||
                        pending_.height != lastRenderedHeight_;
                    targetFrameIntervalMs = ResolveTargetFrameIntervalMs(pending_, sizeChanged);
                    lastTargetFrameIntervalMs_ = targetFrameIntervalMs;
                    if (!forceRender && !sizeChanged && targetFrameIntervalMs > 0 && renderedCount_ > 0 &&
                        lastRenderFinishedAt_ != std::chrono::steady_clock::time_point{}) {
                        const auto nextRenderAt = lastRenderFinishedAt_ +
                            std::chrono::milliseconds(targetFrameIntervalMs);
                        const auto now = std::chrono::steady_clock::now();
                        if (now < nextRenderAt) {
                            ++throttledCount_;
                            condition_.wait_until(lock, nextRenderAt);
                            continue;
                        }
                    }

                    frame = std::move(pending_);
                    hasPending_ = false;
                    break;
                }
            }

            RgbaFrame view = {
                frame.data,
                frame.width,
                frame.height,
                frame.strideBytes,
                frame.label,
                frame.dirty,
                frame.sequence,
                frame.dirtySequenceStart,
            };
            const bool forcedRender = frame.forceRender;
            const auto renderStart = std::chrono::steady_clock::now();
            SurfacePaintResult paint = RenderSurfaceRgbaFrame(view);
            const auto renderEnd = std::chrono::steady_clock::now();
            const uint32_t renderUs = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(renderEnd - renderStart).count());

            uint64_t rendered = 0;
            uint64_t failed = 0;
            uint64_t partialRendered = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                lastRenderUs_ = renderUs;
                lastRenderedWidth_ = frame.width;
                lastRenderedHeight_ = frame.height;
                lastRenderFinishedAt_ = renderEnd;
                if (paint.ok) {
                    ++renderedCount_;
                    if (paint.partial) {
                        ++partialRenderedCount_;
                    } else {
                        ++fullRenderedCount_;
                    }
                    totalRenderUs_ += renderUs;
                } else {
                    ++failedCount_;
                }
                rendered = renderedCount_;
                failed = failedCount_;
                partialRendered = partialRenderedCount_;
            }

            if (paint.ok) {
                if (forcedRender || rendered <= 3 || rendered % 60 == 0 ||
                    (paint.partial && (partialRendered <= 3 || partialRendered % 60 == 0))) {
                    EmitNativeLog("Render thread painted frame " + std::to_string(rendered) +
                        " render=" + std::to_string(renderUs / 1000.0) + "ms pace=" +
                        std::to_string(targetFrameIntervalMs) + "ms " + paint.message);
                }
            } else if (failed <= 3 || failed % 120 == 0) {
                EmitNativeLog("Render thread paint failed: " + paint.message);
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_;
    PendingFrame pending_;
    bool running_ = false;
    bool hasPending_ = false;
    uint64_t queuedCount_ = 0;
    uint64_t renderedCount_ = 0;
    uint64_t failedCount_ = 0;
    uint64_t replacedCount_ = 0;
    uint64_t throttledCount_ = 0;
    uint64_t fullRenderedCount_ = 0;
    uint64_t partialRenderedCount_ = 0;
    uint64_t totalCopyUs_ = 0;
    uint64_t totalRenderUs_ = 0;
    uint32_t lastCopyUs_ = 0;
    uint32_t lastRenderUs_ = 0;
    DirtyFrameStats lastDirty_;
    uint32_t lastRenderedWidth_ = 0;
    uint32_t lastRenderedHeight_ = 0;
    uint64_t frameSequence_ = 0;
    uint32_t lastTargetFrameIntervalMs_ = 0;
    std::chrono::steady_clock::time_point lastRenderFinishedAt_;
    std::chrono::steady_clock::time_point statsStartedAt_ = std::chrono::steady_clock::now();
};

LatestFrameRenderer g_frameRenderer;

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    return g_frameRenderer.Enqueue(frame, message, forceRender);
}

void StartRenderPipeline()
{
    g_frameRenderer.Start();
}

void StopRenderPipeline()
{
    g_frameRenderer.Stop();
}

std::string BuildRenderStatsLog()
{
    const RenderStatsSnapshot stats = g_frameRenderer.Snapshot();
    std::ostringstream out;
    out << "render "
        << (stats.running ? "running" : "stopped")
        << " queued=" << stats.queued
        << " rendered=" << stats.rendered
        << " failed=" << stats.failed
        << " replaced=" << stats.replaced
        << " paced=" << stats.throttled
        << " full=" << stats.fullRendered
        << " partial=" << stats.partialRendered
        << " paceMs=" << stats.targetFrameIntervalMs
        << " pending=" << stats.pending
        << " fps=" << (stats.fpsX100 / 100) << "."
        << std::setw(2) << std::setfill('0') << (stats.fpsX100 % 100)
        << std::setfill(' ')
        << " copyMs=" << (stats.lastCopyUs / 1000) << "."
        << std::setw(3) << std::setfill('0') << (stats.lastCopyUs % 1000)
        << std::setfill(' ')
        << " avgCopyMs=" << (stats.avgCopyUs / 1000) << "."
        << std::setw(3) << std::setfill('0') << (stats.avgCopyUs % 1000)
        << std::setfill(' ')
        << " renderMs=" << (stats.lastRenderUs / 1000) << "."
        << std::setw(3) << std::setfill('0') << (stats.lastRenderUs % 1000)
        << std::setfill(' ')
        << " avgRenderMs=" << (stats.avgRenderUs / 1000) << "."
        << std::setw(3) << std::setfill('0') << (stats.avgRenderUs % 1000)
        << std::setfill(' ')
        << " last=" << stats.lastWidth << "x" << stats.lastHeight
        << " " << DescribeDirtyStats(stats.lastDirty);
    return out.str();
}

void OnXComponentSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceCreated(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface changed");
    RequestSurfaceRepaint("surface changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface destroyed");
}

void OnXComponentTouchEvent(OH_NativeXComponent*, void*)
{
    g_surface.OnTouchEvent();
}

bool RegisterNativeXComponent(napi_env env, napi_value exports)
{
    napi_value nativeXComponentValue = nullptr;
    napi_status status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &nativeXComponentValue);
    if (status != napi_ok || nativeXComponentValue == nullptr) {
        return false;
    }

    OH_NativeXComponent* component = nullptr;
    status = napi_unwrap(env, nativeXComponentValue, reinterpret_cast<void**>(&component));
    if (status != napi_ok || component == nullptr) {
        return false;
    }

    static OH_NativeXComponent_Callback callback = {
        OnXComponentSurfaceCreated,
        OnXComponentSurfaceChanged,
        OnXComponentSurfaceDestroyed,
        OnXComponentTouchEvent,
    };

    int32_t rc = OH_NativeXComponent_RegisterCallback(component, &callback);
    const bool ok = rc == OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
    g_surface.Register(component, ok);
    if (ok) {
        g_events.log.Emit("XComponent callback registered: " + ReadXComponentId(component));
    }
    return ok;
}

class RdpSession {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    enum class QueuedInputType {
        Pointer,
        Key,
        Unicode,
    };

    struct QueuedInputEvent {
        QueuedInputType type = QueuedInputType::Pointer;
        uint16_t flags = 0;
        uint16_t x = 0;
        uint16_t y = 0;
        uint32_t scancode = 0;
        uint32_t code = 0;
        bool down = false;
        bool repeat = false;
    };
#endif

public:
    ~RdpSession()
    {
        Disconnect();
    }

    bool Connect(const ConnectParams& params, std::string& message)
    {
        if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
            message = "host, port, username, and password are required";
            g_events.error.Emit(message);
            return false;
        }

        Disconnect();

        running_.store(true);
        connected_.store(false);
        ResetInputState();
        message = "native worker started";
        worker_ = std::thread([this, params]() {
            WorkerMain(params);
        });
        return true;
    }

    void Disconnect()
    {
        RequestDisconnect();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    bool RequestDisconnect()
    {
        running_.store(false);
        connected_.store(false);
        ClearInputQueue();
        ClearRdpDesktopSize();
        RequestNativeDisconnect();
        return worker_.joinable();
    }

    bool IsConnected() const
    {
        return connected_.load();
    }

    bool SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        QueuedInputEvent event;
        event.type = QueuedInputType::Pointer;
        event.flags = flags;
        event.x = x;
        event.y = y;
        return EnqueueInput(event, "pointer event queued", message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        QueuedInputEvent event;
        event.type = QueuedInputType::Key;
        event.scancode = rdpScancode;
        event.down = down;
        event.repeat = repeat;
        return EnqueueInput(event, down ? (repeat ? "key down queued repeat" : "key down queued") : "key up queued",
            message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendUnicode(uint32_t code, bool down, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (code == 0 || code > 0xFFFFU) {
            message = "unicode input requires a BMP UTF-16 code unit";
            return false;
        }
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        QueuedInputEvent event;
        event.type = QueuedInputType::Unicode;
        event.code = code;
        event.down = down;
        return EnqueueInput(event, down ? "unicode key down queued" : "unicode key up queued", message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    uint32_t InputQueueDepth() const
    {
        return inputQueueDepth_.load();
    }

    uint32_t InputQueuedCount() const
    {
        return inputQueuedCount_.load();
    }

    uint32_t InputSentCount() const
    {
        return inputSentCount_.load();
    }

    uint32_t InputDroppedCount() const
    {
        return inputDroppedCount_.load();
    }

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (g_avc420SurfaceOutputEnabled.load()) {
            message = "AVC420 surface output owns XComponent";
            return false;
        }
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeContext_ == nullptr || activeContext_->gdi == nullptr) {
            message = "FreeRDP GDI context is not ready";
            return false;
        }

        rdpGdi* gdi = activeContext_->gdi;
        if (gdi->suppressOutput || gdi->primary_buffer == nullptr || gdi->width <= 0 ||
            gdi->height <= 0 || gdi->stride == 0) {
            message = "FreeRDP GDI primary buffer is not ready";
            return false;
        }

        RgbaFrame frame = {
            gdi->primary_buffer,
            static_cast<uint32_t>(gdi->width),
            static_cast<uint32_t>(gdi->height),
            static_cast<int32_t>(gdi->stride),
            reason.empty() ? "surface repaint" : reason,
            DirtyFrameStats{},
        };

        std::string queueMessage;
        if (!QueueSurfaceRgbaFrame(frame, queueMessage, true)) {
            message = queueMessage;
            return false;
        }

        message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " current-gdi";
        return true;
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        constexpr uint32_t minDimension = 200;
        constexpr uint32_t maxDimension = 8192;
        width = std::clamp(width, minDimension, maxDimension);
        height = std::clamp(height, minDimension, maxDimension);
        width -= width % 2U;
        if (width < minDimension) {
            width = minDimension;
        }

        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeDisp_ == nullptr || activeDisp_->SendMonitorLayout == nullptr) {
            message = "display-control channel is not ready";
            return false;
        }
        if (!displayControlCapsReady_) {
            message = "display-control caps are not ready";
            return false;
        }
        if (lastDynamicResizeWidth_ == width && lastDynamicResizeHeight_ == height) {
            message = "display-control resize unchanged: " + std::to_string(width) + "x" +
                std::to_string(height);
            return true;
        }

        DISPLAY_CONTROL_MONITOR_LAYOUT layout = {};
        layout.Flags = DISPLAY_CONTROL_MONITOR_PRIMARY;
        layout.Left = 0;
        layout.Top = 0;
        layout.Width = width;
        layout.Height = height;
        layout.PhysicalWidth = width;
        layout.PhysicalHeight = height;
        layout.Orientation = ORIENTATION_LANDSCAPE;
        layout.DesktopScaleFactor = 100;
        layout.DeviceScaleFactor = 100;

        const UINT rc = activeDisp_->SendMonitorLayout(activeDisp_, 1, &layout);
        if (rc != CHANNEL_RC_OK) {
            message = "display-control resize failed: " + std::to_string(rc);
            return false;
        }

        lastDynamicResizeWidth_ = width;
        lastDynamicResizeHeight_ = height;
        message = "display-control resize requested after " + reason + ": " +
            std::to_string(width) + "x" + std::to_string(height);
        return true;
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

private:
    void EmitState(const std::string& state)
    {
        g_events.state.Emit(state);
    }

    void EmitLog(const std::string& line)
    {
        g_events.log.Emit(line);
    }

    bool SleepInterruptibly(int milliseconds)
    {
        constexpr int stepMs = 25;
        int elapsed = 0;
        while (elapsed < milliseconds) {
            if (!running_.load()) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
            elapsed += stepMs;
        }
        return running_.load();
    }

    void ClearInputQueue()
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        std::lock_guard<std::mutex> lock(inputMutex_);
        inputQueue_.clear();
        inputQueueDepth_.store(0);
#else
        inputQueueDepth_.store(0);
#endif
    }

    void ResetInputState()
    {
        ClearInputQueue();
        inputQueuedCount_.store(0);
        inputSentCount_.store(0);
        inputDroppedCount_.store(0);
        inputDispatchLogCount_.store(0);
        inputFailureLogCount_.store(0);
        inputBackpressureLogCount_.store(0);
    }

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    const char* InputTypeName(const QueuedInputEvent& event) const
    {
        if (event.type == QueuedInputType::Pointer) {
            return "pointer";
        }
        if (event.type == QueuedInputType::Key) {
            return "key";
        }
        return "unicode";
    }

    bool IsPointerWheelEvent(const QueuedInputEvent& event) const
    {
        return event.type == QueuedInputType::Pointer &&
            (event.flags & (PTR_FLAGS_WHEEL | PTR_FLAGS_HWHEEL)) != 0;
    }

    bool IsPointerMotionEvent(const QueuedInputEvent& event) const
    {
        return event.type == QueuedInputType::Pointer &&
            (event.flags & PTR_FLAGS_MOVE) != 0 &&
            !IsPointerWheelEvent(event);
    }

    bool HasSamePointerMotionClass(const QueuedInputEvent& lhs, const QueuedInputEvent& rhs) const
    {
        constexpr uint16_t pointerStateMask = PTR_FLAGS_BUTTON1 | PTR_FLAGS_BUTTON2 | PTR_FLAGS_BUTTON3 | PTR_FLAGS_DOWN;
        return (lhs.flags & pointerStateMask) == (rhs.flags & pointerStateMask);
    }

    bool IsDroppablePointerEvent(const QueuedInputEvent& event) const
    {
        return IsPointerMotionEvent(event) || IsPointerWheelEvent(event);
    }

    bool DropOldestDroppablePointerEventLocked()
    {
        for (auto iter = inputQueue_.begin(); iter != inputQueue_.end(); ++iter) {
            if (IsDroppablePointerEvent(*iter)) {
                inputQueue_.erase(iter);
                inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
                return true;
            }
        }
        return false;
    }

    bool EnqueueInput(const QueuedInputEvent& event, const char* okMessage, std::string& message)
    {
        constexpr size_t maxInputQueue = 4096;
        bool droppedOldPointer = false;
        bool droppedNewEvent = false;

        {
            std::lock_guard<std::mutex> lock(inputMutex_);

            if (IsPointerMotionEvent(event) && !inputQueue_.empty() &&
                IsPointerMotionEvent(inputQueue_.back()) &&
                HasSamePointerMotionClass(event, inputQueue_.back())) {
                inputQueue_.back() = event;
                inputQueuedCount_.fetch_add(1);
                message = okMessage;
                return true;
            }

            if (inputQueue_.size() >= maxInputQueue) {
                const bool mustProtectNewEvent = event.type != QueuedInputType::Pointer || !IsDroppablePointerEvent(event);
                if (mustProtectNewEvent && DropOldestDroppablePointerEventLocked()) {
                    inputDroppedCount_.fetch_add(1);
                    droppedOldPointer = true;
                }
            }

            if (inputQueue_.size() >= maxInputQueue) {
                inputDroppedCount_.fetch_add(1);
                message = std::string("FreeRDP input queue is full; dropped ") + InputTypeName(event) + " event";
                droppedNewEvent = true;
            } else {
                inputQueue_.push_back(event);
                inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
                inputQueuedCount_.fetch_add(1);
                message = okMessage;
            }
        }

        if (droppedOldPointer) {
            LogInputBackpressure(std::string("FreeRDP input queue protected ") + InputTypeName(event) +
                " event by dropping pending pointer motion");
        }
        if (droppedNewEvent) {
            LogInputFailure(message);
            return false;
        }
        return true;
    }

    void DrainInputQueue(FreerdpRuntimeApi* api, rdpContext* context)
    {
        std::deque<QueuedInputEvent> pending;
        {
            std::lock_guard<std::mutex> lock(inputMutex_);
            if (inputQueue_.empty()) {
                inputQueueDepth_.store(0);
                return;
            }
            pending.swap(inputQueue_);
            inputQueueDepth_.store(0);
        }

        if (api == nullptr || context == nullptr || context->input == nullptr) {
            inputDroppedCount_.fetch_add(static_cast<uint32_t>(pending.size()));
            LogInputFailure("FreeRDP input context is not ready; queued input dropped");
            return;
        }

        uint32_t sent = 0;
        for (const QueuedInputEvent& event : pending) {
            BOOL ok = FALSE;
            if (event.type == QueuedInputType::Pointer) {
                if (api->inputSendMouseEvent != nullptr) {
                    ok = api->inputSendMouseEvent(context->input, event.flags, event.x, event.y);
                }
            } else if (event.type == QueuedInputType::Key) {
                if (api->inputSendKeyboardEventEx != nullptr) {
                    ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE,
                        event.repeat ? TRUE : FALSE,
                        event.scancode);
                }
            } else {
                if (api->inputSendUnicodeKeyboardEvent != nullptr) {
                    const UINT16 flags = event.down ? 0 : KBD_FLAGS_RELEASE;
                    ok = api->inputSendUnicodeKeyboardEvent(context->input, flags, static_cast<UINT16>(event.code));
                }
            }

            if (ok) {
                ++sent;
            } else {
                inputDroppedCount_.fetch_add(1);
                LogInputFailure("FreeRDP input dispatch failed on worker thread");
            }
        }

        if (sent == 0) {
            return;
        }

        const uint32_t totalSent = inputSentCount_.fetch_add(sent) + sent;
        const uint32_t logIndex = inputDispatchLogCount_.fetch_add(1);
        if (logIndex < 5 || totalSent % 200 == 0) {
            EmitLog("FreeRDP input dispatched on worker thread: " + std::to_string(sent) +
                " event(s), total=" + std::to_string(totalSent));
        }
    }

    void LogInputFailure(const std::string& message)
    {
        const uint32_t logIndex = inputFailureLogCount_.fetch_add(1);
        if (logIndex < 5 || logIndex % 100 == 0) {
            EmitLog(message);
        }
    }

    void LogInputBackpressure(const std::string& message)
    {
        const uint32_t logIndex = inputBackpressureLogCount_.fetch_add(1);
        if (logIndex < 5 || logIndex % 100 == 0) {
            EmitLog(message);
        }
    }

    void SetActiveNative(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context)
    {
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            activeApi_ = api;
            activeInstance_ = instance;
            activeContext_ = context;
            activeDisp_ = nullptr;
            activeGfx_ = nullptr;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }

        RegisterSession(context, this);
        if (api != nullptr && api->pubSubSubscribe != nullptr && context != nullptr &&
            context->pubSub != nullptr) {
            (void)api->pubSubSubscribe(context->pubSub, "ChannelConnected", OnChannelConnected);
            (void)api->pubSubSubscribe(context->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        }
    }

    void ClearActiveNative(freerdp* instance)
    {
        rdpContext* oldContext = nullptr;
        FreerdpRuntimeApi* oldApi = nullptr;
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeInstance_ != instance) {
            return;
        }

        oldContext = activeContext_;
        oldApi = activeApi_;
        DetachGraphicsPipelineLocked(activeGfx_);
        if (oldApi != nullptr && oldApi->pubSubUnsubscribe != nullptr && oldContext != nullptr &&
            oldContext->pubSub != nullptr) {
            (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelConnected", OnChannelConnected);
            (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        }
        UnregisterSession(oldContext);
        activeApi_ = nullptr;
        activeInstance_ = nullptr;
        activeContext_ = nullptr;
        activeDisp_ = nullptr;
        activeGfx_ = nullptr;
        displayControlCapsReady_ = false;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
    }

    void RequestNativeDisconnect()
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeApi_ != nullptr && activeContext_ != nullptr) {
            activeApi_->abortConnectContext(activeContext_);
        }
    }

    static std::mutex& RegistryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<rdpContext*, RdpSession*>& Registry()
    {
        static std::unordered_map<rdpContext*, RdpSession*> registry;
        return registry;
    }

    static void RegisterSession(rdpContext* context, RdpSession* session)
    {
        if (context == nullptr || session == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        Registry()[context] = session;
    }

    static void UnregisterSession(rdpContext* context)
    {
        if (context == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        Registry().erase(context);
    }

    static RdpSession* FindSession(rdpContext* context)
    {
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(context);
        return iter == Registry().end() ? nullptr : iter->second;
    }

    static bool IsDisplayControlChannel(const char* name)
    {
        return name != nullptr &&
            (std::strcmp(name, DISP_CHANNEL_NAME) == 0 ||
                std::strcmp(name, DISP_DVC_CHANNEL_NAME) == 0);
    }

    static bool IsGraphicsPipelineChannel(const char* name)
    {
        return name != nullptr && std::strcmp(name, RDPGFX_DVC_CHANNEL_NAME) == 0;
    }

    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event)
    {
        if (context == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }

        RdpSession* session = FindSession(static_cast<rdpContext*>(context));
        if (session == nullptr) {
            return;
        }
        if (IsDisplayControlChannel(event->name)) {
            session->AttachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
        } else if (IsGraphicsPipelineChannel(event->name)) {
            session->AttachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
        }
    }

    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
    {
        if (context == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }

        RdpSession* session = FindSession(static_cast<rdpContext*>(context));
        if (session == nullptr) {
            return;
        }
        if (IsDisplayControlChannel(event->name)) {
            session->DetachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
        } else if (IsGraphicsPipelineChannel(event->name)) {
            session->DetachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
        }
    }

    static UINT DisplayControlCaps(DispClientContext* disp, UINT32 maxNumMonitors,
        UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB)
    {
        auto* session = disp == nullptr ? nullptr : static_cast<RdpSession*>(disp->custom);
        if (session != nullptr) {
            session->HandleDisplayControlCaps(maxNumMonitors, maxMonitorAreaFactorA,
                maxMonitorAreaFactorB);
        }
        return CHANNEL_RC_OK;
    }

    void HandleDisplayControlCaps(UINT32 maxNumMonitors, UINT32 maxMonitorAreaFactorA,
        UINT32 maxMonitorAreaFactorB)
    {
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            displayControlCapsReady_ = true;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }

        EmitLog("display-control caps: maxMonitors=" + std::to_string(maxNumMonitors) +
            " areaFactor=" + std::to_string(maxMonitorAreaFactorA) + "/" +
            std::to_string(maxMonitorAreaFactorB));

        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        if (snapshot.width > 0 && snapshot.height > 0) {
            std::string resizeMessage;
            if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                "display-control caps", resizeMessage)) {
                EmitLog(resizeMessage);
            } else {
                EmitLog("display-control resize skipped after display-control caps: " +
                    resizeMessage);
            }
        }
    }

    void AttachDisplayControl(DispClientContext* disp)
    {
        if (disp == nullptr) {
            EmitLog("display-control connected without client context");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            activeDisp_ = disp;
            activeDisp_->custom = this;
            activeDisp_->DisplayControlCaps = DisplayControlCaps;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }
        EmitLog("display-control connected to HarmonyOS window resize bridge");

        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        if (snapshot.width > 0 && snapshot.height > 0) {
            std::string resizeMessage;
            if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                "display-control connected", resizeMessage)) {
                EmitLog(resizeMessage);
            } else {
                EmitLog("display-control resize skipped after display-control connected: " +
                    resizeMessage);
            }
        }
    }

    void DetachDisplayControl(DispClientContext* disp)
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeDisp_ != nullptr && activeDisp_ == disp) {
            activeDisp_->custom = nullptr;
            activeDisp_ = nullptr;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
            EmitLog("display-control disconnected from HarmonyOS window resize bridge");
        }
    }

    void AttachGraphicsPipeline(RdpgfxClientContext* gfx)
    {
        std::string message;
        bool attached = false;
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            if (gfx == nullptr) {
                message = "rdpgfx connected without client context";
            } else if (activeApi_ == nullptr || activeContext_ == nullptr || activeContext_->gdi == nullptr) {
                g_rdpgfxInitFailedCount.fetch_add(1);
                message = "rdpgfx connected before GDI context was ready";
            } else if (activeApi_->gdiGraphicsPipelineInit == nullptr) {
                g_rdpgfxInitFailedCount.fetch_add(1);
                message = "rdpgfx GDI pipeline init symbol unavailable";
            } else {
                if (activeGfx_ != nullptr && activeGfx_ != gfx) {
                    DetachGraphicsPipelineLocked(activeGfx_);
                }
                if (activeApi_->gdiGraphicsPipelineInit(activeContext_->gdi, gfx)) {
                    InstallRdpgfxDiagnosticsHooks(gfx);
                    activeGfx_ = gfx;
                    g_rdpgfxBridgeAttached.store(true);
                    g_rdpgfxConnectedCount.fetch_add(1);
                    attached = true;
                    message = "rdpgfx connected to FreeRDP GDI graphics pipeline";
                } else {
                    g_rdpgfxInitFailedCount.fetch_add(1);
                    message = "rdpgfx GDI graphics pipeline init failed";
                }
            }
        }

        EmitLog(message);
        if (attached) {
            RequestSurfaceRepaint("rdpgfx connected");
        }
    }

    void DetachGraphicsPipeline(RdpgfxClientContext* gfx)
    {
        bool detached = false;
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            detached = DetachGraphicsPipelineLocked(gfx);
        }
        if (detached) {
            EmitLog("rdpgfx disconnected from FreeRDP GDI graphics pipeline");
        }
    }

    bool DetachGraphicsPipelineLocked(RdpgfxClientContext* gfx)
    {
        if (activeGfx_ == nullptr || activeGfx_ != gfx) {
            return false;
        }
        RestoreRdpgfxDiagnosticsHooks(activeGfx_);
        if (activeApi_ != nullptr && activeApi_->gdiGraphicsPipelineUninit != nullptr &&
            activeContext_ != nullptr && activeContext_->gdi != nullptr) {
            activeApi_->gdiGraphicsPipelineUninit(activeContext_->gdi, activeGfx_);
        }
        activeGfx_ = nullptr;
        g_rdpgfxBridgeAttached.store(false);
        g_rdpgfxDisconnectedCount.fetch_add(1);
        return true;
    }
#else
    void RequestNativeDisconnect() {}
#endif

    void WorkerMain(ConnectParams params)
    {
        EmitLog("native worker accepted params");
        EmitLog("target=" + params.host + ":" + params.port);
        EmitLog("graphicsMode=" + ParseGraphicsPipelineConfig(params).mode);

        if (!running_.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Resolving");
        EmitLog("state=Resolving");
        EmitLog("resolving target host");

        TcpConnectResult tcp = TestTcpConnect(params.host, params.port, 3000);
        if (!running_.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }
        if (!tcp.ok) {
            std::string message = "tcp check failed: " + tcp.message;
            EmitState("Failed");
            EmitLog(message);
            g_events.error.Emit(message);
            running_.store(false);
            return;
        }

        EmitState("TCP connected");
        EmitLog("state=TCP connected");
        EmitLog(tcp.message);
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Negotiating");
        EmitLog("state=Negotiating");
        EmitLog("starting FreeRDP persistent connect");
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Authenticating");
        EmitLog("state=Authenticating");
        if (IsAutoInitialResolution(params.resolution)) {
            SurfaceSnapshot snapshot = g_surface.Snapshot();
            if (snapshot.width >= 320 && snapshot.height >= 240) {
                params.resolution = std::to_string(snapshot.width) + "x" +
                    std::to_string(snapshot.height);
                EmitLog("FreeRDP initial resolution auto from surface: " + params.resolution);
            } else {
                EmitLog("FreeRDP initial resolution auto fallback: surface is not ready");
            }
        }
        RdpSessionRunResult session;
        const std::vector<std::string> graphicsModes = BuildGraphicsFallbackModes(params);
        EmitLog("graphics fallback ladder: " + JoinGraphicsModes(graphicsModes));
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        for (size_t attempt = 0; attempt < graphicsModes.size(); ++attempt) {
            ConnectParams attemptParams = params;
            attemptParams.graphicsMode = graphicsModes[attempt];
            bool attemptConnected = false;
            EmitLog("graphics attempt " + std::to_string(attempt + 1) + "/" +
                std::to_string(graphicsModes.size()) + ": mode=" + attemptParams.graphicsMode);
            session = RunFreerdpSession(attemptParams, running_,
                [this](FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context) {
                    SetActiveNative(api, instance, context);
                },
                [this](freerdp* instance) {
                    ClearActiveNative(instance);
                },
                [this](const std::string& line) {
                    EmitLog(line);
                },
                [this, &attemptConnected, selectedMode = attemptParams.graphicsMode]() {
                    attemptConnected = true;
                    connected_.store(true);
                    EmitState("Connected");
                    EmitLog("state=Connected");
                    EmitLog("graphics mode selected: " + selectedMode);
                    EmitLog("FreeRDP persistent session loop is active");
                    EmitLog("FreeRDP input bridge is using worker-thread dispatch");
                    const SurfaceSnapshot snapshot = g_surface.Snapshot();
                    if (snapshot.width > 0 && snapshot.height > 0) {
                        std::string resizeMessage;
                        if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                            "session connected", resizeMessage)) {
                            EmitLog(resizeMessage);
                        } else {
                            EmitLog("display-control resize skipped after session connected: " +
                                resizeMessage);
                        }
                    }
                },
                [this](FreerdpRuntimeApi* api, rdpContext* context) {
                    DrainInputQueue(api, context);
                });
            ClearInputQueue();

            if (session.cancelled || !running_.load()) {
                break;
            }

            if (ShouldRetryGraphicsFallback(session, attemptConnected, attemptParams.graphicsMode,
                attempt, graphicsModes.size())) {
                connected_.store(false);
                EmitLog("graphics mode " + attemptParams.graphicsMode +
                    " failed before connection: " + session.message);
                EmitLog("graphics fallback retry: " + attemptParams.graphicsMode + " -> " +
                    graphicsModes[attempt + 1]);
                EmitState("Negotiating");
                EmitLog("state=Negotiating");
                continue;
            }

            if (session.failed && !attemptConnected && attempt + 1 < graphicsModes.size()) {
                EmitLog("graphics fallback skipped for non-graphics failure: " +
                    session.message);
            }
            break;
        }
#else
        (void)graphicsModes;
        session = RunFreerdpSessionUnavailable();
#endif
        ClearInputQueue();

        if (session.cancelled || !running_.load()) {
            connected_.store(false);
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        if (session.failed) {
            std::string message = session.available ? session.message : "FreeRDP runtime unavailable: " + session.message;
            connected_.store(false);
            EmitState("Failed");
            EmitLog(message);
            g_events.error.Emit(message);
            running_.store(false);
            return;
        }

        connected_.store(false);
        EmitState("Disconnected");
        EmitLog(session.message);
        running_.store(false);
    }

    std::atomic_bool running_ = false;
    std::atomic_bool connected_ = false;
    std::thread worker_;
    std::atomic_uint32_t inputQueueDepth_{0};
    std::atomic_uint32_t inputQueuedCount_{0};
    std::atomic_uint32_t inputSentCount_{0};
    std::atomic_uint32_t inputDroppedCount_{0};
    std::atomic_uint32_t inputDispatchLogCount_{0};
    std::atomic_uint32_t inputFailureLogCount_{0};
    std::atomic_uint32_t inputBackpressureLogCount_{0};
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    static bool IsAutoInitialResolution(const std::string& resolution)
    {
        const std::string normalized = ToLowerAscii(TrimAscii(resolution));
        return normalized.empty() || normalized == "auto" || normalized == "window";
    }

    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
    DispClientContext* activeDisp_ = nullptr;
    RdpgfxClientContext* activeGfx_ = nullptr;
    bool displayControlCapsReady_ = false;
    uint32_t lastDynamicResizeWidth_ = 0;
    uint32_t lastDynamicResizeHeight_ = 0;
    std::mutex inputMutex_;
    std::deque<QueuedInputEvent> inputQueue_;
#endif
};

RdpSession g_session;

void RequestSurfaceRepaint(const std::string& reason)
{
    static std::atomic_uint32_t repaintLogCount{0};
    static std::atomic_uint32_t repaintSkipLogCount{0};
    std::string message;
    if (g_session.RequestCurrentFrameRender(reason, message)) {
        const uint32_t count = ++repaintLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog("Surface repaint queued after " + reason + ": " + message +
                " count=" + std::to_string(count));
        }
        return;
    }

    const uint32_t skipCount = ++repaintSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("Surface repaint skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

void RequestRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& reason)
{
    static std::atomic_uint32_t resizeLogCount{0};
    static std::atomic_uint32_t resizeSkipLogCount{0};
    std::string message;
    if (g_session.RequestDynamicDesktopResize(width, height, reason, message)) {
        const uint32_t count = ++resizeLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog(message + " count=" + std::to_string(count));
        }
        return;
    }

    const uint32_t skipCount = ++resizeSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("display-control resize skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

ConnectParams ReadConnectParams(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    ConnectParams params;
    if (argc < 1 || args[0] == nullptr) {
        return params;
    }

    napi_valuetype type = napi_undefined;
    napi_typeof(env, args[0], &type);
    if (type != napi_object) {
        return params;
    }

    params.host = GetStringProperty(env, args[0], "host");
    params.port = GetStringProperty(env, args[0], "port");
    params.username = GetStringProperty(env, args[0], "username");
    params.password = GetStringProperty(env, args[0], "password");
    params.resolution = GetStringProperty(env, args[0], "resolution");
    params.certPolicy = GetStringProperty(env, args[0], "certPolicy");
    params.graphicsMode = GetStringProperty(env, args[0], "graphicsMode");
    params.appFilesDir = GetStringProperty(env, args[0], "appFilesDir");
    return params;
}

std::string BuildGraphicsPipelineStatsLog()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return "rdpgfx stats unavailable: " + error;
    }

    std::ostringstream out;
    out << "rdpgfx stats: compiled=yes"
        << " runtime=" << (g_rdpgfxRuntimeRequested.load() ? "requested" : "off")
        << " h264=" << (g_rdpgfxH264Requested.load() ? "requested" : "off")
        << " bridge=" << (g_rdpgfxBridgeAttached.load() ? "attached" : "detached")
        << " connected=" << g_rdpgfxConnectedCount.load()
        << " disconnected=" << g_rdpgfxDisconnectedCount.load()
        << " initFailed=" << g_rdpgfxInitFailedCount.load()
        << " frames=" << g_rdpgfxStartFrameCount.load() << "/" << g_rdpgfxEndFrameCount.load()
        << " surfaceCommands=" << g_rdpgfxSurfaceCommandCount.load()
        << " codecs=raw:" << g_rdpgfxCodecUncompressedCount.load()
        << ",progressive:" << g_rdpgfxCodecProgressiveCount.load()
        << ",cavideo:" << g_rdpgfxCodecCavideoCount.load()
        << ",clear:" << g_rdpgfxCodecClearCodecCount.load()
        << ",planar:" << g_rdpgfxCodecPlanarCount.load()
        << ",avc420:" << g_rdpgfxCodecAvc420Count.load()
        << ",avc444:" << g_rdpgfxCodecAvc444Count.load()
        << ",avc444v2:" << g_rdpgfxCodecAvc444v2Count.load()
        << ",alpha:" << g_rdpgfxCodecAlphaCount.load()
        << ",unknown:" << g_rdpgfxCodecUnknownCount.load()
        << " lastCodec=" << RdpgfxCodecName(g_rdpgfxLastCodecId.load())
        << "(" << g_rdpgfxLastCodecId.load() << ")"
        << " lastSurface=" << g_rdpgfxLastSurfaceId.load()
        << " lastSize=" << g_rdpgfxLastCommandWidth.load() << "x" << g_rdpgfxLastCommandHeight.load()
        << " symbols=gdiInit:" << (api.gdiGraphicsPipelineInit != nullptr ? "yes" : "no")
        << ",gdiUninit:" << (api.gdiGraphicsPipelineUninit != nullptr ? "yes" : "no")
        << ",ctxNew:" << (api.rdpgfxClientContextNew != nullptr ? "yes" : "no")
        << ",ctxFree:" << (api.rdpgfxClientContextFree != nullptr ? "yes" : "no");
    return out.str();
#else
    return "rdpgfx stats unavailable: FreeRDP headers not found at build time";
#endif
}

std::string BuildOHAudioStatsLog()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return "OHAudio stats unavailable: " + error;
    }
    std::string rdpsndClientDiagnostics;
    if (api.rdpsndClientGetDiagnostics != nullptr) {
        const char* diagnostics = api.rdpsndClientGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            rdpsndClientDiagnostics = " | ";
            rdpsndClientDiagnostics += diagnostics;
        }
    }
    if (api.rdpsndOhosGetDiagnostics != nullptr) {
        const char* diagnostics = api.rdpsndOhosGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            return std::string(diagnostics) + rdpsndClientDiagnostics;
        }
    }
    if (api.rdpsndOhosGetStats == nullptr) {
        return "OHAudio stats unavailable: backend symbol not exported" + rdpsndClientDiagnostics;
    }

    UINT64 registeredCount = 0;
    UINT64 openCount = 0;
    UINT64 closeCount = 0;
    UINT64 playCount = 0;
    UINT64 playBytes = 0;
    UINT64 callbackCount = 0;
    UINT64 renderedBytes = 0;
    UINT64 underrunBytes = 0;
    UINT32 lastRate = 0;
    UINT16 lastChannels = 0;
    UINT16 lastBits = 0;
    UINT32 lastLatencyMs = 0;
    if (!api.rdpsndOhosGetStats(&registeredCount, &openCount, &closeCount, &playCount,
        &playBytes, &callbackCount, &renderedBytes, &underrunBytes, &lastRate, &lastChannels,
        &lastBits, &lastLatencyMs)) {
        return "OHAudio stats unavailable: backend query failed";
    }

    std::ostringstream out;
    out << "OHAudio stats: registered=" << registeredCount
        << " open=" << openCount
        << " close=" << closeCount
        << " playCalls=" << playCount
        << " playBytes=" << playBytes
        << " callbacks=" << callbackCount
        << " renderedBytes=" << renderedBytes
        << " underrunBytes=" << underrunBytes
        << " lastFormat=" << lastRate << "Hz/" << lastChannels << "ch/" << lastBits
        << "bit latency=" << lastLatencyMs << "ms";
    return out.str() + rdpsndClientDiagnostics;
#else
    return "OHAudio stats unavailable: FreeRDP headers not found at build time";
#endif
}

napi_value Probe(napi_env env, napi_callback_info info)
{
    FreerdpProbeResult freerdp = LoadFreerdpProbe();
    SurfaceSnapshot surface = g_surface.Snapshot();
    const std::string featureSummary =
        "core RDP/TLS/NLA + queued software GDI renderer; client channels on; "
        "cliprdr/rdpdr/drive/printer/smartcard/rdpsnd/audin/rdpgfx/disp compiled; "
        "H264 + FFmpeg + OpenH264 enabled; RD Gateway core enabled; "
        "static cliprdr text bridge, disp dynamic resolution, and rdpsnd/OHAudio playback requested; "
        "rdpgfx runtime gated by graphicsMode; other optional channel negotiation off";

    const std::string audioStats = BuildOHAudioStatsLog();
    const std::string renderStats = BuildRenderStatsLog();
    const std::string graphicsStats = BuildGraphicsPipelineStatsLog();

    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.8.3");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", freerdp.freerdpVersion);
    SetString(env, result, "winprVersion", freerdp.winprVersion);
    SetString(env, result, "opensslVersion", freerdp.opensslVersion);
    SetString(env, result, "featureSummary", featureSummary);
    SetString(env, result, "audioStats", audioStats);
    SetString(env, result, "renderStats", renderStats);
    SetString(env, result, "graphicsStats", graphicsStats);
    SetString(env, result, "inputDispatchMode", "worker-thread-queue");
    SetString(env, result, "probeJson", freerdp.json);
    SetString(env, result, "probeError", freerdp.error);
    SetBool(env, result, "freeRdpLinked", freerdp.linked);
    SetBool(env, result, "surfaceRegistered", surface.registered);
    SetBool(env, result, "surfaceReady", surface.ready);
    SetString(env, result, "surfaceId", surface.id);
    SetUint32(env, result, "surfaceWidth", surface.width);
    SetUint32(env, result, "surfaceHeight", surface.height);
    SetUint32(env, result, "surfaceViewportX", surface.viewportX);
    SetUint32(env, result, "surfaceViewportY", surface.viewportY);
    SetUint32(env, result, "surfaceViewportWidth", surface.viewportWidth);
    SetUint32(env, result, "surfaceViewportHeight", surface.viewportHeight);
    SetUint32(env, result, "surfaceCreatedCount", surface.createdCount);
    SetUint32(env, result, "surfaceChangedCount", surface.changedCount);
    SetUint32(env, result, "surfaceDestroyedCount", surface.destroyedCount);
    SetUint32(env, result, "surfaceTouchCount", surface.touchCount);
    SetUint32(env, result, "surfacePaintCount", surface.paintCount);
    SetString(env, result, "surfaceLastPaintMessage", surface.lastPaintMessage);
    SetBool(env, result, "sessionConnected", g_session.IsConnected());
    SetUint32(env, result, "desktopWidth", g_rdpDesktopWidth.load());
    SetUint32(env, result, "desktopHeight", g_rdpDesktopHeight.load());
    SetUint32(env, result, "inputQueueDepth", g_session.InputQueueDepth());
    SetUint32(env, result, "inputQueuedCount", g_session.InputQueuedCount());
    SetUint32(env, result, "inputSentCount", g_session.InputSentCount());
    SetUint32(env, result, "inputDroppedCount", g_session.InputDroppedCount());

    std::vector<std::string> logs = {
        "N-API bridge loaded",
        "Native calls are available: probe, connect, disconnect, sendPointer, sendKey, sendUnicode",
        "FreeRDP input dispatch: worker-thread queue",
        "FreeRDP channel dispatch: libfreerdp-client static addin provider",
        "FreeRDP build features: " + featureSummary,
        renderStats,
        graphicsStats,
        audioStats,
        "Certificate policy: tofu stores first untrusted certificate through FreeRDP, strict rejects untrusted certificates"
    };
    if (freerdp.linked) {
        logs.push_back("FreeRDP probe library loaded");
        logs.push_back("FreeRDP " + freerdp.freerdpVersion);
        logs.push_back("WinPR " + freerdp.winprVersion);
        logs.push_back(freerdp.opensslVersion);
    } else {
        logs.push_back("FreeRDP probe library not loaded: " + freerdp.error);
    }
    if (surface.ready) {
        logs.push_back("XComponent surface ready: " + surface.id + " " +
            std::to_string(surface.width) + "x" + std::to_string(surface.height));
        if (surface.viewportWidth > 0 && surface.viewportHeight > 0) {
            logs.push_back("XComponent render viewport: " + std::to_string(surface.viewportX) + "," +
                std::to_string(surface.viewportY) + " " + std::to_string(surface.viewportWidth) + "x" +
                std::to_string(surface.viewportHeight));
        }
        if (!surface.lastPaintMessage.empty()) {
            logs.push_back(surface.lastPaintMessage);
        }
    } else if (surface.registered) {
        logs.push_back("XComponent callback registered; surface not created");
    } else {
        logs.push_back("XComponent callback not registered");
    }
    const uint32_t desktopWidth = g_rdpDesktopWidth.load();
    const uint32_t desktopHeight = g_rdpDesktopHeight.load();
    if (desktopWidth > 0 && desktopHeight > 0) {
        logs.push_back("FreeRDP desktop size ready: " + std::to_string(desktopWidth) + "x" +
            std::to_string(desktopHeight));
    }
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Connect(napi_env env, napi_callback_info info)
{
    ConnectParams params = ReadConnectParams(env, info);
    std::vector<std::string> logs = {"native connect invoked"};

    napi_value result = MakeObject(env);
    if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "host, port, username, and password are required");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    logs.push_back("target=" + params.host + ":" + params.port);
    logs.push_back("username=" + params.username);
    logs.push_back("resolution=" + params.resolution);
    logs.push_back("certPolicy=" + params.certPolicy);
    logs.push_back("appFilesDir=" + params.appFilesDir);
    logs.push_back("starting native worker");

    std::string message;
    bool started = g_session.Connect(params, message);
    if (!started) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", message);
        logs.push_back("native worker start failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Resolving");
    SetString(env, result, "message", message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Disconnect(napi_env env, napi_callback_info info)
{
    const bool closing = g_session.RequestDisconnect();
    g_events.state.Emit("Disconnected");
    g_events.log.Emit("native disconnect requested");

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Disconnected");
    SetString(env, result, "message", closing ? "native bridge session closing" : "native bridge session already closed");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "native disconnect requested",
        closing ? "native worker stopping asynchronously" : "native worker was not running"
    }));
    return result;
}

napi_value SendPointer(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native pointer input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "pointer input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t flags = GetUint32Property(env, arg, "flags");
    const uint32_t x = GetUint32Property(env, arg, "x");
    const uint32_t y = GetUint32Property(env, arg, "y");
    logs.push_back("flags=" + std::to_string(flags) + " x=" + std::to_string(x) + " y=" + std::to_string(y));

    std::string message;
    const bool ok = g_session.SendPointer(static_cast<uint16_t>(flags & 0xFFFFU),
        static_cast<uint16_t>(std::min(x, 0xFFFFU)), static_cast<uint16_t>(std::min(y, 0xFFFFU)), message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value SendKey(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native keyboard input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "keyboard input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t scancode = GetUint32Property(env, arg, "scancode");
    const bool down = GetBoolProperty(env, arg, "down");
    const bool repeat = GetBoolProperty(env, arg, "repeat");
    logs.push_back("scancode=" + std::to_string(scancode) + (down ? " down" : " up") +
        (repeat ? " repeat" : ""));

    std::string message;
    const bool ok = g_session.SendKey(scancode, down, repeat, message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value SendUnicode(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native unicode keyboard input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "unicode keyboard input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t code = GetUint32Property(env, arg, "code");
    const bool down = GetBoolProperty(env, arg, "down");
    logs.push_back("code=" + std::to_string(code) + (down ? " down" : " up"));

    std::string message;
    const bool ok = g_session.SendUnicode(code, down, message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value NotifySurfaceLayout(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native surface layout notify invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "surface layout notify requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t width = GetUint32Property(env, arg, "width");
    const uint32_t height = GetUint32Property(env, arg, "height");
    logs.push_back("width=" + std::to_string(width) + " height=" + std::to_string(height));

    std::string message;
    const bool changed = g_surface.OnSurfaceLayout(width, height, message);
    if (changed) {
        EmitNativeLog(message);
        UpdateAvc420SurfaceOutputIfActive("surface layout changed");
        RequestRemoteDesktopResize(width, height, "surface layout changed");
        RequestSurfaceRepaint("surface layout changed");
    }

    SetBool(env, result, "ok", width > 0 && height > 0);
    SetString(env, result, "state", changed ? "Updated" : "Unchanged");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value RegisterCallback(napi_env env, napi_callback_info info, EventSink& sink, const char* name,
    bool mirrorToHilog = false)
{
    napi_value callback = GetFirstArgument(env, info);
    bool ok = callback != nullptr && sink.Set(env, callback, name, mirrorToHilog);

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Idle" : "Failed");
    SetString(env, result, "message", ok ? "callback registered" : "callback must be a function");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        ok ? std::string(name) + " registered" : std::string(name) + " registration failed"
    }));
    return result;
}

napi_value OnState(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.state, "rdpStateCallback");
}

napi_value OnLog(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.log, "rdpLogCallback", true);
}

napi_value OnError(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.error, "rdpErrorCallback", true);
}

} // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"probe", nullptr, Probe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendPointer", nullptr, SendPointer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendKey", nullptr, SendKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendUnicode", nullptr, SendUnicode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"notifySurfaceLayout", nullptr, NotifySurfaceLayout, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onLog", nullptr, OnLog, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    RegisterNativeXComponent(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module rdpNativeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&rdpNativeModule);
}
