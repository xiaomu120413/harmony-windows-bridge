#include "xrdp/xrdp_server_bridge.h"

#include "common/bridge_log.h"
#include "xrdp/xrdp_display_geometry.h"
#include "xrdp/xrdp_input_injector.h"
#include "xrdp/xrdp_screen_capture_bridge.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <dlfcn.h>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <string>
#include <sys/stat.h>
#include <thread>
#include <utility>
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
    XrdpSubmitFrameFn submitFrameFn = nullptr;
    XrdpSetInputCallbackFn setInputCallbackFn = nullptr;
    XrdpSetBackendEventCallbackFn setEventCallbackFn = nullptr;
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

struct XrdpVideoSubmitterStats {
    bool running = false;
    bool hasPending = false;
    bool submitting = false;
    uint64_t queuedCount = 0;
    uint64_t replacedCount = 0;
    uint64_t submittedCount = 0;
    uint64_t failedCount = 0;
    uint64_t backoffDropCount = 0;
    uint64_t preCopyDropCount = 0;
    uint64_t bufferAllocatedCount = 0;
    uint64_t bufferReusedCount = 0;
    uint32_t lastCopyUs = 0;
    uint32_t lastSubmitUs = 0;
    int lastStatus = 0;
    size_t freeBufferCount = 0;
};

class XrdpVideoFrameSubmitter {
public:
    bool Enqueue(const xrdp_ohos_frame& frame, XrdpSubmitFrameFn submitFn, std::string& message)
    {
        if (submitFn == nullptr) {
            message = "xrdp video backend is not loaded";
            return false;
        }

        const int32_t sourceStride = frame.stride > 0 ? frame.stride :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width <= 0 || frame.height <= 0 ||
            frame.width > kMaxFrameDimension || frame.height > kMaxFrameDimension ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            message = "invalid xrdp video frame";
            return false;
        }
        if (frame.format != XRDP_OHOS_FRAME_FORMAT_BGRA_8888 &&
            frame.format != XRDP_OHOS_FRAME_FORMAT_RGBA_8888) {
            message = "unsupported xrdp video frame format=" + std::to_string(frame.format);
            return false;
        }

        const size_t rowBytes = static_cast<size_t>(frame.width) * 4U;
        if (frame.height > std::numeric_limits<size_t>::max() / rowBytes) {
            message = "xrdp video frame is too large";
            return false;
        }

        const auto now = std::chrono::steady_clock::now();
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (lastStatus_ == XRDP_OHOS_BACKEND_STATUS_NO_ACTIVE_SESSION &&
                lastStatusAt_ != std::chrono::steady_clock::time_point {} &&
                now - lastStatusAt_ < std::chrono::milliseconds(500)) {
                const uint64_t dropped = ++backoffDropCount_;
                message = "xrdp video backoff: no active mstsc session dropped=" + std::to_string(dropped);
                return false;
            }
        }

        const size_t frameBytes = rowBytes * static_cast<size_t>(frame.height);
        PendingFrame next;
        next.width = frame.width;
        next.height = frame.height;
        next.submitFn = submitFn;
        next.format = frame.format;
        next.sourceSequence = frame.source_sequence;
        next.pixelBytes = frameBytes;
        bool reusedBuffer = false;
        next.pixels = TakeReusableBuffer(frameBytes, reusedBuffer);
        if (next.pixels == nullptr) {
            next.pixels = AllocateBytes(frameBytes);
            if (next.pixels != nullptr) {
                NoteAllocatedBuffer();
            }
        }
        if (next.pixels == nullptr) {
            message = "xrdp video frame allocation failed";
            return false;
        }
        next.reusedBuffer = reusedBuffer;

        const auto copyStart = std::chrono::steady_clock::now();
        for (uint32_t y = 0; y < frame.height; ++y) {
            const auto* source = static_cast<const uint8_t*>(frame.data) +
                static_cast<size_t>(y) * static_cast<size_t>(sourceStride);
            auto* target = next.pixels.get() + static_cast<size_t>(y) * rowBytes;
            std::memcpy(target, source, rowBytes);
        }
        const auto copyEnd = std::chrono::steady_clock::now();
        next.copyUs = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(copyEnd - copyStart).count());
        const uint32_t copyUs = next.copyUs;
        const bool copiedToReusedBuffer = next.reusedBuffer;

        uint64_t queued = 0;
        uint64_t replaced = 0;
        bool replacedPending = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            StartLocked();
            next.sequence = ++sequence_;
            if (hasPending_) {
                replacedPending = true;
                ++replacedCount_;
                RecycleBufferLocked(std::move(pending_.pixels), pending_.pixelBytes);
            }
            pending_ = std::move(next);
            hasPending_ = true;
            queued = ++queuedCount_;
            replaced = replacedCount_;
        }
        condition_.notify_one();

        message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " xrdp-video queued mode=latest copy=" + std::to_string(copyUs / 1000.0) +
            "ms queued=" + std::to_string(queued) +
            " replaced=" + std::to_string(replaced) +
            " replacedPending=" + std::string(replacedPending ? "true" : "false") +
            " buffer=" + std::string(copiedToReusedBuffer ? "reused" : "new");
        return true;
    }

    void Stop(const std::string& reason)
    {
        std::thread worker;
        bool shouldLog = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!running_ && !worker_.joinable()) {
                return;
            }
            running_ = false;
            if (hasPending_) {
                RecycleBufferLocked(std::move(pending_.pixels), pending_.pixelBytes);
            }
            hasPending_ = false;
            pending_ = PendingFrame {};
            worker = std::move(worker_);
            shouldLog = true;
        }
        condition_.notify_one();
        if (worker.joinable()) {
            worker.join();
        }
        if (shouldLog) {
            EmitHilogInfo("xrdp video submitter stopped after " + reason);
        }
    }

    XrdpVideoSubmitterStats Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        XrdpVideoSubmitterStats stats;
        stats.running = running_;
        stats.hasPending = hasPending_;
        stats.submitting = submitting_;
        stats.queuedCount = queuedCount_;
        stats.replacedCount = replacedCount_;
        stats.submittedCount = submittedCount_;
        stats.failedCount = failedCount_;
        stats.backoffDropCount = backoffDropCount_;
        stats.preCopyDropCount = preCopyDropCount_;
        stats.bufferAllocatedCount = bufferAllocatedCount_;
        stats.bufferReusedCount = bufferReusedCount_;
        stats.lastCopyUs = lastCopyUs_;
        stats.lastSubmitUs = lastSubmitUs_;
        stats.lastStatus = lastStatus_;
        stats.freeBufferCount = freeBuffers_.size();
        return stats;
    }

