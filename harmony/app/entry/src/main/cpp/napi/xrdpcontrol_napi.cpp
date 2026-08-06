#include "napi/native_api.h"
#include "xrdp/xrdp_server_bridge.h"

#include <string>

namespace {

std::string GetStringProperty(napi_env env, napi_value object, const char* name)
{
    napi_value value = nullptr;
    if (napi_get_named_property(env, object, name, &value) != napi_ok || value == nullptr) {
        return "";
    }
    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return "";
    }
    std::string result(length + 1, '\0');
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, value, result.data(), length + 1, &copied) != napi_ok) {
        return "";
    }
    result.resize(copied);
    return result;
}

bool GetBoolProperty(napi_env env, napi_value object, const char* name)
{
    napi_value value = nullptr;
    bool result = false;
    return napi_get_named_property(env, object, name, &value) == napi_ok && value != nullptr &&
        napi_get_value_bool(env, value, &result) == napi_ok && result;
}

void SetString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value nativeValue = nullptr;
    napi_create_string_utf8(env, value.c_str(), value.size(), &nativeValue);
    napi_set_named_property(env, object, name, nativeValue);
}

void SetBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value nativeValue = nullptr;
    napi_get_boolean(env, value, &nativeValue);
    napi_set_named_property(env, object, name, nativeValue);
}

void SetInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value nativeValue = nullptr;
    napi_create_int32(env, value, &nativeValue);
    napi_set_named_property(env, object, name, nativeValue);
}

napi_value ToJsResult(napi_env env, const rdp_bridge::XrdpServerCommandResult& result)
{
    napi_value object = nullptr;
    napi_create_object(env, &object);
    SetBool(env, object, "ok", result.ok);
    SetString(env, object, "state", result.state);
    SetString(env, object, "message", result.message);
    SetInt32(env, object, "pid", result.pid);
    SetInt32(env, object, "port", static_cast<int32_t>(result.port));
    SetInt32(env, object, "lastExitCode", result.lastExitCode);
    return object;
}

napi_value FirstArgument(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);
    return argc > 0 ? argv[0] : nullptr;
}

napi_value Start(napi_env env, napi_callback_info info)
{
    napi_value arg = FirstArgument(env, info);
    rdp_bridge::XrdpServerParams params;
    if (arg != nullptr) {
        params.appFilesDir = GetStringProperty(env, arg, "appFilesDir");
        params.accessCode = GetStringProperty(env, arg, "accessCode");
        params.accessCodeGateEnabled = GetBoolProperty(env, arg, "accessCodeGateEnabled");
        params.restartIfRunning = GetBoolProperty(env, arg, "restartIfRunning");
    }
    return ToJsResult(env, rdp_bridge::StartXrdpServer(params));
}

napi_value Diagnostics(napi_env env, napi_callback_info info)
{
    (void)info;
    return ToJsResult(env, rdp_bridge::GetXrdpServerDiagnostics());
}

napi_value Stop(napi_env env, napi_callback_info info)
{
    napi_value arg = FirstArgument(env, info);
    std::string reason;
    if (arg != nullptr) {
        size_t length = 0;
        if (napi_get_value_string_utf8(env, arg, nullptr, 0, &length) == napi_ok) {
            reason.resize(length + 1);
            size_t copied = 0;
            (void)napi_get_value_string_utf8(env, arg, reason.data(), length + 1, &copied);
            reason.resize(copied);
        }
    }
    return ToJsResult(env, rdp_bridge::StopXrdpServer(reason));
}

napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor properties[] = {
        {"start", nullptr, Start, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"diagnostics", nullptr, Diagnostics, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"stop", nullptr, Stop, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(properties) / sizeof(properties[0]), properties);
    return exports;
}

static napi_module controlModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "xrdpcontrol",
    .nm_priv = nullptr,
    .reserved = {0},
};

} // namespace

extern "C" __attribute__((constructor)) void RegisterXrdpControlModule(void)
{
    napi_module_register(&controlModule);
}
