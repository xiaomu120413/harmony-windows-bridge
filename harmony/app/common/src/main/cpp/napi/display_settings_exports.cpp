#include "napi/display_settings_exports.h"
#include "napi/native_bridge_context.h"
#include "napi/napi_utils.h"
#include <cmath>

namespace rdp_bridge {
namespace {
napi_value Get(napi_env env, napi_callback_info)
{
    const auto state = DisplaySettings().Snapshot();
    auto value = MakeObject(env);
    SetBool(env, value, "connected", BridgeSession().IsConnected());
    SetBool(env, value, "multimon", state.multimon);
    SetBool(env, value, "pending", state.pending);
    SetString(env, value, "mode", state.fixed ? "fixed" : "follow");
    SetString(env, value, "status", state.status);
    SetUint32(env, value, "requestedWidth", state.requestedWidth);
    SetUint32(env, value, "requestedHeight", state.requestedHeight);
    SetUint32(env, value, "actualWidth", state.actualWidth);
    SetUint32(env, value, "actualHeight", state.actualHeight);
    return value;
}
napi_value Command(napi_env env, bool ok, const std::string& message)
{
    auto value = MakeObject(env);
    SetBool(env, value, "ok", ok);
    SetString(env, value, "state", ok ? "Accepted" : "Failed");
    SetString(env, value, "message", message);
    return value;
}
bool ReadDimension(napi_env env, napi_value value, uint32_t& result)
{
    double number = 0;
    if (!value || napi_get_value_double(env, value, &number) != napi_ok ||
        !std::isfinite(number) || number < 0 || number > 4096 || std::floor(number) != number) { return false; }
    result = static_cast<uint32_t>(number);
    return true;
}
napi_value Set(napi_env env, napi_callback_info info)
{
    size_t count = 3;
    napi_value args[3] = {};
    napi_get_cb_info(env, info, &count, args, nullptr, nullptr);
    char mode[16] = {};
    size_t length = 0;
    uint32_t width = 0, height = 0;
    if (count != 3 || napi_get_value_string_utf8(env, args[0], mode, sizeof(mode), &length) != napi_ok ||
        (std::string(mode) != "follow" && std::string(mode) != "fixed") ||
        !ReadDimension(env, args[1], width) || !ReadDimension(env, args[2], height)) {
        return Command(env, false, "分辨率参数无效");
    }
    std::string message;
    const bool ok = SetSessionDisplayResolution(std::string(mode) == "fixed", width, height, message);
    return Command(env, ok, message);
}
napi_value Refresh(napi_env env, napi_callback_info)
{
    std::string message;
    const bool ok = RefreshSessionDisplay(message);
    return Command(env, ok, message);
}
}
void RegisterDisplaySettingsExports(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"getDisplaySettings", nullptr, Get, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setDisplayResolution", nullptr, Set, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"refreshDisplay", nullptr, Refresh, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
}
}
