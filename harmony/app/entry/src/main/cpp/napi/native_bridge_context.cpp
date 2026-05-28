#include "napi/native_bridge_context.h"

#include "common/bridge_log.h"
#include "common/string_utils.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_runtime.h"
#include "input/xcomponent_input_bridge.h"
#include "surface/latest_frame_renderer.h"
#include "surface/render_output_owner.h"
#include "xrdp/xrdp_server_bridge.h"

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {
namespace {

SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;

void EmitNativeLog(const std::string& line)
{
    BridgeLogger::Debug(line);
}

class ResizeCoordinator {
public:
    void Reset(const std::string& reason)
    {
        bool cleared = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (pending_) {
                pending_ = false;
                skippedFrameCount_ = 0;
                cleared = true;
            }
        }
        if (cleared) {
            EmitNativeLog("resize target cleared after " + reason);
        }
    }

    void Begin(uint32_t width, uint32_t height, const std::string& reason)
    {
        if (width == 0 || height == 0) {
            return;
        }

        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_ = true;
            targetWidth_ = width;
            targetHeight_ = height;
            skippedFrameCount_ = 0;
            generation = ++generation_;
        }
        EmitNativeLog("resize target pending after " + reason + ": target=" +
            std::to_string(width) + "x" + std::to_string(height) +
            " generation=" + std::to_string(generation));
    }

    bool ShouldQueueFrame(const RgbaFrame& frame, std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pending_) {
            return true;
        }

        if (FrameMatchesTarget(frame.width, frame.height)) {
            pending_ = false;
            skippedFrameCount_ = 0;
            message = "resize target accepted by frame: target=" +
                std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
                " frame=" + std::to_string(frame.width) + "x" +
                std::to_string(frame.height) + " label=" + FrameLabel(frame);
            return true;
        }

        ++skippedFrameCount_;
        message = "resize pending target=" + std::to_string(targetWidth_) + "x" +
            std::to_string(targetHeight_) + " skipped frame=" +
            std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " label=" + FrameLabel(frame) + " skipped=" +
            std::to_string(skippedFrameCount_);
        return false;
    }

private:
    static constexpr uint32_t kFrameAlignmentTolerance = 16;

    static std::string FrameLabel(const RgbaFrame& frame)
    {
        return frame.label.empty() ? "frame" : frame.label;
    }

    bool FrameMatchesTarget(uint32_t width, uint32_t height) const
    {
        if (width == 0 || height == 0) {
            return false;
        }
        if (width == targetWidth_ && height == targetHeight_) {
            return true;
        }

        const uint32_t widthDelta = width > targetWidth_ ? width - targetWidth_ : targetWidth_ - width;
        const uint32_t heightDelta = height > targetHeight_ ? height - targetHeight_ : targetHeight_ - height;
        return widthDelta < kFrameAlignmentTolerance && heightDelta < kFrameAlignmentTolerance;
    }

    std::mutex mutex_;
    bool pending_ = false;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    uint64_t generation_ = 0;
    uint32_t skippedFrameCount_ = 0;
};

ResizeCoordinator g_resizeCoordinator;

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    static std::atomic_uint32_t xrdpQueuedLogCount{0};
    static std::atomic_uint32_t xrdpSkippedLogCount{0};

    std::string xrdpMessage;
    if (SubmitXrdpRgbaFrame(frame, xrdpMessage)) {
        const uint32_t count = ++xrdpQueuedLogCount;
        if (count <= 3 || count % 60 == 0) {
            EmitNativeLog("xrdp video frame queued from surface render: " + xrdpMessage);
        }
    } else if (!xrdpMessage.empty() && xrdpMessage != "xrdp server is not running") {
        const uint32_t count = ++xrdpSkippedLogCount;
        if (count <= 3 || count % 120 == 0) {
            EmitNativeLog("xrdp video frame skipped from surface render: " + xrdpMessage +
                " count=" + std::to_string(count));
        }
    }
    return g_surface.RenderRgbaFrame(frame);
}

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    if (IsAvc444GpuRenderOutputOwner()) {
        message = "render output owned by " + CurrentRenderOutputOwnerName();
        return false;
    }
    std::string resizeMessage;
    if (!g_resizeCoordinator.ShouldQueueFrame(frame, resizeMessage)) {
        message = resizeMessage;
        return false;
    }
    if (!resizeMessage.empty()) {
        EmitNativeLog(resizeMessage);
    }
    return g_frameRenderer.Enqueue(frame, message, forceRender);
}

void DropPendingRenderFrame(const std::string& reason)
{
    std::string message;
    if (g_frameRenderer.DropPending(reason, message)) {
        EmitNativeLog(message);
    }
}

void StartRenderPipeline()
{
    static std::atomic_uint32_t skipLogCount{0};
    if (IsAvc444GpuRenderOutputOwner()) {
        const uint32_t count = ++skipLogCount;
        if (count <= 3 || count % 60 == 0) {
            EmitNativeLog("GDI render pipeline start skipped: outputOwner=" +
                CurrentRenderOutputOwnerName() + " count=" + std::to_string(count));
        }
        return;
    }
    g_frameRenderer.SetCallbacks(RenderSurfaceRgbaFrame, EmitNativeLog);
    g_frameRenderer.Start();
}

