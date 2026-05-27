#include "napi/napi_exports.h"

#include "napi/native_api.h"
#include "napi/native_bridge_context.h"
#include "common/bridge_types.h"
#include "common/bridge_log.h"
#include "napi/clipboard_permission_bridge.h"
#include "napi/location_bridge.h"
#include "napi/microphone_permission_bridge.h"
#include "napi/napi_event_sink.h"
#include "napi/napi_utils.h"
#include "xrdp/xrdp_server_bridge.h"

#include <cstdlib>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using namespace rdp_bridge;

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
    const std::string resolution = GetStringProperty(env, args[0], "resolution");
    if (!resolution.empty()) {
        params.resolution = resolution;
    }
    params.certPolicy = GetStringProperty(env, args[0], "certPolicy");
    const std::string graphicsMode = GetStringProperty(env, args[0], "graphicsMode");
    if (!graphicsMode.empty()) {
        params.graphicsMode = graphicsMode;
    }
    params.appFilesDir = GetStringProperty(env, args[0], "appFilesDir");
    return params;
}

uint32_t ParseUint32String(const std::string& value, uint32_t fallback)
{
    if (value.empty()) {
        return fallback;
    }

    char* end = nullptr;
    const unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (end == value.c_str() || parsed > 0xFFFFFFFFUL) {
        return fallback;
    }
    return static_cast<uint32_t>(parsed);
}

XrdpServerParams ReadXrdpServerParams(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    XrdpServerParams params;
    if (arg == nullptr || type != napi_object) {
        return params;
    }

    params.appFilesDir = GetStringProperty(env, arg, "appFilesDir");
    params.runtimeRoot = GetStringProperty(env, arg, "runtimeRoot");
    params.hnpRoot = GetStringProperty(env, arg, "hnpRoot");
    params.libraryPath = GetStringProperty(env, arg, "libraryPath");
    params.libDir = GetStringProperty(env, arg, "libDir");
    params.modulePath = GetStringProperty(env, arg, "modulePath");
    params.configPath = GetStringProperty(env, arg, "configPath");
    params.sharePath = GetStringProperty(env, arg, "sharePath");
    params.accessCode = GetStringProperty(env, arg, "accessCode");
    params.accessCodeGateEnabled = GetBoolProperty(env, arg, "accessCodeGateEnabled");
    params.restartIfRunning = GetBoolProperty(env, arg, "restartIfRunning");
    params.port = GetUint32Property(env, arg, "port", 0);
    if (params.port == 0) {
        params.port = ParseUint32String(GetStringProperty(env, arg, "port"), 0);
    }
    return params;
}

void SetXrdpCommonResult(napi_env env, napi_value result, const XrdpServerCommandResult& command)
{
    SetBool(env, result, "ok", command.ok);
    SetString(env, result, "state", command.state);
    SetString(env, result, "message", command.message);
    SetString(env, result, "libraryPath", command.libraryPath);
    SetString(env, result, "runtimeRoot", command.runtimeRoot);
    SetString(env, result, "configPath", command.configPath);
    SetString(env, result, "modulePath", command.modulePath);
    SetString(env, result, "logPath", command.logPath);
    SetBool(env, result, "activeMstscSession", command.activeMstscSession);
    SetUint32(env, result, "port", command.port);
    SetNamed(env, result, "logs", MakeStringArray(env, command.logs));
}

napi_value MakeXrdpServerResult(napi_env env, const XrdpServerCommandResult& command)
{
    napi_value result = MakeObject(env);
    SetXrdpCommonResult(env, result, command);
    return result;
}

napi_value MakeXrdpDiagnosticsResult(napi_env env, const XrdpServerDiagnostics& diagnostics)
{
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", diagnostics.ok);
    SetBool(env, result, "running", diagnostics.running);
    SetBool(env, result, "activeMstscSession", diagnostics.activeMstscSession);
    SetUint32(env, result, "port", diagnostics.port);
    SetUint32(env, result, "sessionWidth", diagnostics.sessionWidth);
    SetUint32(env, result, "sessionHeight", diagnostics.sessionHeight);
    SetUint32(env, result, "sessionBpp", diagnostics.sessionBpp);
    SetUint32(env, result, "backendEventCount", diagnostics.backendEventCount);
    SetUint32(env, result, "inputEventCount", diagnostics.inputEventCount);
    SetString(env, result, "state", diagnostics.state);
    SetString(env, result, "message", diagnostics.message);
    SetString(env, result, "lastBackendEvent", diagnostics.lastBackendEvent);
    SetString(env, result, "lastDisconnectReason", diagnostics.lastDisconnectReason);
    SetString(env, result, "libraryPath", diagnostics.libraryPath);
    SetString(env, result, "backendLibraryPath", diagnostics.backendLibraryPath);
    SetString(env, result, "runtimeRoot", diagnostics.runtimeRoot);
    SetString(env, result, "configPath", diagnostics.configPath);
    SetString(env, result, "modulePath", diagnostics.modulePath);
    SetString(env, result, "sharePath", diagnostics.sharePath);
    SetString(env, result, "logPath", diagnostics.logPath);
    SetNamed(env, result, "logs", MakeStringArray(env, diagnostics.logs));
    return result;
}

