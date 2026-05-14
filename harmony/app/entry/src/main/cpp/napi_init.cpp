#include "napi/native_api.h"

#include <algorithm>
#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/codec/color.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <freerdp/update.h>
#include <winpr/synch.h>
#endif

namespace {

struct ConnectParams {
    std::string host;
    std::string port;
    std::string username;
    std::string password;
    std::string resolution;
    std::string certPolicy;
};

struct FreerdpProbeResult {
    bool linked = false;
    std::string json;
    std::string error;
    std::string freerdpVersion = "not-linked";
    std::string winprVersion = "not-linked";
    std::string opensslVersion = "not-linked";
};

struct CallbackData {
    std::string value;
};

struct TcpConnectResult {
    bool ok = false;
    std::string message;
};

struct SurfacePaintResult {
    bool ok = false;
    std::string message;
    std::vector<std::string> logs;
};

struct RgbaFrame {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t strideBytes = 0;
    std::string label;
};

void EmitNativeLog(const std::string& line);
SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame);

std::string SystemErrorMessage(int errorCode)
{
    if (errorCode == 0) {
        return "ok";
    }
    return std::strerror(errorCode);
}

void CloseSocket(int fd)
{
    if (fd >= 0) {
        ::close(fd);
    }
}

TcpConnectResult TryConnectAddress(const addrinfo* address, int timeoutMs)
{
    int fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (fd < 0) {
        return {false, "socket failed: " + SystemErrorMessage(errno)};
    }

    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "fcntl nonblock failed: " + SystemErrorMessage(error)};
    }

    int rc = ::connect(fd, address->ai_addr, address->ai_addrlen);
    if (rc == 0) {
        CloseSocket(fd);
        return {true, "tcp socket connected"};
    }

    if (errno != EINPROGRESS) {
        int error = errno;
        CloseSocket(fd);
        return {false, "connect failed: " + SystemErrorMessage(error)};
    }

    pollfd pollTarget = {};
    pollTarget.fd = fd;
    pollTarget.events = POLLOUT;
    rc = ::poll(&pollTarget, 1, timeoutMs);
    if (rc == 0) {
        CloseSocket(fd);
        return {false, "connect timed out"};
    }
    if (rc < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "poll failed: " + SystemErrorMessage(error)};
    }

    int socketError = 0;
    socklen_t socketErrorLength = sizeof(socketError);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorLength) < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "getsockopt failed: " + SystemErrorMessage(error)};
    }

    CloseSocket(fd);
    if (socketError == 0) {
        return {true, "tcp socket connected"};
    }
    return {false, "connect failed: " + SystemErrorMessage(socketError)};
}

TcpConnectResult TestTcpConnect(const std::string& host, const std::string& port, int timeoutMs)
{
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    int resolveStatus = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses);
    if (resolveStatus != 0) {
        return {false, "resolve failed: " + std::string(::gai_strerror(resolveStatus))};
    }

    std::string lastMessage = "no address candidates";
    for (const addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        TcpConnectResult result = TryConnectAddress(address, timeoutMs);
        if (result.ok) {
            ::freeaddrinfo(addresses);
            return result;
        }
        lastMessage = result.message;
    }

    ::freeaddrinfo(addresses);
    return {false, lastMessage};
}

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

UserParts SplitDomainUsername(const std::string& value)
{
    size_t separator = value.find('\\');
    if (separator == std::string::npos || separator == 0 || separator + 1 >= value.size()) {
        return {"", value};
    }
    return {value.substr(0, separator), value.substr(separator + 1)};
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
        }

        loaded_ = LoadFreerdpSymbol("freerdp_new", freerdpNew, error) &&
            LoadFreerdpSymbol("freerdp_free", freerdpFree, error) &&
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
            LoadWinprSymbol("WaitForMultipleObjects", waitForMultipleObjects, error);
        return loaded_;
    }

    using FreerdpNewFn = freerdp* (*)();
    using FreerdpFreeFn = void (*)(freerdp*);
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
    using WaitForMultipleObjectsFn = DWORD (*)(DWORD, const HANDLE*, BOOL, DWORD);

    FreerdpNewFn freerdpNew = nullptr;
    FreerdpFreeFn freerdpFree = nullptr;
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
    WaitForMultipleObjectsFn waitForMultipleObjects = nullptr;