private:
    static constexpr uint32_t kMaxFrameDimension = 8192;
    static constexpr size_t kMaxReusableBuffers = 2;
    using ByteBuffer = std::unique_ptr<uint8_t[]>;

    struct ReusableBuffer {
        ByteBuffer pixels;
        size_t bytes = 0;
    };

    struct PendingFrame {
        ByteBuffer pixels;
        size_t pixelBytes = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t copyUs = 0;
        uint64_t sequence = 0;
        uint64_t sourceSequence = 0;
        int format = XRDP_OHOS_FRAME_FORMAT_RGBA_8888;
        bool reusedBuffer = false;
        XrdpSubmitFrameFn submitFn = nullptr;
    };

    static ByteBuffer AllocateBytes(size_t bytes)
    {
        return ByteBuffer(new (std::nothrow) uint8_t[bytes]);
    }

    ByteBuffer TakeReusableBuffer(size_t bytes, bool& reused)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto iter = freeBuffers_.begin(); iter != freeBuffers_.end(); ++iter) {
            if (iter->bytes == bytes && iter->pixels != nullptr) {
                ByteBuffer pixels = std::move(iter->pixels);
                freeBuffers_.erase(iter);
                reused = true;
                ++bufferReusedCount_;
                return pixels;
            }
        }
        reused = false;
        return ByteBuffer();
    }

    void RecycleBuffer(ByteBuffer pixels, size_t bytes)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RecycleBufferLocked(std::move(pixels), bytes);
    }

    void RecycleBufferLocked(ByteBuffer pixels, size_t bytes)
    {
        if (pixels == nullptr || bytes == 0U) {
            return;
        }
        if (freeBuffers_.size() >= kMaxReusableBuffers) {
            return;
        }
        freeBuffers_.push_back(ReusableBuffer { std::move(pixels), bytes });
    }

    void NoteAllocatedBuffer()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++bufferAllocatedCount_;
    }

    void StartLocked()
    {
        if (running_) {
            return;
        }

        running_ = true;
        worker_ = std::thread([this]() { WorkerLoop(); });
        EmitHilogInfo("xrdp video submitter started");
    }

    void WorkerLoop()
    {
        for (;;) {
            PendingFrame frame;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                condition_.wait(lock, [this]() { return !running_ || hasPending_; });
                if (!running_ && !hasPending_) {
                    return;
                }
                frame = std::move(pending_);
                hasPending_ = false;
                submitting_ = true;
            }

            xrdp_ohos_frame submittedFrame {};
            submittedFrame.data = frame.pixels.get();
            submittedFrame.width = static_cast<int>(frame.width);
            submittedFrame.height = static_cast<int>(frame.height);
            submittedFrame.stride = static_cast<int>(frame.width * 4U);
            submittedFrame.format = frame.format;
            submittedFrame.source_sequence = frame.sourceSequence;
            const auto submitStart = std::chrono::steady_clock::now();
            const int status = frame.pixels != nullptr && frame.pixelBytes != 0U && frame.submitFn != nullptr ?
                frame.submitFn(&submittedFrame) :
                XRDP_OHOS_BACKEND_STATUS_INVALID_FRAME;
            const auto submitEnd = std::chrono::steady_clock::now();
            const uint32_t submitUs = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(submitEnd - submitStart).count());

            uint64_t submitted = 0;
            uint64_t failed = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                submitting_ = false;
                lastStatus_ = status;
                lastStatusAt_ = std::chrono::steady_clock::now();
                lastCopyUs_ = frame.copyUs;
                lastSubmitUs_ = submitUs;
                if (status == 0) {
                    submitted = ++submittedCount_;
                } else {
                    failed = ++failedCount_;
                }
            }

            if (status == 0) {
                if (submitted <= 3 || (submitted % 60U) == 0U) {
                    EmitHilogInfo("xrdp video frame submitted: seq=" + std::to_string(frame.sequence) +
                        " size=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                        " copy=" + std::to_string(frame.copyUs / 1000.0) +
                        "ms submit=" + std::to_string(submitUs / 1000.0) +
                        "ms pixel=" + std::string(frame.format == XRDP_OHOS_FRAME_FORMAT_BGRA_8888 ? "bgra" : "rgba") +
                        " buffer=" + std::string(frame.reusedBuffer ? "reused" : "new") +
                        " sourceSeq=" + std::to_string(frame.sourceSequence) +
                        " count=" + std::to_string(submitted));
                }
            } else if (status == XRDP_OHOS_BACKEND_STATUS_NO_ACTIVE_SESSION) {
                if (failed <= 3 || (failed % 120U) == 0U) {
                    EmitHilogInfo("xrdp video frame skipped: no active mstsc session status=-4 count=" +
                        std::to_string(failed));
                }
            } else if (failed <= 3 || (failed % 120U) == 0U) {
                EmitHilogError("xrdp video frame submit failed: status=" + std::to_string(status) +
                    " size=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                    " count=" + std::to_string(failed));
            }
            RecycleBuffer(std::move(frame.pixels), frame.pixelBytes);
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_;
    PendingFrame pending_;
    bool running_ = false;
    bool hasPending_ = false;
    bool submitting_ = false;
    uint64_t sequence_ = 0;
    uint64_t queuedCount_ = 0;
    uint64_t replacedCount_ = 0;
    uint64_t submittedCount_ = 0;
    uint64_t failedCount_ = 0;
    uint64_t backoffDropCount_ = 0;
    uint64_t preCopyDropCount_ = 0;
    uint64_t bufferAllocatedCount_ = 0;
    uint64_t bufferReusedCount_ = 0;
    uint32_t lastCopyUs_ = 0;
    uint32_t lastSubmitUs_ = 0;
    int lastStatus_ = 0;
    std::chrono::steady_clock::time_point lastStatusAt_;
    std::vector<ReusableBuffer> freeBuffers_;
};

