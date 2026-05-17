#include "napi/native_api.h"
#include "bridge_types.h"
#include "bridge_log.h"
#include "certificate_policy.h"
#include "frame_utils.h"
#include "freerdp_gdi_bridge.h"
#include "freerdp_runtime.h"
#include "graphics_config.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_diagnostics.h"
#include "channels/rdpgfx_pipeline.h"
#include "surface/avc444_surface_pool.h"
#include "surface/gpu_rgba_renderer.h"
#include "surface/latest_frame_renderer.h"
#include "surface/surface_bridge.h"
#include "napi_event_sink.h"
#include "napi_utils.h"
#include "net_utils.h"
#include "probe_utils.h"
#include "rdp_channel_config.h"
#include "rdp_session_core.h"
#include "string_utils.h"

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <dlfcn.h>
#include <fcntl.h>
#include <functional>
#include <iomanip>
#include <limits>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <new>
#include <poll.h>
#include <string>
#include <sys/stat.h>
#include <sys/socket.h>
#include <thread>
#include <unordered_map>
#include <unistd.h>
#include <vector>

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <native_buffer/native_buffer.h>
#include <native_image/native_image.h>
#include <native_window/external_window.h>
#include <database/pasteboard/oh_pasteboard.h>
#include <database/pasteboard/oh_pasteboard_err_code.h>
#include <database/udmf/udmf.h>
#include <database/udmf/udmf_err_code.h>
#include <database/udmf/uds.h>
#include <hilog/log.h>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/addin.h>
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/rdpgfx.h>
#include <freerdp/codec/color.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gfx.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <freerdp/update.h>
#include <winpr/clipboard.h>
#include <winpr/synch.h>
#endif

namespace {

using namespace rdp_bridge;

#ifndef GL_TEXTURE_EXTERNAL_OES
#define GL_TEXTURE_EXTERNAL_OES 0x8D65
#endif

std::atomic_uint64_t g_avc444SurfaceFrameCallbackCount{0};

void EmitNativeLog(const std::string& line);
SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame);
bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender = false);
void RequestSurfaceRepaint(const std::string& reason);
void RequestRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& reason);
void StartRenderPipeline();
void StopRenderPipeline();
DecoderSurfaceTarget SnapshotDecoderSurfaceTarget();
bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const std::function<void(const std::string&)>& log);
void OnAvc444SurfaceFrameDecoded(uint32_t surfaceId, uint32_t width, uint32_t height,
    uint32_t op, uint32_t codecId, void*);
std::string BuildRenderStatsLog();
SessionEventHub g_events;

void EmitNativeLog(const std::string& line)
{
    g_events.log.Emit(line);
}



SurfaceBridge g_surface;

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

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

LatestFrameRenderer g_frameRenderer;
RdpSession g_session;

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