private:
    template <typename Fn>
    bool LoadFreerdpSymbol(const char* name, Fn& target, std::string& error)
    {
        return LoadSymbolFrom(freerdpHandle_, "libfreerdp3.so", name, target, error);
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

std::atomic_uint32_t g_freerdpRenderedFrameCount{0};

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
    };
    SurfacePaintResult paint = RenderSurfaceRgbaFrame(frame);
    const uint32_t frameCount = ++g_freerdpRenderedFrameCount;
    if (!paint.ok) {
        EmitNativeLog("FreeRDP GDI frame render skipped: " + paint.message);
    } else if (frameCount <= 3 || frameCount % 60 == 0) {
        EmitNativeLog(paint.message);
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
    if (width == 0 || height == 0 || !api.gdiResize(context->gdi, width, height)) {
        EmitNativeLog("FreeRDP desktop resize failed");
        return FALSE;
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
    g_freerdpRenderedFrameCount.store(0);
    EmitNativeLog("FreeRDP GDI callbacks registered");
    return TRUE;
}

void HarmonyPostDisconnect(freerdp* instance)
{
    if (instance == nullptr || instance->context == nullptr || instance->context->gdi == nullptr) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiFree != nullptr) {
        api.gdiFree(instance);
        EmitNativeLog("FreeRDP GDI resources released");
    }
}

