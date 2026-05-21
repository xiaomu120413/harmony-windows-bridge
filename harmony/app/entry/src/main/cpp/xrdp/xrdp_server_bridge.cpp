#include "xrdp/xrdp_server_bridge.h"

#include "common/bridge_log.h"
#include "ohos/ohos_capture_controller.h"
#include "ohos/ohos_frame_submitter.h"
#include "xrdp/xrdp_display_geometry.h"
#include "xrdp/xrdp_input_injector.h"
#include "xrdp/xrdp_screen_capture_bridge.h"

#include <atomic>
#include <cerrno>
#include <cstddef>
#include <cstdlib>
#include <cstdint>
#include <dlfcn.h>
#include <mutex>
#include <sstream>
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
constexpr int32_t kXrdpWmMouseMove = XRDP_OHOS_WM_MOUSEMOVE;

using XrdpMainFn = int (*)(int, char**);
using XrdpStopFn = int (*)(void);
using XrdpSubmitFrameFn = int (*)(const xrdp_ohos_frame*);
using XrdpSubmitEncodedFrameFn = int (*)(const xrdp_ohos_encoded_frame*);
using XrdpSubmitAudioFrameFn = int (*)(const xrdp_ohos_audio_frame*);
using XrdpGetAbiInfoFn = int (*)(xrdp_ohos_abi_info*);
using XrdpSetInputCallbackFn = int (*)(xrdp_ohos_input_event_fn, void*);
using XrdpSetBackendEventCallbackFn = int (*)(xrdp_ohos_backend_event_fn, void*);

struct XrdpLoadedServer {
    void* handle = nullptr;
    XrdpMainFn mainFn = nullptr;
    XrdpStopFn stopFn = nullptr;
    std::string libraryPath;
};

struct XrdpLoadedBackend {
    void* handle = nullptr;
    XrdpGetAbiInfoFn getAbiInfoFn = nullptr;
    XrdpSubmitFrameFn submitFrameFn = nullptr;
    XrdpSubmitEncodedFrameFn submitEncodedFrameFn = nullptr;
    XrdpSubmitAudioFrameFn submitAudioFrameFn = nullptr;
    XrdpSetInputCallbackFn setInputCallbackFn = nullptr;
    XrdpSetBackendEventCallbackFn setEventCallbackFn = nullptr;
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
    std::string sharePath;
    std::string pidPath;
    std::string logPath;
};

XrdpServerState& ServerState()
{
    static XrdpServerState state;
    return state;
}

xrdp_ohos::FrameSubmitter& VideoSubmitter()
{
    static xrdp_ohos::FrameSubmitter submitter;
    return submitter;
}

std::atomic<uint64_t> g_xrdpEncodedBackpressureCount { 0 };

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

