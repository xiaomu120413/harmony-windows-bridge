#include "xrdp/xrdp_server_bridge.h"

#include "common/bridge_log.h"
#include "xrdp/xrdp_display_geometry.h"
#include "xrdp/xrdp_input_injector.h"
#include "xrdp/xrdp_screen_capture_bridge.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <condition_variable>
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

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

namespace rdp_bridge {
namespace {

constexpr const char* kDefaultRuntimeRoot = "/data/storage/el2/base/files/xrdp";
constexpr const char* kServerLibraryName = "libxrdpserver.so";
constexpr const char* kBackendLibraryName = "libxrdpohos.so";
constexpr uint32_t kDefaultPort = 3390;
constexpr int32_t kXrdpWmMouseMove = 100;

using XrdpMainFn = int (*)(int, char**);
using XrdpStopFn = int (*)(void);
using XrdpSubmitBgraFrameFn = int (*)(const void*, int, int, int);

using XrdpInputEventCallbackFn = void (*)(const XrdpOhosInputEvent*, void*);
using XrdpSetInputCallbackFn = int (*)(XrdpInputEventCallbackFn, void*);

struct XrdpLoadedServer {
    void* handle = nullptr;
    XrdpMainFn mainFn = nullptr;
    XrdpStopFn stopFn = nullptr;
    std::string libraryPath;
};

struct XrdpLoadedBackend {
    void* handle = nullptr;
    XrdpSubmitBgraFrameFn submitBgraFrameFn = nullptr;
    XrdpSetInputCallbackFn setInputCallbackFn = nullptr;
    std::string libraryPath;
};

struct XrdpServerState {
    std::mutex mutex;
    XrdpLoadedServer loaded;
    XrdpLoadedBackend backend;
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

class XrdpVideoFrameSubmitter {
public:
    bool Enqueue(const RgbaFrame& frame, XrdpSubmitBgraFrameFn submitFn, std::string& message)
    {
        if (submitFn == nullptr) {
            message = "xrdp video backend is not loaded";
            return false;
        }

        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
            frame.width > kMaxFrameDimension || frame.height > kMaxFrameDimension ||
            sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            message = "invalid xrdp video frame";
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
            if (lastStatus_ == -4 && lastStatusAt_ != std::chrono::steady_clock::time_point {} &&
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
        next.label = frame.label.empty() ? "video frame" : frame.label;
        next.pixelFormat = frame.pixelFormat;
        next.pixelBytes = frameBytes;
        next.pixels = AllocateBytes(frameBytes);
        if (next.pixels == nullptr) {
            message = "xrdp video frame allocation failed";
            return false;
        }

        const auto copyStart = std::chrono::steady_clock::now();
        for (uint32_t y = 0; y < frame.height; ++y) {
            const auto* source = frame.data + static_cast<size_t>(y) * static_cast<size_t>(sourceStride);
            auto* target = next.pixels.get() + static_cast<size_t>(y) * rowBytes;
            std::memcpy(target, source, rowBytes);
        }
        const auto copyEnd = std::chrono::steady_clock::now();
        next.copyUs = static_cast<uint32_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(copyEnd - copyStart).count());

        uint64_t queued = 0;
        uint64_t replaced = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            StartLocked();
            next.sequence = ++sequence_;
            if (hasPending_) {
                ++replacedCount_;
            }
            pending_ = std::move(next);
            hasPending_ = true;
            queued = ++queuedCount_;
            replaced = replacedCount_;
        }
        condition_.notify_one();

        message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " xrdp-video queued copy=" + std::to_string(next.copyUs / 1000.0) +
            "ms queued=" + std::to_string(queued) +
            " replaced=" + std::to_string(replaced);
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

private:
    static constexpr uint32_t kMaxFrameDimension = 8192;
    using ByteBuffer = std::unique_ptr<uint8_t[]>;

    struct PendingFrame {
        ByteBuffer pixels;
        size_t pixelBytes = 0;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t copyUs = 0;
        uint64_t sequence = 0;
        std::string label;
        FramePixelFormat pixelFormat = FramePixelFormat::Rgba;
        XrdpSubmitBgraFrameFn submitFn = nullptr;
    };

