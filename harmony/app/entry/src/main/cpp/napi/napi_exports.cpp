#include "napi/napi_exports.h"

#include "napi/native_api.h"
#include "napi/native_bridge_context.h"
#include "common/bridge_types.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/graphics_config.h"
#include "napi/clipboard_permission_bridge.h"
#include "napi/microphone_permission_bridge.h"
#include "napi/napi_event_sink.h"
#include "napi/napi_utils.h"

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
    const std::string graphicsModeError = GraphicsModeValidationError(params.graphicsMode);
    if (!graphicsModeError.empty()) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", graphicsModeError);
        logs.push_back("graphics mode validation failed");
        logs.push_back(graphicsModeError);
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    logs.push_back(RedactedEndpointLog(params));
    logs.push_back(RedactedValueLog("username", params.username));
    logs.push_back("resolution=" + params.resolution);
    logs.push_back("certPolicy=" + params.certPolicy);
    logs.push_back("graphicsMode=" + params.graphicsMode);
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

napi_value ReleaseAllKeys(napi_env env, napi_callback_info info)
{
    (void)info;
    std::vector<std::string> logs = {"native release all keys invoked"};
    std::string message;
    const bool ok = BridgeSession().ReleaseAllKeys(message);
    BridgeEvents().log.Emit(message);

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

napi_value OnLog(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, BridgeEvents().log, "rdpLogCallback", true);
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
        {"releaseAllKeys", nullptr, ReleaseAllKeys, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onLog", nullptr, OnLog, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onMicrophonePermissionRequest", nullptr, OnMicrophonePermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"onClipboardPermissionRequest", nullptr, OnClipboardPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"completeClipboardPermissionRequest", nullptr, CompleteClipboardPermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
        {"completeMicrophonePermissionRequest", nullptr, CompleteMicrophonePermissionRequest, nullptr, nullptr,
            nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    InitializeNativeBridgeContext();
    RegisterNativeXComponent(env, exports);
    return exports;
}