XrdpServerState& ServerState()
{
    static XrdpServerState state;
    return state;
}

XrdpVideoFrameSubmitter& VideoSubmitter()
{
    static XrdpVideoFrameSubmitter submitter;
    return submitter;
}

struct XrdpClientCaptureState {
    std::mutex mutex;
    bool requested = false;
    uint32_t width = 0;
    uint32_t height = 0;
    std::chrono::steady_clock::time_point lastFailure;
};

XrdpClientCaptureState& ClientCaptureState()
{
    static XrdpClientCaptureState state;
    return state;
}

std::atomic<bool> g_xrdpInputAuthorizationPrimed { false };

void AppendXrdpDiagnosticsLogs(XrdpServerDiagnostics& diagnostics)
{
    const XrdpScreenCaptureDiagnostics capture = GetXrdpScreenCaptureDiagnostics();
    const XrdpVideoSubmitterStats video = VideoSubmitter().Snapshot();
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
        " lastExitCode=" + std::to_string(diagnostics.lastExitCode));
    diagnostics.logs.push_back("xrdp capture running=" + std::string(capture.running ? "true" : "false") +
        " target=" + std::to_string(capture.width) + "x" + std::to_string(capture.height) +
        "@" + std::to_string(capture.frameRate) + "fps" +
        " cursor=" + std::string(capture.showCursor ? "on" : "off") +
        " ready=" + std::to_string(capture.readyCount) +
        " queued=" + std::to_string(capture.submittedCount) +
        " dropped=" + std::to_string(capture.droppedCount) +
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

void ResetXrdpClientCaptureState(const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(ClientCaptureState().mutex);
        ClientCaptureState().requested = false;
        ClientCaptureState().width = 0;
        ClientCaptureState().height = 0;
    }
    StopXrdpScreenCapture(reason);
    ResetXrdpInputInjector(reason);
}

