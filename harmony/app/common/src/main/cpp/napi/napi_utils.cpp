#include "napi/napi_utils.h"

namespace rdp_bridge {

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

napi_value MakeUint64(napi_env env, uint64_t value)
{
    napi_value result = nullptr;
    napi_create_double(env, static_cast<double>(value), &result);
    return result;
}

napi_value MakeObject(napi_env env)
{
    napi_value result = nullptr;
    napi_create_object(env, &result);
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

void SetUint64(napi_env env, napi_value object, const char* name, uint64_t value)
{
    SetNamed(env, object, name, MakeUint64(env, value));
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

uint32_t GetUint32Property(napi_env env, napi_value object, const char* name, uint32_t fallback)
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

int32_t GetInt32Property(napi_env env, napi_value object, const char* name, int32_t fallback)
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

    int32_t out = fallback;
    napi_get_value_int32(env, value, &out);
    return out;
}

bool GetBoolProperty(napi_env env, napi_value object, const char* name, bool fallback)
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

} // namespace rdp_bridge
