#include "napi/napi_exports.h"

#include "napi/native_api.h"

namespace {

static napi_value Init(napi_env env, napi_value exports)
{
    return RegisterRdpNativeExports(env, exports);
}

static napi_module rdpNativeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

} // namespace

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&rdpNativeModule);
}
