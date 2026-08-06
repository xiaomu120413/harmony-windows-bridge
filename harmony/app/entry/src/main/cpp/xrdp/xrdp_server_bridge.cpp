#include "xrdp/xrdp_server_internal.h"

#include "common/bridge_log.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace rdp_bridge {
using namespace xrdp_bridge_internal;

namespace xrdp_bridge_internal {
XrdpServerState& ServerState()
{
    static XrdpServerState state;
    return state;
}
} // namespace xrdp_bridge_internal

namespace {

struct XrdpServerDiagnostics {
    bool ok = true;
    bool running = false;
    bool activeMstscSession = false;
    uint32_t port = 0;
    uint32_t sessionWidth = 0;
    uint32_t sessionHeight = 0;
    uint32_t sessionBpp = 0;
    uint32_t backendEventCount = 0;
    uint32_t inputEventCount = 0;
    uint32_t rdpecamDeviceCount = 0;
    uint32_t rdpecamErrorCount = 0;
    uint32_t rdpecamFormat = 0;
    uint32_t rdpecamWidth = 0;
    uint32_t rdpecamHeight = 0;
    uint64_t rdpecamSampleCount = 0;
    uint64_t rdpecamBytes = 0;
    std::string rdpecamDeviceName;
    int lastExitCode = 0;
    std::string state;
    std::string message;
    std::string lastBackendEvent;
    std::string lastDisconnectReason;
    std::string libraryPath;
    std::string backendLibraryPath;
    std::string runtimeRoot;
    std::string configPath;
    std::string modulePath;
    std::string sharePath;
    std::string logPath;
    std::vector<std::string> logs;
};

std::string BoolText(bool value)
{
    return value ? "true" : "false";
}

std::string BoolText(uint32_t value)
{
    return value != 0U ? "true" : "false";
}

void AppendXrdpDiagnosticsLogs(XrdpServerDiagnostics& diagnostics, const XrdpLoadedBackend& backend)
{
    xrdp_ohos_capture_diagnostics capture {};
    int captureStatus = XRDP_OHOS_BACKEND_STATUS_UNSUPPORTED_FORMAT;
    if (backend.captureDiagnosticsFn != nullptr) {
        capture.size = sizeof(capture);
        captureStatus = backend.captureDiagnosticsFn(&capture);
    }
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
        " encodedBackpressure=" + std::to_string(capture.encoded_backpressure_count) +
        " lastExitCode=" + std::to_string(diagnostics.lastExitCode));
    diagnostics.logs.push_back("xrdp rdpecam devices=" + std::to_string(diagnostics.rdpecamDeviceCount) +
        " active=" + diagnostics.rdpecamDeviceName +
        " format=" + std::to_string(diagnostics.rdpecamFormat) +
        " size=" + std::to_string(diagnostics.rdpecamWidth) + "x" +
            std::to_string(diagnostics.rdpecamHeight) +
        " samples=" + std::to_string(diagnostics.rdpecamSampleCount) +
        " bytes=" + std::to_string(diagnostics.rdpecamBytes) +
        " errors=" + std::to_string(diagnostics.rdpecamErrorCount));
    diagnostics.logs.push_back("xrdp videoOwner=internal-capture externalFrameSubmit=disabled");
    if (captureStatus != XRDP_OHOS_BACKEND_STATUS_OK) {
        diagnostics.logs.push_back("xrdp capture diagnostics unavailable status=" +
            std::to_string(captureStatus));
        return;
    }
    diagnostics.logs.push_back("xrdp capture running=" + BoolText(capture.running) +
        " target=" + std::to_string(capture.width) + "x" + std::to_string(capture.height) +
        "@" + std::to_string(capture.frame_rate) + "fps" +
        " cursor=" + std::string(capture.show_cursor != 0U ? "on" : "off") +
        " ready=" + std::to_string(capture.ready_count) +
        " queued=" + std::to_string(capture.submitted_count) +
        " dropped=" + std::to_string(capture.dropped_count) +
        " audioReady=" + std::to_string(capture.audio_ready_count) +
        " audioQueued=" + std::to_string(capture.audio_submitted_count) +
        " audioDropped=" + std::to_string(capture.audio_dropped_count) +
        " audioBytes=" + std::to_string(capture.audio_bytes) +
        " errors=" + std::to_string(capture.capture_error_count));
    diagnostics.logs.push_back("xrdp video running=" + BoolText(capture.video_submitter_running) +
        " pending=" + BoolText(capture.video_submitter_has_pending) +
        " submitting=" + BoolText(capture.video_submitter_submitting) +
        " queued=" + std::to_string(capture.video_queued_count) +
        " submitted=" + std::to_string(capture.video_submitted_count) +
        " failed=" + std::to_string(capture.video_failed_count) +
        " replaced=" + std::to_string(capture.video_replaced_count) +
        " drops preCopy=" + std::to_string(capture.video_precopy_drop_count) +
        " backoff=" + std::to_string(capture.video_backoff_drop_count) +
        " lastStatus=" + std::to_string(capture.video_last_status) +
        " copy=" + std::to_string(capture.video_last_copy_us / 1000.0) +
        "ms submit=" + std::to_string(capture.video_last_submit_us / 1000.0) +
        "ms buffers allocated=" + std::to_string(capture.video_buffer_allocated_count) +
        " reused=" + std::to_string(capture.video_buffer_reused_count) +
        " free=" + std::to_string(capture.video_free_buffer_count));
}