std::string BuildRenderStatsLog()
{
    return g_frameRenderer.BuildStatsLog();
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

ConnectParams ReadConnectParams(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = {nullptr};
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    ConnectParams params;
    if (argc < 1 || args[0] == nullptr) {
        return params;
    }

    napi_valuetype type = napi_undefined;
    napi_typeof(env, args[0], &type);
    if (type != napi_object) {
        return params;
    }

    params.host = GetStringProperty(env, args[0], "host");
    params.port = GetStringProperty(env, args[0], "port");
    params.username = GetStringProperty(env, args[0], "username");
    params.password = GetStringProperty(env, args[0], "password");
    params.resolution = GetStringProperty(env, args[0], "resolution");
    params.certPolicy = GetStringProperty(env, args[0], "certPolicy");
    params.graphicsMode = GetStringProperty(env, args[0], "graphicsMode");
    params.appFilesDir = GetStringProperty(env, args[0], "appFilesDir");
    return params;
}

napi_value Probe(napi_env env, napi_callback_info info)
{
    FreerdpProbeResult freerdp = LoadFreerdpProbe();
    SurfaceSnapshot surface = g_surface.Snapshot();
    const std::string featureSummary =
        "core RDP/TLS/NLA + queued software GDI renderer; client channels on; "
        "cliprdr/rdpdr/drive/printer/smartcard/rdpsnd/audin/rdpgfx/disp compiled; "
        "H264 + FFmpeg + OpenH264 enabled; RD Gateway core enabled; "
        "static cliprdr text bridge, disp dynamic resolution, and rdpsnd/OHAudio playback requested; "
        "rdpgfx runtime gated by graphicsMode; other optional channel negotiation off";

    const std::string audioStats = BuildOHAudioStatsLog();
    const std::string renderStats = BuildRenderStatsLog();
    const std::string graphicsStats = BuildGraphicsPipelineStatsLog();

    napi_value result = MakeObject(env);
    SetString(env, result, "bridgeVersion", "0.8.3");
    SetString(env, result, "abi", CurrentAbi());
    SetString(env, result, "freeRdpVersion", freerdp.freerdpVersion);
    SetString(env, result, "winprVersion", freerdp.winprVersion);
    SetString(env, result, "opensslVersion", freerdp.opensslVersion);
    SetString(env, result, "featureSummary", featureSummary);
    SetString(env, result, "audioStats", audioStats);
    SetString(env, result, "renderStats", renderStats);
    SetString(env, result, "graphicsStats", graphicsStats);
    SetString(env, result, "inputDispatchMode", "worker-thread-queue");
    SetString(env, result, "probeJson", freerdp.json);
    SetString(env, result, "probeError", freerdp.error);
    SetBool(env, result, "freeRdpLinked", freerdp.linked);
    SetBool(env, result, "surfaceRegistered", surface.registered);
    SetBool(env, result, "surfaceReady", surface.ready);
    SetString(env, result, "surfaceId", surface.id);
    SetUint32(env, result, "surfaceWidth", surface.width);
    SetUint32(env, result, "surfaceHeight", surface.height);
    SetUint32(env, result, "surfaceViewportX", surface.viewportX);
    SetUint32(env, result, "surfaceViewportY", surface.viewportY);
    SetUint32(env, result, "surfaceViewportWidth", surface.viewportWidth);
    SetUint32(env, result, "surfaceViewportHeight", surface.viewportHeight);
    SetUint32(env, result, "surfaceCreatedCount", surface.createdCount);
    SetUint32(env, result, "surfaceChangedCount", surface.changedCount);
    SetUint32(env, result, "surfaceDestroyedCount", surface.destroyedCount);
    SetUint32(env, result, "surfaceTouchCount", surface.touchCount);
    SetUint32(env, result, "surfacePaintCount", surface.paintCount);
    SetString(env, result, "surfaceLastPaintMessage", surface.lastPaintMessage);
    SetBool(env, result, "sessionConnected", g_session.IsConnected());
    SetUint32(env, result, "desktopWidth", RdpDesktopWidth());
    SetUint32(env, result, "desktopHeight", RdpDesktopHeight());
    SetUint32(env, result, "inputQueueDepth", g_session.InputQueueDepth());
    SetUint32(env, result, "inputQueuedCount", g_session.InputQueuedCount());
    SetUint32(env, result, "inputSentCount", g_session.InputSentCount());
    SetUint32(env, result, "inputDroppedCount", g_session.InputDroppedCount());

    std::vector<std::string> logs = {
        "N-API bridge loaded",
        "Native calls are available: probe, connect, disconnect, sendPointer, sendKey, sendUnicode",
        "FreeRDP input dispatch: worker-thread queue",
        "FreeRDP channel dispatch: libfreerdp-client static addin provider",
        "FreeRDP build features: " + featureSummary,
        renderStats,
        graphicsStats,
        audioStats,
        "Certificate policy: tofu stores first untrusted certificate through FreeRDP, strict rejects untrusted certificates"
    };
    if (freerdp.linked) {
        logs.push_back("FreeRDP probe library loaded");
        logs.push_back("FreeRDP " + freerdp.freerdpVersion);
        logs.push_back("WinPR " + freerdp.winprVersion);
        logs.push_back(freerdp.opensslVersion);
    } else {
        logs.push_back("FreeRDP probe library not loaded: " + freerdp.error);
    }
    if (surface.ready) {
        logs.push_back("XComponent surface ready: " + surface.id + " " +
            std::to_string(surface.width) + "x" + std::to_string(surface.height));
        if (surface.viewportWidth > 0 && surface.viewportHeight > 0) {
            logs.push_back("XComponent render viewport: " + std::to_string(surface.viewportX) + "," +
                std::to_string(surface.viewportY) + " " + std::to_string(surface.viewportWidth) + "x" +
                std::to_string(surface.viewportHeight));
        }
        if (!surface.lastPaintMessage.empty()) {
            logs.push_back(surface.lastPaintMessage);
        }
    } else if (surface.registered) {
        logs.push_back("XComponent callback registered; surface not created");
    } else {
        logs.push_back("XComponent callback not registered");
    }
    const uint32_t desktopWidth = RdpDesktopWidth();
    const uint32_t desktopHeight = RdpDesktopHeight();
    if (desktopWidth > 0 && desktopHeight > 0) {
        logs.push_back("FreeRDP desktop size ready: " + std::to_string(desktopWidth) + "x" +
            std::to_string(desktopHeight));
    }
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Connect(napi_env env, napi_callback_info info)
{
    ConnectParams params = ReadConnectParams(env, info);
    std::vector<std::string> logs = {"native connect invoked"};

    napi_value result = MakeObject(env);
    if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "host, port, username, and password are required");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    logs.push_back("target=" + params.host + ":" + params.port);
    logs.push_back("username=" + params.username);
    logs.push_back("resolution=" + params.resolution);
    logs.push_back("certPolicy=" + params.certPolicy);
    logs.push_back("appFilesDir=" + params.appFilesDir);
    logs.push_back("starting native worker");

    std::string message;
    bool started = g_session.Connect(params, message);
    if (!started) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", message);
        logs.push_back("native worker start failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Resolving");
    SetString(env, result, "message", message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value Disconnect(napi_env env, napi_callback_info info)
{
    const bool closing = g_session.RequestDisconnect();
    g_events.state.Emit("Disconnected");
    g_events.log.Emit("native disconnect requested");

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", true);
    SetString(env, result, "state", "Disconnected");
    SetString(env, result, "message", closing ? "native bridge session closing" : "native bridge session already closed");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        "native disconnect requested",
        closing ? "native worker stopping asynchronously" : "native worker was not running"
    }));
    return result;
}

