#include "napi/native_bridge_context.h"

#include "common/bridge_log.h"
#include "common/string_utils.h"
#include "channels/rdpgfx_pipeline.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_diagnostics.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/freerdp_runtime.h"
#include "input/xcomponent_input_bridge.h"
#include "input/remote_ime_client.h"
#include "session/rdp_display_layout_monitor.h"
#include "session/rdp_display_request_coalescer.h"
#include "session/rdp_display_resize_coordinator.h"
#include "surface/latest_frame_renderer.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {
namespace {

SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;
RdpDisplayResizeCoordinator g_resizeCoordinator;
RdpDisplayRequestCoalescer g_resizeRequestCoalescer;
RdpDisplayLayoutMonitor g_displayLayoutMonitor;
RemoteImeClient g_remoteIme;
std::atomic_bool g_multimonActive {false};

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

DisplayResizeResult RequestRemoteDesktopResize(const DisplayResizeRequest& request)
{
    return g_session.RequestDynamicDesktopResizeEx(request);
}

void ApplyRemoteDesktopResize(const DisplayResizeRequest& request)
{
    const DisplayResizeResult resizeResult = RequestRemoteDesktopResize(request);
    if (resizeResult.status == DisplayResizeStatus::Sent) {
        g_resizeCoordinator.ApplyResult(resizeResult, request.reason);
        DropPendingRenderFrame(request.reason);
    } else if (resizeResult.status != DisplayResizeStatus::Unchanged) {
        g_resizeCoordinator.ApplyResult(resizeResult, request.reason);
    }
    BridgeLogger::LogPublic(BridgeLogLevel::Info,
        std::string("RDP_DISPLAY event=resize_request source=") + request.reason + " requested=" +
            std::to_string(request.width) + "x" + std::to_string(request.height) + " orientation=" +
            std::to_string(request.orientation) + " physical_mm=" +
            std::to_string(request.physicalWidth) + "x" +
            std::to_string(request.physicalHeight) + " scale=" +
            std::to_string(request.desktopScaleFactor) + "/" +
            std::to_string(request.deviceScaleFactor) + " normalized=" +
            std::to_string(resizeResult.normalizedWidth) + "x" +
            std::to_string(resizeResult.normalizedHeight) + " sent=" +
            std::to_string(resizeResult.sentWidth) + "x" +
            std::to_string(resizeResult.sentHeight) + " sent_orientation=" +
            std::to_string(resizeResult.orientation) + " status=" +
            DisplayResizeStatusName(resizeResult.status));
    if (resizeResult.status != DisplayResizeStatus::Sent) {
        RequestSurfaceRepaint(request.reason + " current-frame repaint");
    }
}

uint32_t ScalePhysicalDimension(uint32_t surfacePixels, uint32_t displayPixels,
    uint32_t displayMillimeters)
{
    if (displayPixels == 0 || displayMillimeters == 0) {
        return std::clamp(surfacePixels, 10U, 10000U);
    }
    const auto millimeters = static_cast<long>(std::lround(
        static_cast<double>(surfacePixels) * displayMillimeters / displayPixels));
    return static_cast<uint32_t>(std::clamp(millimeters, 10L, 10000L));
}

DisplayResizeRequest CurrentSurfaceResizeRequest(const std::string& reason)
{
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    DisplayResizeRequest request;
    request.width = snapshot.width;
    request.height = snapshot.height;
    request.orientation = g_session.DisplayOrientation();
    request.reason = reason;
    const auto layout = g_displayLayoutMonitor.Snapshot();
    if (layout.size() == 1) {
        const auto& monitor = layout.front();
        request.physicalWidth = ScalePhysicalDimension(
            snapshot.width, monitor.width, monitor.physicalWidth);
        request.physicalHeight = ScalePhysicalDimension(
            snapshot.height, monitor.height, monitor.physicalHeight);
        request.orientation = monitor.orientation;
        request.desktopScaleFactor = monitor.desktopScaleFactor;
        request.deviceScaleFactor = monitor.deviceScaleFactor;
    } else {
        request.physicalWidth = std::clamp(snapshot.width, 10U, 10000U);
        request.physicalHeight = std::clamp(snapshot.height, 10U, 10000U);
    }
    return request;
}

void UpdateDesiredSingleMonitorLayout(const DisplayResizeRequest& request)
{
    if (request.width == 0 || request.height == 0) {
        EmitNativeLog("desired single monitor layout skipped: source=" + request.reason +
            " reason=empty_surface");
        return;
    }
    const bool sessionConnected = g_session.IsConnected();
    FREERDP_OHOS_MONITOR_LAYOUT monitor {
        sizeof(FREERDP_OHOS_MONITOR_LAYOUT), FREERDP_OHOS_MONITOR_LAYOUT_VERSION,
        0, 0, request.width, request.height, request.physicalWidth, request.physicalHeight,
        request.orientation, request.desktopScaleFactor, request.deviceScaleFactor, TRUE,
    };
    std::string message;
    if (!g_session.SetMonitorLayout({monitor}, message)) {
        EmitNativeLog("desired single monitor layout update failed: source=" + request.reason +
            " session_connected=" + (sessionConnected ? "yes" : "no") + " detail=" + message);
        return;
    }
    EmitNativeLog("desired single monitor layout updated: source=" + request.reason +
        " session_connected=" + (sessionConnected ? "yes" : "no") + " size=" +
        std::to_string(request.width) + "x" + std::to_string(request.height) +
        " physical_mm=" + std::to_string(request.physicalWidth) + "x" +
        std::to_string(request.physicalHeight) + " scale=" +
        std::to_string(request.desktopScaleFactor) + "/" +
        std::to_string(request.deviceScaleFactor) + " orientation=" +
        std::to_string(request.orientation) + " detail=" + message);
}

void ScheduleCurrentSurfaceResize(const std::string& reason)
{
    if (g_multimonActive.load()) {
        return;
    }
    const DisplayResizeRequest request = CurrentSurfaceResizeRequest(reason);
    if (request.width > 0 && request.height > 0) {
        if (!g_session.IsConnected()) {
            UpdateDesiredSingleMonitorLayout(request);
            return;
        }
        g_resizeRequestCoalescer.Schedule(request);
    }
}

void FlushCurrentSurfaceResize(const std::string& reason)
{
    const DisplayResizeRequest request = CurrentSurfaceResizeRequest(reason);
    if (request.width > 0 && request.height > 0) {
        if (!g_session.IsConnected()) {
            UpdateDesiredSingleMonitorLayout(request);
            return;
        }
        g_resizeRequestCoalescer.Flush(request);
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
                ReleaseAllXComponentInput("disconnected");
                g_resizeCoordinator.Reset("session disconnected");
                std::string imeMessage;
                (void)g_remoteIme.Close(imeMessage);
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
    RefreshXComponentInputDensity();
    g_resizeCoordinator.Reset("surface created");
    UpdateRdpgfxSurfaceTargetIfReady("surface created");
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
    ScheduleCurrentSurfaceResize("surface_created_stable");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    ReleaseAllXComponentInput("surfaceChanged");
    g_surface.OnSurfaceChanged(component, window);
    RefreshXComponentInputDensity();
    UpdateRdpgfxSurfaceTargetIfReady("surface changed");
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    ScheduleCurrentSurfaceResize("surface_changed_stable");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    ReleaseAllXComponentInput("surfaceDestroyed");
    std::string imeMessage;
    (void)g_remoteIme.Close(imeMessage);
    g_surface.OnSurfaceDestroyed(component, window);
    g_resizeRequestCoalescer.Cancel();
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
    ReleaseAllXComponentInput("orientationChanged");
    g_session.SetDisplayOrientation(orientation);
    RefreshXComponentInputDensity();

    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    message = "display orientation updated";
    BridgeLogger::LogPublic(BridgeLogLevel::Info,
        std::string("RDP_DISPLAY event=orientation_change previous=") +
            std::to_string(previous) + " current=" + std::to_string(orientation) +
            " surface=" + std::to_string(snapshot.width) + "x" +
            std::to_string(snapshot.height) + " ready=" + (snapshot.ready ? "true" : "false"));
    return true;
}

bool BindImeHostWindow(uint32_t windowId, std::string& message)
{
    if (windowId == 0) {
        message = "IME host windowId must be non-zero";
        return false;
    }
    g_remoteIme.BindHostWindow(&g_session, windowId);
    message = "IME host window bound: windowId=" + std::to_string(windowId);
    return true;
}

void InitializeNativeBridgeContext()
{
    g_resizeRequestCoalescer.SetCallback(ApplyRemoteDesktopResize);
    ConfigureRdpgfxPipelineCallbacks();
    ConfigureRdpSessionCallbacks();
    ConfigureXComponentInputBridge(&g_session, &g_remoteIme, EmitNativeLog);
    std::string layoutMessage;
    if (!g_displayLayoutMonitor.Start(
        [](uint32_t orientation, const std::vector<FREERDP_OHOS_MONITOR_LAYOUT>& layout,
            const std::string& source) {
            const bool multimon = layout.size() > 1;
            const bool wasMultimon = g_multimonActive.exchange(multimon);
            const bool orientationChanged = g_session.DisplayOrientation() != orientation;
            if (multimon) {
                g_resizeRequestCoalescer.Cancel();
            }
            std::string updateMessage;
            if (!UpdateDisplayOrientation(orientation, updateMessage)) {
                EmitNativeLog("native display orientation rejected: source=" + source + " " +
                    updateMessage);
            }
            if (multimon) {
                std::string monitorMessage;
                if (!g_session.SetMonitorLayout(layout, monitorMessage)) {
                    EmitNativeLog("native multimon layout failed: source=" + source + " " +
                        monitorMessage);
                }
                return;
            }
            if (wasMultimon) {
                UpdateDesiredSingleMonitorLayout(
                    CurrentSurfaceResizeRequest("multimon_to_single"));
            } else {
                ScheduleCurrentSurfaceResize(orientationChanged ?
                    "orientation_changed_stable" : "native_display_changed_stable");
            }
        }, EmitNativeLog, layoutMessage)) {
        EmitNativeLog(layoutMessage);
    }
}

bool RegisterNativeXComponentInstance(OH_NativeXComponent* component)
{
    if (component == nullptr) {
        return false;
    }
    g_surface.SetLogSink(EmitNativeLog);

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

std::string BuildNativeDiagnostics()
{
    const SurfaceSnapshot surface = g_surface.Snapshot();
    const auto monitors = g_displayLayoutMonitor.Snapshot();
    const DisplayResizeCoordinatorSnapshot resize = g_resizeCoordinator.Snapshot();
    std::string runtimeError;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    const bool runtimeLoaded = EnsureFreerdpRuntimeLoaded(api, runtimeError);
    std::ostringstream out;
    const char* resizeState = resize.state == DisplayResizeWaitState::WaitingForTarget ? "waiting" :
        resize.state == DisplayResizeWaitState::Fallback ? "fallback" : "idle";
    out << "schema=2"
        << " session=" << (g_session.IsConnected() ? "connected" : "idle")
        << " session_id=" << g_session.DiagnosticSessionId()
        << " orientation=" << g_session.DisplayOrientation()
        << " multimon=" << (g_multimonActive.load() ? "active" : "inactive")
        << " monitors=" << monitors.size()
        << " surface_registered=" << (surface.registered ? "yes" : "no")
        << " surface_ready=" << (surface.ready ? "yes" : "no")
        << " surface=" << surface.width << "x" << surface.height
        << " viewport=" << surface.viewportX << "," << surface.viewportY << ","
        << surface.viewportWidth << "x" << surface.viewportHeight
        << " remote=" << surface.viewportRemoteWidth << "x" << surface.viewportRemoteHeight
        << " paints=" << surface.paintCount
        << " input_depth=" << g_session.InputQueueDepth()
        << " input_queued=" << g_session.InputQueuedCount()
        << " input_sent=" << g_session.InputSentCount()
        << " input_dropped=" << g_session.InputDroppedCount()
        << " resize_state=" << resizeState
        << " resize_generation=" << resize.targetGeneration
        << " resize_target=" << resize.targetWidth << "x" << resize.targetHeight
        << " resize_skipped=" << resize.skippedFrameCount
        << " runtime=" << (runtimeLoaded ? "loaded" : "unavailable")
        << " channel_clipboard=" << (runtimeLoaded && api.ohosClipboardGetDiagnostics != nullptr ?
            "available" : "unavailable")
        << " channel_rdpsnd=" << (runtimeLoaded && api.rdpsndOhosGetDiagnostics != nullptr ?
            "available" : "unavailable")
        << " channel_audin=" << (runtimeLoaded && api.audinOhosGetDiagnostics != nullptr ?
            "available" : "unavailable")
        << " channel_rdpecam=" << (runtimeLoaded && api.rdpecamOhosSetPermissionCallback != nullptr ?
            "available" : "unavailable")
        << " channel_location=" << (runtimeLoaded && api.ohosLocationSetPermissionCallback != nullptr ?
            "available" : "unavailable")
        << " | " << g_frameRenderer.BuildStatsLog()
        << " | " << BuildGraphicsPipelineStatsLog()
        << " | " << BuildOHAudioStatsLog()
        << " | " << SnapshotClipboardDiagnostics();
    return out.str();
}

} // namespace rdp_bridge