void AppendXrdpDiagnosticsLogs(XrdpServerDiagnostics& diagnostics)
{
    const XrdpScreenCaptureDiagnostics capture = GetXrdpScreenCaptureDiagnostics();
    const xrdp_ohos::FrameSubmitterStats video = VideoSubmitter().Snapshot();
    diagnostics.logs.push_back("xrdp running=" + std::string(diagnostics.running ? "true" : "false") +
        " activeMstscSession=" + std::string(diagnostics.activeMstscSession ? "true" : "false") +
        " port=" + std::to_string(diagnostics.port));
    diagnostics.logs.push_back("xrdp session=" + std::to_string(diagnostics.sessionWidth) +
        "x" + std::to_string(diagnostics.sessionHeight) +
        " bpp=" + std::to_string(diagnostics.sessionBpp));
    diagnostics.logs.push_back("xrdp configPath=" + diagnostics.configPath);
    diagnostics.logs.push_back("xrdp modulePath=" + diagnostics.modulePath);
    diagnostics.logs.push_back("xrdp logPath=" + diagnostics.logPath);
    diagnostics.logs.push_back("xrdp lastBackendEvent=" + diagnostics.lastBackendEvent);
    diagnostics.logs.push_back("xrdp lastDisconnectReason=" + diagnostics.lastDisconnectReason);
    diagnostics.logs.push_back("xrdp counters backendEvents=" + std::to_string(diagnostics.backendEventCount) +
        " inputEvents=" + std::to_string(diagnostics.inputEventCount) +
        " encodedBackpressure=" + std::to_string(g_xrdpEncodedBackpressureCount.load()) +
        " lastExitCode=" + std::to_string(diagnostics.lastExitCode));
    diagnostics.logs.push_back("xrdp capture running=" + std::string(capture.running ? "true" : "false") +
        " target=" + std::to_string(capture.width) + "x" + std::to_string(capture.height) +
        "@" + std::to_string(capture.frameRate) + "fps" +
        " cursor=" + std::string(capture.showCursor ? "on" : "off") +
        " ready=" + std::to_string(capture.readyCount) +
        " queued=" + std::to_string(capture.submittedCount) +
        " dropped=" + std::to_string(capture.droppedCount) +
        " audioReady=" + std::to_string(capture.audioReadyCount) +
        " audioQueued=" + std::to_string(capture.audioSubmittedCount) +
        " audioDropped=" + std::to_string(capture.audioDroppedCount) +
        " audioBytes=" + std::to_string(capture.audioBytes) +
        " errors=" + std::to_string(capture.captureErrorCount));
    diagnostics.logs.push_back("xrdp video running=" + std::string(video.running ? "true" : "false") +
        " pending=" + std::string(video.hasPending ? "true" : "false") +
        " submitting=" + std::string(video.submitting ? "true" : "false") +
        " queued=" + std::to_string(video.queuedCount) +
        " submitted=" + std::to_string(video.submittedCount) +
        " failed=" + std::to_string(video.failedCount) +
        " replaced=" + std::to_string(video.replacedCount) +
        " drops preCopy=" + std::to_string(video.preCopyDropCount) +
        " backoff=" + std::to_string(video.backoffDropCount) +
        " lastStatus=" + std::to_string(video.lastStatus) +
        " copy=" + std::to_string(video.lastCopyUs / 1000.0) +
        "ms submit=" + std::to_string(video.lastSubmitUs / 1000.0) +
        "ms buffers allocated=" + std::to_string(video.bufferAllocatedCount) +
        " reused=" + std::to_string(video.bufferReusedCount) +
        " free=" + std::to_string(video.freeBufferCount));
}

XrdpServerDiagnostics SnapshotXrdpDiagnosticsLocked(const XrdpServerState& state)
{
    XrdpServerDiagnostics diagnostics;
    diagnostics.running = state.running.load();
    diagnostics.activeMstscSession = state.activeMstscSession;
    diagnostics.port = state.port;
    diagnostics.sessionWidth = state.sessionWidth;
    diagnostics.sessionHeight = state.sessionHeight;
    diagnostics.sessionBpp = state.sessionBpp;
    diagnostics.backendEventCount = state.backendEventCount;
    diagnostics.inputEventCount = state.inputEventCount;
    diagnostics.lastExitCode = state.lastExitCode.load();
    diagnostics.message = state.lastMessage;
    diagnostics.lastBackendEvent = state.lastBackendEvent.empty() ? "none" : state.lastBackendEvent;
    diagnostics.lastDisconnectReason = state.lastDisconnectReason.empty() ? "none" : state.lastDisconnectReason;
    diagnostics.libraryPath = state.loaded.libraryPath;
    diagnostics.backendLibraryPath = state.backend.libraryPath;
    diagnostics.runtimeRoot = state.runtimeRoot;
    diagnostics.configPath = state.configPath;
    diagnostics.modulePath = state.modulePath;
    diagnostics.sharePath = state.sharePath;
    diagnostics.logPath = state.logPath;
    if (diagnostics.running) {
        diagnostics.state = diagnostics.activeMstscSession ? "ActiveSession" : "Listening";
    } else if (diagnostics.lastExitCode != 0) {
        diagnostics.state = "Exited";
    } else {
        diagnostics.state = "Stopped";
    }
    if (diagnostics.message.empty()) {
        diagnostics.message = "xrdp diagnostics snapshot";
    }
    AppendXrdpDiagnosticsLogs(diagnostics);
    return diagnostics;
}

