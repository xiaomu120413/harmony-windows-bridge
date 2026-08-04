#include "napi/native_bridge_context.h"

#include "common/bridge_log.h"
#include "common/string_utils.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/freerdp_runtime.h"
#include "input/xcomponent_input_bridge.h"
#include "session/rdp_display_resize_coordinator.h"
#include "surface/latest_frame_renderer.h"
#include "surface/render_output_owner.h"

#include <cstdint>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {
namespace {

SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;
RdpDisplayResizeCoordinator g_resizeCoordinator;

void EmitNativeLog(const std::string& line)
{
    BridgeLogger::Debug(line);
}

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    if (CurrentRenderOutputOwner() != RenderOutputOwner::Gdi) {
        message = "render output owned by " + CurrentRenderOutputOwnerName();
        return false;
    }
    std::string resizeMessage;
    if (!g_resizeCoordinator.ShouldQueueFrame(frame.width, frame.height,
        frame.label.empty() ? "frame" : frame.label, resizeMessage)) {
        message = resizeMessage;
        return false;
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
    if (CurrentRenderOutputOwner() != RenderOutputOwner::Gdi) {
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
    std::string message;
    (void)g_session.RequestCurrentFrameRender(reason, message);
}

std::string BuildRenderStatsLog()
{
    return g_frameRenderer.BuildStatsLog();
}

DisplayResizeResult RequestRemoteDesktopResize(uint32_t width, uint32_t height,
    const std::string& reason)
{
    return g_session.RequestDynamicDesktopResizeEx(
        width, height, g_session.DisplayOrientation(), reason);
}

void ApplyRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& source)
{
    DropPendingRenderFrame(source);
    const DisplayResizeResult resizeResult = RequestRemoteDesktopResize(width, height, source);
    g_resizeCoordinator.ApplyResult(resizeResult, source);
    BridgeLogger::LogPublic(BridgeLogLevel::Info,
        std::string("RDP_DISPLAY event=resize_request source=") + source + " requested=" +
            std::to_string(width) + "x" + std::to_string(height) + " orientation=" +
            std::to_string(g_session.DisplayOrientation()) + " normalized=" +
            std::to_string(resizeResult.normalizedWidth) + "x" +
            std::to_string(resizeResult.normalizedHeight) + " sent=" +
            std::to_string(resizeResult.sentWidth) + "x" +
            std::to_string(resizeResult.sentHeight) + " sent_orientation=" +
            std::to_string(resizeResult.orientation) + " status=" +
            DisplayResizeStatusName(resizeResult.status));
    if (resizeResult.status != DisplayResizeStatus::Sent) {
        RequestSurfaceRepaint(source + " immediate fallback");
    }
}

void ConfigureRdpgfxPipelineCallbacks()
{
    SetRdpgfxPipelineCallbacks({
        SnapshotDecoderSurfaceTarget,
        StartRenderPipeline,
        StopRenderPipeline,
        ReleaseSurfaceRenderTarget,
        SnapshotRdpPrimaryFrame,
        EmitNativeLog,
    });
}

void ConfigureRdpSessionCallbacks()
{
    g_session.SetCallbacks({
        [](const std::string& state) {
            if (state == "Disconnected") {
                g_resizeCoordinator.Reset("session disconnected");
            }
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
    g_resizeCoordinator.SetTimeoutCallback([](uint64_t generation, const std::string& reason) {
        if (!g_resizeCoordinator.IsFallbackGeneration(generation)) {
            return;
        }
        EmitNativeLog(reason + " generation=" + std::to_string(generation));
        RequestSurfaceRepaint("resize timeout fallback");
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
    ApplyRemoteDesktopResize(snapshot.width, snapshot.height, "surface_changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
    g_resizeCoordinator.Reset("surface destroyed");
    UpdateRdpgfxSurfaceTargetIfReady("surface destroyed");
    UpdateAvc420SurfaceOutputIfActive("surface destroyed");

    const RenderOutputOwnerTransition transition = TransitionRenderOutputOwner(
        RenderOutputOwner::Gdi, RenderOutputOwnerTransitionReason::SurfaceDestroyed);
    if (transition.previous == RenderOutputOwner::Avc444Gpu ||
        transition.previous == RenderOutputOwner::Avc420Gpu) {
        EmitNativeLog("render output owner reset after surface destroyed: " +
            RenderOutputOwnerName(transition.previous) + " -> gdi transitionReason=" +
            RenderOutputOwnerTransitionReasonName(transition.reason));
    }
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

bool UpdateDisplayOrientation(uint32_t orientation, std::string& message)
{
    if (orientation != ORIENTATION_LANDSCAPE && orientation != ORIENTATION_PORTRAIT &&
        orientation != ORIENTATION_LANDSCAPE_FLIPPED &&
        orientation != ORIENTATION_PORTRAIT_FLIPPED) {
        message = "display orientation must be 0, 90, 180, or 270";
        return false;
    }

    const uint32_t previous = g_session.DisplayOrientation();
    if (previous == orientation) {
        message = "display orientation unchanged";
        return true;
    }
    g_session.SetDisplayOrientation(orientation);

    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    message = "display orientation updated";
    BridgeLogger::LogPublic(BridgeLogLevel::Info,
        std::string("RDP_DISPLAY event=orientation_change previous=") +
            std::to_string(previous) + " current=" + std::to_string(orientation) +
            " surface=" + std::to_string(snapshot.width) + "x" +
            std::to_string(snapshot.height) + " ready=" + (snapshot.ready ? "true" : "false"));
    if (snapshot.ready && snapshot.width > 0 && snapshot.height > 0) {
        ApplyRemoteDesktopResize(snapshot.width, snapshot.height, "orientation_changed");
    }
    return true;
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
    (void)inputRc;
    g_surface.Register(component, ok);
    return ok;
}

} // namespace rdp_bridge
