#include "native_bridge_context.h"

#include "bridge_log.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp_runtime.h"
#include "input/xcomponent_input_bridge.h"
#include "string_utils.h"
#include "surface/latest_frame_renderer.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {
namespace {

std::atomic_uint64_t g_avc444SurfaceFrameCallbackCount{0};
SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::mutex g_avcSurfacePoolMutex;
freerdpOhosAvcSurfacePool* g_avcSurfacePool = nullptr;
#endif

void EmitNativeLog(const std::string& line)
{
    g_events.log.Emit(line);
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
    return g_surface.RenderRgbaFrame(frame);
}

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
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

void OnAvc444SurfaceFrameDecoded(uint32_t surfaceId, uint32_t width, uint32_t height,
    uint32_t op, uint32_t codecId, void*)
{
    const uint64_t count = ++g_avc444SurfaceFrameCallbackCount;
    if (count <= 3 || (count % 120) == 0) {
        EmitNativeLog("OHOS AVC444 surface frame callback: count=" + std::to_string(count) +
            " surfaceId=" + std::to_string(surfaceId) +
            " size=" + std::to_string(width) + "x" + std::to_string(height) +
            " op=" + std::to_string(op) +
            " codec=" + Hex32(codecId));
    }
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    NotifyOhosCompositorAvc444Frame(
        SharedFreerdpRuntimeApi(), surfaceId, width, height, op, codecId);
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
freerdpOhosAvcSurfacePool* EnsureAvcSurfacePool(FreerdpRuntimeApi& api, std::string& error)
{
    std::lock_guard<std::mutex> lock(g_avcSurfacePoolMutex);
    if (g_avcSurfacePool != nullptr) {
        return g_avcSurfacePool;
    }
    if (api.ohosAvcSurfacePoolNew == nullptr) {
        error = "FreeRDP OHOS AVC surface pool symbol unavailable";
        return nullptr;
    }
    g_avcSurfacePool = api.ohosAvcSurfacePoolNew();
    if (g_avcSurfacePool == nullptr) {
        error = "FreeRDP OHOS AVC surface pool allocation failed";
        return nullptr;
    }
    return g_avcSurfacePool;
}

void DestroyAvcSurfacePool(FreerdpRuntimeApi& api, const std::string& reason)
{
    std::lock_guard<std::mutex> lock(g_avcSurfacePoolMutex);
    if (g_avcSurfacePool == nullptr) {
        return;
    }
    if (api.ohosAvcSurfacePoolDestroy != nullptr) {
        api.ohosAvcSurfacePoolDestroy(g_avcSurfacePool);
        EmitNativeLog("OHOS AVC NativeImage decode surfaces destroyed after " + reason);
    }
}

void ResetAvc444DecodeSurfaces(const std::string& reason)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
    }
    DestroyAvcSurfacePool(api, reason);
}

bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const FreerdpLogFn& log)
{
    if (api.ohosAvcodecSetAvc444OutputSurfaces == nullptr) {
        log("OHOS AVC444 NativeImage surface registration skipped: FreeRDP symbol unavailable");
        return false;
    }
    if (api.ohosAvcSurfacePoolEnsureAvc444 == nullptr) {
        log("OHOS AVC444 NativeImage surface registration skipped: OHOS client surface pool symbol unavailable");
        return false;
    }

    std::string error;
    freerdpOhosAvcSurfacePool* pool = EnsureAvcSurfacePool(api, error);
    if (pool == nullptr) {
        log("OHOS AVC444 NativeImage surface registration failed: " + error);
        return false;
    }

    FREERDP_OHOS_AVC444_SURFACE_TARGETS targets {};
    std::array<char, 256> message {};
    if (!api.ohosAvcSurfacePoolEnsureAvc444(pool, width, height, &targets, message.data(),
            message.size())) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        log("OHOS AVC444 NativeImage surface registration failed: " +
            std::string(message[0] == '\0' ? "unknown error" : message.data()));
        return false;
    }

    api.ohosAvcodecSetAvc444OutputSurfaces(
        targets.lumaWindow, targets.chromaWindow, targets.width, targets.height, TRUE);
    if (!RegisterOhosCompositorAvc444DecodeSurfaces(api, targets, log)) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        return false;
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(OnAvc444SurfaceFrameDecoded, nullptr);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    log("OHOS AVC444 NativeImage decode surfaces registered: " +
        std::to_string(targets.width) + "x" + std::to_string(targets.height) +
        " lumaTex=" + std::to_string(targets.lumaTexture) +
        " chromaTex=" + std::to_string(targets.chromaTexture) +
        " lumaSurface=" + std::to_string(targets.lumaSurfaceId) +
        " chromaSurface=" + std::to_string(targets.chromaSurfaceId) +
        " route=disabled-until-avc444-negotiated owner=FreeRDP-client-OHOS");
    return true;
}
#endif

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
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        RegisterAvc444DecodeSurfaces,
#endif
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
            g_events.log.Emit(line);
        },
        [](const std::string& message) {
            g_events.error.Emit(message);
        },
        []() {
            return g_surface.Snapshot();
        },
        QueueSurfaceRgbaFrame,
        StartRenderPipeline,
        StopRenderPipeline,
        RequestSurfaceRepaint,
    });
}

void OnXComponentSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceCreated(component, window);
    g_resizeCoordinator.Reset("surface created");
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    ResetAvc444DecodeSurfaces("surface changed");
#endif
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    g_resizeCoordinator.Begin(snapshot.width, snapshot.height, "surface changed");
    DropPendingRenderFrame("surface changed");
    RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
    g_resizeCoordinator.Reset("surface destroyed");
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    ResetAvc444DecodeSurfaces("surface destroyed");
#endif
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

SurfaceSnapshot BridgeSurfaceSnapshot()
{
    return g_surface.Snapshot();
}

std::string BridgeRenderStatsLog()
{
    return g_frameRenderer.BuildStatsLog();
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
        g_events.log.Emit("XComponent callback registered: " + g_surface.Snapshot().id +
            " mouseRc=" + std::to_string(inputRc.mouseRc) +
            " focusRc=" + std::to_string(inputRc.focusRc) +
            " blurRc=" + std::to_string(inputRc.blurRc) +
            " keyRc=" + std::to_string(inputRc.keyRc) +
            " softKeyboardRc=" + std::to_string(inputRc.softKeyboardRc) +
            " axisRc=" + std::to_string(inputRc.axisRc));
    }
    return ok;
}

bool NotifyBridgeSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
{
    const bool changed = g_surface.OnSurfaceLayout(width, height, message);
    if (changed) {
        EmitNativeLog(message);
        UpdateAvc420SurfaceOutputIfActive("surface layout changed");
        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        g_resizeCoordinator.Begin(snapshot.width, snapshot.height, "surface layout changed");
        DropPendingRenderFrame("surface layout changed");
        RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface layout changed");
    }
    return changed;
}

} // namespace rdp_bridge
