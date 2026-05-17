#include "napi_exports.h"

#include "napi/native_api.h"
#include "native_bridge_context.h"
#include "bridge_types.h"
#include "freerdp_gdi_bridge.h"
#include "channels/audio_diagnostics.h"
#include "channels/rdpgfx_diagnostics.h"
#include "input/ohos_keyboard_adapter.h"
#include "napi_event_sink.h"
#include "napi_utils.h"
#include "probe_utils.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace {

using namespace rdp_bridge;

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
    SurfaceSnapshot surface = BridgeSurfaceSnapshot();
    const std::string featureSummary =
        "core RDP/TLS/NLA + queued software GDI renderer; client channels on; "
        "cliprdr/rdpdr/drive/printer/smartcard/rdpsnd/audin/rdpgfx/disp compiled; "
        "H264 + FFmpeg + OpenH264 enabled; RD Gateway core enabled; "
        "static cliprdr text bridge, disp dynamic resolution, and rdpsnd/OHAudio playback requested; "
        "rdpgfx runtime gated by graphicsMode; other optional channel negotiation off";

    const std::string audioStats = BuildOHAudioStatsLog();
    const std::string renderStats = BridgeRenderStatsLog();
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
    SetBool(env, result, "sessionConnected", BridgeSession().IsConnected());
    SetUint32(env, result, "desktopWidth", RdpDesktopWidth());
    SetUint32(env, result, "desktopHeight", RdpDesktopHeight());
    SetUint32(env, result, "inputQueueDepth", BridgeSession().InputQueueDepth());
    SetUint32(env, result, "inputQueuedCount", BridgeSession().InputQueuedCount());
    SetUint32(env, result, "inputSentCount", BridgeSession().InputSentCount());
    SetUint32(env, result, "inputDroppedCount", BridgeSession().InputDroppedCount());

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
    bool started = BridgeSession().Connect(params, message);
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
    const bool closing = BridgeSession().RequestDisconnect();
    BridgeEvents().state.Emit("Disconnected");
    BridgeEvents().log.Emit("native disconnect requested");

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
    const bool ok = BridgeSession().SendPointer(static_cast<uint16_t>(flags & 0xFFFFU),
        static_cast<uint16_t>(std::min(x, 0xFFFFU)), static_cast<uint16_t>(std::min(y, 0xFFFFU)), message);
    BridgeEvents().log.Emit(message);

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
    const bool ok = BridgeSession().SendKey(scancode, down, repeat, message);
    BridgeEvents().log.Emit(message);

    SetBool(env, result, "ok", ok);
    SetString(env, result, "state", ok ? "Connected" : "Disconnected");
    SetString(env, result, "message", message);
    logs.push_back(message);
    SetNamed(env, result, "logs", MakeStringArray(env, logs));
    return result;
}

napi_value SendPlatformKey(napi_env env, napi_callback_info info)
{
    napi_value arg = GetFirstArgument(env, info);
    napi_valuetype type = napi_undefined;
    if (arg != nullptr) {
        napi_typeof(env, arg, &type);
    }

    std::vector<std::string> logs = {"native platform key input invoked"};
    napi_value result = MakeObject(env);
    if (arg == nullptr || type != napi_object) {
        SetBool(env, result, "ok", false);
        SetString(env, result, "state", "Failed");
        SetString(env, result, "message", "platform key input requires an object argument");
        logs.push_back("parameter validation failed");
        SetNamed(env, result, "logs", MakeStringArray(env, logs));
        return result;
    }

    OhosKeyEvent event;
    event.keyCode = GetUint32Property(env, arg, "keyCode");
    event.down = GetBoolProperty(env, arg, "down");
    event.repeat = GetBoolProperty(env, arg, "repeat");
    event.ctrl = GetBoolProperty(env, arg, "ctrl");
    event.shift = GetBoolProperty(env, arg, "shift");
    event.alt = GetBoolProperty(env, arg, "alt");
    event.meta = GetBoolProperty(env, arg, "meta");

    std::string message;
    const bool ok = BridgeSession().SendPlatformKey(event, message);
    BridgeEvents().log.Emit(message);

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
    const bool ok = BridgeSession().SendUnicode(code, down, message);
    BridgeEvents().log.Emit(message);

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
    const bool changed = NotifyBridgeSurfaceLayout(width, height, message);

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
    return RegisterCallback(env, info, BridgeEvents().state, "rdpStateCallback");
}

napi_value OnLog(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, BridgeEvents().log, "rdpLogCallback", true);
}

napi_value OnError(napi_env env, napi_callback_info info)
{
    return RegisterCallback(env, info, BridgeEvents().error, "rdpErrorCallback", true);
}

} // namespace

napi_value RegisterRdpNativeExports(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        {"probe", nullptr, Probe, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"connect", nullptr, Connect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"disconnect", nullptr, Disconnect, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendPointer", nullptr, SendPointer, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendKey", nullptr, SendKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendPlatformKey", nullptr, SendPlatformKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendUnicode", nullptr, SendUnicode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"notifySurfaceLayout", nullptr, NotifySurfaceLayout, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onState", nullptr, OnState, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onLog", nullptr, OnLog, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"onError", nullptr, OnError, nullptr, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    InitializeNativeBridgeContext();
    RegisterNativeXComponent(env, exports);
    return exports;
}
