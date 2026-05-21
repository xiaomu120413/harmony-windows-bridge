#include "xrdp/xrdp_server_internal.h"

#include "common/bridge_log.h"
#include "ohos/ohos_capture_controller.h"
#include "ohos/ohos_frame_submitter.h"
#include "xrdp/xrdp_display_geometry.h"
#include "xrdp/xrdp_screen_capture_bridge.h"

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

xrdp_ohos::FrameSubmitter& VideoSubmitter()
{
    static xrdp_ohos::FrameSubmitter submitter;
    return submitter;
}

std::atomic<uint64_t> g_xrdpEncodedBackpressureCount { 0 };

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
        nullptr,
        nullptr,
        DescribeControllerGeometry,
        nullptr,
    });
    return controller;
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

    CaptureController().HandleBackendEvent(*event);
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