std::string RedactedEndpointLog(const ConnectParams& params)
{
    return params.port.empty() ? "target=<redacted>" : "target=<redacted>:" + params.port;
}

std::string RedactedValueLog(const char* name, const std::string& value)
{
    return std::string(name) + "=" + (value.empty() ? "<empty>" : "<redacted>");
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

    logs.push_back(RedactedEndpointLog(params));
    logs.push_back(RedactedValueLog("username", params.username));
    logs.push_back("certPolicy=" + params.certPolicy);
    logs.push_back(RedactedValueLog("appFilesDir", params.appFilesDir));
    logs.push_back("starting native worker");

    std::string message;
    bool started = BridgeSession().Connect(params, message);
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

napi_value EnsureXrdpServerStarted(napi_env env, napi_callback_info info)
{
    const XrdpServerParams params = ReadXrdpServerParams(env, info);
    return MakeXrdpServerResult(env, rdp_bridge::StartXrdpServer(params));
}

napi_value GetXrdpServerDiagnostics(napi_env env, napi_callback_info)
{
    return MakeXrdpDiagnosticsResult(env, rdp_bridge::GetXrdpServerDiagnostics());
}

napi_value ReleaseAllKeys(napi_env env, napi_callback_info info)
{
    (void)info;
    std::vector<std::string> logs = {"native release all keys invoked"};
    std::string message;
    const bool ok = BridgeSession().ReleaseAllKeys(message);
    EmitHilogInfo(message);

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
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
    return RegisterCallback(env, info, BridgeEvents().state, "rdpStateCallback");
}

napi_value OnError(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, BridgeEvents().error, "rdpErrorCallback", true);
}

napi_value OnMicrophonePermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, MicrophonePermissionRequestSink(),
        "rdpMicrophonePermissionRequestCallback", true);
}

napi_value OnClipboardPermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, ClipboardPermissionRequestSink(),
        "rdpClipboardPermissionRequestCallback", true);
}

napi_value OnLocationPermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, LocationPermissionRequestSink(),
        "rdpLocationPermissionRequestCallback", true);
}

napi_value CompleteClipboardPermissionRequest(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "clipboard permission completion requires an object argument");
        SetNamed(env, result, "logs", MakeStringArray(env, {"parameter validation failed"}));
        return result;
    }

    const uint32_t requestId = GetUint32Property(env, arg, "requestId");
    const bool granted = GetBoolProperty(env, arg, "granted");
    const bool ok = CompleteClipboardPermissionRequestFromUi(requestId, granted);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", ok ? "clipboard permission result accepted" :
        "clipboard permission request is not pending");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "requestId=" + std::to_string(requestId) +
            " granted=" + std::string(granted ? "true" : "false")
    }));
    return result;
}

napi_value CompleteLocationPermissionRequest(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "location permission completion requires an object argument");
        SetNamed(env, result, "logs", MakeStringArray(env, {"parameter validation failed"}));
        return result;
    }

    const uint32_t requestId = GetUint32Property(env, arg, "requestId");
    const bool granted = GetBoolProperty(env, arg, "granted");
    const bool ok = CompleteLocationPermissionRequestFromUi(requestId, granted);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", ok ? "location permission result accepted" :
        "location permission request is not pending");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "requestId=" + std::to_string(requestId) +
            " granted=" + std::string(granted ? "true" : "false")
    }));
    return result;
}

napi_value CompleteMicrophonePermissionRequest(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "microphone permission completion requires an object argument");
        SetNamed(env, result, "logs", MakeStringArray(env, {"parameter validation failed"}));
        return result;
    }

    const uint32_t requestId = GetUint32Property(env, arg, "requestId");
    const bool granted = GetBoolProperty(env, arg, "granted");
    const bool ok = CompleteMicrophonePermissionRequestFromUi(requestId, granted);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", ok ? "microphone permission result accepted" :
        "microphone permission request is not pending");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "requestId=" + std::to_string(requestId) +
            " granted=" + std::string(granted ? "true" : "false")
    }));
    return result;
}

} // namespace

napi_value RegisterRdpNativeExports(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"ensureXrdpServerStarted", nullptr, EnsureXrdpServerStarted, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getXrdpServerDiagnostics", nullptr, GetXrdpServerDiagnostics, nullptr, nullptr, nullptr, napi_default,
            nullptr},
        {"releaseAllKeys", nullptr, ReleaseAllKeys, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onMicrophonePermissionRequest", nullptr, OnMicrophonePermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onClipboardPermissionRequest", nullptr, OnClipboardPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onLocationPermissionRequest", nullptr, OnLocationPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"completeClipboardPermissionRequest", nullptr, CompleteClipboardPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeMicrophonePermissionRequest", nullptr, CompleteMicrophonePermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeLocationPermissionRequest", nullptr, CompleteLocationPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    InitializeNativeBridgeContext();
    RegisterNativeXComponent(env, exports);
    return exports;
}
