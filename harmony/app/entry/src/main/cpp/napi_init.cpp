#include "napi/native_api.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
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
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>
#include <database/pasteboard/oh_pasteboard.h>
#include <database/pasteboard/oh_pasteboard_err_code.h>
#include <database/udmf/udmf.h>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/uds.h>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/addin.h>
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/codec/color.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <freerdp/update.h>
#include <winpr/clipboard.h>
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
            LoadWinprSymbol("PubSub_Subscribe", pubSubSubscribe, error) &&
            LoadWinprSymbol("PubSub_Unsubscribe", pubSubUnsubscribe, error) &&
            LoadWinprSymbol("WaitForMultipleObjects", waitForMultipleObjects, error);
        if (loaded_) {
            LoadOptionalClientSymbol("freerdp_rdpsnd_ohos_get_stats", rdpsndOhosGetStats);
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
    using PubSubSubscribeFn = int (*)(wPubSub*, const char*, ...);
    using PubSubUnsubscribeFn = int (*)(wPubSub*, const char*, ...);
    using RdpsndOhosGetStatsFn = BOOL (*)(UINT64*, UINT64*, UINT64*, UINT64*, UINT64*, UINT64*,
        UINT64*, UINT64*, UINT32*, UINT16*, UINT16*, UINT32*);
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
    PubSubSubscribeFn pubSubSubscribe = nullptr;
    PubSubUnsubscribeFn pubSubUnsubscribe = nullptr;
    RdpsndOhosGetStatsFn rdpsndOhosGetStats = nullptr;
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
        const uint32_t skipCount = ++g_freerdpRenderSkipCount;
        if (skipCount <= 3 || skipCount % 120 == 0) {
            EmitNativeLog("FreeRDP GDI frame render skipped: " + paint.message);
        }
    } else {
        g_freerdpRenderSkipCount.store(0);
        if (frameCount <= 3 || frameCount % 60 == 0) {
            EmitNativeLog(paint.message);
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
    if (width == 0 || height == 0 || !api.gdiResize(context->gdi, width, height)) {
        EmitNativeLog("FreeRDP desktop resize failed");
        return FALSE;
    }

    SetRdpDesktopSize(width, height);
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
    if (instance == nullptr || instance->context == nullptr || instance->context->gdi == nullptr) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiFree != nullptr) {
        api.gdiFree(instance);
        EmitNativeLog("FreeRDP GDI resources released");
    }
    ClearRdpDesktopSize();
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
    const FreerdpLogFn& log, std::string& error)
{
    if (!SetFreerdpBool(api, settings, FreeRDP_SupportDynamicChannels, false, "SupportDynamicChannels", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportDisplayControl, false, "SupportDisplayControl", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SupportGraphicsPipeline, false, "SupportGraphicsPipeline", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxH264, false, "GfxH264", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444, false, "GfxAVC444", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_GfxAVC444v2, false, "GfxAVC444v2", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectClipboard, true, "RedirectClipboard", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ClipboardFeatureMask,
            CLIPRDR_FLAG_LOCAL_TO_REMOTE | CLIPRDR_FLAG_REMOTE_TO_LOCAL,
            "ClipboardFeatureMask", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_DeviceRedirection, false, "DeviceRedirection", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AudioPlayback, true, "AudioPlayback", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AudioCapture, false, "AudioCapture", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectDrives, false, "RedirectDrives", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectPrinters, false, "RedirectPrinters", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_RedirectSmartCards, false, "RedirectSmartCards", error)) {
        return false;
    }

    log("FreeRDP enhanced runtime libraries packaged; clipboard text redirection and audio playback enabled");
    log("FreeRDP graphics pipeline disabled at runtime; using stable software GDI frame rendering");
    log("FreeRDP redirect devices compiled; drive/printer/smartcard runtime toggles remain disabled by default");
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
    if (api.clientAddStaticChannel == nullptr) {
        error = "FreeRDP static channel helper is not loaded";
        return false;
    }

    const char* params[] = {"rdpsnd", "sys:ohos"};
    if (!api.clientAddStaticChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set rdpsnd static channel failed";
        return false;
    }

    log("FreeRDP audio playback requested: static rdpsnd sys:ohos");
    log("FreeRDP dynamic channels remain disabled; microphone capture remains disabled");
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

        pasteboard_ = OH_Pasteboard_Create();
        if (pasteboard_ == nullptr) {
            Log("HarmonyOS Pasteboard create failed; cliprdr will advertise no local text");
            return true;
        }

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
                }
            }
            requestedFormatId_ = requested;
        }

        Log("cliprdr server format list received: " + std::to_string(formatList.numFormats));
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
        } else if (ok && request.requestedFormatId == CF_TEXT) {
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
            Log("cliprdr remote text response failed");
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
        } else if (requested == CF_TEXT) {
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
        if (!OH_Pasteboard_HasType(pasteboard_, PASTEBOARD_MIMETYPE_TEXT_PLAIN)) {
            return false;
        }

        int status = ERR_OK;
        OH_UdmfData* data = OH_Pasteboard_GetData(pasteboard_, &status);
        if (status != ERR_OK || data == nullptr) {
            error = "OH_Pasteboard_GetData status=" + std::to_string(status);
            return false;
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
        return !text.empty();
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
        clipboardBridge.Uninitialize();
        ClearRdpDesktopSize();
        clearActive(instance);
        UnregisterCertificatePolicy(instance);
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

    if (!ConfigureEnhancedRdpSettings(api, settings, log, error)) {
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

    while (running.load() && !api.shallDisconnectContext(instance->context)) {
        pumpInput(&api, instance->context);

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
    struct RenderViewport {
        uint32_t x = 0;
        uint32_t y = 0;
        uint32_t width = 0;
        uint32_t height = 0;
    };

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
        const RenderViewport viewport = FitFrameIntoTarget(
            targetAreaWidth, targetAreaHeight, frame.width, frame.height);
        if (viewport.width == 0 || viewport.height == 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "render viewport is invalid";
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
        int32_t mappedRowBytes = 0;
        rc = OH_NativeBuffer_Map(nativeBuffer, &mappedAddress);
        if (rc != 0 || mappedAddress == nullptr) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer map failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        OH_NativeBuffer_Config config = {};
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
        if (mappedRowBytes <= 0 && config.stride >= static_cast<int32_t>(targetAreaWidth * 4U)) {
            mappedRowBytes = config.stride;
        }
        if (mappedRowBytes <= 0) {
            mappedRowBytes = rowBytes;
        }

        if (mappedRowBytes < static_cast<int32_t>(targetAreaWidth * 4U)) {
            OH_NativeBuffer_Unmap(nativeBuffer);
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer row stride is invalid: " + std::to_string(mappedRowBytes);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        BufferHandle mappedHandle = *handle;
        mappedHandle.virAddr = mappedAddress;
        FillNativeLetterbox(mappedHandle, mappedRowBytes, targetAreaWidth, targetAreaHeight,
            viewport, 0, 0, 0, 0xFF);
        CopyScaledRgbaToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride,
            frame.width, frame.height, viewport);
        OH_NativeBuffer_Unmap(nativeBuffer);

        Region::Rect dirtyRect = {0, 0, targetAreaWidth, targetAreaHeight};
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
        viewportX_ = viewport.x;
        viewportY_ = viewport.y;
        viewportWidth_ = viewport.width;
        viewportHeight_ = viewport.height;
        result.ok = true;
        const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
        result.message = "NativeWindow RGBA frame rendered: " + frameLabel + " " +
            std::to_string(viewport.width) + "x" + std::to_string(viewport.height) +
            " viewport=" + std::to_string(viewport.x) + "," + std::to_string(viewport.y);
        result.logs.push_back(result.message);
        result.logs.push_back("RGBA source=" + std::to_string(frame.width) + "x" +
            std::to_string(frame.height) + " stride=" + std::to_string(sourceStride));
        result.logs.push_back("NativeWindow format=" + std::to_string(handle->format) +
            " stride=" + std::to_string(handle->stride) +
            " rowBytes=" + std::to_string(mappedRowBytes));
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
        configuredWindow_ = nullptr;
        configuredWidth_ = 0;
        configuredHeight_ = 0;
        configuredFormat_ = 0;
        configuredUsage_ = 0;
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

        constexpr uint64_t usage = NATIVEBUFFER_USAGE_CPU_READ | NATIVEBUFFER_USAGE_CPU_WRITE |
            NATIVEBUFFER_USAGE_MEM_DMA;
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
        running_.store(false);
        connected_.store(false);
        ClearInputQueue();
        ClearRdpDesktopSize();
        RequestNativeDisconnect();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    bool IsConnected() const
    {
        return connected_.load();
    }

    bool Resize(uint32_t width, uint32_t height, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (width < 320 || height < 200 || width > 8192 || height > 8192) {
            message = "resize requires width 320-8192 and height 200-8192";
            return false;
        }
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        message = "dynamic resize is not available in this build: FreeRDP display-control channel is disabled";
        return false;
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
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

    bool SendKey(uint32_t rdpScancode, bool down, std::string& message)
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
        return EnqueueInput(event, down ? "key down queued" : "key up queued", message);
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
    }

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    bool IsCoalesciblePointerMove(const QueuedInputEvent& event) const
    {
        return event.type == QueuedInputType::Pointer && event.flags == PTR_FLAGS_MOVE;
    }

    bool EnqueueInput(const QueuedInputEvent& event, const char* okMessage, std::string& message)
    {
        constexpr size_t maxInputQueue = 1024;
        std::lock_guard<std::mutex> lock(inputMutex_);

        if (IsCoalesciblePointerMove(event) && !inputQueue_.empty() &&
            IsCoalesciblePointerMove(inputQueue_.back())) {
            inputQueue_.back() = event;
            inputQueuedCount_.fetch_add(1);
            message = okMessage;
            return true;
        }

        if (inputQueue_.size() >= maxInputQueue) {
            inputDroppedCount_.fetch_add(1);
            message = "FreeRDP input queue is full";
            return false;
        }

        inputQueue_.push_back(event);
        inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
        inputQueuedCount_.fetch_add(1);
        message = okMessage;
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
                    ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE, FALSE,
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
                connected_.store(true);
                EmitState("Connected");
                EmitLog("state=Connected");
                EmitLog("FreeRDP persistent session loop is active");
                EmitLog("FreeRDP input bridge is using worker-thread dispatch");
            },
            [this](FreerdpRuntimeApi* api, rdpContext* context) {
                DrainInputQueue(api, context);
            });
#else
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
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
    std::mutex inputMutex_;
    std::deque<QueuedInputEvent> inputQueue_;
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

uint32_t GetUint32Property(napi_env env, napi_value object, const char* name, uint32_t fallback = 0)
{
    bool hasProperty = false;
    napi_has_named_property(env, object, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value = nullptr;
    napi_get_named_property(env, object, name, &value);

    napi_valuetype type = napi_undefined;
    napi_typeof(env, value, &type);
    if (type != napi_number) {
        return fallback;
    }

    uint32_t result = fallback;
    napi_get_value_uint32(env, value, &result);
    return result;
}

bool GetBoolProperty(napi_env env, napi_value object, const char* name, bool fallback = false)
{
    bool hasProperty = false;
    napi_has_named_property(env, object, name, &hasProperty);
    if (!hasProperty) {
        return fallback;
    }

    napi_value value = nullptr;
    napi_get_named_property(env, object, name, &value);

    napi_valuetype type = napi_undefined;
    napi_typeof(env, value, &type);
    if (type != napi_boolean) {
        return fallback;
    }

    bool result = fallback;
    napi_get_value_bool(env, value, &result);
    return result;
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

std::string BuildOHAudioStatsLog()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return "OHAudio stats unavailable: " + error;
    }
    if (api.rdpsndOhosGetStats == nullptr) {
        return "OHAudio stats unavailable: backend symbol not exported";
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
    return out.str();
#else
    return "OHAudio stats unavailable: FreeRDP headers not found at build time";
#endif
}

napi_value Probe(napi_env env, napi_callback_info info)
{
    FreerdpProbeResult freerdp = LoadFreerdpProbe();
    SurfaceSnapshot surface = g_surface.Snapshot();
    const std::string featureSummary =
        "core RDP/TLS/NLA + software GDI; client channels on; "
        "cliprdr/rdpdr/drive/printer/smartcard/rdpsnd/audin/rdpgfx/disp compiled; "
        "H264 + FFmpeg + OpenH264 enabled; RD Gateway core enabled; "
        "static cliprdr text bridge and rdpsnd/OHAudio playback requested; "
        "other optional channel negotiation off";

    const std::string audioStats = BuildOHAudioStatsLog();

    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.8.1");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", freerdp.freerdpVersion);
    SetString(env, result, "winprVersion", freerdp.winprVersion);
    SetString(env, result, "opensslVersion", freerdp.opensslVersion);
    SetString(env, result, "featureSummary", featureSummary);
    SetString(env, result, "audioStats", audioStats);
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
        "Native calls are available: probe, connect, disconnect, resize, paintTestPattern, sendPointer, sendKey, sendUnicode",
        "FreeRDP input dispatch: worker-thread queue",
        "FreeRDP channel dispatch: libfreerdp-client static addin provider",
        "FreeRDP build features: " + featureSummary,
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

napi_value Resize(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native resize invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "resize requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t width = GetUint32Property(env, arg, "width");
    const uint32_t height = GetUint32Property(env, arg, "height");
    logs.push_back("size=" + std::to_string(width) + "x" + std::to_string(height));

    std::string message;
    const bool ok = g_session.Resize(width, height, message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
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
    logs.push_back("scancode=" + std::to_string(scancode) + (down ? " down" : " up"));

    std::string message;
    const bool ok = g_session.SendKey(scancode, down, message);
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
        {"resize", nullptr, Resize, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"paintTestPattern", nullptr, PaintTestPattern, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendPointer", nullptr, SendPointer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendKey", nullptr, SendKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendUnicode", nullptr, SendUnicode, nullptr, nullptr, nullptr, napi_default, nullptr},
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