void StoreResolvedPathsLocked(XrdpServerState& state, const XrdpResolvedPaths& paths, uint32_t port)
{
    state.port = port;
    state.runtimeRoot = paths.runtimeRoot;
    state.configPath = paths.configPath;
    state.modulePath = paths.modulePath;
    state.sharePath = paths.sharePath;
    state.logPath = paths.logPath;
}

bool StartControllerCapture(const xrdp_ohos::CaptureOptions& options, std::string& message, void*)
{
    XrdpScreenCaptureOptions bridgeOptions;
    bridgeOptions.width = options.width;
    bridgeOptions.height = options.height;
    bridgeOptions.frameRate = options.frameRate;
    bridgeOptions.showCursor = options.showCursor;
    return StartXrdpScreenCapture(bridgeOptions, message);
}

void StopControllerCapture(const std::string& reason, void*)
{
    StopXrdpScreenCapture(reason);
}

void UpdateControllerCaptureTarget(uint32_t width, uint32_t height, void*)
{
    UpdateXrdpScreenCaptureTarget(width, height);
}

void PrimeControllerInputAuthorization(const std::string& reason, void*)
{
    PrimeXrdpInputInjectorAuthorization(reason);
}

void ResetControllerInput(const std::string& reason, void*)
{
    ResetXrdpInputInjector(reason);
}

std::string DescribeControllerGeometry(void*)
{
    return FormatXrdpDisplayGeometry(QueryXrdpDisplayGeometry());
}

xrdp_ohos::CaptureController& CaptureController()
{
    static xrdp_ohos::CaptureController controller({
        StartControllerCapture,
        StopControllerCapture,
        UpdateControllerCaptureTarget,
        PrimeControllerInputAuthorization,
        ResetControllerInput,
        DescribeControllerGeometry,
        nullptr,
    });
    return controller;
}

const char* XrdpBackendEventTypeName(int type)
{
    switch (type) {
        case XRDP_OHOS_BACKEND_EVENT_SESSION_CONNECT:
            return "session-connect";
        case XRDP_OHOS_BACKEND_EVENT_SESSION_DISCONNECT:
            return "session-disconnect";
        case XRDP_OHOS_BACKEND_EVENT_FRAME_ACK:
            return "frame-ack";
        case XRDP_OHOS_BACKEND_EVENT_SUPPRESS_OUTPUT:
            return "suppress-output";
        case XRDP_OHOS_BACKEND_EVENT_MONITOR_RESIZE:
            return "monitor-resize";
        case XRDP_OHOS_BACKEND_EVENT_MONITOR_FULL_INVALIDATE:
            return "monitor-full-invalidate";
        default:
            return "unknown";
    }
}