using FreerdpSetActiveFn = std::function<void(FreerdpRuntimeApi*, freerdp*, rdpContext*)>;
using FreerdpClearActiveFn = std::function<void(freerdp*)>;
using FreerdpLogFn = std::function<void(const std::string&)>;
using FreerdpConnectedFn = std::function<void()>;

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const FreerdpSetActiveFn& setActive, const FreerdpClearActiveFn& clearActive,
    const FreerdpLogFn& log, const FreerdpConnectedFn& onConnected)
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
    auto cleanup = [&]() {
        clearActive(instance);
        if (contextCreated && instance->context != nullptr) {
            api.abortConnectContext(instance->context);
            api.disconnect(instance);
            api.contextFree(instance);
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
    setActive(&api, instance, instance->context);

    rdpSettings* settings = instance->context == nullptr ? nullptr : instance->context->settings;
    if (settings == nullptr) {
        result.message = "FreeRDP settings unavailable";
        result.failed = true;
        cleanup();
        return result;
    }

    UserParts user = SplitDomainUsername(params.username);
    const bool acceptCertificate = params.certPolicy != "deny";

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
        !SetFreerdpBool(api, settings, FreeRDP_IgnoreCertificate, acceptCertificate, "IgnoreCertificate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AutoAcceptCertificate, acceptCertificate, "AutoAcceptCertificate", error)) {
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

    log("FreeRDP target configured");
    log("FreeRDP mode=PersistentSession");
    log("FreeRDP GDI renderer configured");
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

    while (running.load() && !api.shallDisconnectContext(instance->context)) {
        HANDLE handles[MAXIMUM_WAIT_OBJECTS] = {};
        DWORD count = api.getEventHandles(instance->context, handles, MAXIMUM_WAIT_OBJECTS);
        if (count == 0) {
            uint32_t errorCode = api.getLastError(instance->context);
            result.failed = true;
            result.message = "freerdp_get_event_handles failed: " + LastErrorMessage(api, errorCode);
            break;
        }

        DWORD waitStatus = api.waitForMultipleObjects(count, handles, FALSE, 250);
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

    bool Set(napi_env env, napi_value callback, const char* name)
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
        return true;
    }

    void Emit(const std::string& value)
    {
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
    }

private:
    std::mutex mutex_;
    napi_threadsafe_function function_ = nullptr;
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
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        g_events.log.Emit("XComponent surface changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height));
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

    SurfacePaintResult PaintTestPattern()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            SurfacePaintResult result;
            result.message = "XComponent surface is not ready for paint";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        std::vector<uint8_t> pixels(static_cast<size_t>(width_) * height_ * 4U);
        const int32_t strideBytes = static_cast<int32_t>(width_ * 4U);
        FillTestPatternRgba(pixels.data(), width_, height_, strideBytes, paintCount_ + 1);
        RgbaFrame frame = {
            pixels.data(),
            width_,
            height_,
            strideBytes,
            "test pattern",
        };
        return RenderRgbaFrameLocked(frame);
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

private:
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

        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        const int32_t targetWidth = static_cast<int32_t>(width_);
        const int32_t targetHeight = static_cast<int32_t>(height_);

        int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(
            nativeWindow, SET_BUFFER_GEOMETRY, targetWidth, targetHeight);
        if (rc != 0) {
            result.message = "NativeWindow SET_BUFFER_GEOMETRY failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        rc = OH_NativeWindow_NativeWindowHandleOpt(
            nativeWindow, SET_FORMAT, static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888));
        if (rc != 0) {
            result.logs.push_back("NativeWindow SET_FORMAT warning: " + std::to_string(rc));
        }

        constexpr uint64_t usage = NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE |
            NATIVEBUFFER_USAGE_MEM_DMA;
        rc = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_USAGE, usage);
        if (rc != 0) {
            result.logs.push_back("NativeWindow SET_USAGE warning: " + std::to_string(rc));
        }

        OHNativeWindowBuffer* buffer = nullptr;
        int fenceFd = -1;
        rc = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &buffer, &fenceFd);
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
        const uint32_t drawWidth = std::min({width_, bufferWidth, frame.width});
        const uint32_t drawHeight = std::min({height_, bufferHeight, frame.height});
        const int32_t rowBytes = ResolveRowBytes(*handle, drawWidth, drawHeight);
        if (drawWidth == 0 || drawHeight == 0 || rowBytes <= 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer geometry is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        OH_NativeBuffer* nativeBuffer = nullptr;
        rc = OH_NativeBuffer_FromNativeWindowBuffer(buffer, &nativeBuffer);
        if (rc != 0 || nativeBuffer == nullptr) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer conversion failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        void* mappedAddress = nullptr;
        OH_NativeBuffer_Planes planes = {};
        int32_t mappedRowBytes = 0;
        uint64_t mappedOffset = 0;
        rc = OH_NativeBuffer_MapPlanes(nativeBuffer, &mappedAddress, &planes);
        if (rc == 0 && mappedAddress != nullptr) {
            if (planes.planeCount > 0 && planes.planes[0].rowStride > 0) {
                mappedRowBytes = static_cast<int32_t>(planes.planes[0].rowStride);
                mappedOffset = planes.planes[0].offset;
            }
            result.logs.push_back("NativeBuffer mapped with planes");
        } else {
            mappedAddress = nullptr;
            rc = OH_NativeBuffer_Map(nativeBuffer, &mappedAddress);
            if (rc != 0 || mappedAddress == nullptr) {
                OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
                result.message = "NativeBuffer map failed: " + std::to_string(rc);
                result.logs.push_back(result.message);
                lastPaintMessage_ = result.message;
                return result;
            }
            result.logs.push_back("NativeBuffer mapped");
        }

        OH_NativeBuffer_Config config = {};
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
        if (mappedRowBytes <= 0 && config.stride >= static_cast<int32_t>(drawWidth * 4U)) {
            mappedRowBytes = config.stride;
        }
        if (mappedRowBytes <= 0) {
            mappedRowBytes = rowBytes;
        }

        if (mappedRowBytes < static_cast<int32_t>(drawWidth * 4U)) {
            OH_NativeBuffer_Unmap(nativeBuffer);
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer row stride is invalid: " + std::to_string(mappedRowBytes);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        BufferHandle mappedHandle = *handle;
        mappedHandle.virAddr = static_cast<uint8_t*>(mappedAddress) + mappedOffset;
        CopyRgbaToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride, drawWidth, drawHeight);
        OH_NativeBuffer_Unmap(nativeBuffer);

        Region::Rect dirtyRect = {0, 0, drawWidth, drawHeight};
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
        result.ok = true;
        const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
        result.message = "NativeWindow RGBA frame rendered: " + frameLabel + " " +
            std::to_string(drawWidth) + "x" + std::to_string(drawHeight);
        result.logs.push_back(result.message);
        result.logs.push_back("RGBA source=" + std::to_string(frame.width) + "x" +
            std::to_string(frame.height) + " stride=" + std::to_string(sourceStride));
        result.logs.push_back("NativeWindow format=" + std::to_string(handle->format) +
            " stride=" + std::to_string(handle->stride) +
            " rowBytes=" + std::to_string(mappedRowBytes));
        lastPaintMessage_ = result.message;
        return result;
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

    static void FillTestPatternRgba(uint8_t* base, uint32_t width, uint32_t height,
        int32_t rowBytes, uint32_t frameIndex)
    {
        const uint32_t safeWidth = std::max(width, 1U);
        const uint32_t safeHeight = std::max(height, 1U);
        for (uint32_t y = 0; y < height; ++y) {
            uint8_t* row = base + static_cast<int64_t>(rowBytes) * y;
            for (uint32_t x = 0; x < width; ++x) {
                const bool grid = (x % 96U) < 3U || (y % 96U) < 3U;
                uint8_t r = static_cast<uint8_t>((x * 255U) / safeWidth);
                uint8_t g = static_cast<uint8_t>((y * 255U) / safeHeight);
                uint8_t b = static_cast<uint8_t>(96U + ((x + y + frameIndex * 23U) % 128U));
                if (grid) {
                    r = 255;
                    g = 255;
                    b = 255;
                }
                WriteRgbaPixel(row + x * 4, r, g, b);
            }
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

    SurfaceSnapshot SnapshotLocked() const
    {
        return SurfaceSnapshot{
            registered_,
            ready_,
            id_.empty() ? "unknown" : id_,
            width_,
            height_,
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
    uint32_t createdCount_ = 0;
    uint32_t changedCount_ = 0;
    uint32_t destroyedCount_ = 0;
    uint32_t touchCount_ = 0;
    uint32_t paintCount_ = 0;
    std::string lastPaintMessage_;
};

SurfaceBridge g_surface;

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

void OnXComponentSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceCreated(component, window);
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
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
        message = "native worker started";
        worker_ = std::thread([this, params]() {
            WorkerMain(params);
        });
        return true;
    }

    void Disconnect()
    {
        running_.store(false);
        RequestNativeDisconnect();
        if (worker_.joinable()) {
            worker_.join();
        }
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

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    void SetActiveNative(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context)
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        activeApi_ = api;
        activeInstance_ = instance;
        activeContext_ = context;
    }

    void ClearActiveNative(freerdp* instance)
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeInstance_ != instance) {
            return;
        }

        activeApi_ = nullptr;
        activeInstance_ = nullptr;
        activeContext_ = nullptr;
    }

    void RequestNativeDisconnect()
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeApi_ != nullptr && activeContext_ != nullptr) {
            activeApi_->abortConnectContext(activeContext_);
        }
    }
