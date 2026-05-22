#include "xrdp/xrdp_server_internal.h"

#include "common/bridge_log.h"

#include <atomic>
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
    if (event->type != XRDP_OHOS_BACKEND_EVENT_FRAME_ACK) {
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

}

} // namespace xrdp_bridge_internal

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
            result.ok = false;
            result.state = "Failed";
            result.message = "xrdp OHOS backend could not be loaded";
            EmitHilogError(result.message);
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

bool SubmitXrdpRgbaFrame(const RgbaFrame& frame, std::string& message)
{
    XrdpCaptureSubmitFrameFn submitFn = nullptr;

    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (!state.running.load()) {
            message = "xrdp server is not running";
            return false;
        }
        submitFn = state.backend.captureSubmitFrameFn;
    }

    if (submitFn == nullptr) {
        message = "xrdp capture frame submit API is not loaded";
        return false;
    }

    xrdp_ohos_frame xrdpFrame {};
    xrdpFrame.data = frame.data;
    xrdpFrame.width = frame.width;
    xrdpFrame.height = frame.height;
    xrdpFrame.stride = frame.strideBytes;
    xrdpFrame.format = XRDP_OHOS_FRAME_FORMAT_RGBA_8888;
    xrdpFrame.source_sequence = frame.sequence;

    const int status = submitFn(&xrdpFrame);
    if (status != XRDP_OHOS_BACKEND_STATUS_OK) {
        message = "xrdp capture frame submit status=" + std::to_string(status);
        return false;
    }
    message = "xrdp capture frame submitted";
    return true;
}

} // namespace rdp_bridge