napi_value SendPointer(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native pointer input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "pointer input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t flags = GetUint32Property(env, arg, "flags");
    const uint32_t x = GetUint32Property(env, arg, "x");
    const uint32_t y = GetUint32Property(env, arg, "y");
    logs.push_back("flags=" + std::to_string(flags) + " x=" + std::to_string(x) + " y=" + std::to_string(y));

    std::string message;
    const bool ok = g_session.SendPointer(static_cast<uint16_t>(flags & 0xFFFFU),
        static_cast<uint16_t>(std::min(x, 0xFFFFU)), static_cast<uint16_t>(std::min(y, 0xFFFFU)), message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value SendKey(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native keyboard input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "keyboard input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t scancode = GetUint32Property(env, arg, "scancode");
    const bool down = GetBoolProperty(env, arg, "down");
    const bool repeat = GetBoolProperty(env, arg, "repeat");
    logs.push_back("scancode=" + std::to_string(scancode) + (down ? " down" : " up") +
        (repeat ? " repeat" : ""));

    std::string message;
    const bool ok = g_session.SendKey(scancode, down, repeat, message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value SendUnicode(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native unicode keyboard input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Disconnected");
        SetString(env, result, "message", "unicode keyboard input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t code = GetUint32Property(env, arg, "code");
    const bool down = GetBoolProperty(env, arg, "down");
    logs.push_back("code=" + std::to_string(code) + (down ? " down" : " up"));

    std::string message;
    const bool ok = g_session.SendUnicode(code, down, message);
    g_events.log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value NotifySurfaceLayout(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native surface layout notify invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "surface layout notify requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    const uint32_t width = GetUint32Property(env, arg, "width");
    const uint32_t height = GetUint32Property(env, arg, "height");
    logs.push_back("width=" + std::to_string(width) + " height=" + std::to_string(height));

    std::string message;
    const bool changed = g_surface.OnSurfaceLayout(width, height, message);
    if (changed) {
        EmitNativeLog(message);
        UpdateAvc420SurfaceOutputIfActive("surface layout changed");
        RequestRemoteDesktopResize(width, height, "surface layout changed");
        RequestSurfaceRepaint("surface layout changed");
    }

    SetBool(env, result, "ok", width > 0 && height > 0);
    SetString(env, result, "state", changed ? "Updated" : "Unchanged");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value RegisterCallback(napi_env env, napi_callback_info info, EventSink& sink, const char* name,
    bool mirrorToHilog = false)
{
    napi_value callback = GetFirstArgument(env, info);
    bool ok = callback != nullptr && sink.Set(env, callback, name, mirrorToHilog);

    napi_value result = MakeObject(env);
    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Idle" : "Failed");
    SetString(env, result, "message", ok ? "callback registered" : "callback must be a function");
    SetNamed(env, result, "logs", MakeStringArray(env, {
        ok ? std::string(name) + " registered" : std::string(name) + " registration failed"
    }));
    return result;
}

napi_value OnState(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.state, "rdpStateCallback");
}

napi_value OnLog(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.log, "rdpLogCallback", true);
}

napi_value OnError(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, g_events.error, "rdpErrorCallback", true);
}

} // namespace

EXTERN_C_START
static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"probe", nullptr, Probe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendPointer", nullptr, SendPointer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendKey", nullptr, SendKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendUnicode", nullptr, SendUnicode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"notifySurfaceLayout", nullptr, NotifySurfaceLayout, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onLog", nullptr, OnLog, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    ConfigureRdpgfxPipelineCallbacks();
    ConfigureRdpSessionCallbacks();
    RegisterNativeXComponent(env, exports);
    return exports;
}
EXTERN_C_END

static napi_module rdpNativeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "entry",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterEntryModule(void)
{
    napi_module_register(&rdpNativeModule);
}