#else
    void RequestNativeDisconnect() {}
#endif

    void WorkerMain(ConnectParams params)
    {
        EmitLog("native worker accepted params");
        EmitLog("target=" + params.host + ":" + params.port);

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
        RdpSessionRunResult session;
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        session = RunFreerdpSession(params, running_,
            [this](FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context) {
                SetActiveNative(api, instance, context);
            },
            [this](freerdp* instance) {
                ClearActiveNative(instance);
            },
            [this](const std::string& line) {
                EmitLog(line);
            },
            [this]() {
                EmitState("Connected");
                EmitLog("state=Connected");
                EmitLog("FreeRDP persistent session loop is active");
            });
#else
        session = RunFreerdpSessionUnavailable();
#endif

        if (session.cancelled || !running_.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        if (session.failed) {
            std::string message = session.available ? session.message : "FreeRDP runtime unavailable: " + session.message;
            EmitState("Failed");
            EmitLog(message);
            g_events.error.Emit(message);
            running_.store(false);
            return;
        }

        EmitState("Disconnected");
        EmitLog(session.message);
        running_.store(false);
    }

    std::atomic_bool running_ = false;
    std::thread worker_;
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
#endif
};

RdpSession g_session;

napi_value MakeString(napi_env env, const std::string& value)
{
    napi_value result = nullptr;
    napi_create_string_utf8(env, value.c_str(), value.size(), &result);
    return result;
}

