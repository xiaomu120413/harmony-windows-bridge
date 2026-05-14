#include "napi/native_api.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <dlfcn.h>
#include <fcntl.h>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <poll.h>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace {

struct ConnectParams {
    std::string host;
    std::string port;
    std::string username;
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

class RdpSession {
public:
    ~RdpSession()
    {
        Disconnect();
    }

    bool Connect(const ConnectParams& params, std::string& message)
    {
        if (params.host.empty() || params.port.empty() || params.username.empty()) {
            message = "host, port, and username are required";
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

        const std::vector<std::string> remainingStates = {
            "Negotiating",
            "Authenticating",
            "Connected"
        };

        for (const auto& state : remainingStates) {
            if (!running_.load()) {
                EmitState("Disconnected");
                EmitLog("native worker cancelled");
                return;
            }
            EmitState(state);
            EmitLog("state=" + state + " (simulated until FreeRDP connect/auth is wired)");
            if (!SleepInterruptibly(250)) {
                EmitState("Disconnected");
                EmitLog("native worker cancelled");
                return;
            }
        }

        EmitLog("M4.2 TCP reachability verified; real freerdp_connect starts in the next step");
    }

    std::atomic_bool running_ = false;
    std::thread worker_;
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

    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.3.0");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", freerdp.freerdpVersion);
    SetString(env, result, "winprVersion", freerdp.winprVersion);
    SetString(env, result, "opensslVersion", freerdp.opensslVersion);
    SetString(env, result, "probeJson", freerdp.json);
    SetString(env, result, "probeError", freerdp.error);
    SetBool(env, result, "freeRdpLinked", freerdp.linked);

    std::vector<std::string> logs = {
        "N-API bridge loaded",
        "Native calls are available: probe, connect, disconnect"
    };
    if (freerdp.linked) {
        logs.push_back("FreeRDP probe library loaded");
        logs.push_back("FreeRDP " + freerdp.freerdpVersion);
        logs.push_back("WinPR " + freerdp.winprVersion);
        logs.push_back(freerdp.opensslVersion);
    } else {
        logs.push_back("FreeRDP probe library not loaded: " + freerdp.error);
    }
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Connect(napi_env env, napi_callback_info info)
{
    ConnectParams params = ReadConnectParams(env, info);
    std::vector<std::string> logs = {"native connect invoked"};

    napi_value result = MakeObject(env);
    if (params.host.empty() || params.port.empty() || params.username.empty()) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "host, port, and username are required");
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
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onLog", nullptr, OnLog, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
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
