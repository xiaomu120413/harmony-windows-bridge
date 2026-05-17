#include "rdp_session_core.h"

#include "bridge_log.h"
#include "certificate_policy.h"
#include "channels/audio_diagnostics.h"
#include "channels/clipboard_bridge.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp_gdi_bridge.h"
#include "freerdp_runtime.h"
#include "graphics_config.h"
#include "net_utils.h"
#include "rdp_channel_config.h"
#include "rdp_session_channels.h"
#include "rdp_session_input.h"
#include "string_utils.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/channels/cliprdr.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/rdpgfx.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/input.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#include <winpr/synch.h>
#endif

namespace rdp_bridge {
namespace {

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

void StopRenderPipeline(const RdpSessionCallbacks& callbacks)
{
    if (callbacks.stopRenderPipeline != nullptr) {
        callbacks.stopRenderPipeline();
    }
}

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const RdpSessionCallbacks& callbacks, const FreerdpSetActiveFn& setActive,
    const FreerdpClearActiveFn& clearActive, const FreerdpLogFn& log,
    const FreerdpConnectedFn& onConnected, const FreerdpInputPumpFn& pumpInput)
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
            StopRenderPipeline(callbacks);
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

    if (!ConfigureFreerdpStoragePaths(api, settings, params, log, error) ||
        !ConfigureEnhancedRdpSettings(api, settings, graphicsConfig, log, error) ||
        !ConfigureAvc420SurfaceOutput(api, graphicsConfig, log, error) ||
        !ConfigureGraphicsPipelineChannel(api, settings, graphicsConfig, log, error) ||
        !ConfigureClipboardChannel(api, settings, log, error) ||
        !ConfigureDisplayControlChannel(api, settings, log, error)) {
        result.message = error;
        result.failed = true;
        cleanup();
        return result;
    }

    if (!clipboardBridge.Initialize(instance->context, api, log, error) ||
        !ConfigureAudioPlaybackChannel(api, settings, log, error)) {
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
        callbacks.queueSurfaceRgbaFrame,
        callbacks.startRenderPipeline,
        callbacks.stopRenderPipeline,
        log,
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

void EmitCallback(const std::function<void(const std::string&)>& callback, const std::string& line)
{
    if (callback != nullptr) {
        callback(line);
    }
}

SurfaceSnapshot EmptySurfaceSnapshot()
{
    return {};
}

} // namespace

struct RdpSession::Impl {
    RdpSessionCallbacks callbacks;
    std::atomic_bool running = false;
    std::atomic_bool connected = false;
    std::thread worker;
    RdpSessionInput input;
    RdpSessionChannels channels;

    Impl()
    {
        SetCallbacks({});
    }

    ~Impl()
    {
        Disconnect();
    }

    void SetCallbacks(RdpSessionCallbacks nextCallbacks)
    {
        callbacks = std::move(nextCallbacks);
        channels.SetCallbacks({
            [this](const std::string& line) {
                EmitLog(line);
            },
            [this]() {
                return SurfaceSnapshotValue();
            },
            callbacks.queueSurfaceRgbaFrame,
            callbacks.requestSurfaceRepaint,
        });
    }

    bool Connect(const ConnectParams& params, std::string& message)
    {
        if (params.host.empty() || params.port.empty() || params.username.empty() || params.password.empty()) {
            message = "host, port, username, and password are required";
            EmitError(message);
            return false;
        }

        Disconnect();

        running.store(true);
        connected.store(false);
        input.Reset();
        message = "native worker started";
        worker = std::thread([this, params]() {
            WorkerMain(params);
        });
        return true;
    }

    void Disconnect()
    {
        RequestDisconnect();
        if (worker.joinable()) {
            worker.join();
        }
    }

    bool RequestDisconnect()
    {
        running.store(false);
        connected.store(false);
        input.Clear();
        ClearRdpDesktopSize();
        channels.RequestDisconnect();
        return worker.joinable();
    }

    bool IsConnected() const
    {
        return connected.load();
    }

