#include "xrdp/xrdp_server_internal.h"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <vector>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <unistd.h>

#include <hilog/log.h>

extern char** environ;

namespace rdp_bridge {
using namespace xrdp_bridge_internal;
namespace {

constexpr unsigned int kLogDomain = 0xD002D00;
constexpr const char* kLogTag = "MuHubXrdpControl";

void LogInfo(const std::string& message)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, kLogDomain, kLogTag, "%{public}s", message.c_str());
}

void RefreshProcessLocked(XrdpServerState& state)
{
    if (state.pid <= 0) {
        return;
    }
    int status = 0;
    const pid_t waited = waitpid(state.pid, &status, WNOHANG);
    if (waited == 0) {
        return;
    }
    if (waited < 0) {
        if (errno != ECHILD) {
            state.lastMessage = "waitpid failed: " + std::string(std::strerror(errno));
        }
        state.pid = -1;
        return;
    }
    state.lastExitCode = WIFEXITED(status) ? WEXITSTATUS(status) :
        (WIFSIGNALED(status) ? 128 + WTERMSIG(status) : status);
    state.lastMessage = "xrdp process exited with code " + std::to_string(state.lastExitCode);
    state.pid = -1;
    LogInfo(state.lastMessage);
}

XrdpServerCommandResult SnapshotLocked(XrdpServerState& state)
{
    RefreshProcessLocked(state);
    XrdpServerCommandResult result;
    result.pid = state.pid > 0 ? static_cast<int32_t>(state.pid) : 0;
    result.lastExitCode = state.lastExitCode;
    result.port = kDefaultPort;
    result.ok = state.pid > 0;
    result.state = state.pid > 0 ? "Listening" :
        (state.lastExitCode == 0 ? "Stopped" : "Exited");
    result.message = state.lastMessage;
    return result;
}

bool StopLocked(XrdpServerState& state, const std::string& reason,
    XrdpServerCommandResult& result)
{
    RefreshProcessLocked(state);
    if (state.pid <= 0) {
        result = SnapshotLocked(state);
        result.state = "Stopped";
        result.message = "xrdp process is not running";
        return true;
    }
    const pid_t pid = state.pid;
    if (kill(pid, SIGTERM) != 0 && errno != ESRCH) {
        result.ok = false;
        result.state = "Failed";
        result.message = "xrdp SIGTERM failed: " + std::string(std::strerror(errno));
        return false;
    }
    for (int attempt = 0; attempt < 30; attempt++) {
        int status = 0;
        const pid_t waited = waitpid(pid, &status, WNOHANG);
        if (waited == pid || (waited < 0 && errno == ECHILD)) {
            state.pid = -1;
            state.lastExitCode = waited == pid && WIFEXITED(status) ? WEXITSTATUS(status) : 0;
            state.lastMessage = "xrdp process stopped: " + reason;
            result = SnapshotLocked(state);
            result.state = "Stopped";
            result.message = state.lastMessage;
            LogInfo(state.lastMessage);
            return true;
        }
        (void)usleep(100000);
    }
    (void)kill(pid, SIGKILL);
    (void)waitpid(pid, nullptr, 0);
    state.pid = -1;
    state.lastExitCode = 128 + SIGKILL;
    state.lastMessage = "xrdp process required SIGKILL: " + reason;
    result = SnapshotLocked(state);
    result.state = "Stopped";
    result.message = state.lastMessage;
    LogInfo(state.lastMessage);
    return true;
}

} // namespace