XrdpServerDiagnostics SnapshotXrdpDiagnosticsLocked(const XrdpServerState& state)
{
    XrdpServerDiagnostics diagnostics;
    diagnostics.running = state.running.load();
    diagnostics.activeMstscSession = state.activeMstscSession;
    diagnostics.port = state.port == 0 ? kDefaultPort : state.port;
    diagnostics.sessionWidth = state.sessionWidth;
    diagnostics.sessionHeight = state.sessionHeight;
    diagnostics.sessionBpp = state.sessionBpp;
    diagnostics.backendEventCount = state.backendEventCount;
    diagnostics.inputEventCount = state.inputEventCount;
    diagnostics.rdpecamDeviceCount = state.rdpecamDeviceCount;
    diagnostics.rdpecamErrorCount = state.rdpecamErrorCount;
    diagnostics.rdpecamFormat = state.rdpecamFormat;
    diagnostics.rdpecamWidth = state.rdpecamWidth;
    diagnostics.rdpecamHeight = state.rdpecamHeight;
    diagnostics.rdpecamSampleCount = state.rdpecamSampleCount;
    diagnostics.rdpecamBytes = state.rdpecamBytes;
    diagnostics.rdpecamDeviceName = state.rdpecamDeviceName.empty() ?
        "none" : state.rdpecamDeviceName;
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
    AppendXrdpDiagnosticsLogs(diagnostics, state.backend);
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

bool RestartXrdpServerIfRequested(XrdpServerState& state, const XrdpServerParams& params,
    const XrdpResolvedPaths& paths, uint32_t port, XrdpServerCommandResult& result)
{
    if (!params.restartIfRunning) {
        return true;
    }

    XrdpStopFn stopFn = nullptr;
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        StoreResolvedPathsLocked(state, paths, port);
        if (!state.running.load()) {
            return true;
        }
        stopFn = state.loaded.stopFn;
        state.lastMessage = "xrdp server restart requested";
    }

    if (stopFn == nullptr) {
        result.ok = false;
        result.state = "Failed";
        result.message = "xrdp server restart requested but stop API is unavailable";
        BridgeLogger::Error(result.message);
        return false;
    }

    result.logs.push_back("xrdp server restart requested to apply configuration");
    const int stopCode = stopFn();
    result.logs.push_back("xrdp server stop requested rc=" + std::to_string(stopCode));
    for (int attempt = 0; attempt < 30 && state.running.load(); attempt++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if (state.running.load()) {
        result.ok = false;
        result.state = "Stopping";
        result.message = "xrdp server is still stopping";
        BridgeLogger::Error(result.message);
        return false;
    }
    return true;
}

} // namespace

