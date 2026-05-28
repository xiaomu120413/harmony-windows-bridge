#include "xrdp/xrdp_server_internal.h"

#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <dlfcn.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace rdp_bridge {
namespace xrdp_bridge_internal {
namespace {

constexpr const char* kDefaultHnpRoot = "/data/service/hnp/xrdp.org/xrdp_0.1.0";

std::string HexFlags(uint32_t flags)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << flags;
    return stream.str();
}

std::string FormatBackendAbi(const xrdp_ohos_abi_info& info)
{
    return "api=" + std::to_string(info.api_version) +
        " mod=" + std::to_string(info.mod_version) +
        " inputEvent=" + std::to_string(info.input_event_version) +
        " backendEvent=" + std::to_string(info.backend_event_version) +
        " features=" + HexFlags(info.feature_flags);
}

bool BackendHandlesInputDirectly(bool abiInfoValid, const xrdp_ohos_abi_info& info)
{
    return abiInfoValid && (info.feature_flags & XRDP_OHOS_FEATURE_DIRECT_INPUT) != 0U;
}

bool BackendOwnsCaptureDirectly(bool abiInfoValid, const xrdp_ohos_abi_info& info)
{
    constexpr uint32_t requiredFlags =
        XRDP_OHOS_FEATURE_INTERNAL_CAPTURE | XRDP_OHOS_FEATURE_CAPTURE_DIAGNOSTICS;
    return abiInfoValid && (info.feature_flags & requiredFlags) == requiredFlags;
}

bool IsAbsolutePath(const std::string& value)
{
    return !value.empty() && value[0] == '/';
}

std::string TrimTrailingSlash(std::string value)
{
    while (value.size() > 1 && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

std::string DirName(const std::string& path)
{
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return "";
    }
    if (slash == 0) {
        return "/";
    }
    return path.substr(0, slash);
}

bool EnsureDirectory(const std::string& path, std::vector<std::string>& logs)
{
    if (path.empty() || IsDirectory(path)) {
        return true;
    }

    std::string current;
    size_t index = 0;
    if (path[0] == '/') {
        current = "/";
        index = 1;
    }

    while (index < path.size()) {
        const size_t slash = path.find('/', index);
        const std::string part = path.substr(index, slash == std::string::npos ? std::string::npos : slash - index);
        if (!part.empty()) {
            current = current == "/" ? current + part : JoinPath(current, part);
            if (!IsDirectory(current)) {
                if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) {
                    logs.push_back("mkdir failed: " + current + " error=" + std::strerror(errno));
                    return false;
                }
            }
        }
        if (slash == std::string::npos) {
            break;
        }
        index = slash + 1;
    }
    return true;
}

void AddUnique(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty()) {
        return;
    }
    for (const std::string& existing : values) {
        if (existing == value) {
            return;
        }
    }
    values.push_back(value);
}

std::string ResolveNativeLibDir()
{
    Dl_info info {};
    if (dladdr(reinterpret_cast<void*>(&ResolveNativeLibDir), &info) != 0 && info.dli_fname != nullptr) {
        return DirName(info.dli_fname);
    }
    return "";
}

std::string ResolveRuntimeRoot(const XrdpServerParams& params)
{
    if (!params.appFilesDir.empty()) {
        return JoinPath(TrimTrailingSlash(params.appFilesDir), "xrdp");
    }
    return kDefaultRuntimeRoot;
}

std::string ResolveDefaultHnpChild(const std::string& child)
{
    const std::string direct = JoinPath(kDefaultHnpRoot, child);
    if (PathExists(direct)) {
        return direct;
    }
    return JoinPath(JoinPath(kDefaultHnpRoot, "xrdp"), child);
}

std::vector<std::string> BuildLibraryCandidates(const XrdpResolvedPaths& paths)
{
    std::vector<std::string> candidates;
    AddUnique(candidates, paths.libraryPath);
    AddUnique(candidates, JoinPath(paths.nativeLibDir, kServerLibraryName));
    AddUnique(candidates, JoinPath(paths.modulePath, kServerLibraryName));
    AddUnique(candidates, JoinPath(JoinPath(paths.runtimeRoot, "lib"), kServerLibraryName));
    AddUnique(candidates, ResolveDefaultHnpChild("lib/libxrdpserver.so"));
    AddUnique(candidates, "/data/app/bin/libxrdpserver.so");
    AddUnique(candidates, kServerLibraryName);
    return candidates;
}

std::vector<std::string> BuildBackendCandidates(const XrdpResolvedPaths& paths)
{
    std::vector<std::string> candidates;
    AddUnique(candidates, JoinPath(paths.modulePath, kBackendLibraryName));
    AddUnique(candidates, JoinPath(paths.nativeLibDir, kBackendLibraryName));
    AddUnique(candidates, JoinPath(JoinPath(paths.runtimeRoot, "lib"), kBackendLibraryName));
    AddUnique(candidates, ResolveDefaultHnpChild("lib/libxrdpohos.so"));
    AddUnique(candidates, kBackendLibraryName);
    return candidates;
}

