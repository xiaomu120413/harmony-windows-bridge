#include "napi/napi_exports.h"

#include "napi/native_api.h"
#include "napi/native_bridge_context.h"
#include "common/bridge_types.h"
#include "common/bridge_log.h"
#include "input/xcomponent_input_bridge.h"
#include "surface/xcomponent_native_host.h"
#include "napi/camera_permission_bridge.h"
#include "napi/clipboard_permission_bridge.h"
#include "napi/location_bridge.h"
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

napi_value ReleaseAllInput(napi_env env, napi_callback_info info)
{
    (void)info;
    ReleaseAllXComponentInput("arktsLifecycle");
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Released");
    SetString(env, result, "message", "all active XComponent input released");
    return result;
}

napi_value GetDiagnostics(napi_env env, napi_callback_info info)
{
    (void)info;
    return MakeString(env, BuildNativeDiagnostics());
}

napi_value AttachXComponentContent(napi_env env, napi_callback_info info)
{
    napi_value nodeContent = GetFirstArgument(env, info);
    std::string message;
    const bool ok = nodeContent != nullptr &&
        AttachNativeXComponentContent(env, nodeContent, message);
    if (nodeContent == nullptr) {
        message = "XComponent NodeContent is required";
    }
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Attached" : "Failed");
    SetString(env, result, "message", message);
    return result;
}

napi_value DetachXComponentContent(napi_env env, napi_callback_info info)
{
    (void)info;
    DetachNativeXComponentContent();
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Detached");
    SetString(env, result, "message", "native XComponent content detached");
    return result;
}

napi_value BindImeHostWindow(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    const bool hasWindowId = arg != nullptr && napi_typeof(env, arg, &type) == napi_ok &&
        type == napi_number;
    uint32_t windowId = 0;
    if (hasWindowId) {
        (void)napi_get_value_uint32(env, arg, &windowId);
    }
    std::string message;
    const bool ok = hasWindowId && rdp_bridge::BindImeHostWindow(windowId, message);
    if (!hasWindowId) {
        message = "IME host windowId must be a number";
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

using CompletePermissionRequestFn = bool (*)(uint32_t, bool);

struct PermissionRoute {
    const char* type;
    EventSink& (*sink)();
    CompletePermissionRequestFn complete;
};

const PermissionRoute kPermissionRoutes[] = {
    {"microphone", MicrophonePermissionRequestSink, CompleteMicrophonePermissionRequestFromUi},
    {"camera", CameraPermissionRequestSink, CompleteCameraPermissionRequestFromUi},
    {"clipboard", ClipboardPermissionRequestSink, CompleteClipboardPermissionRequestFromUi},
    {"location", LocationPermissionRequestSink, CompleteLocationPermissionRequestFromUi},
};

const PermissionRoute* FindPermissionRoute(const std::string& type)
{
    for (const PermissionRoute& route : kPermissionRoutes) {
        if (type == route.type) {
            return &route;
        }
    }
    return nullptr;
}

napi_value OnPermissionRequest(napi_env env, napi_callback_info info)
{
    napi_value callback = GetFirstArgument(env, info);
    bool ok = callback != nullptr;
    for (const PermissionRoute& route : kPermissionRoutes) {
        ok = ok && route.sink().Set(env, callback,
            "rdpPermissionRequestCallback", false, route.type);
    }
    if (!ok) {
        for (const PermissionRoute& route : kPermissionRoutes) {
            route.sink().Reset();
        }
        BridgeLogger::Error("unified permission callback registration failed");
    }
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Idle" : "Failed");
    SetString(env, result, "message", ok ? "permission callback registered" :
        "permission callback must be a function");
    return result;
}

napi_value CompletePermissionRequest(napi_env env, napi_callback_info info)
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
        SetString(env, result, "message", "permission completion requires an object argument");
        BridgeLogger::Error("permission completion parameter validation failed");
        return result;
    }

    const std::string typeName = GetStringProperty(env, arg, "type");
    const PermissionRoute* route = FindPermissionRoute(typeName);
    if (route == nullptr) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "unknown permission type: " + typeName);
        BridgeLogger::Error("unknown permission completion type: " + typeName);
        return result;
    }
    const uint32_t requestId = GetUint32Property(env, arg, "requestId");
    const bool granted = GetBoolProperty(env, arg, "granted");
    const bool ok = route->complete(requestId, granted);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Updated" : "Failed");
    SetString(env, result, "message", ok ? typeName + " permission result accepted" :
        typeName + " permission request is not pending");
    const std::string logLine = typeName + " permission completion requestId=" +
        std::to_string(requestId) +
        " granted=" + std::string(granted ? "true" : "false");
    if (!ok) {
        BridgeLogger::Error(logLine + " failed: request is not pending");
    }
    return result;
}

} // namespace

napi_value RegisterRdpNativeExports(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"bindImeHostWindow", nullptr, BindImeHostWindow, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"attachXComponentContent", nullptr, AttachXComponentContent, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"detachXComponentContent", nullptr, DetachXComponentContent, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"releaseAllInput", nullptr, ReleaseAllInput, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getDiagnostics", nullptr, GetDiagnostics, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onPermissionRequest", nullptr, OnPermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
        {"completePermissionRequest", nullptr, CompletePermissionRequest, nullptr, nullptr, nullptr,
            napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    InitializeNativeBridgeContext();
    return exports;
}