namespace xrdp_bridge_internal {

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
    if (event->type == XRDP_OHOS_BACKEND_EVENT_SESSION_CONNECT ||
        event->type == XRDP_OHOS_BACKEND_EVENT_SESSION_DISCONNECT) {
        BridgeLogger::Info("xrdp backend " + std::string(XrdpBackendEventTypeName(event->type)) +
            ": desktop=" + std::to_string(event->width) + "x" + std::to_string(event->height) +
            " bpp=" + std::to_string(event->bpp) +
            " connected=" + std::to_string(event->connected));
    }

}

void OnXrdpRdpecamEvent(const xrdp_ohos_rdpecam_event* event, void*)
{
    if (event == nullptr || event->size < sizeof(xrdp_ohos_rdpecam_event) ||
        event->version != XRDP_OHOS_RDPECAM_EVENT_VERSION) {
        return;
    }
    uint64_t sampleCount = 0;
    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (event->type == XRDP_OHOS_RDPECAM_EVENT_DEVICE_ADDED) {
            state.rdpecamDeviceCount++;
            state.rdpecamDeviceName = event->device_name;
        } else if (event->type == XRDP_OHOS_RDPECAM_EVENT_DEVICE_REMOVED) {
            state.rdpecamDeviceCount = state.rdpecamDeviceCount > 0 ?
                state.rdpecamDeviceCount - 1 : 0;
        } else if (event->type == XRDP_OHOS_RDPECAM_EVENT_STREAM_STARTED) {
            state.rdpecamFormat = event->format;
            state.rdpecamWidth = event->width;
            state.rdpecamHeight = event->height;
        } else if (event->type == XRDP_OHOS_RDPECAM_EVENT_SAMPLE) {
            state.rdpecamSampleCount++;
            state.rdpecamBytes += event->data_bytes;
            sampleCount = state.rdpecamSampleCount;
        } else if (event->type == XRDP_OHOS_RDPECAM_EVENT_ERROR) {
            state.rdpecamErrorCount++;
        }
    }
    if (event->type != XRDP_OHOS_RDPECAM_EVENT_SAMPLE ||
        sampleCount <= 3 || (sampleCount % 300) == 0) {
        BridgeLogger::Info("xrdp rdpecam event=" + std::to_string(event->type) +
            " device=" + std::string(event->device_name) +
            " format=" + std::to_string(event->format) +
            " size=" + std::to_string(event->width) + "x" + std::to_string(event->height) +
            " sample=" + std::to_string(sampleCount) +
            " bytes=" + std::to_string(event->data_bytes) +
            " status=" + std::to_string(event->status));
    }
}

} // namespace xrdp_bridge_internal

