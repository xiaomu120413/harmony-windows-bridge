#pragma once

#include "napi/native_api.h"

#include <cstdint>
#include <string>
#include <vector>

namespace rdp_bridge {

napi_value MakeString(napi_env env, const std::string& value);
napi_value MakeBool(napi_env env, bool value);
napi_value MakeUint32(napi_env env, uint32_t value);
napi_value MakeObject(napi_env env);
napi_value MakeStringArray(napi_env env, const std::vector<std::string>& values);

void SetNamed(napi_env env, napi_value object, const char* name, napi_value value);
void SetString(napi_env env, napi_value object, const char* name, const std::string& value);
void SetBool(napi_env env, napi_value object, const char* name, bool value);
void SetUint32(napi_env env, napi_value object, const char* name, uint32_t value);

std::string GetStringProperty(napi_env env, napi_value object, const char* name);
uint32_t GetUint32Property(napi_env env, napi_value object, const char* name, uint32_t fallback = 0);
bool GetBoolProperty(napi_env env, napi_value object, const char* name, bool fallback = false);
napi_value GetFirstArgument(napi_env env, napi_callback_info info);

} // namespace rdp_bridge
