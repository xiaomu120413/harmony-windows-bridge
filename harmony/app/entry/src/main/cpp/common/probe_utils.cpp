#include "common/probe_utils.h"

#include <dlfcn.h>

namespace rdp_bridge {

namespace {

std::string ExtractJsonString(const std::string& json, const std::string& key)
{
    const std::string marker = "\"" + key + "\":\"";
    const size_t start = json.find(marker);
    if (start == std::string::npos) {
        return "";
    }

    size_t valueStart = start + marker.size();
    std::string result;
    for (size_t i = valueStart; i < json.size(); ++i) {
        char c = json[i];
        if (c == '\\' && i + 1 < json.size()) {
            result.push_back(json[i + 1]);
            ++i;
            continue;
        }
        if (c == '"') {
            break;
        }
        result.push_back(c);
    }
    return result;
}

} // namespace

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

FreerdpProbeResult LoadFreerdpProbe()
{
    FreerdpProbeResult result;
    void* handle = dlopen("libfreerdp_ohos_probe.so", RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        const char* error = dlerror();
        result.error = error == nullptr ? "dlopen failed" : error;
        return result;
    }

    using ProbeFn = const char* (*)();
    dlerror();
    auto probe = reinterpret_cast<ProbeFn>(dlsym(handle, "freerdp_ohos_probe"));
    const char* symbolError = dlerror();
    if (symbolError != nullptr || probe == nullptr) {
        result.error = symbolError == nullptr ? "dlsym failed" : symbolError;
        dlclose(handle);
        return result;
    }

    const char* json = probe();
    result.json = json == nullptr ? "" : json;
    result.linked = !result.json.empty();
    result.freerdpVersion = ExtractJsonString(result.json, "freerdpVersion");
    result.winprVersion = ExtractJsonString(result.json, "winprVersion");
    result.opensslVersion = ExtractJsonString(result.json, "opensslVersion");
    if (result.freerdpVersion.empty()) {
        result.freerdpVersion = "unknown";
    }
    if (result.winprVersion.empty()) {
        result.winprVersion = "unknown";
    }
    if (result.opensslVersion.empty()) {
        result.opensslVersion = "unknown";
    }
    dlclose(handle);
    return result;
}

} // namespace rdp_bridge
