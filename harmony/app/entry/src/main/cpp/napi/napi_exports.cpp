#include "napi/napi_exports.h"

#include "napi/native_api.h"
#include "napi/native_bridge_context.h"
#include "common/bridge_types.h"
#include "common/bridge_log.h"
#include "napi/camera_permission_bridge.h"
#include "napi/clipboard_permission_bridge.h"
#include "napi/location_bridge.h"
#include "napi/microphone_permission_bridge.h"
#include "napi/napi_event_sink.h"
#include "napi/napi_utils.h"
#include "xrdp/xrdp_server_bridge.h"

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
    params.accessCode = GetStringProperty(env, arg, "accessCode");
    params.accessCodeGateEnabled = GetBoolProperty(env, arg, "accessCodeGateEnabled");
    params.restartIfRunning = GetBoolProperty(env, arg, "restartIfRunning");
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
}

napi_value MakeXrdpServerResult(napi_env env, const XrdpServerCommandResult& command)
{
    napi_value result = MakeObject(env);
    SetXrdpCommonResult(env, result, command);
    return result;
}

void EmitDebugLogs(const std::vector<std::string>& logs)
{
    for (const std::string& line : logs) {
        BridgeLogger::Debug(line);
    }
}

napi_value Connect(napi_env env, napi_callback_info info)
{
    ConnectParams params = ReadConnectParams(env, info);

    napi_value result = MakeObject(env);
    if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "host, port, username, and password are required");
        BridgeLogger::Error("native connect parameter validation failed");
        return result;
    }

    std::string message;
    bool started = BridgeSession().Connect(params, message);
    if (!started) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", message);
        BridgeLogger::Error("native worker start failed: " + message);
        return result;
    }

    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Resolving");
    SetString(env, result, "message", message);
    return result;
}

napi_value EnsureXrdpServerStarted(napi_env env, napi_callback_info info)
{
    const XrdpServerParams params = ReadXrdpServerParams(env, info);
    XrdpServerCommandResult result = rdp_bridge::StartXrdpServer(params);
    if (!result.ok) {
        EmitDebugLogs(result.logs);
    }
    return MakeXrdpServerResult(env, result);
}

napi_value GetXrdpServerDiagnostics(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeXrdpServerResult(env, rdp_bridge::GetXrdpServerDiagnostics());
}

napi_value ReleaseAllKeys(napi_env env, napi_callback_info info)
{
    (void)info;
    std::string message;
    const bool ok = BridgeSession().ReleaseAllKeys(message);
    const bool noActiveSession = !ok && message == "no active FreeRDP session";
    if (!ok && !noActiveSession) {
        BridgeLogger::Error(message);
    }

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok || noActiveSession);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    return result;
}

napi_value ConfigureHostWindow(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    const bool hasHostWindow = arg != nullptr && napi_typeof(env, arg, &type) == napi_ok &&
        type == napi_object;
    const uint32_t windowId = hasHostWindow ? GetUint32Property(env, arg, "windowId") : 0;
    const uint32_t displayId = hasHostWindow ? GetUint32Property(env, arg, "displayId") : 0;
    std::string message;
    const bool ok = hasHostWindow && rdp_bridge::ConfigureHostWindow(windowId, displayId, message);
    if (!hasHostWindow) {
        message = "host window must be an object";
    }

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", message);
    return result;
}

napi_value RegisterCallback(napi_env env, napi_callback_info info, EventSink& sink, const char* name,
    bool mirrorToHilog = false, bool logRegistration = false)
{
    napi_value callback = GetFirstArgument(env, info);
    bool ok = callback != nullptr && sink.Set(env, callback, name, mirrorToHilog);

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Idle" : "Failed");
    SetString(env, result, "message", ok ? "callback registered" : "callback must be a function");
    if (ok) {
        if (logRegistration) {
            BridgeLogger::Debug(std::string(name) + " registered");
        }
    } else {
        BridgeLogger::Error(std::string(name) + " registration failed");
    }
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
        "rdpMicrophonePermissionRequestCallback");
}

napi_value OnCameraPermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, CameraPermissionRequestSink(),
        "rdpCameraPermissionRequestCallback");
}

napi_value OnClipboardPermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, ClipboardPermissionRequestSink(),
        "rdpClipboardPermissionRequestCallback");
}

napi_value OnLocationPermissionRequest(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, LocationPermissionRequestSink(),
        "rdpLocationPermissionRequestCallback");
}

using CompletePermissionRequestFn = bool (*)(uint32_t, bool);

napi_value CompletePermissionRequest(napi_env env, napi_callback_info info, const char* label,
    CompletePermissionRequestFn complete)
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
        SetString(env, result, "message",
            std::string(label) + " permission completion requires an object argument");
        BridgeLogger::Error(std::string(label) + " permission completion parameter validation failed");
        return result;
    }

    const uint32_t requestId = GetUint32Property(env, arg, "requestId");
    const bool granted = GetBoolProperty(env, arg, "granted");
    const bool ok = complete(requestId, granted);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", ok ? std::string(label) + " permission result accepted" :
        std::string(label) + " permission request is not pending");
    const std::string logLine = std::string(label) + " permission completion requestId=" +
        std::to_string(requestId) +
        " granted=" + std::string(granted ? "true" : "false");
    if (!ok) {
        BridgeLogger::Error(logLine + " failed: request is not pending");
    }
    return result;
}

napi_value CompleteClipboardPermissionRequest(napi_env env, napi_callback_info info)
{
    return CompletePermissionRequest(env, info, "clipboard", CompleteClipboardPermissionRequestFromUi);
}

napi_value CompleteLocationPermissionRequest(napi_env env, napi_callback_info info)
{
    return CompletePermissionRequest(env, info, "location", CompleteLocationPermissionRequestFromUi);
}

napi_value CompleteCameraPermissionRequest(napi_env env, napi_callback_info info)
{
    return CompletePermissionRequest(env, info, "camera", CompleteCameraPermissionRequestFromUi);
}

napi_value CompleteMicrophonePermissionRequest(napi_env env, napi_callback_info info)
{
    return CompletePermissionRequest(env, info, "microphone", CompleteMicrophonePermissionRequestFromUi);
}

} // namespace

napi_value RegisterRdpNativeExports(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"ensureXrdpServerStarted", nullptr, EnsureXrdpServerStarted, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getXrdpServerDiagnostics", nullptr, GetXrdpServerDiagnostics, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"configureHostWindow", nullptr, ConfigureHostWindow, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"releaseAllKeys", nullptr, ReleaseAllKeys, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onMicrophonePermissionRequest", nullptr, OnMicrophonePermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onCameraPermissionRequest", nullptr, OnCameraPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onClipboardPermissionRequest", nullptr, OnClipboardPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onLocationPermissionRequest", nullptr, OnLocationPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"completeClipboardPermissionRequest", nullptr, CompleteClipboardPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeMicrophonePermissionRequest", nullptr, CompleteMicrophonePermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeCameraPermissionRequest", nullptr, CompleteCameraPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeLocationPermissionRequest", nullptr, CompleteLocationPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    InitializeNativeBridgeContext();
    RegisterNativeXComponent(env, exports);
    return exports;
}