XrdpServerCommandResult StartXrdpServer(const XrdpServerParams& params)
{
    XrdpServerCommandResult result;
    XrdpServerState& state = ServerState();
    const XrdpResolvedPaths paths = ResolvePaths(params);
    const uint32_t port = kDefaultPort;
    FillPathResult(result, paths);
    result.port = port;

    if (!PrepareRuntime(paths, result.logs)) {
        result.ok = false;
        result.state = "Failed";
        result.message = "xrdp runtime directories are not writable";
        BridgeLogger::Error(result.message);
        return result;
    }
    if (!PrepareSecureRuntimeConfig(params, paths, port, result.logs)) {
        result.ok = false;
        result.state = "Failed";
        result.message = "xrdp TLS runtime config could not be prepared";
        BridgeLogger::Error(result.message);
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

    if (!RestartXrdpServerIfRequested(state, params, paths, port, result)) {
        return result;
    }

    {
        std::lock_guard<std::mutex> lock(state.mutex);
        StoreResolvedPathsLocked(state, paths, port);
        if (state.running.load()) {
            result.ok = true;
            result.state = "Listening";
            result.message = "xrdp server is already running";
            result.libraryPath = state.loaded.libraryPath;
            XrdpServerDiagnostics diagnostics = SnapshotXrdpDiagnosticsLocked(state);
            result.activeMstscSession = diagnostics.activeMstscSession;
            result.rdpecamDeviceName = diagnostics.rdpecamDeviceName;
            result.rdpecamFormat = diagnostics.rdpecamFormat;
            result.rdpecamWidth = diagnostics.rdpecamWidth;
            result.rdpecamHeight = diagnostics.rdpecamHeight;
            result.rdpecamSampleCount = diagnostics.rdpecamSampleCount;
            result.rdpecamBytes = diagnostics.rdpecamBytes;
            result.rdpecamErrors = diagnostics.rdpecamErrorCount;
            result.logs.insert(result.logs.end(), diagnostics.logs.begin(), diagnostics.logs.end());
            return result;
        }
        if (!LoadServerLocked(paths, result)) {
            result.ok = false;
            result.state = "Failed";
            result.message = "xrdp embedded server library could not be loaded";
            BridgeLogger::Error(result.message);
            return result;
        }
        if (!LoadBackendLocked(paths, result)) {
            result.ok = false;
            result.state = "Failed";
            result.message = "xrdp OHOS backend could not be loaded";
            BridgeLogger::Error(result.message);
            return result;
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
        state.rdpecamDeviceCount = 0;
        state.rdpecamErrorCount = 0;
        state.rdpecamFormat = 0;
        state.rdpecamWidth = 0;
        state.rdpecamHeight = 0;
        state.rdpecamSampleCount = 0;
        state.rdpecamBytes = 0;
        state.rdpecamDeviceName.clear();
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
        XrdpCaptureResetFn resetFn = nullptr;
        {
            std::lock_guard<std::mutex> lock(threadState.mutex);
            resetFn = threadState.backend.captureResetFn;
        }
        if (resetFn != nullptr) {
            resetFn("xrdp server exit");
        }
        BridgeLogger::Info(exitMessage);
    }).detach();

    result.ok = true;
    result.state = "Listening";
    result.message = "xrdp server start requested on port " + std::to_string(port);
    result.logs.push_back(result.message);
    result.logs.push_back("xrdp internal screen capture waits for an active mstsc session");
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        XrdpServerDiagnostics diagnostics = SnapshotXrdpDiagnosticsLocked(state);
        result.activeMstscSession = diagnostics.activeMstscSession;
        result.rdpecamDeviceName = diagnostics.rdpecamDeviceName;
        result.rdpecamFormat = diagnostics.rdpecamFormat;
        result.rdpecamWidth = diagnostics.rdpecamWidth;
        result.rdpecamHeight = diagnostics.rdpecamHeight;
        result.rdpecamSampleCount = diagnostics.rdpecamSampleCount;
        result.rdpecamBytes = diagnostics.rdpecamBytes;
        result.rdpecamErrors = diagnostics.rdpecamErrorCount;
        result.logs.insert(result.logs.end(), diagnostics.logs.begin(), diagnostics.logs.end());
    }
    BridgeLogger::Info(result.message);
    return result;
}

XrdpServerCommandResult GetXrdpServerDiagnostics()
{
    XrdpServerCommandResult result;
    std::lock_guard<std::mutex> lock(ServerState().mutex);
    const XrdpServerDiagnostics diagnostics = SnapshotXrdpDiagnosticsLocked(ServerState());
    result.ok = diagnostics.running;
    result.state = diagnostics.state;
    result.message = diagnostics.message;
    result.logs = diagnostics.logs;
    result.libraryPath = diagnostics.libraryPath;
    result.runtimeRoot = diagnostics.runtimeRoot;
    result.configPath = diagnostics.configPath;
    result.modulePath = diagnostics.modulePath;
    result.logPath = diagnostics.logPath;
    result.activeMstscSession = diagnostics.activeMstscSession;
    result.port = diagnostics.port;
    result.rdpecamDeviceName = diagnostics.rdpecamDeviceName;
    result.rdpecamFormat = diagnostics.rdpecamFormat;
    result.rdpecamWidth = diagnostics.rdpecamWidth;
    result.rdpecamHeight = diagnostics.rdpecamHeight;
    result.rdpecamSampleCount = diagnostics.rdpecamSampleCount;
    result.rdpecamBytes = diagnostics.rdpecamBytes;
    result.rdpecamErrors = diagnostics.rdpecamErrorCount;
    return result;
}

} // namespace rdp_bridge