void OnXrdpBackendEvent(const xrdp_ohos_backend_event* event, void*)
{
    static std::atomic<uint32_t> backendEventCount { 0 };
    if (event == nullptr) {
        return;
    }

    const uint32_t count = backendEventCount.fetch_add(1) + 1;
    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        state.backendEventCount = count;
        state.activeMstscSession = event->connected != 0;
        state.sessionWidth = event->width > 0 ? static_cast<uint32_t>(event->width) : 0;
        state.sessionHeight = event->height > 0 ? static_cast<uint32_t>(event->height) : 0;
        state.sessionBpp = event->bpp > 0 ? static_cast<uint32_t>(event->bpp) : 0;
        state.lastBackendEvent = XrdpBackendEventTypeName(event->type);
        state.lastMessage = "xrdp backend event " + state.lastBackendEvent;
        if (event->type == XRDP_OHOS_BACKEND_EVENT_SESSION_DISCONNECT) {
            state.lastDisconnectReason = "backend session-disconnect event";
        }
    }
    if (count <= 40U || event->type != XRDP_OHOS_BACKEND_EVENT_FRAME_ACK || (count % 200U) == 0U) {
        EmitHilogInfo("xrdp backend callback: count=" + std::to_string(count) +
            " type=" + XrdpBackendEventTypeName(event->type) +
            " version=" + std::to_string(event->version) +
            " desktop=" + std::to_string(event->width) + "x" + std::to_string(event->height) +
            " bpp=" + std::to_string(event->bpp) +
            " connected=" + std::to_string(event->connected) +
            " suppress=" + std::to_string(event->suppress) +
            " rect=(" + std::to_string(event->left) + "," + std::to_string(event->top) +
            "," + std::to_string(event->right) + "," + std::to_string(event->bottom) + ")" +
            " frameId=" + std::to_string(event->frame_id) +
            " sourceSeq=" + std::to_string(event->source_sequence) +
            " ackFromAcquire=" + std::to_string(
                (event->ack_us > event->capture_acquire_us && event->capture_acquire_us != 0) ?
                    (event->ack_us - event->capture_acquire_us) / 1000.0 : 0.0) + "ms" +
            " flags=" + std::to_string(event->flags));
    }

    CaptureController().HandleBackendEvent(*event);
}

void OnXrdpInputEvent(const XrdpOhosInputEvent* event, void*)
{
    static std::atomic<uint32_t> inputEventCount { 0 };

    if (event == nullptr) {
        return;
    }

    const uint32_t count = inputEventCount.fetch_add(1) + 1;
    const bool sampledMove = event->msg == kXrdpWmMouseMove && (count <= 32 || (count % 256U) == 0U);
    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        ServerState().inputEventCount = count;
    }

    if (event->msg != kXrdpWmMouseMove || sampledMove) {
        EmitHilogInfo("xrdp input callback: count=" + std::to_string(count) +
            " version=" + std::to_string(event->version) +
            " msg=" + std::to_string(event->msg) +
            " p=(" + std::to_string(event->param1) +
            "," + std::to_string(event->param2) +
            "," + std::to_string(event->param3) +
            "," + std::to_string(event->param4) +
            ") desktop=" + std::to_string(event->width) +
            "x" + std::to_string(event->height) +
            " bpp=" + std::to_string(event->bpp) +
            " connected=" + std::to_string(event->connected));
    }

    std::string inputMessage;
    DispatchXrdpInputEvent(*event, inputMessage);
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