void StopXrdpCaptureForClient(const std::string& reason)
{
    {
        std::lock_guard<std::mutex> lock(ClientCaptureState().mutex);
        ClientCaptureState().requested = false;
        ClientCaptureState().width = 0;
        ClientCaptureState().height = 0;
    }
    StopXrdpScreenCapture(reason);
}

void StartXrdpCaptureForClient(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0 || width > 8192 || height > 8192) {
        return;
    }

    XrdpScreenCaptureOptions options {};
    options.width = width;
    options.height = height;
    options.frameRate = 15;
    options.showCursor = false;
    const XrdpDisplayGeometry geometry = QueryXrdpDisplayGeometry();
    bool restartCapture = false;

    {
        std::lock_guard<std::mutex> lock(ClientCaptureState().mutex);
        XrdpClientCaptureState& state = ClientCaptureState();
        if (state.requested && state.width == width && state.height == height) {
            return;
        }
        restartCapture = state.requested && (state.width != width || state.height != height);
        const auto now = std::chrono::steady_clock::now();
        if (state.lastFailure != std::chrono::steady_clock::time_point {} &&
            now - state.lastFailure < std::chrono::seconds(3)) {
            return;
        }
        state.requested = true;
        state.width = width;
        state.height = height;
    }

    EmitHilogInfo("xrdp active mstsc session detected; scheduling screen capture desktop=" +
        std::to_string(width) + "x" + std::to_string(height) + " " +
        FormatXrdpDisplayGeometry(geometry) +
        " inputMapping=desktop-content-fit-to-display" +
        (restartCapture ? " restartCapture=1" : " restartCapture=0"));

    std::thread([options, restartCapture]() {
        if (restartCapture) {
            StopXrdpScreenCapture("xrdp desktop size changed to " +
                std::to_string(options.width) + "x" + std::to_string(options.height));
        }
        std::string message;
        if (StartXrdpScreenCapture(options, message)) {
            EmitHilogInfo("xrdp client screen capture active: " + message);
            return;
        }

        {
            std::lock_guard<std::mutex> lock(ClientCaptureState().mutex);
            ClientCaptureState().requested = false;
            ClientCaptureState().lastFailure = std::chrono::steady_clock::now();
        }
        EmitHilogError("xrdp client screen capture failed: " + message);
    }).detach();
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
            " flags=" + std::to_string(event->flags));
    }

    if (event->width > 0 && event->height > 0) {
        UpdateXrdpScreenCaptureTarget(static_cast<uint32_t>(event->width),
            static_cast<uint32_t>(event->height));
    }

    switch (event->type) {
        case XRDP_OHOS_BACKEND_EVENT_SESSION_CONNECT:
            if (event->width > 0 && event->height > 0) {
                StartXrdpCaptureForClient(static_cast<uint32_t>(event->width),
                    static_cast<uint32_t>(event->height));
            }
            if (!g_xrdpInputAuthorizationPrimed.exchange(true)) {
                PrimeXrdpInputInjectorAuthorization("xrdp client connected");
            }
            break;
        case XRDP_OHOS_BACKEND_EVENT_SESSION_DISCONNECT:
            g_xrdpInputAuthorizationPrimed.store(false);
            ResetXrdpClientCaptureState("xrdp client disconnected");
            break;
        case XRDP_OHOS_BACKEND_EVENT_MONITOR_RESIZE:
        case XRDP_OHOS_BACKEND_EVENT_MONITOR_FULL_INVALIDATE:
            if (event->connected != 0 && event->width > 0 && event->height > 0) {
                StartXrdpCaptureForClient(static_cast<uint32_t>(event->width),
                    static_cast<uint32_t>(event->height));
            }
            break;
        case XRDP_OHOS_BACKEND_EVENT_SUPPRESS_OUTPUT:
            if (event->suppress != 0) {
                StopXrdpCaptureForClient("xrdp output suppressed");
            } else if (event->connected != 0 && event->width > 0 && event->height > 0) {
                StartXrdpCaptureForClient(static_cast<uint32_t>(event->width),
                    static_cast<uint32_t>(event->height));
            }
            break;
        default:
            break;
    }
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
        if (state.backend.setInputCallbackFn != nullptr) {
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
        auto setEventCallbackFn = reinterpret_cast<XrdpSetBackendEventCallbackFn>(
            dlsym(handle, "xrdp_ohos_backend_set_event_callback"));

        state.backend.handle = handle;
        state.backend.submitFrameFn = submitFn;
        state.backend.setInputCallbackFn = setInputCallbackFn;
        state.backend.setEventCallbackFn = setEventCallbackFn;
        state.backend.libraryPath = candidate;
        result.logs.push_back("xrdp OHOS backend loaded: " + candidate);
        if (setInputCallbackFn != nullptr) {
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
        ResetXrdpClientCaptureState("xrdp server exit");
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
