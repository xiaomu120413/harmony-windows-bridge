#include "xrdp/xrdp_server_bridge.h"

#include "common/bridge_log.h"

#include <atomic>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <dlfcn.h>
#include <mutex>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>
#include <vector>

namespace rdp_bridge {
namespace {

constexpr const char* kDefaultRuntimeRoot = "/data/storage/el2/base/files/xrdp";
constexpr const char* kServerLibraryName = "libxrdpserver.so";
constexpr const char* kBackendLibraryName = "libxrdpohos.so";
constexpr uint32_t kDefaultPort = 3390;

using XrdpMainFn = int (*)(int, char**);
using XrdpStopFn = int (*)(void);

struct XrdpLoadedServer {
    void* handle = nullptr;
    XrdpMainFn mainFn = nullptr;
    XrdpStopFn stopFn = nullptr;
    std::string libraryPath;
};

struct XrdpServerState {
    std::mutex mutex;
    XrdpLoadedServer loaded;
    std::atomic<bool> running { false };
    std::atomic<int> lastExitCode { 0 };
    std::string lastMessage;
};

struct XrdpResolvedPaths {
    std::string runtimeRoot;
    std::string nativeLibDir;
    std::string libraryPath;
    std::string libDir;
    std::string modulePath;
    std::string configPath;
    std::string sharePath;
    std::string pidPath;
    std::string logPath;
};

XrdpServerState& ServerState()
{
    static XrdpServerState state;
    return state;
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
    if (!params.runtimeRoot.empty()) {
        return TrimTrailingSlash(params.runtimeRoot);
    }
    if (!params.appFilesDir.empty()) {
        return JoinPath(TrimTrailingSlash(params.appFilesDir), "xrdp");
    }
    return kDefaultRuntimeRoot;
}

std::string ResolveHnpChild(const std::string& hnpRoot, const std::string& child)
{
    if (hnpRoot.empty()) {
        return "";
    }
    const std::string root = TrimTrailingSlash(hnpRoot);
    const std::string direct = JoinPath(root, child);
    if (PathExists(direct)) {
        return direct;
    }
    return JoinPath(JoinPath(root, "xrdp"), child);
}

XrdpResolvedPaths ResolvePaths(const XrdpServerParams& params)
{
    XrdpResolvedPaths paths;
    paths.runtimeRoot = ResolveRuntimeRoot(params);
    paths.nativeLibDir = ResolveNativeLibDir();
    paths.libDir = !params.libDir.empty() ? TrimTrailingSlash(params.libDir) : paths.nativeLibDir;

    const std::string nativeXrdpRoot = JoinPath(paths.nativeLibDir, "xrdp");
    const std::string hnpLibDir = ResolveHnpChild(params.hnpRoot, "lib");
    const std::string hnpConfigPath = ResolveHnpChild(params.hnpRoot, "config/xrdp.ini");
    const std::string hnpSharePath = ResolveHnpChild(params.hnpRoot, "share");

    paths.libraryPath = params.libraryPath;
    if (paths.libraryPath.empty() && !paths.libDir.empty()) {
        paths.libraryPath = JoinPath(paths.libDir, kServerLibraryName);
    }

    paths.modulePath = !params.modulePath.empty() ? TrimTrailingSlash(params.modulePath) : "";
    if (paths.modulePath.empty() && !paths.libDir.empty() && PathExists(JoinPath(paths.libDir, kBackendLibraryName))) {
        paths.modulePath = paths.libDir;
    }
    if (paths.modulePath.empty() && !hnpLibDir.empty()) {
        paths.modulePath = hnpLibDir;
    }
    if (paths.modulePath.empty()) {
        paths.modulePath = JoinPath(paths.runtimeRoot, "lib");
    }

    paths.configPath = params.configPath;
    if (paths.configPath.empty() && PathExists(JoinPath(nativeXrdpRoot, "config/xrdp.ini"))) {
        paths.configPath = JoinPath(nativeXrdpRoot, "config/xrdp.ini");
    }
    if (paths.configPath.empty() && PathExists(hnpConfigPath)) {
        paths.configPath = hnpConfigPath;
    }
    if (paths.configPath.empty()) {
        paths.configPath = JoinPath(paths.runtimeRoot, "config/xrdp.ini");
    }

    paths.sharePath = !params.sharePath.empty() ? TrimTrailingSlash(params.sharePath) : "";
    if (paths.sharePath.empty() && IsDirectory(JoinPath(nativeXrdpRoot, "share"))) {
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

std::vector<std::string> BuildLibraryCandidates(const XrdpServerParams& params, const XrdpResolvedPaths& paths)
{
    std::vector<std::string> candidates;
    AddUnique(candidates, params.libraryPath);
    AddUnique(candidates, paths.libraryPath);
    AddUnique(candidates, JoinPath(paths.nativeLibDir, kServerLibraryName));
    AddUnique(candidates, JoinPath(paths.modulePath, kServerLibraryName));
    AddUnique(candidates, JoinPath(JoinPath(paths.runtimeRoot, "lib"), kServerLibraryName));

    if (!params.hnpRoot.empty()) {
        AddUnique(candidates, ResolveHnpChild(params.hnpRoot, "lib/libxrdpserver.so"));
    }

    AddUnique(candidates, "/data/service/hnp/xrdp.org/xrdp_0.1.0/lib/libxrdpserver.so");
    AddUnique(candidates, "/data/app/bin/libxrdpserver.so");
    AddUnique(candidates, kServerLibraryName);
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

bool PrepareRuntime(const XrdpResolvedPaths& paths, std::vector<std::string>& logs)
{
    bool ok = true;
    ok = EnsureDirectory(paths.runtimeRoot, logs) && ok;
    ok = EnsureDirectory(paths.pidPath, logs) && ok;
    ok = EnsureDirectory(paths.logPath, logs) && ok;

    SetEnvPath("XRDP_CFG_PATH", DirName(paths.configPath), logs);
    SetEnvPath("XRDP_SHARE_PATH", paths.sharePath, logs);
    SetEnvPath("XRDP_MODULE_PATH", paths.modulePath, logs);
    SetEnvPath("XRDP_PID_PATH", paths.pidPath, logs);
    SetEnvPath("XRDP_LOG_PATH", paths.logPath, logs);
    return ok;
}

bool LoadServerLocked(const XrdpServerParams& params, const XrdpResolvedPaths& paths,
    XrdpServerCommandResult& result)
{
    XrdpServerState& state = ServerState();
    if (state.loaded.handle != nullptr && state.loaded.mainFn != nullptr && state.loaded.stopFn != nullptr) {
        result.libraryPath = state.loaded.libraryPath;
        result.logs.push_back("xrdp server library already loaded: " + state.loaded.libraryPath);
        return true;
    }

    for (const std::string& candidate : BuildLibraryCandidates(params, paths)) {
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

void FillPathResult(XrdpServerCommandResult& result, const XrdpResolvedPaths& paths)
{
    result.runtimeRoot = paths.runtimeRoot;
    result.configPath = paths.configPath;
    result.modulePath = paths.modulePath;
    result.logs.push_back("nativeLibDir=" + paths.nativeLibDir);
    result.logs.push_back("runtimeRoot=" + paths.runtimeRoot);
    result.logs.push_back("configPath=" + paths.configPath);
    result.logs.push_back("modulePath=" + paths.modulePath);
    result.logs.push_back("sharePath=" + paths.sharePath);
}

} // namespace

XrdpServerCommandResult ProbeXrdpServer(const XrdpServerParams& params)
{
    XrdpServerCommandResult result;
    const XrdpResolvedPaths paths = ResolvePaths(params);
    FillPathResult(result, paths);

    std::lock_guard<std::mutex> lock(ServerState().mutex);
    result.ok = LoadServerLocked(params, paths, result);
    result.state = result.ok ? (ServerState().running.load() ? "Listening" : "Ready") : "Failed";
    result.message = result.ok ? "xrdp embedded server is available" : "xrdp embedded server is not available";
    return result;
}

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params)
{
    XrdpServerCommandResult result;
    XrdpServerState& state = ServerState();
    const XrdpResolvedPaths paths = ResolvePaths(params);
    FillPathResult(result, paths);

    if (!PrepareRuntime(paths, result.logs)) {
        result.ok = false;
        result.state = "Failed";
        result.message = "xrdp runtime directories are not writable";
        EmitHilogError(result.message);
        return result;
    }
    if (!PathExists(paths.configPath)) {
        result.logs.push_back("xrdp.ini missing: " + paths.configPath);
    }
    if (!IsDirectory(paths.sharePath)) {
        result.logs.push_back("xrdp share directory missing: " + paths.sharePath);
    }
    if (!PathExists(JoinPath(paths.modulePath, kBackendLibraryName))) {
        result.logs.push_back("xrdp OHOS backend missing: " + JoinPath(paths.modulePath, kBackendLibraryName));
    }

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (state.running.load()) {
            result.ok = true;
            result.state = "Listening";
            result.message = "xrdp server is already running";
            result.libraryPath = state.loaded.libraryPath;
            return result;
        }
        if (!LoadServerLocked(params, paths, result)) {
            result.ok = false;
            result.state = "Failed";
            result.message = "xrdp embedded server library could not be loaded";
            EmitHilogError(result.message);
            return result;
        }
    }

    const uint32_t port = params.port == 0 ? kDefaultPort : params.port;
    std::vector<std::string> argvStorage = {
        "xrdp",
        "-n",
        "-c",
        paths.configPath,
        "-p",
        std::to_string(port)
    };

    XrdpMainFn mainFn = state.loaded.mainFn;
    state.running.store(true);
    state.lastMessage = "xrdp server thread starting";
    result.libraryPath = state.loaded.libraryPath;

    std::thread([mainFn, argvStorage]() mutable {
        std::vector<char*> argv;
        argv.reserve(argvStorage.size());
        for (std::string& arg : argvStorage) {
            argv.push_back(arg.data());
        }
        const int code = mainFn(static_cast<int>(argv.size()), argv.data());
        XrdpServerState& threadState = ServerState();
        threadState.lastExitCode.store(code);
        threadState.running.store(false);
        threadState.lastMessage = "xrdp server exited with code " + std::to_string(code);
        EmitHilogInfo(threadState.lastMessage);
    }).detach();

    result.ok = true;
    result.state = "Listening";
    result.message = "xrdp server start requested on port " + std::to_string(port);
    result.logs.push_back(result.message);
    EmitHilogInfo(result.message);
    return result;
}

XrdpServerCommandResult StopXrdpServer()
{
    XrdpServerCommandResult result;
    XrdpServerState& state = ServerState();

    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.loaded.stopFn == nullptr) {
        result.ok = !state.running.load();
        result.state = "Idle";
        result.message = "xrdp server library is not loaded";
        result.logs.push_back(result.message);
        return result;
    }

    const int status = state.loaded.stopFn();
    result.ok = status == 0;
    result.state = state.running.load() ? "Stopping" : "Idle";
    result.message = result.ok ? "xrdp server stop requested" : "xrdp server stop request failed";
    result.libraryPath = state.loaded.libraryPath;
    result.logs.push_back(result.message);
    result.logs.push_back("lastExitCode=" + std::to_string(state.lastExitCode.load()));
    if (!state.lastMessage.empty()) {
        result.logs.push_back(state.lastMessage);
    }
    EmitHilogInfo(result.message);
    return result;
}

} // namespace rdp_bridge