std::vector<std::string> BuildBackendCandidates(const XrdpServerParams& params, const XrdpResolvedPaths& paths)
{
    std::vector<std::string> candidates;
    AddUnique(candidates, JoinPath(paths.modulePath, kBackendLibraryName));
    AddUnique(candidates, JoinPath(paths.nativeLibDir, kBackendLibraryName));
    AddUnique(candidates, JoinPath(JoinPath(paths.runtimeRoot, "lib"), kBackendLibraryName));

    if (!params.hnpRoot.empty()) {
        AddUnique(candidates, ResolveHnpChild(params.hnpRoot, "lib/libxrdpohos.so"));
    }

    AddUnique(candidates, "/data/service/hnp/xrdp.org/xrdp_0.1.0/lib/libxrdpohos.so");
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

bool LoadBackendLocked(const XrdpServerParams& params, const XrdpResolvedPaths& paths,
    XrdpServerCommandResult& result)
{
    XrdpServerState& state = ServerState();
    if (state.backend.handle != nullptr && state.backend.submitFrameFn != nullptr) {
        result.logs.push_back("xrdp OHOS backend already loaded: " + state.backend.libraryPath);
        if (state.backend.abiInfoValid) {
            result.logs.push_back("xrdp OHOS backend ABI: " + FormatBackendAbi(state.backend.abiInfo));
        } else {
            result.logs.push_back("xrdp OHOS backend ABI unavailable");
        }
        result.logs.push_back(std::string("xrdp OHOS backend audio callback ") +
            (state.backend.submitAudioFrameFn != nullptr ? "loaded" : "missing"));
        result.logs.push_back(std::string("xrdp OHOS backend encoded frame callback ") +
            (state.backend.submitEncodedFrameFn != nullptr ? "loaded" : "missing"));
        if (BackendHandlesInputDirectly(state.backend.abiInfoValid, state.backend.abiInfo)) {
            result.logs.push_back("xrdp OHOS backend input callback skipped: direct native input enabled");
        } else if (state.backend.setInputCallbackFn != nullptr) {
            const int rc = state.backend.setInputCallbackFn(OnXrdpInputEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend input callback register rc=" + std::to_string(rc));
        }
        if (state.backend.setEventCallbackFn != nullptr) {
            const int rc = state.backend.setEventCallbackFn(OnXrdpBackendEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend event callback register rc=" + std::to_string(rc));
        }
        return true;
    }

    for (const std::string& candidate : BuildBackendCandidates(params, paths)) {
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

        auto submitFn = reinterpret_cast<XrdpSubmitFrameFn>(
            dlsym(handle, "xrdp_ohos_backend_submit_frame"));
        if (submitFn == nullptr) {
            result.logs.push_back("xrdp OHOS backend frame symbol missing in: " + candidate);
            dlclose(handle);
            continue;
        }
        auto setInputCallbackFn = reinterpret_cast<XrdpSetInputCallbackFn>(
            dlsym(handle, "xrdp_ohos_backend_set_input_callback"));
        auto getAbiInfoFn = reinterpret_cast<XrdpGetAbiInfoFn>(
            dlsym(handle, "xrdp_ohos_backend_get_abi_info"));
        auto setEventCallbackFn = reinterpret_cast<XrdpSetBackendEventCallbackFn>(
            dlsym(handle, "xrdp_ohos_backend_set_event_callback"));
        auto submitAudioFn = reinterpret_cast<XrdpSubmitAudioFrameFn>(
            dlsym(handle, "xrdp_ohos_backend_submit_audio_frame"));
        auto submitEncodedFn = reinterpret_cast<XrdpSubmitEncodedFrameFn>(
            dlsym(handle, "xrdp_ohos_backend_submit_encoded_frame"));

        xrdp_ohos_abi_info abiInfo {};
        bool abiInfoValid = false;
        if (getAbiInfoFn != nullptr) {
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
        }

        state.backend.handle = handle;
        state.backend.getAbiInfoFn = getAbiInfoFn;
        state.backend.submitFrameFn = submitFn;
        state.backend.submitEncodedFrameFn = submitEncodedFn;
        state.backend.submitAudioFrameFn = submitAudioFn;
        state.backend.setInputCallbackFn = setInputCallbackFn;
        state.backend.setEventCallbackFn = setEventCallbackFn;
        state.backend.abiInfo = abiInfo;
        state.backend.abiInfoValid = abiInfoValid;
        state.backend.libraryPath = candidate;
        result.logs.push_back("xrdp OHOS backend loaded: " + candidate);
        if (abiInfoValid) {
            result.logs.push_back("xrdp OHOS backend ABI: " + FormatBackendAbi(abiInfo));
        } else {
            result.logs.push_back("xrdp OHOS backend ABI symbol missing in: " + candidate);
        }
        if (submitAudioFn != nullptr) {
            result.logs.push_back("xrdp OHOS backend audio callback loaded");
        } else {
            result.logs.push_back("xrdp OHOS backend audio callback symbol missing in: " + candidate);
        }
        if (submitEncodedFn != nullptr) {
            result.logs.push_back("xrdp OHOS backend encoded frame callback loaded");
        } else {
            result.logs.push_back("xrdp OHOS backend encoded frame callback symbol missing in: " + candidate);
        }
        if (BackendHandlesInputDirectly(abiInfoValid, abiInfo)) {
            result.logs.push_back("xrdp OHOS backend input callback skipped: direct native input enabled");
        } else if (setInputCallbackFn != nullptr) {
            const int rc = setInputCallbackFn(OnXrdpInputEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend input callback register rc=" + std::to_string(rc));
        } else {
            result.logs.push_back("xrdp OHOS backend input callback symbol missing in: " + candidate);
        }
        if (setEventCallbackFn != nullptr) {
            const int rc = setEventCallbackFn(OnXrdpBackendEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend event callback register rc=" + std::to_string(rc));
        } else {
            result.logs.push_back("xrdp OHOS backend event callback symbol missing in: " + candidate);
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
    result.logs.push_back("modulePath=" + paths.modulePath);
    result.logs.push_back("sharePath=" + paths.sharePath);
    result.logs.push_back("logPath=" + paths.logPath);
}

} // namespace

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params)
{
    XrdpServerCommandResult result;
    XrdpServerState& state = ServerState();
    const XrdpResolvedPaths paths = ResolvePaths(params);
    const uint32_t port = params.port == 0 ? kDefaultPort : params.port;
    FillPathResult(result, paths);
    result.port = port;

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
        StoreResolvedPathsLocked(state, paths, port);
        if (state.running.load()) {
            result.ok = true;
            result.state = "Listening";
            result.message = "xrdp server is already running";
            result.libraryPath = state.loaded.libraryPath;
            result.activeMstscSession = state.activeMstscSession;
            XrdpServerDiagnostics diagnostics = SnapshotXrdpDiagnosticsLocked(state);
            result.logs.insert(result.logs.end(), diagnostics.logs.begin(), diagnostics.logs.end());
            return result;
        }
        if (!LoadServerLocked(params, paths, result)) {
            result.ok = false;
            result.state = "Failed";
            result.message = "xrdp embedded server library could not be loaded";
            EmitHilogError(result.message);
            return result;
        }
        if (!LoadBackendLocked(params, paths, result)) {
            result.logs.push_back("xrdp server can still start, but external frame push is unavailable");
        }
    }

    std::vector<std::string> argvStorage = {
        "xrdp",
        "-n",
        "-c",
        paths.configPath,
        "-p",
        std::to_string(port)
    };

    XrdpMainFn mainFn = state.loaded.mainFn;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.running.store(true);
        state.lastExitCode.store(0);
        state.activeMstscSession = false;
        state.sessionWidth = 0;
        state.sessionHeight = 0;
        state.sessionBpp = 0;
        state.lastMessage = "xrdp server thread starting";
        state.lastBackendEvent = "none";
        state.lastDisconnectReason = "none";
        result.libraryPath = state.loaded.libraryPath;
    }

    std::thread([mainFn, argvStorage]() mutable {
        std::vector<char*> argv;
        argv.reserve(argvStorage.size());
        for (std::string& arg : argvStorage) {
            argv.push_back(arg.data());
        }
        const int code = mainFn(static_cast<int>(argv.size()), argv.data());
        XrdpServerState& threadState = ServerState();
        std::string exitMessage;
        {
            std::lock_guard<std::mutex> lock(threadState.mutex);
            threadState.lastExitCode.store(code);
            threadState.running.store(false);
            threadState.activeMstscSession = false;
            threadState.lastMessage = "xrdp server exited with code " + std::to_string(code);
            threadState.lastDisconnectReason = threadState.lastMessage;
            exitMessage = threadState.lastMessage;
        }
        CaptureController().Reset("xrdp server exit");
        VideoSubmitter().Stop("xrdp server exit");
        EmitHilogInfo(exitMessage);
    }).detach();

    result.ok = true;
    result.state = "Listening";
    result.message = "xrdp server start requested on port " + std::to_string(port);
    result.logs.push_back(result.message);
    result.logs.push_back("xrdp raw screen capture waits for an active mstsc session");
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        XrdpServerDiagnostics diagnostics = SnapshotXrdpDiagnosticsLocked(state);
        result.activeMstscSession = diagnostics.activeMstscSession;
        result.logs.insert(result.logs.end(), diagnostics.logs.begin(), diagnostics.logs.end());
    }
    EmitHilogInfo(result.message);
    return result;
}

XrdpServerDiagnostics GetXrdpServerDiagnostics()
{
    std::lock_guard<std::mutex> lock(ServerState().mutex);
    return SnapshotXrdpDiagnosticsLocked(ServerState());
}

bool QueueXrdpVideoFrame(const xrdp_ohos_frame& frame, std::string& message)
{
    XrdpSubmitFrameFn submitFn = nullptr;

    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (!state.running.load()) {
            message = "xrdp server is not running";
            return false;
        }
        submitFn = state.backend.submitFrameFn;
    }

    return VideoSubmitter().Enqueue(frame, submitFn, message);
}

bool QueueXrdpEncodedVideoFrame(const xrdp_ohos_encoded_frame& frame, std::string& message)
{
    XrdpSubmitEncodedFrameFn submitFn = nullptr;

    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (!state.running.load()) {
            message = "xrdp server is not running";
            return false;
        }
        submitFn = state.backend.submitEncodedFrameFn;
    }

    if (submitFn == nullptr) {
        message = "xrdp encoded video backend is not loaded";
        return false;
    }
    if (frame.data == nullptr || frame.bytes <= 0 || frame.width <= 0 || frame.height <= 0 ||
        frame.format != XRDP_OHOS_ENCODED_FRAME_FORMAT_H264_AVC420) {
        message = "invalid xrdp encoded video frame";
        return false;
    }

    const int status = submitFn(&frame);
    if (status != XRDP_OHOS_BACKEND_STATUS_OK) {
        if (status == XRDP_OHOS_BACKEND_STATUS_BACKPRESSURE) {
            const uint64_t count = g_xrdpEncodedBackpressureCount.fetch_add(1) + 1;
            message = "xrdp encoded video backpressure count=" + std::to_string(count);
            if (count <= 3 || (count % 60U) == 0U) {
                EmitHilogInfo(message);
            }
            return false;
        }
        message = "xrdp encoded video submit status=" + std::to_string(status);
        return false;
    }
    message = "xrdp encoded video queued bytes=" + std::to_string(frame.bytes);
    return true;
}