void StopRenderPipeline()
{
    g_frameRenderer.Stop();
}

void ReleaseSurfaceRenderTarget(const std::string& reason)
{
    g_surface.ReleaseRenderTarget(reason);
}

DecoderSurfaceTarget SnapshotDecoderSurfaceTarget()
{
    return g_surface.DecoderSurface();
}

void RequestSurfaceRepaint(const std::string& reason)
{
    static std::atomic_uint32_t repaintLogCount{0};
    static std::atomic_uint32_t repaintSkipLogCount{0};
    std::string message;
    if (g_session.RequestCurrentFrameRender(reason, message)) {
        const uint32_t count = ++repaintLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog("Surface repaint queued after " + reason + ": " + message +
                " count=" + std::to_string(count));
        }
        return;
    }

    if (message.empty()) {
        return;
    }
    const uint32_t skipCount = ++repaintSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("Surface repaint skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

std::string BuildRenderStatsLog()
{
    return g_frameRenderer.BuildStatsLog();
}

void RequestRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& reason)
{
    static std::atomic_uint32_t resizeLogCount{0};
    static std::atomic_uint32_t resizeSkipLogCount{0};
    std::string message;
    if (g_session.RequestDynamicDesktopResize(width, height, reason, message)) {
        const uint32_t count = ++resizeLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog(message + " count=" + std::to_string(count));
        }
        return;
    }

    const uint32_t skipCount = ++resizeSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("display-control resize skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

void ConfigureRdpgfxPipelineCallbacks()
{
    SetRdpgfxPipelineCallbacks({
        SnapshotDecoderSurfaceTarget,
        StartRenderPipeline,
        StopRenderPipeline,
        ReleaseSurfaceRenderTarget,
        EmitNativeLog,
    });
}

void ConfigureRdpSessionCallbacks()
{
    g_session.SetCallbacks({
        [](const std::string& state) {
            g_events.state.Emit(state);
        },
        [](const std::string& line) {
            EmitNativeLog(line);
        },
        [](const std::string& message) {
            BridgeLogger::Error(message);
            g_events.error.Emit(message);
        },
        []() {
            return g_surface.Snapshot();
        },
        QueueSurfaceRgbaFrame,
        StartRenderPipeline,
        StopRenderPipeline,
        RequestSurfaceRepaint,
        BuildRenderStatsLog,
    });
}

void OnXComponentSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceCreated(component, window);
    g_resizeCoordinator.Reset("surface created");
    UpdateRdpgfxSurfaceTargetIfReady("surface created");
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
    UpdateRdpgfxSurfaceTargetIfReady("surface changed");
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    g_resizeCoordinator.Begin(snapshot.width, snapshot.height, "surface changed");
    DropPendingRenderFrame("surface changed");
    RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    const RenderOutputOwner previous = ExchangeRenderOutputOwner(RenderOutputOwner::Gdi);
    if (previous == RenderOutputOwner::Avc444Gpu) {
        EmitNativeLog("render output owner reset after surface destroyed: avc444-gpu -> gdi");
    }
    g_surface.OnSurfaceDestroyed(component, window);
    g_resizeCoordinator.Reset("surface destroyed");
    UpdateRdpgfxSurfaceTargetIfReady("surface destroyed");
    UpdateAvc420SurfaceOutputIfActive("surface destroyed");
}


} // namespace

SessionEventHub& BridgeEvents()
{
    return g_events;
}

RdpSession& BridgeSession()
{
    return g_session;
}

void InitializeNativeBridgeContext()
{
    ConfigureRdpgfxPipelineCallbacks();
    ConfigureRdpSessionCallbacks();
    ConfigureXComponentInputBridge(&g_session, EmitNativeLog);
}

bool RegisterNativeXComponent(napi_env env, napi_value exports)
{
    g_surface.SetLogSink(EmitNativeLog);

    napi_value nativeXComponentValue = nullptr;
    napi_status status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &nativeXComponentValue);
    if (status != napi_ok || nativeXComponentValue == nullptr) {
        return false;
    }

    OH_NativeXComponent* component = nullptr;
    status = napi_unwrap(env, nativeXComponentValue, reinterpret_cast<void**>(&component));
    if (status != napi_ok || component == nullptr) {
        return false;
    }

    static OH_NativeXComponent_Callback callback = {
        OnXComponentSurfaceCreated,
        OnXComponentSurfaceChanged,
        OnXComponentSurfaceDestroyed,
        OnXComponentTouchEvent,
    };

    int32_t rc = OH_NativeXComponent_RegisterCallback(component, &callback);
    const bool ok = rc == OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
    const XComponentInputRegisterResult inputRc = RegisterXComponentInputCallbacks(component);
    g_surface.Register(component, ok);
    if (ok) {
        EmitNativeLog("XComponent callback registered: " + g_surface.Snapshot().id +
            " mouseRc=" + std::to_string(inputRc.mouseRc) +
            " focusRc=" + std::to_string(inputRc.focusRc) +
            " blurRc=" + std::to_string(inputRc.blurRc) +
            " keyRc=" + std::to_string(inputRc.keyRc) +
            " softKeyboardRc=" + std::to_string(inputRc.softKeyboardRc) +
            " axisRc=" + std::to_string(inputRc.axisRc));
    }
    return ok;
}

} // namespace rdp_bridge
