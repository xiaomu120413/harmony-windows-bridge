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
#include "rdp_session_input.h"
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
struct UserParts {
    std::string domain;
    std::string username;
};

UserParts SplitDomainUsername(const std::string& value)
{
    size_t separator = value.find('\\');
    size_t separatorLength = 1;
    const size_t ideographicSeparator = value.find("\xE3\x80\x81");
    if (separator == std::string::npos ||
        (ideographicSeparator != std::string::npos && ideographicSeparator < separator)) {
        separator = ideographicSeparator;
        separatorLength = 3;
    }

    if (separator == std::string::npos || separator == 0 || separator + separatorLength >= value.size()) {
        return {"", TrimAscii(value)};
    }

    std::string domain = TrimAscii(value.substr(0, separator));
    std::string username = TrimAscii(value.substr(separator + separatorLength));
    if (domain.empty() || username.empty()) {
        return {"", TrimAscii(value)};
    }
    return {domain, username};
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
using FreerdpSetActiveFn = std::function<void(FreerdpRuntimeApi*, freerdp*, rdpContext*)>;
using FreerdpClearActiveFn = std::function<void(freerdp*)>;
using FreerdpConnectedFn = std::function<void()>;
using FreerdpInputPumpFn = std::function<void(FreerdpRuntimeApi*, rdpContext*)>;

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const FreerdpSetActiveFn& setActive, const FreerdpClearActiveFn& clearActive,
    const FreerdpLogFn& log, const FreerdpConnectedFn& onConnected,
    const FreerdpInputPumpFn& pumpInput)
{
    RdpSessionRunResult result;
    result.available = true;

    std::string modulesPath = EnsureOpenSslModulesPath();
    if (!modulesPath.empty()) {
        log("OPENSSL_MODULES=" + modulesPath);
    }

    uint32_t port = 0;
    if (!ParsePort(params.port, port)) {
        result.message = "invalid RDP port: " + params.port;
        result.failed = true;
        return result;
    }

    uint32_t width = 1280;
    uint32_t height = 720;
    ParseResolutionOrDefault(params.resolution, width, height);

    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        result.available = false;
        result.message = error;
        result.failed = true;
        return result;
    }
    log("FreeRDP runtime symbols loaded");

    freerdp* instance = api.freerdpNew();
    if (instance == nullptr) {
        result.message = "freerdp_new failed";
        result.failed = true;
        return result;
    }

    bool contextCreated = false;
    HarmonyClipboardBridge clipboardBridge;
    auto cleanup = [&]() {
        ResetAvcSurfaceOutput(api);
        clipboardBridge.Uninitialize();
        ClearRdpDesktopSize();
        UnregisterCertificatePolicy(instance);
        ClearCertificatePolicyLogSink();
        if (contextCreated && instance->context != nullptr) {
            api.abortConnectContext(instance->context);
            api.disconnect(instance);
            StopRenderPipeline();
            clearActive(instance);
            api.contextFree(instance);
        } else {
            clearActive(instance);
        }
        api.freerdpFree(instance);
    };

    if (!api.contextNew(instance)) {
        result.message = "freerdp_context_new failed";
        result.failed = true;
        cleanup();
        return result;
    }
    contextCreated = true;
    if (!EnableFreerdpClientChannels(api, instance, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }
    setActive(&api, instance, instance->context);

    rdpSettings* settings = instance->context == nullptr ? nullptr : instance->context->settings;
    if (settings == nullptr) {
        result.message = "FreeRDP settings unavailable";
        result.failed = true;
        cleanup();
        return result;
    }
    SetCertificatePolicyLogSink(log);

    UserParts user = SplitDomainUsername(params.username);
    const CertificatePolicy certificatePolicy = ParseCertificatePolicy(params.certPolicy);
    const bool ignoreCertificate = certificatePolicy == CertificatePolicy::Ignore;
    const GraphicsPipelineConfig graphicsConfig = ParseGraphicsPipelineConfig(params);

    if (!SetFreerdpString(api, settings, FreeRDP_ServerHostname, params.host, "ServerHostname", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ServerPort, port, "ServerPort", error) ||
        !SetFreerdpString(api, settings, FreeRDP_Username, user.username, "Username", error) ||
        !SetFreerdpString(api, settings, FreeRDP_Password, params.password, "Password", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_DesktopWidth, width, "DesktopWidth", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_DesktopHeight, height, "DesktopHeight", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_ColorDepth, 32, "ColorDepth", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_TcpConnectTimeout, 5000, "TcpConnectTimeout", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_OsMajorType, OSMAJORTYPE_UNIX, "OsMajorType", error) ||
        !SetFreerdpUint32(api, settings, FreeRDP_OsMinorType, OSMINORTYPE_NATIVE_WAYLAND, "OsMinorType", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AuthenticationOnly, false, "AuthenticationOnly", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_Authentication, true, "Authentication", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_SoftwareGdi, true, "SoftwareGdi", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_NegotiateSecurityLayer, true, "NegotiateSecurityLayer", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_CertificateCallbackPreferPEM, true, "CertificateCallbackPreferPEM", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_IgnoreCertificate, ignoreCertificate, "IgnoreCertificate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AutoAcceptCertificate, false, "AutoAcceptCertificate", error) ||
        !SetFreerdpBool(api, settings, FreeRDP_AutoDenyCertificate, false, "AutoDenyCertificate", error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureFreerdpStoragePaths(api, settings, params, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureEnhancedRdpSettings(api, settings, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureGraphicsPipelineChannel(api, settings, graphicsConfig, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureClipboardChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureDisplayControlChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!clipboardBridge.Initialize(instance->context, api, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!ConfigureAudioPlaybackChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!user.domain.empty() &&
        !SetFreerdpString(api, settings, FreeRDP_Domain, user.domain, "Domain", error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    instance->PostConnect = HarmonyPostConnect;
    instance->PostDisconnect = HarmonyPostDisconnect;
    instance->VerifyCertificateEx = HarmonyVerifyCertificateEx;
    instance->VerifyChangedCertificateEx = HarmonyVerifyChangedCertificateEx;
    SetGdiBridgeCallbacks({
        IsAvc420SurfaceOutputEnabled,
        QueueSurfaceRgbaFrame,
        StartRenderPipeline,
        StopRenderPipeline,
        EmitNativeLog,
    });
    RegisterCertificatePolicy(instance, certificatePolicy);
    ClearRdpDesktopSize();

    log("FreeRDP target configured");
    log("FreeRDP mode=PersistentSession");
    log("FreeRDP GDI renderer configured");
    log(std::string("FreeRDP certificate policy=") + CertificatePolicyName(certificatePolicy));
    if (!user.domain.empty()) {
        log("FreeRDP domain parsed from username");
    }

    BOOL rc = api.connect(instance);
    uint32_t lastError = instance->context == nullptr ? UINT32_MAX : api.getLastError(instance->context);
    log(std::string("freerdp_connect returned ") + (rc ? "true" : "false"));

    if (!running.load()) {
        result.cancelled = true;
        result.message = "FreeRDP connect cancelled";
        cleanup();
        return result;
    }

    if (!rc) {
        result.failed = true;
        result.message = "FreeRDP connect failed: " + LastErrorMessage(api, lastError);
        cleanup();
        return result;
    }

    result.connected = true;
    result.message = "FreeRDP session connected";
    onConnected();
    log("FreeRDP event loop started");
    auto nextAudioDiagnosticsLog = std::chrono::steady_clock::now() + std::chrono::seconds(10);

    while (running.load() && !api.shallDisconnectContext(instance->context)) {
        pumpInput(&api, instance->context);
        const auto now = std::chrono::steady_clock::now();
        if (now >= nextAudioDiagnosticsLog) {
            EmitHilogInfo("FreeRDP audio diagnostics: " + BuildOHAudioStatsLog());
            nextAudioDiagnosticsLog = now + std::chrono::seconds(10);
        }

        HANDLE handles[MAXIMUM_WAIT_OBJECTS] = {};
        DWORD count = api.getEventHandles(instance->context, handles, MAXIMUM_WAIT_OBJECTS);
        if (count == 0) {
            uint32_t errorCode = api.getLastError(instance->context);
            result.failed = true;
            result.message = "freerdp_get_event_handles failed: " + LastErrorMessage(api, errorCode);
            break;
        }

        DWORD waitStatus = api.waitForMultipleObjects(count, handles, FALSE, 25);
        if (!running.load()) {
            result.cancelled = true;
            result.message = "FreeRDP session cancelled";
            break;
        }

        if (waitStatus == WAIT_TIMEOUT) {
            continue;
        }

        if (waitStatus == WAIT_FAILED) {
            result.failed = true;
            result.message = "WaitForMultipleObjects failed: " + Hex32(static_cast<uint32_t>(waitStatus));
            break;
        }

        if (!api.checkEventHandles(instance->context)) {
            uint32_t errorCode = api.getLastError(instance->context);
            if (errorCode == FREERDP_ERROR_SUCCESS) {
                result.message = "FreeRDP event loop stopped without error";
            } else {
                result.failed = true;
                result.message = "FreeRDP event loop failed: " + LastErrorMessage(api, errorCode);
            }
            break;
        }

        pumpInput(&api, instance->context);
    }

    if (!result.cancelled && !result.failed && result.message == "FreeRDP session connected") {
        result.message = "FreeRDP session ended";
    }

    cleanup();
    return result;
}
#else
RdpSessionRunResult RunFreerdpSessionUnavailable()
{
    RdpSessionRunResult result;
    result.available = false;
    result.message = "FreeRDP headers not found at build time";
    result.failed = true;
    return result;
}
#endif

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

class RdpSession {
public:
    ~RdpSession()
    {
        Disconnect();
    }

    bool Connect(const ConnectParams& params, std::string& message)
    {
        if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
            message = "host, port, username, and password are required";
            g_events.error.Emit(message);
            return false;
        }

        Disconnect();

        running_.store(true);
        connected_.store(false);
        input_.Reset();
        message = "native worker started";
        worker_ = std::thread([this, params]() {
            WorkerMain(params);
        });
        return true;
    }

    void Disconnect()
    {
        RequestDisconnect();
        if (worker_.joinable()) {
            worker_.join();
        }
    }

    bool RequestDisconnect()
    {
        running_.store(false);
        connected_.store(false);
        input_.Clear();
        ClearRdpDesktopSize();
        RequestNativeDisconnect();
        return worker_.joinable();
    }

    bool IsConnected() const
    {
        return connected_.load();
    }

    bool SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input_.EnqueuePointer(flags, x, y, message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input_.EnqueueKey(rdpScancode, down, repeat, message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendUnicode(uint32_t code, bool down, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input_.EnqueueUnicode(code, down, message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    uint32_t InputQueueDepth() const
    {
        return input_.QueueDepth();
    }

    uint32_t InputQueuedCount() const
    {
        return input_.QueuedCount();
    }

    uint32_t InputSentCount() const
    {
        return input_.SentCount();
    }

    uint32_t InputDroppedCount() const
    {
        return input_.DroppedCount();
    }

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (IsAvc420SurfaceOutputEnabled()) {
            message = "AVC420 surface output owns XComponent";
            return false;
        }
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeContext_ == nullptr || activeContext_->gdi == nullptr) {
            message = "FreeRDP GDI context is not ready";
            return false;
        }

        rdpGdi* gdi = activeContext_->gdi;
        if (gdi->suppressOutput || gdi->primary_buffer == nullptr || gdi->width <= 0 ||
            gdi->height <= 0 || gdi->stride == 0) {
            message = "FreeRDP GDI primary buffer is not ready";
            return false;
        }

        RgbaFrame frame = {
            gdi->primary_buffer,
            static_cast<uint32_t>(gdi->width),
            static_cast<uint32_t>(gdi->height),
            static_cast<int32_t>(gdi->stride),
            reason.empty() ? "surface repaint" : reason,
            DirtyFrameStats{},
        };

        std::string queueMessage;
        if (!QueueSurfaceRgbaFrame(frame, queueMessage, true)) {
            message = queueMessage;
            return false;
        }

        message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " current-gdi";
        return true;
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected_.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        constexpr uint32_t minDimension = 200;
        constexpr uint32_t maxDimension = 8192;
        width = std::clamp(width, minDimension, maxDimension);
        height = std::clamp(height, minDimension, maxDimension);
        width -= width % 2U;
        if (width < minDimension) {
            width = minDimension;
        }

        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeDisp_ == nullptr || activeDisp_->SendMonitorLayout == nullptr) {
            message = "display-control channel is not ready";
            return false;
        }
        if (!displayControlCapsReady_) {
            message = "display-control caps are not ready";
            return false;
        }
        if (lastDynamicResizeWidth_ == width && lastDynamicResizeHeight_ == height) {
            message = "display-control resize unchanged: " + std::to_string(width) + "x" +
                std::to_string(height);
            return true;
        }

        DISPLAY_CONTROL_MONITOR_LAYOUT layout = {};
        layout.Flags = DISPLAY_CONTROL_MONITOR_PRIMARY;
        layout.Left = 0;
        layout.Top = 0;
        layout.Width = width;
        layout.Height = height;
        layout.PhysicalWidth = width;
        layout.PhysicalHeight = height;
        layout.Orientation = ORIENTATION_LANDSCAPE;
        layout.DesktopScaleFactor = 100;
        layout.DeviceScaleFactor = 100;

        const UINT rc = activeDisp_->SendMonitorLayout(activeDisp_, 1, &layout);
        if (rc != CHANNEL_RC_OK) {
            message = "display-control resize failed: " + std::to_string(rc);
            return false;
        }

        lastDynamicResizeWidth_ = width;
        lastDynamicResizeHeight_ = height;
        message = "display-control resize requested after " + reason + ": " +
            std::to_string(width) + "x" + std::to_string(height);
        return true;
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

private:
    void EmitState(const std::string& state)
    {
        g_events.state.Emit(state);
    }

    void EmitLog(const std::string& line)
    {
        g_events.log.Emit(line);
    }

    bool SleepInterruptibly(int milliseconds)
    {
        constexpr int stepMs = 25;
        int elapsed = 0;
        while (elapsed < milliseconds) {
            if (!running_.load()) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
            elapsed += stepMs;
        }
        return running_.load();
    }

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    void SetActiveNative(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context)
    {
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            activeApi_ = api;
            activeInstance_ = instance;
            activeContext_ = context;
            activeDisp_ = nullptr;
            activeGfx_ = nullptr;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }

        RegisterSession(context, this);
        if (api != nullptr && api->pubSubSubscribe != nullptr && context != nullptr &&
            context->pubSub != nullptr) {
            (void)api->pubSubSubscribe(context->pubSub, "ChannelConnected", OnChannelConnected);
            (void)api->pubSubSubscribe(context->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        }
    }

    void ClearActiveNative(freerdp* instance)
    {
        rdpContext* oldContext = nullptr;
        FreerdpRuntimeApi* oldApi = nullptr;
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeInstance_ != instance) {
            return;
        }

        oldContext = activeContext_;
        oldApi = activeApi_;
        DetachGraphicsPipelineLocked(activeGfx_);
        if (oldApi != nullptr && oldApi->pubSubUnsubscribe != nullptr && oldContext != nullptr &&
            oldContext->pubSub != nullptr) {
            (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelConnected", OnChannelConnected);
            (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelDisconnected", OnChannelDisconnected);
        }
        UnregisterSession(oldContext);
        activeApi_ = nullptr;
        activeInstance_ = nullptr;
        activeContext_ = nullptr;
        activeDisp_ = nullptr;
        activeGfx_ = nullptr;
        displayControlCapsReady_ = false;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
    }

    void RequestNativeDisconnect()
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeApi_ != nullptr && activeContext_ != nullptr) {
            activeApi_->abortConnectContext(activeContext_);
        }
    }

    static std::mutex& RegistryMutex()
    {
        static std::mutex mutex;
        return mutex;
    }

    static std::unordered_map<rdpContext*, RdpSession*>& Registry()
    {
        static std::unordered_map<rdpContext*, RdpSession*> registry;
        return registry;
    }

    static void RegisterSession(rdpContext* context, RdpSession* session)
    {
        if (context == nullptr || session == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        Registry()[context] = session;
    }

    static void UnregisterSession(rdpContext* context)
    {
        if (context == nullptr) {
            return;
        }
        std::lock_guard<std::mutex> lock(RegistryMutex());
        Registry().erase(context);
    }

    static RdpSession* FindSession(rdpContext* context)
    {
        std::lock_guard<std::mutex> lock(RegistryMutex());
        auto iter = Registry().find(context);
        return iter == Registry().end() ? nullptr : iter->second;
    }

    static bool IsDisplayControlChannel(const char* name)
    {
        return name != nullptr &&
            (std::strcmp(name, DISP_CHANNEL_NAME) == 0 ||
                std::strcmp(name, DISP_DVC_CHANNEL_NAME) == 0);
    }

    static bool IsGraphicsPipelineChannel(const char* name)
    {
        return name != nullptr && std::strcmp(name, RDPGFX_DVC_CHANNEL_NAME) == 0;
    }

    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event)
    {
        if (context == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }

        RdpSession* session = FindSession(static_cast<rdpContext*>(context));
        if (session == nullptr) {
            return;
        }
        if (IsDisplayControlChannel(event->name)) {
            session->AttachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
        } else if (IsGraphicsPipelineChannel(event->name)) {
            session->AttachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
        }
    }

    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
    {
        if (context == nullptr || event == nullptr || event->name == nullptr) {
            return;
        }

        RdpSession* session = FindSession(static_cast<rdpContext*>(context));
        if (session == nullptr) {
            return;
        }
        if (IsDisplayControlChannel(event->name)) {
            session->DetachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
        } else if (IsGraphicsPipelineChannel(event->name)) {
            session->DetachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
        }
    }

    static UINT DisplayControlCaps(DispClientContext* disp, UINT32 maxNumMonitors,
        UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB)
    {
        auto* session = disp == nullptr ? nullptr : static_cast<RdpSession*>(disp->custom);
        if (session != nullptr) {
            session->HandleDisplayControlCaps(maxNumMonitors, maxMonitorAreaFactorA,
                maxMonitorAreaFactorB);
        }
        return CHANNEL_RC_OK;
    }

    void HandleDisplayControlCaps(UINT32 maxNumMonitors, UINT32 maxMonitorAreaFactorA,
        UINT32 maxMonitorAreaFactorB)
    {
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            displayControlCapsReady_ = true;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }

        EmitLog("display-control caps: maxMonitors=" + std::to_string(maxNumMonitors) +
            " areaFactor=" + std::to_string(maxMonitorAreaFactorA) + "/" +
            std::to_string(maxMonitorAreaFactorB));

        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        if (snapshot.width > 0 && snapshot.height > 0) {
            std::string resizeMessage;
            if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                "display-control caps", resizeMessage)) {
                EmitLog(resizeMessage);
            } else {
                EmitLog("display-control resize skipped after display-control caps: " +
                    resizeMessage);
            }
        }
    }

    void AttachDisplayControl(DispClientContext* disp)
    {
        if (disp == nullptr) {
            EmitLog("display-control connected without client context");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            activeDisp_ = disp;
            activeDisp_->custom = this;
            activeDisp_->DisplayControlCaps = DisplayControlCaps;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
        }
        EmitLog("display-control connected to HarmonyOS window resize bridge");

        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        if (snapshot.width > 0 && snapshot.height > 0) {
            std::string resizeMessage;
            if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                "display-control connected", resizeMessage)) {
                EmitLog(resizeMessage);
            } else {
                EmitLog("display-control resize skipped after display-control connected: " +
                    resizeMessage);
            }
        }
    }

    void DetachDisplayControl(DispClientContext* disp)
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeDisp_ != nullptr && activeDisp_ == disp) {
            activeDisp_->custom = nullptr;
            activeDisp_ = nullptr;
            displayControlCapsReady_ = false;
            lastDynamicResizeWidth_ = 0;
            lastDynamicResizeHeight_ = 0;
            EmitLog("display-control disconnected from HarmonyOS window resize bridge");
        }
    }

    void AttachGraphicsPipeline(RdpgfxClientContext* gfx)
    {
        std::string message;
        bool attached = false;
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            if (gfx == nullptr) {
                message = "rdpgfx connected without client context";
            } else if (activeApi_ == nullptr || activeContext_ == nullptr || activeContext_->gdi == nullptr) {
                IncrementRdpgfxInitFailed();
                message = "rdpgfx connected before GDI context was ready";
            } else if (activeApi_->gdiGraphicsPipelineInit == nullptr) {
                IncrementRdpgfxInitFailed();
                message = "rdpgfx GDI pipeline init symbol unavailable";
            } else {
                if (activeGfx_ != nullptr && activeGfx_ != gfx) {
                    DetachGraphicsPipelineLocked(activeGfx_);
                }
                if (activeApi_->gdiGraphicsPipelineInit(activeContext_->gdi, gfx)) {
                    InstallRdpgfxDiagnosticsHooks(gfx);
                    activeGfx_ = gfx;
                    SetRdpgfxBridgeAttached(true);
                    IncrementRdpgfxConnected();
                    attached = true;
                    message = "rdpgfx connected to FreeRDP GDI graphics pipeline";
                } else {
                    IncrementRdpgfxInitFailed();
                    message = "rdpgfx GDI graphics pipeline init failed";
                }
            }
        }

        EmitLog(message);
        if (attached) {
            RequestSurfaceRepaint("rdpgfx connected");
        }
    }

    void DetachGraphicsPipeline(RdpgfxClientContext* gfx)
    {
        bool detached = false;
        {
            std::lock_guard<std::mutex> lock(activeMutex_);
            detached = DetachGraphicsPipelineLocked(gfx);
        }
        if (detached) {
            EmitLog("rdpgfx disconnected from FreeRDP GDI graphics pipeline");
        }
    }

    bool DetachGraphicsPipelineLocked(RdpgfxClientContext* gfx)
    {
        if (activeGfx_ == nullptr || activeGfx_ != gfx) {
            return false;
        }
        RestoreRdpgfxDiagnosticsHooks(activeGfx_);
        if (activeApi_ != nullptr && activeApi_->gdiGraphicsPipelineUninit != nullptr &&
            activeContext_ != nullptr && activeContext_->gdi != nullptr) {
            activeApi_->gdiGraphicsPipelineUninit(activeContext_->gdi, activeGfx_);
        }
        activeGfx_ = nullptr;
        SetRdpgfxBridgeAttached(false);
        IncrementRdpgfxDisconnected();
        return true;
    }
#else
    void RequestNativeDisconnect() {}
#endif

    void WorkerMain(ConnectParams params)
    {
        EmitLog("native worker accepted params");
        EmitLog("target=" + params.host + ":" + params.port);
        EmitLog("graphicsMode=" + ParseGraphicsPipelineConfig(params).mode);

        if (!running_.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Resolving");
        EmitLog("state=Resolving");
        EmitLog("resolving target host");

        TcpConnectResult tcp = TestTcpConnect(params.host, params.port, 3000);
        if (!running_.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }
        if (!tcp.ok) {
            std::string message = "tcp check failed: " + tcp.message;
            EmitState("Failed");
            EmitLog(message);
            g_events.error.Emit(message);
            running_.store(false);
            return;
        }

        EmitState("TCP connected");
        EmitLog("state=TCP connected");
        EmitLog(tcp.message);
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Negotiating");
        EmitLog("state=Negotiating");
        EmitLog("starting FreeRDP persistent connect");
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Authenticating");
        EmitLog("state=Authenticating");
        if (IsAutoInitialResolution(params.resolution)) {
            SurfaceSnapshot snapshot = g_surface.Snapshot();
            if (snapshot.width >= 320 && snapshot.height >= 240) {
                params.resolution = std::to_string(snapshot.width) + "x" +
                    std::to_string(snapshot.height);
                EmitLog("FreeRDP initial resolution auto from surface: " + params.resolution);
            } else {
                EmitLog("FreeRDP initial resolution auto fallback: surface is not ready");
            }
        }
        RdpSessionRunResult session;
        const std::vector<std::string> graphicsModes = BuildGraphicsFallbackModes(params);
        EmitLog("graphics fallback ladder: " + JoinGraphicsModes(graphicsModes));
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        for (size_t attempt = 0; attempt < graphicsModes.size(); ++attempt) {
            ConnectParams attemptParams = params;
            attemptParams.graphicsMode = graphicsModes[attempt];
            bool attemptConnected = false;
            EmitLog("graphics attempt " + std::to_string(attempt + 1) + "/" +
                std::to_string(graphicsModes.size()) + ": mode=" + attemptParams.graphicsMode);
            session = RunFreerdpSession(attemptParams, running_,
                [this](FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context) {
                    SetActiveNative(api, instance, context);
                },
                [this](freerdp* instance) {
                    ClearActiveNative(instance);
                },
                [this](const std::string& line) {
                    EmitLog(line);
                },
                [this, &attemptConnected, selectedMode = attemptParams.graphicsMode]() {
                    attemptConnected = true;
                    connected_.store(true);
                    EmitState("Connected");
                    EmitLog("state=Connected");
                    EmitLog("graphics mode selected: " + selectedMode);
                    EmitLog("FreeRDP persistent session loop is active");
                    EmitLog("FreeRDP input bridge is using worker-thread dispatch");
                    const SurfaceSnapshot snapshot = g_surface.Snapshot();
                    if (snapshot.width > 0 && snapshot.height > 0) {
                        std::string resizeMessage;
                        if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
                            "session connected", resizeMessage)) {
                            EmitLog(resizeMessage);
                        } else {
                            EmitLog("display-control resize skipped after session connected: " +
                                resizeMessage);
                        }
                    }
                },
                [this](FreerdpRuntimeApi* api, rdpContext* context) {
                    input_.Drain(api, context, [this](const std::string& line) {
                        EmitLog(line);
                    });
                });
            input_.Clear();

            if (session.cancelled || !running_.load()) {
                break;
            }

            if (ShouldRetryGraphicsFallback(session, attemptConnected, attemptParams.graphicsMode,
                attempt, graphicsModes.size())) {
                connected_.store(false);
                EmitLog("graphics mode " + attemptParams.graphicsMode +
                    " failed before connection: " + session.message);
                EmitLog("graphics fallback retry: " + attemptParams.graphicsMode + " -> " +
                    graphicsModes[attempt + 1]);
                EmitState("Negotiating");
                EmitLog("state=Negotiating");
                continue;
            }

            if (session.failed && !attemptConnected && attempt + 1 < graphicsModes.size()) {
                EmitLog("graphics fallback skipped for non-graphics failure: " +
                    session.message);
            }
            break;
        }
#else
        (void)graphicsModes;
        session = RunFreerdpSessionUnavailable();
#endif
        input_.Clear();

        if (session.cancelled || !running_.load()) {
            connected_.store(false);
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        if (session.failed) {
            std::string message = session.available ? session.message : "FreeRDP runtime unavailable: " + session.message;
            connected_.store(false);
            EmitState("Failed");
            EmitLog(message);
            g_events.error.Emit(message);
            running_.store(false);
            return;
        }

        connected_.store(false);
        EmitState("Disconnected");
        EmitLog(session.message);
        running_.store(false);
    }

    std::atomic_bool running_ = false;
    std::atomic_bool connected_ = false;
    std::thread worker_;
    RdpSessionInput input_;
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    static bool IsAutoInitialResolution(const std::string& resolution)
    {
        const std::string normalized = ToLowerAscii(TrimAscii(resolution));
        return normalized.empty() || normalized == "auto" || normalized == "window";
    }

    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
    DispClientContext* activeDisp_ = nullptr;
    RdpgfxClientContext* activeGfx_ = nullptr;
    bool displayControlCapsReady_ = false;
    uint32_t lastDynamicResizeWidth_ = 0;
    uint32_t lastDynamicResizeHeight_ = 0;
#endif
};

RdpSession g_session;

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
