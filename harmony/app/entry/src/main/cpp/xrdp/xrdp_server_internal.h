#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "xrdp/xrdp_server_bridge.h"

namespace rdp_bridge {
namespace xrdp_bridge_internal {

constexpr const char* kDefaultRuntimeRoot = "/data/storage/el2/base/files/xrdp";
constexpr const char* kServerLibraryName = "libxrdpserver.so";
constexpr const char* kBackendLibraryName = "libxrdpohos.so";
constexpr uint32_t kDefaultPort = 3390;

using XrdpMainFn = int (*)(int, char**);
using XrdpStopFn = int (*)(void);
using XrdpGetAbiInfoFn = int (*)(xrdp_ohos_abi_info*);
using XrdpSetBackendEventCallbackFn = int (*)(xrdp_ohos_backend_event_fn, void*);
using XrdpCaptureGetDiagnosticsFn = int (*)(xrdp_ohos_capture_diagnostics*);
using XrdpCaptureSubmitFrameFn = int (*)(const xrdp_ohos_frame*);
using XrdpCaptureResetFn = void (*)(const char*);

struct XrdpLoadedServer {
    void* handle = nullptr;
    XrdpMainFn mainFn = nullptr;
    XrdpStopFn stopFn = nullptr;
    std::string libraryPath;
};

struct XrdpLoadedBackend {
    void* handle = nullptr;
    XrdpGetAbiInfoFn getAbiInfoFn = nullptr;
    XrdpSetBackendEventCallbackFn setEventCallbackFn = nullptr;
    XrdpCaptureGetDiagnosticsFn captureDiagnosticsFn = nullptr;
    XrdpCaptureSubmitFrameFn captureSubmitFrameFn = nullptr;
    XrdpCaptureResetFn captureResetFn = nullptr;
    xrdp_ohos_abi_info abiInfo {};
    bool abiInfoValid = false;
    std::string libraryPath;
};

struct XrdpServerState {
    std::mutex mutex;
    XrdpLoadedServer loaded;
    XrdpLoadedBackend backend;
    std::atomic<bool> running { false };
    std::atomic<int> lastExitCode { 0 };
    bool activeMstscSession = false;
    uint32_t port = 0;
    uint32_t sessionWidth = 0;
    uint32_t sessionHeight = 0;
    uint32_t sessionBpp = 0;
    uint32_t backendEventCount = 0;
    uint32_t inputEventCount = 0;
    std::string lastMessage;
    std::string lastBackendEvent;
    std::string lastDisconnectReason;
    std::string runtimeRoot;
    std::string configPath;
    std::string modulePath;
    std::string sharePath;
    std::string logPath;
};

struct XrdpResolvedPaths {
    std::string runtimeRoot;
    std::string nativeLibDir;
    std::string libraryPath;
    std::string libDir;
    std::string modulePath;
    std::string configPath;
    std::string packagedConfigPath;
    std::string tlsCertificatePath;
    std::string tlsKeyPath;
    std::string sharePath;
    std::string pidPath;
    std::string logPath;
};

XrdpServerState& ServerState();
const char* XrdpBackendEventTypeName(int type);
void OnXrdpBackendEvent(const xrdp_ohos_backend_event* event, void*);

std::string JoinPath(const std::string& left, const std::string& right);
bool PathExists(const std::string& path);
bool IsDirectory(const std::string& path);
XrdpResolvedPaths ResolvePaths(const XrdpServerParams& params);
bool PrepareRuntime(const XrdpResolvedPaths& paths, std::vector<std::string>& logs);
bool BuildSecureXrdpIni(const XrdpResolvedPaths& paths, const XrdpServerParams& params,
    uint32_t port, std::string& ini, std::vector<std::string>& logs);
bool PrepareSecureRuntimeConfig(const XrdpServerParams& params,
    const XrdpResolvedPaths& paths, uint32_t port, std::vector<std::string>& logs);
bool LoadServerLocked(const XrdpServerParams& params, const XrdpResolvedPaths& paths,
    XrdpServerCommandResult& result);
bool LoadBackendLocked(const XrdpServerParams& params, const XrdpResolvedPaths& paths,
    XrdpServerCommandResult& result);
void FillPathResult(XrdpServerCommandResult& result, const XrdpResolvedPaths& paths);

} // namespace xrdp_bridge_internal
} // namespace rdp_bridge
