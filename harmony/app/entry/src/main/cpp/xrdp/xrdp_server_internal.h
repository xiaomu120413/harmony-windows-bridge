#pragma once

#include <mutex>
#include <string>
#include <sys/types.h>
#include <vector>

#include "xrdp/xrdp_server_bridge.h"

namespace rdp_bridge::xrdp_bridge_internal {

constexpr const char* kDefaultHnpRoot = "/data/service/hnp/xrdp.org/xrdp_0.1.0";
constexpr const char* kBackendLibraryName = "libxrdpohos.so";
constexpr uint32_t kDefaultPort = 3390;

struct XrdpResolvedPaths {
    std::string runtimeRoot;
    std::string executablePath;
    std::string modulePath;
    std::string configPath;
    std::string packagedConfigPath;
    std::string tlsCertificatePath;
    std::string tlsKeyPath;
    std::string sharePath;
    std::string pidPath;
    std::string logPath;
};

struct XrdpServerState {
    std::mutex mutex;
    pid_t pid = -1;
    int lastExitCode = 0;
    std::string lastMessage = "xrdp process has not been started";
    XrdpResolvedPaths paths;
};

XrdpServerState& ServerState();
std::string JoinPath(const std::string& left, const std::string& right);
bool PathExists(const std::string& path);
bool IsDirectory(const std::string& path);
XrdpResolvedPaths ResolvePaths(const XrdpServerParams& params);
bool PrepareRuntime(const XrdpResolvedPaths& paths, std::vector<std::string>& logs);
bool BuildSecureXrdpIni(const XrdpResolvedPaths& paths, const XrdpServerParams& params,
    uint32_t port, std::string& ini, std::vector<std::string>& logs);
bool PrepareSecureRuntimeConfig(const XrdpServerParams& params,
    const XrdpResolvedPaths& paths, uint32_t port, std::vector<std::string>& logs);

} // namespace rdp_bridge::xrdp_bridge_internal