void SetEnvPath(const char* name, const std::string& value, std::vector<std::string>& logs)
{
    if (value.empty()) {
        return;
    }
    if (setenv(name, value.c_str(), 1) != 0) {
        logs.push_back(std::string("setenv failed: ") + name + " error=" + std::strerror(errno));
    } else {
        logs.push_back(std::string(name) + "=" + value);
    }
}

} // namespace

std::string JoinPath(const std::string& left, const std::string& right)
{
    if (left.empty()) {
        return right;
    }
    if (right.empty()) {
        return left;
    }
    if (IsAbsolutePath(right)) {
        return right;
    }
    if (left.back() == '/') {
        return left + right;
    }
    return left + "/" + right;
}

bool PathExists(const std::string& path)
{
    struct stat st {};
    return !path.empty() && stat(path.c_str(), &st) == 0;
}

bool IsDirectory(const std::string& path)
{
    struct stat st {};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

XrdpResolvedPaths ResolvePaths(const XrdpServerParams& params)
{
    XrdpResolvedPaths paths;
    paths.runtimeRoot = ResolveRuntimeRoot(params);
    paths.nativeLibDir = ResolveNativeLibDir();
    paths.libDir = paths.nativeLibDir;

    const std::string nativeXrdpRoot = JoinPath(paths.nativeLibDir, "xrdp");
    const std::string hnpLibDirCandidate = ResolveDefaultHnpChild("lib");
    const std::string hnpLibDir = IsDirectory(hnpLibDirCandidate) ? hnpLibDirCandidate : "";
    const std::string hnpConfigPath = ResolveDefaultHnpChild("config/xrdp.ini");
    const std::string hnpSharePath = ResolveDefaultHnpChild("share");

    if (!paths.libDir.empty()) {
        paths.libraryPath = JoinPath(paths.libDir, kServerLibraryName);
    }

    if (!paths.libDir.empty() && PathExists(JoinPath(paths.libDir, kBackendLibraryName))) {
        paths.modulePath = paths.libDir;
    }
    if (paths.modulePath.empty() && !hnpLibDir.empty()) {
        paths.modulePath = hnpLibDir;
    }
    if (paths.modulePath.empty()) {
        paths.modulePath = JoinPath(paths.runtimeRoot, "lib");
    }

    if (PathExists(JoinPath(nativeXrdpRoot, "config/xrdp.ini"))) {
        paths.packagedConfigPath = JoinPath(nativeXrdpRoot, "config/xrdp.ini");
    }
    if (paths.packagedConfigPath.empty() && PathExists(hnpConfigPath)) {
        paths.packagedConfigPath = hnpConfigPath;
    }
    paths.configPath = JoinPath(paths.runtimeRoot, "config/xrdp.ini");
    paths.tlsCertificatePath = JoinPath(paths.runtimeRoot, "config/cert.pem");
    paths.tlsKeyPath = JoinPath(paths.runtimeRoot, "config/key.pem");

    if (IsDirectory(JoinPath(nativeXrdpRoot, "share"))) {
        paths.sharePath = JoinPath(nativeXrdpRoot, "share");
    }
    if (paths.sharePath.empty() && IsDirectory(hnpSharePath)) {
        paths.sharePath = hnpSharePath;
    }
    if (paths.sharePath.empty()) {
        paths.sharePath = JoinPath(paths.runtimeRoot, "share");
    }

    paths.pidPath = JoinPath(paths.runtimeRoot, "run");
    paths.logPath = JoinPath(paths.runtimeRoot, "log");
    return paths;
}

bool PrepareRuntime(const XrdpResolvedPaths& paths, std::vector<std::string>& logs)
{
    bool ok = true;
    ok = EnsureDirectory(paths.runtimeRoot, logs) && ok;
    ok = EnsureDirectory(DirName(paths.configPath), logs) && ok;
    ok = EnsureDirectory(paths.pidPath, logs) && ok;
    ok = EnsureDirectory(paths.logPath, logs) && ok;

    SetEnvPath("XRDP_CFG_PATH", DirName(paths.configPath), logs);
    SetEnvPath("XRDP_SHARE_PATH", paths.sharePath, logs);
    SetEnvPath("XRDP_MODULE_PATH", paths.modulePath, logs);
    SetEnvPath("XRDP_PID_PATH", paths.pidPath, logs);
    SetEnvPath("XRDP_LOG_PATH", paths.logPath, logs);
    return ok;
}

bool LoadServerLocked(const XrdpResolvedPaths& paths, XrdpServerCommandResult& result)
{
    XrdpServerState& state = ServerState();
    if (state.loaded.handle != nullptr && state.loaded.mainFn != nullptr && state.loaded.stopFn != nullptr) {
        result.libraryPath = state.loaded.libraryPath;
        result.logs.push_back("xrdp server library already loaded: " + state.loaded.libraryPath);
        return true;
    }

    for (const std::string& candidate : BuildLibraryCandidates(paths)) {
        if (candidate != kServerLibraryName && !PathExists(candidate)) {
            result.logs.push_back("xrdp server library candidate missing: " + candidate);
            continue;
        }

        dlerror();
        void* handle = dlopen(candidate.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (handle == nullptr) {
            const char* error = dlerror();
            result.logs.push_back("dlopen failed: " + candidate + " error=" + (error == nullptr ? "unknown" : error));
            continue;
        }

        auto mainFn = reinterpret_cast<XrdpMainFn>(dlsym(handle, "xrdp_ohos_server_main"));
        auto stopFn = reinterpret_cast<XrdpStopFn>(dlsym(handle, "xrdp_ohos_server_stop"));
        if (mainFn == nullptr || stopFn == nullptr) {
            result.logs.push_back("xrdp embedded symbols missing in: " + candidate);
            dlclose(handle);
            continue;
        }

        state.loaded.handle = handle;
        state.loaded.mainFn = mainFn;
        state.loaded.stopFn = stopFn;
        state.loaded.libraryPath = candidate;
        result.libraryPath = candidate;
        result.logs.push_back("xrdp server library loaded: " + candidate);
        return true;
    }

    result.logs.push_back("xrdp server library was not found");
    return false;
}

bool LoadBackendLocked(const XrdpResolvedPaths& paths, XrdpServerCommandResult& result)
{
    XrdpServerState& state = ServerState();
    if (state.backend.handle != nullptr && state.backend.getAbiInfoFn != nullptr) {
        result.logs.push_back("xrdp OHOS backend already loaded: " + state.backend.libraryPath);
        if (state.backend.abiInfoValid) {
            result.logs.push_back("xrdp OHOS backend ABI: " + FormatBackendAbi(state.backend.abiInfo));
        } else {
            result.logs.push_back("xrdp OHOS backend ABI unavailable");
        }
        result.logs.push_back(std::string("xrdp OHOS capture diagnostics ") +
            (state.backend.captureDiagnosticsFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS capture frame submit ") +
            (state.backend.captureSubmitFrameFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS backend direct input ") +
            (BackendHandlesInputDirectly(state.backend.abiInfoValid, state.backend.abiInfo) ?
                "enabled" : "unavailable"));
        result.logs.push_back(std::string("xrdp OHOS input authorization prime ") +
            (state.backend.primeInputAuthorizationFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS internal capture ") +
            (BackendOwnsCaptureDirectly(state.backend.abiInfoValid, state.backend.abiInfo) ?
                "enabled" : "unavailable"));
        if (state.backend.setEventCallbackFn != nullptr) {
            const int rc = state.backend.setEventCallbackFn(OnXrdpBackendEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend event callback register rc=" + std::to_string(rc));
        }
        if (state.backend.primeInputAuthorizationFn != nullptr) {
            const int rc = state.backend.primeInputAuthorizationFn("xrdp server ensure");
            result.logs.push_back("xrdp OHOS input authorization prime rc=" + std::to_string(rc));
        }
        return true;
    }

    for (const std::string& candidate : BuildBackendCandidates(paths)) {
        if (candidate != kBackendLibraryName && !PathExists(candidate)) {
            result.logs.push_back("xrdp OHOS backend candidate missing: " + candidate);
            continue;
        }

        dlerror();
        void* handle = dlopen(candidate.c_str(), RTLD_NOW | RTLD_GLOBAL);
        if (handle == nullptr) {
            const char* error = dlerror();
            result.logs.push_back("dlopen backend failed: " + candidate + " error=" +
                (error == nullptr ? "unknown" : error));
            continue;
        }

        auto getAbiInfoFn = reinterpret_cast<XrdpGetAbiInfoFn>(
            dlsym(handle, "xrdp_ohos_backend_get_abi_info"));
        if (getAbiInfoFn == nullptr) {
            result.logs.push_back("xrdp OHOS backend ABI symbol missing in: " + candidate);
            dlclose(handle);
            continue;
        }
        auto setEventCallbackFn = reinterpret_cast<XrdpSetBackendEventCallbackFn>(
            dlsym(handle, "xrdp_ohos_backend_set_event_callback"));
        auto captureDiagnosticsFn = reinterpret_cast<XrdpCaptureGetDiagnosticsFn>(
            dlsym(handle, "xrdp_ohos_capture_get_diagnostics"));
        auto captureSubmitFrameFn = reinterpret_cast<XrdpCaptureSubmitFrameFn>(
            dlsym(handle, "xrdp_ohos_capture_submit_frame"));
        auto captureResetFn = reinterpret_cast<XrdpCaptureResetFn>(
            dlsym(handle, "xrdp_ohos_capture_reset"));
        auto primeInputAuthorizationFn = reinterpret_cast<XrdpPrimeInputAuthorizationFn>(
            dlsym(handle, "xrdp_ohos_backend_prime_input_authorization"));

        xrdp_ohos_abi_info abiInfo {};
        bool abiInfoValid = false;
        abiInfo.size = sizeof(abiInfo);
        const int abiRc = getAbiInfoFn(&abiInfo);
        if (abiRc != XRDP_OHOS_BACKEND_STATUS_OK ||
            abiInfo.api_version != XRDP_OHOS_API_VERSION ||
            abiInfo.mod_version != XRDP_OHOS_MOD_VERSION) {
            result.logs.push_back("xrdp OHOS backend ABI mismatch in: " + candidate +
                " rc=" + std::to_string(abiRc) +
                " " + FormatBackendAbi(abiInfo));
            dlclose(handle);
            continue;
        }
        abiInfoValid = true;
        if (!BackendOwnsCaptureDirectly(abiInfoValid, abiInfo) ||
            captureDiagnosticsFn == nullptr || captureSubmitFrameFn == nullptr || captureResetFn == nullptr) {
            result.logs.push_back("xrdp OHOS backend capture facade unavailable in: " + candidate +
                " " + FormatBackendAbi(abiInfo) +
                " diagnostics=" + std::to_string(captureDiagnosticsFn != nullptr) +
                " submit=" + std::to_string(captureSubmitFrameFn != nullptr) +
                " reset=" + std::to_string(captureResetFn != nullptr));
            dlclose(handle);
            continue;
        }

        state.backend.handle = handle;
        state.backend.getAbiInfoFn = getAbiInfoFn;
        state.backend.setEventCallbackFn = setEventCallbackFn;
        state.backend.captureDiagnosticsFn = captureDiagnosticsFn;
        state.backend.captureSubmitFrameFn = captureSubmitFrameFn;
        state.backend.captureResetFn = captureResetFn;
        state.backend.primeInputAuthorizationFn = primeInputAuthorizationFn;
        state.backend.abiInfo = abiInfo;
        state.backend.abiInfoValid = abiInfoValid;
        state.backend.libraryPath = candidate;
        result.logs.push_back("xrdp OHOS backend loaded: " + candidate);
        result.logs.push_back("xrdp OHOS backend ABI: " + FormatBackendAbi(abiInfo));
        result.logs.push_back(std::string("xrdp OHOS capture diagnostics ") +
            (captureDiagnosticsFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS capture frame submit ") +
            (captureSubmitFrameFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS capture reset ") +
            (captureResetFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS backend direct input ") +
            (BackendHandlesInputDirectly(abiInfoValid, abiInfo) ? "enabled" : "unavailable"));
        result.logs.push_back(std::string("xrdp OHOS input authorization prime ") +
            (primeInputAuthorizationFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS internal capture ") +
            (BackendOwnsCaptureDirectly(abiInfoValid, abiInfo) ? "enabled" : "unavailable"));
        if (setEventCallbackFn != nullptr) {
            const int rc = setEventCallbackFn(OnXrdpBackendEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend event callback register rc=" + std::to_string(rc));
        } else {
            result.logs.push_back("xrdp OHOS backend event callback symbol missing in: " + candidate);
        }
        if (primeInputAuthorizationFn != nullptr) {
            const int rc = primeInputAuthorizationFn("xrdp server start");
            result.logs.push_back("xrdp OHOS input authorization prime rc=" + std::to_string(rc));
        }
        return true;
    }

    result.logs.push_back("xrdp OHOS backend was not found");
    return false;
}

void FillPathResult(XrdpServerCommandResult& result, const XrdpResolvedPaths& paths)
{
    result.runtimeRoot = paths.runtimeRoot;
    result.configPath = paths.configPath;
    result.modulePath = paths.modulePath;
    result.logPath = paths.logPath;
    result.logs.push_back("nativeLibDir=" + paths.nativeLibDir);
    result.logs.push_back("runtimeRoot=" + paths.runtimeRoot);
    result.logs.push_back("configPath=" + paths.configPath);
    if (!paths.packagedConfigPath.empty()) {
        result.logs.push_back("packagedConfigPath=" + paths.packagedConfigPath);
    }
    result.logs.push_back("tlsCertificatePath=" + paths.tlsCertificatePath);
    result.logs.push_back("modulePath=" + paths.modulePath);
    result.logs.push_back("sharePath=" + paths.sharePath);
    result.logs.push_back("logPath=" + paths.logPath);
}

} // namespace xrdp_bridge_internal
} // namespace rdp_bridge