napi_value MakeBool(napi_env env, bool value)
{
    napi_value result = nullptr;
    napi_get_boolean(env, value, &result);
    return result;
}

napi_value MakeUint32(napi_env env, uint32_t value)
{
    napi_value result = nullptr;
    napi_create_uint32(env, value, &result);
    return result;
}

napi_value MakeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
    return result;
}

void SetNamed(napi_env env, napi_value object, const char* name, napi_value value)
{
    napi_set_named_property(env, object, name, value);
}

void SetString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    SetNamed(env, object, name, MakeString(env, value));
}

void SetBool(napi_env env, napi_value object, const char* name, bool value)
{
    SetNamed(env, object, name, MakeBool(env, value));
}

void SetUint32(napi_env env, napi_value object, const char* name, uint32_t value)
{
    SetNamed(env, object, name, MakeUint32(env, value));
}

std::string ExtractJsonString(const std::string& json, const std::string& key)
{
    const std::string marker = "\"" + key + "\":\"";
    const size_t start = json.find(marker);
    if (start == std::string::npos) {
        return "";
    }

    size_t valueStart = start + marker.size();
    std::string result;
    for (size_t i = valueStart; i < json.size(); ++i) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            result.push_back(json[i + 1]);
            ++i;
            continue;
        }
        if (c == '"') {
            break;
        }
        result.push_back(c);
    }
    return result;
}

FreerdpProbeResult LoadFreerdpProbe()
{
    FreerdpProbeResult result;
    void* handle = dlopen("libfreerdp_ohos_probe.so", RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        const char* error = dlerror();
        result.error = error == nullptr ? "dlopen failed" : error;
        return result;
    }

    using ProbeFn = const char* (*)();
    dlerror();
    auto probe = reinterpret_cast<ProbeFn>(dlsym(handle, "freerdp_ohos_probe"));
    const char* symbolError = dlerror();
    if (symbolError != nullptr || probe == nullptr) {
        result.error = symbolError == nullptr ? "dlsym failed" : symbolError;
        dlclose(handle);
        return result;
    }

    const char* json = probe();
    result.json = json == nullptr ? "" : json;
    result.linked = !result.json.empty();
    result.freerdpVersion = ExtractJsonString(result.json, "freerdpVersion");
    result.winprVersion = ExtractJsonString(result.json, "winprVersion");
    result.opensslVersion = ExtractJsonString(result.json, "opensslVersion");
    if (result.freerdpVersion.empty()) {
        result.freerdpVersion = "unknown";
    }
    if (result.winprVersion.empty()) {
        result.winprVersion = "unknown";
    }
    if (result.opensslVersion.empty()) {
        result.opensslVersion = "unknown";
    }
    dlclose(handle);
    return result;
}

napi_value MakeStringArray(napi_env env, const std::vector<std::string>& values)
{
    napi_value array = nullptr;
    napi_create_array_with_length(env, values.size(), &array);
    for (size_t i = 0; i < values.size(); ++i) {
        napi_set_element(env, array, i, MakeString(env, values[i]));
    }
    return array;
}