    bool SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueuePointer(flags, x, y, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueKey(rdpScancode, down, repeat, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendUnicode(uint32_t code, bool down, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueUnicode(code, down, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (IsAvc420SurfaceOutputEnabled()) {
            message = "AVC420 surface output owns XComponent";
            return false;
        }
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        return channels.RequestCurrentFrameRender(reason, message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        return channels.RequestDynamicDesktopResize(width, height, reason, message);
#else
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    void EmitState(const std::string& state)
    {
        EmitCallback(callbacks.emitState, state);
    }

    void EmitLog(const std::string& line)
    {
        EmitCallback(callbacks.emitLog, line);
    }

    void EmitError(const std::string& message)
    {
        EmitCallback(callbacks.emitError, message);
    }

    SurfaceSnapshot SurfaceSnapshotValue()
    {
        if (callbacks.surfaceSnapshot != nullptr) {
            return callbacks.surfaceSnapshot();
        }
        return EmptySurfaceSnapshot();
    }

    bool SleepInterruptibly(int milliseconds)
    {
        constexpr int stepMs = 25;
        int elapsed = 0;
        while (elapsed < milliseconds) {
            if (!running.load()) {
                return false;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(stepMs));
            elapsed += stepMs;
        }
        return running.load();
    }

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    void SetActiveNative(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context)
    {
        channels.SetActive(api, instance, context);
    }

    void ClearActiveNative(freerdp* instance)
    {
        channels.ClearActive(instance);
    }
#endif

    static bool IsAutoInitialResolution(const std::string& resolution)
    {
        const std::string normalized = ToLowerAscii(TrimAscii(resolution));
        return normalized.empty() || normalized == "auto" || normalized == "window";
    }

    void WorkerMain(ConnectParams params)
    {
        EmitLog("native worker accepted params");
        EmitLog("target=" + params.host + ":" + params.port);
        EmitLog("graphicsMode=" + ParseGraphicsPipelineConfig(params).mode);

        if (!running.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Resolving");
        EmitLog("state=Resolving");
        EmitLog("resolving target host");

        TcpConnectResult tcp = TestTcpConnect(params.host, params.port, 3000);
        if (!running.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }
        if (!tcp.ok) {
            std::string message = "tcp check failed: " + tcp.message;
            EmitState("Failed");
            EmitLog(message);
            EmitError(message);
            running.store(false);
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
            SurfaceSnapshot snapshot = SurfaceSnapshotValue();
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
            session = RunFreerdpSession(attemptParams, running, callbacks,
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
                    connected.store(true);
                    EmitState("Connected");
                    EmitLog("state=Connected");
                    EmitLog("graphics mode selected: " + selectedMode);
                    EmitLog("FreeRDP persistent session loop is active");
                    EmitLog("FreeRDP input bridge is using worker-thread dispatch");
                    const SurfaceSnapshot snapshot = SurfaceSnapshotValue();
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
                    input.Drain(api, context, [this](const std::string& line) {
                        EmitLog(line);
                    });
                });
            input.Clear();

            if (session.cancelled || !running.load()) {
                break;
            }

            if (ShouldRetryGraphicsFallback(session, attemptConnected, attemptParams.graphicsMode,
                attempt, graphicsModes.size())) {
                connected.store(false);
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
        input.Clear();

        if (session.cancelled || !running.load()) {
            connected.store(false);
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        if (session.failed) {
            std::string message = session.available ? session.message : "FreeRDP runtime unavailable: " + session.message;
            connected.store(false);
            EmitState("Failed");
            EmitLog(message);
            EmitError(message);
            running.store(false);
            return;
        }

        connected.store(false);
        EmitState("Disconnected");
        EmitLog(session.message);
        running.store(false);
    }
};

RdpSession::RdpSession() : impl_(std::make_unique<Impl>()) {}

RdpSession::~RdpSession() = default;

void RdpSession::SetCallbacks(RdpSessionCallbacks callbacks)
{
    impl_->SetCallbacks(std::move(callbacks));
}

bool RdpSession::Connect(const ConnectParams& params, std::string& message)
{
    return impl_->Connect(params, message);
}

void RdpSession::Disconnect()
{
    impl_->Disconnect();
}

bool RdpSession::RequestDisconnect()
{
    return impl_->RequestDisconnect();
}

bool RdpSession::IsConnected() const
{
    return impl_->IsConnected();
}

bool RdpSession::SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message)
{
    return impl_->SendPointer(flags, x, y, message);
}

bool RdpSession::SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
{
    return impl_->SendKey(rdpScancode, down, repeat, message);
}

bool RdpSession::SendUnicode(uint32_t code, bool down, std::string& message)
{
    return impl_->SendUnicode(code, down, message);
}

uint32_t RdpSession::InputQueueDepth() const
{
    return impl_->input.QueueDepth();
}

uint32_t RdpSession::InputQueuedCount() const
{
    return impl_->input.QueuedCount();
}

uint32_t RdpSession::InputSentCount() const
{
    return impl_->input.SentCount();
}

uint32_t RdpSession::InputDroppedCount() const
{
    return impl_->input.DroppedCount();
}

bool RdpSession::RequestCurrentFrameRender(const std::string& reason, std::string& message)
{
    return impl_->RequestCurrentFrameRender(reason, message);
}

bool RdpSession::RequestDynamicDesktopResize(uint32_t width, uint32_t height,
    const std::string& reason, std::string& message)
{
    return impl_->RequestDynamicDesktopResize(width, height, reason, message);
}

} // namespace rdp_bridge
