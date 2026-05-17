#include "native_bridge_context.h"

#include "bridge_log.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp_runtime.h"
#include "string_utils.h"
#include "surface/latest_frame_renderer.h"

#include <atomic>
#include <cstdint>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {
namespace {

std::atomic_uint64_t g_avc444SurfaceFrameCallbackCount{0};
SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;

void EmitNativeLog(const std::string& line)
{
    g_events.log.Emit(line);
}

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    return g_frameRenderer.Enqueue(frame, message, forceRender);
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

DecoderSurfaceTarget SnapshotDecoderSurfaceTarget()
{
    return g_surface.DecoderSurface();
}

bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
    std::string& error)
{
    return g_surface.EnsureAvc444SurfaceTargets(width, height, targets, error);
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
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const FreerdpLogFn& log)
{
    if (api.ohosAvcodecSetAvc444OutputSurfaces == nullptr) {
        log("OHOS AVC444 NativeImage surface registration skipped: FreeRDP symbol unavailable");
        return false;
    }

    Avc444SurfaceTargets targets;
    std::string error;
    if (!EnsureAvc444SurfaceTargets(width, height, targets, error)) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        log("OHOS AVC444 NativeImage surface registration failed: " + error);
        return false;
    }

    api.ohosAvcodecSetAvc444OutputSurfaces(
        targets.lumaWindow, targets.chromaWindow, targets.width, targets.height, TRUE);
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
        " route=disabled-until-compositor");
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
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface changed");
    RequestSurfaceRepaint("surface changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface destroyed");
}

void OnXComponentTouchEvent(OH_NativeXComponent*, void*)
{
    g_surface.OnTouchEvent();
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
    g_surface.Register(component, ok);
    if (ok) {
        g_events.log.Emit("XComponent callback registered: " + g_surface.Snapshot().id);
    }
    return ok;
}

bool NotifyBridgeSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
{
    const bool changed = g_surface.OnSurfaceLayout(width, height, message);
    if (changed) {
        EmitNativeLog(message);
        UpdateAvc420SurfaceOutputIfActive("surface layout changed");
        RequestRemoteDesktopResize(width, height, "surface layout changed");
        RequestSurfaceRepaint("surface layout changed");
    }
    return changed;
}

} // namespace rdp_bridge