std::string GetStringProperty(napi_env env, napi_value object, const char* name)
{
    bool hasProperty = false;
    napi_has_named_property(env, object, name, &hasProperty);
    if (!hasProperty) {
        return "";
    }

    napi_value value = nullptr;
    napi_get_named_property(env, object, name, &value);

    napi_valuetype type = napi_undefined;
    napi_typeof(env, value, &type);
    if (type != napi_string) {
        return "";
    }

    size_t length = 0;
    napi_get_value_string_utf8(env, value, nullptr, 0, &length);
    std::vector<char> buffer(length + 1);
    napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length);
    return std::string(buffer.data(), length);
}

napi_value GetFirstArgument(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    return argc > 0 ? args[0] : nullptr;
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
    return params;
}

std::string CurrentAbi()
{
#if defined(__aarch64__)
    return "arm64-v8a";
#elif defined(__x86_64__)
    return "x86_64";
#elif defined(__arm__)
    return "armeabi-v7a";
#else
    return "unknown";
#endif
}

napi_value Probe(napi_env env, napi_callback_info info)
{
    FreerdpProbeResult freerdp = LoadFreerdpProbe();
    SurfaceSnapshot surface = g_surface.Snapshot();

    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.5.3");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", freerdp.freerdpVersion);
    SetString(env, result, "winprVersion", freerdp.winprVersion);
    SetString(env, result, "opensslVersion", freerdp.opensslVersion);
    SetString(env, result, "probeJson", freerdp.json);
    SetString(env, result, "probeError", freerdp.error);
    SetBool(env, result, "freeRdpLinked", freerdp.linked);
    SetBool(env, result, "surfaceRegistered", surface.registered);
    SetBool(env, result, "surfaceReady", surface.ready);
    SetString(env, result, "surfaceId", surface.id);
    SetUint32(env, result, "surfaceWidth", surface.width);
    SetUint32(env, result, "surfaceHeight", surface.height);
    SetUint32(env, result, "surfaceCreatedCount", surface.createdCount);
    SetUint32(env, result, "surfaceChangedCount", surface.changedCount);
    SetUint32(env, result, "surfaceDestroyedCount", surface.destroyedCount);
    SetUint32(env, result, "surfaceTouchCount", surface.touchCount);
    SetUint32(env, result, "surfacePaintCount", surface.paintCount);
    SetString(env, result, "surfaceLastPaintMessage", surface.lastPaintMessage);

    std::vector<std::string> logs = {
        "N-API bridge loaded",
        "Native calls are available: probe, connect, disconnect, paintTestPattern"
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
        if (!surface.lastPaintMessage.empty()) {
            logs.push_back(surface.lastPaintMessage);
        }
    } else if (surface.registered) {
        logs.push_back("XComponent callback registered; surface not created");
    } else {
        logs.push_back("XComponent callback not registered");
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
    g_session.Disconnect();
    g_events.state.Emit("Disconnected");
    g_events.log.Emit("native disconnect invoked");

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Disconnected");
    SetString(env, result, "message", "native bridge session closed");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "native disconnect invoked",
        "native worker stopped"
    }));
    return result;
}

napi_value PaintTestPattern(napi_env env, napi_callback_info info)
{
    SurfacePaintResult paint = g_surface.PaintTestPattern();
    g_events.log.Emit(paint.message);

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", paint.ok);
    SetString(env, result, "state", paint.ok ? "Bridge ready" : "Failed");
    SetString(env, result, "message", paint.message);
    SetNamed(env, result, "logs", MakeStringArray(env, paint.logs));
    return result;
}

napi_value RegisterCallback(napi_env env, napi_callback_info info, EventSink& sink, const char* name)
{
    napi_value callback = GetFirstArgument(env, info);
    bool ok = callback != nullptr && sink.Set(env, callback, name);

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
    return RegisterCallback(env, info, g_events.log, "rdpLogCallback");
}

napi_value OnError(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.error, "rdpErrorCallback");
}

} // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"probe", nullptr, Probe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"paintTestPattern", nullptr, PaintTestPattern, nullptr, nullptr, nullptr, napi_default, nullptr},
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