namespace xrdp_bridge_internal {
XrdpServerState& ServerState()
{
    static XrdpServerState state;
    return state;
}
} // namespace xrdp_bridge_internal

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params)
{
    XrdpServerCommandResult result;
    XrdpServerState& state = ServerState();
    const XrdpResolvedPaths paths = ResolvePaths(params);
    result.port = kDefaultPort;
    if (!PrepareRuntime(paths, result.logs)) {
        result.state = "Failed";
        result.message = "HNP xrdp runtime is unavailable";
        return result;
    }
    if (!PrepareSecureRuntimeConfig(params, paths, kDefaultPort, result.logs)) {
        result.state = "Failed";
        result.message = "xrdp TLS runtime config could not be prepared";
        return result;
    }

    std::lock_guard<std::mutex> lock(state.mutex);
    RefreshProcessLocked(state);
    if (state.pid > 0 && !params.restartIfRunning) {
        return SnapshotLocked(state);
    }
    if (state.pid > 0 && !StopLocked(state, "configuration restart", result)) {
        return result;
    }

    // fork 前构造独立 envp，避免子进程调用 setenv（违反 POSIX async-signal-safety），
    // 同时避免父进程 environ 被临时改写（尤其 LD_LIBRARY_PATH）。
    std::vector<std::string> envStorage;
    for (char** e = environ; *e != nullptr; ++e) {
        envStorage.emplace_back(*e);
    }
    auto overrideEnv = [&envStorage](const std::string& key, const std::string& value) {
        const std::string prefix = key + "=";
        for (auto& entry : envStorage) {
            if (entry.compare(0, prefix.size(), prefix) == 0) {
                entry = prefix + value;
                return;
            }
        }
        envStorage.push_back(prefix + value);
    };
    overrideEnv("XRDP_CFG_PATH", paths.runtimeRoot + "/config");
    overrideEnv("XRDP_SHARE_PATH", paths.sharePath);
    overrideEnv("XRDP_MODULE_PATH", paths.modulePath);
    overrideEnv("XRDP_PID_PATH", paths.pidPath);
    overrideEnv("XRDP_LOG_PATH", paths.logPath);
    overrideEnv("LD_LIBRARY_PATH", paths.modulePath);
    std::vector<char*> envp;
    for (auto& s : envStorage) {
        envp.push_back(s.data());
    }
    envp.push_back(nullptr);

    const pid_t pid = fork();
    if (pid < 0) {
        result.state = "Failed";
        result.message = "fork xrdp failed: " + std::string(std::strerror(errno));
        return result;
    }
    if (pid == 0) {
        (void)prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (getppid() == 1) {
            _exit(125);
        }
        std::string port = std::to_string(kDefaultPort);
        char* const argv[] = {
            const_cast<char*>(paths.executablePath.c_str()),
            const_cast<char*>("-n"),
            const_cast<char*>("-c"),
            const_cast<char*>(paths.configPath.c_str()),
            const_cast<char*>("-p"),
            port.data(),
            nullptr
        };
        execve(paths.executablePath.c_str(), argv, envp.data());
        _exit(126);
    }

    state.pid = pid;
    state.lastExitCode = 0;
    state.paths = paths;
    state.lastMessage = "xrdp HNP process started pid=" + std::to_string(pid);
    LogInfo(state.lastMessage);
    // 持锁 usleep 150ms 为启动健康检查让步，阻塞并发 GetXrdpServerDiagnostics/StopXrdpServer 最多 150ms；
    // 非正确性缺陷，仅响应性，保留以避免启动期 waitpid 竞态。
    (void)usleep(150000);
    result = SnapshotLocked(state);
    if (!result.ok) {
        result.state = "Failed";
        result.message = "xrdp HNP process exited during startup code=" +
            std::to_string(result.lastExitCode);
    }
    return result;
}

XrdpServerCommandResult GetXrdpServerDiagnostics()
{
    std::lock_guard<std::mutex> lock(ServerState().mutex);
    return SnapshotLocked(ServerState());
}

XrdpServerCommandResult StopXrdpServer(const std::string& reason)
{
    XrdpServerCommandResult result;
    std::lock_guard<std::mutex> lock(ServerState().mutex);
    (void)StopLocked(ServerState(), reason.empty() ? "UI request" : reason, result);
    return result;
}

} // namespace rdp_bridge