    static ByteBuffer AllocateBytes(size_t bytes)
    {
        return ByteBuffer(new (std::nothrow) uint8_t[bytes]);
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

    static bool ConvertRgbaToBgra(const PendingFrame& frame, ByteBuffer& bgra)
    {
        const size_t rowBytes = static_cast<size_t>(frame.width) * 4U;
        const size_t frameBytes = rowBytes * static_cast<size_t>(frame.height);
        if (frame.pixels == nullptr || frame.pixelBytes != frameBytes || frame.width == 0 || frame.height == 0) {
            return false;
        }

        bgra = AllocateBytes(frameBytes);
        if (bgra == nullptr) {
            return false;
        }
        for (uint32_t y = 0; y < frame.height; ++y) {
            const uint8_t* source = frame.pixels.get() + static_cast<size_t>(y) * rowBytes;
            uint8_t* target = bgra.get() + static_cast<size_t>(y) * rowBytes;
#if defined(__aarch64__)
            size_t x = 0;
            const uint8x16_t shuffle = { 2, 1, 0, 3, 6, 5, 4, 7, 10, 9, 8, 11, 14, 13, 12, 15 };
            for (; x + 16U <= rowBytes; x += 16U) {
                const uint8x16_t rgba = vld1q_u8(source + x);
                const uint8x16_t converted = vqtbl1q_u8(rgba, shuffle);
                vst1q_u8(target + x, converted);
            }
            for (; x < rowBytes; x += 4U) {
                target[x + 0U] = source[x + 2U];
                target[x + 1U] = source[x + 1U];
                target[x + 2U] = source[x + 0U];
                target[x + 3U] = source[x + 3U];
            }
#else
            for (uint32_t x = 0; x < frame.width; ++x) {
                const size_t offset = static_cast<size_t>(x) * 4U;
                uint32_t pixel = 0;
                std::memcpy(&pixel, source + offset, sizeof(pixel));
                pixel = (pixel & 0xFF00FF00U) | ((pixel & 0x000000FFU) << 16U) |
                    ((pixel & 0x00FF0000U) >> 16U);
                std::memcpy(target + offset, &pixel, sizeof(pixel));
            }
#endif
        }
        return true;
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
            }

            ByteBuffer bgra;
            const auto convertStart = std::chrono::steady_clock::now();
            const uint8_t* bgraData = nullptr;
            bool converted = false;
            if (frame.pixelFormat == FramePixelFormat::Bgra) {
                converted = frame.pixels != nullptr && frame.pixelBytes != 0U;
                bgraData = frame.pixels.get();
            } else {
                converted = ConvertRgbaToBgra(frame, bgra);
                bgraData = bgra.get();
            }
            const auto convertEnd = std::chrono::steady_clock::now();
            const uint32_t convertUs = static_cast<uint32_t>(
                std::chrono::duration_cast<std::chrono::microseconds>(convertEnd - convertStart).count());
            const int status = converted && frame.submitFn != nullptr ?
                frame.submitFn(bgraData, static_cast<int>(frame.width), static_cast<int>(frame.height),
                    static_cast<int>(frame.width * 4U)) :
                -1;

            uint64_t submitted = 0;
            uint64_t failed = 0;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                lastStatus_ = status;
                lastStatusAt_ = std::chrono::steady_clock::now();
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
                        "ms convert=" + std::to_string(convertUs / 1000.0) +
                        "ms pixel=" + std::string(frame.pixelFormat == FramePixelFormat::Bgra ? "bgra" : "rgba") +
                        " count=" + std::to_string(submitted) +
                        " label=" + frame.label);
                }
            } else if (status == -4) {
                if (failed <= 3 || (failed % 120U) == 0U) {
                    EmitHilogInfo("xrdp video frame skipped: no active mstsc session status=-4 count=" +
                        std::to_string(failed));
                }
            } else if (failed <= 3 || (failed % 120U) == 0U) {
                EmitHilogError("xrdp video frame submit failed: status=" + std::to_string(status) +
                    " size=" + std::to_string(frame.width) + "x" + std::to_string(frame.height) +
                    " count=" + std::to_string(failed));
            }
        }
    }

    std::mutex mutex_;
    std::condition_variable condition_;
    std::thread worker_;
    PendingFrame pending_;
    bool running_ = false;
    bool hasPending_ = false;
    uint64_t sequence_ = 0;
    uint64_t queuedCount_ = 0;
    uint64_t replacedCount_ = 0;
    uint64_t submittedCount_ = 0;
    uint64_t failedCount_ = 0;
    uint64_t backoffDropCount_ = 0;
    int lastStatus_ = 0;
    std::chrono::steady_clock::time_point lastStatusAt_;
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

    {
        std::lock_guard<std::mutex> lock(ClientCaptureState().mutex);
        XrdpClientCaptureState& state = ClientCaptureState();
        if (state.requested && state.width == width && state.height == height) {
            return;
        }
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
        " inputMapping=desktop-content-fit-to-display");

    std::thread([options]() {
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

void OnXrdpInputEvent(const XrdpOhosInputEvent* event, void*)
{
    static std::atomic<uint32_t> inputEventCount { 0 };
    static std::atomic<bool> inputAuthorizationPrimed { false };

    if (event == nullptr) {
        return;
    }

    const uint32_t count = inputEventCount.fetch_add(1) + 1;
    const bool sampledMove = event->msg == kXrdpWmMouseMove && (count <= 32 || (count % 256U) == 0U);

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

    if (event->width > 0 && event->height > 0) {
        UpdateXrdpScreenCaptureTarget(static_cast<uint32_t>(event->width),
            static_cast<uint32_t>(event->height));
    }
    if (event->connected == 0) {
        inputAuthorizationPrimed.store(false);
        ResetXrdpClientCaptureState("xrdp client disconnected");
        return;
    }
    if (event->width > 0 && event->height > 0) {
        StartXrdpCaptureForClient(static_cast<uint32_t>(event->width),
            static_cast<uint32_t>(event->height));
    }
    if (!inputAuthorizationPrimed.exchange(true)) {
        PrimeXrdpInputInjectorAuthorization("xrdp client connected");
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
    if (state.backend.handle != nullptr && state.backend.submitBgraFrameFn != nullptr) {
        result.logs.push_back("xrdp OHOS backend already loaded: " + state.backend.libraryPath);
        if (state.backend.setInputCallbackFn != nullptr) {
            const int rc = state.backend.setInputCallbackFn(OnXrdpInputEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend input callback register rc=" + std::to_string(rc));
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

        auto submitFn = reinterpret_cast<XrdpSubmitBgraFrameFn>(
            dlsym(handle, "xrdp_ohos_backend_submit_bgra_frame"));
        if (submitFn == nullptr) {
            result.logs.push_back("xrdp OHOS backend frame symbol missing in: " + candidate);
            dlclose(handle);
            continue;
        }
        auto setInputCallbackFn = reinterpret_cast<XrdpSetInputCallbackFn>(
            dlsym(handle, "xrdp_ohos_backend_set_input_callback"));

        state.backend.handle = handle;
        state.backend.submitBgraFrameFn = submitFn;
        state.backend.setInputCallbackFn = setInputCallbackFn;
        state.backend.libraryPath = candidate;
        result.logs.push_back("xrdp OHOS backend loaded: " + candidate);
        if (setInputCallbackFn != nullptr) {
            const int rc = setInputCallbackFn(OnXrdpInputEvent, nullptr);
            result.logs.push_back("xrdp OHOS backend input callback register rc=" + std::to_string(rc));
        } else {
            result.logs.push_back("xrdp OHOS backend input callback symbol missing in: " + candidate);
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
    result.logs.push_back("nativeLibDir=" + paths.nativeLibDir);
    result.logs.push_back("runtimeRoot=" + paths.runtimeRoot);
    result.logs.push_back("configPath=" + paths.configPath);
    result.logs.push_back("modulePath=" + paths.modulePath);
    result.logs.push_back("sharePath=" + paths.sharePath);
}

} // namespace

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
        if (!LoadBackendLocked(params, paths, result)) {
            result.logs.push_back("xrdp server can still start, but external frame push is unavailable");
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
        ResetXrdpClientCaptureState("xrdp server exit");
        VideoSubmitter().Stop("xrdp server exit");
        EmitHilogInfo(threadState.lastMessage);
    }).detach();

    result.ok = true;
    result.state = "Listening";
    result.message = "xrdp server start requested on port " + std::to_string(port);
    result.logs.push_back(result.message);
    result.logs.push_back("xrdp raw screen capture waits for an active mstsc session");
    EmitHilogInfo(result.message);
    return result;
}

bool QueueXrdpVideoFrame(const RgbaFrame& frame, std::string& message)
{
    XrdpSubmitBgraFrameFn submitFn = nullptr;

    {
        std::lock_guard<std::mutex> lock(ServerState().mutex);
        XrdpServerState& state = ServerState();
        if (!state.running.load()) {
            message = "xrdp server is not running";
            return false;
        }
        submitFn = state.backend.submitBgraFrameFn;
    }

    return VideoSubmitter().Enqueue(frame, submitFn, message);
}

} // namespace rdp_bridge
