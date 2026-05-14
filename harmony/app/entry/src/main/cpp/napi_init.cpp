#include "napi/native_api.h"

#include <string>
#include <vector>

namespace {

struct ConnectParams {
    std::string host;
    std::string port;
    std::string username;
    std::string resolution;
    std::string certPolicy;
};

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
    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.2.0");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", "not-linked");
    SetBool(env, result, "freeRdpLinked", false);
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "N-API bridge loaded",
        "FreeRDP dependency is pending M3 cross compile",
        "Native calls are available: probe, connect, disconnect"
    }));
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
    logs.push_back("RDP connect/auth loop will be implemented after FreeRDP is linked");

    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Bridge ready");
    SetString(env, result, "message", "native bridge accepted connection parameters");
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Disconnect(napi_env env, napi_callback_info info)
{
    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Disconnected");
    SetString(env, result, "message", "native bridge session closed");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "native disconnect invoked",
        "no FreeRDP session is active in M2"
    }));
    return result;
}

} // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"probe", nullptr, Probe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
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