bool QueueXrdpAudioFrame(const xrdp_ohos_audio_frame& frame, std::string& message)
{
    XrdpSubmitAudioFrameFn submitFn = nullptr;

    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (!state.running.load()) {
            message = "xrdp server is not running";
            return false;
        }
        submitFn = state.backend.submitAudioFrameFn;
    }

    if (submitFn == nullptr) {
        message = "xrdp audio backend is not loaded";
        return false;
    }

    const int status = submitFn(&frame);
    if (status != XRDP_OHOS_BACKEND_STATUS_OK) {
        message = "xrdp audio submit status=" + std::to_string(status);
        return false;
    }
    message = "xrdp audio queued bytes=" + std::to_string(frame.bytes);
    return true;
}

bool QueueXrdpRgbaFrame(const RgbaFrame& frame, std::string& message)
{
    xrdp_ohos_frame xrdpFrame {};
    xrdpFrame.data = frame.data;
    xrdpFrame.width = frame.width;
    xrdpFrame.height = frame.height;
    xrdpFrame.stride = frame.strideBytes;
    xrdpFrame.format = XRDP_OHOS_FRAME_FORMAT_RGBA_8888;
    xrdpFrame.source_sequence = frame.sequence;
    return QueueXrdpVideoFrame(xrdpFrame, message);
}

} // namespace rdp_bridge
