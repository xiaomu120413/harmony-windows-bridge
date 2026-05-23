#include "session/rdp_session_core.h"

#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "session/freerdp_session_runner.h"
#include "freerdp/graphics_config.h"
#include "common/net_utils.h"
#include "session/rdp_session_channels.h"
#include "session/rdp_session_input.h"
#include "common/string_utils.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace rdp_bridge {
namespace {
constexpr uint32_t kH264DesktopAlignment = 16;

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
        const std::string graphicsModeError = GraphicsModeValidationError(params.graphicsMode);
        if (!graphicsModeError.empty()) {
            message = graphicsModeError;
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

    bool SendLocalPointer(const LocalPointerEvent& pointer, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueLocalPointer(pointer, SurfaceSnapshotValue(), RdpDesktopWidth(),
            RdpDesktopHeight(), message, [this](const std::string& line) {
                EmitLog(line);
            });
#else
        (void)pointer;
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

    bool SendPlatformKey(const OhosKeyEvent& event, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueuePlatformKey(event, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        (void)event;
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

    bool SendCommittedText(const std::u16string& text, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueCommittedText(text, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        (void)text;
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool SendFocusIn(uint16_t toggleStates, std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueFocusIn(toggleStates, message, [this](const std::string& line) {
            EmitLog(line);
        });
#else
        (void)toggleStates;
        message = "FreeRDP headers not found at build time";
        return false;
#endif
    }

    bool ReleaseAllKeys(std::string& message)
    {
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        if (!connected.load()) {
            input.Clear();
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueReleaseAllKeys(message, [this](const std::string& line) {
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

    bool WaitForAutoInitialResolution(ConnectParams& params)
    {
        if (!IsAutoResolution(params.resolution)) {
            return true;
        }

        constexpr int maxWaitMs = 3000;
        constexpr int stepMs = 50;
        bool waitLogged = false;
        int elapsed = 0;
        while (running.load()) {
            const SurfaceSnapshot snapshot = SurfaceSnapshotValue();
            if (snapshot.ready && snapshot.width >= 320 && snapshot.height >= 240) {
                params.resolution = std::to_string(snapshot.width) + "x" +
                    std::to_string(snapshot.height);
                EmitLog("FreeRDP initial resolution auto from XComponent surface: " +
                    params.resolution);
                return true;
            }

            if (!waitLogged) {
                EmitLog("FreeRDP initial resolution auto waiting for XComponent surface");
                waitLogged = true;
            }
            if (elapsed >= maxWaitMs) {
                break;
            }
            if (!SleepInterruptibly(stepMs)) {
                EmitState("Disconnected");
                EmitLog("native worker cancelled");
                return false;
            }
            elapsed += stepMs;
        }

        const std::string message =
            "FreeRDP initial resolution auto failed: XComponent surface is not ready";
        EmitState("Failed");
        EmitLog(message);
        EmitError(message);
        running.store(false);
        return false;
    }

    void WorkerMain(ConnectParams params)
    {
        const GraphicsPipelineConfig graphicsConfig = ParseGraphicsPipelineConfig(params);
        EmitLog("native worker accepted params");
        EmitLog("target=" + params.host + ":" + params.port);
        EmitLog("graphicsMode=" + graphicsConfig.mode);
        EmitLog("avc444GpuCompositor=" +
            std::string(graphicsConfig.avc444GpuCompositor ? "auto-on" : "off"));

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
        if (!WaitForAutoInitialResolution(params)) {
            return;
        }
        RdpSessionRunResult session;
        const std::vector<std::string> graphicsModes = BuildGraphicsFallbackModes(params);
        EmitLog("graphics fallback ladder: " + JoinGraphicsModes(graphicsModes));
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        for (size_t attempt = 0; attempt < graphicsModes.size(); ++attempt) {
            ConnectParams attemptParams = params;
            attemptParams.graphicsMode = graphicsModes[attempt];
            const GraphicsPipelineConfig attemptGraphicsConfig =
                ParseGraphicsPipelineConfig(attemptParams);
            channels.SetDynamicResizeAlignment(attemptGraphicsConfig.h264 ?
                kH264DesktopAlignment : 1U);
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
                    std::string focusMessage;
                    if (SendFocusIn(0, focusMessage)) {
                        EmitLog("FreeRDP focus-in queued after session connected: " +
                            focusMessage);
                    } else {
                        EmitLog("FreeRDP focus-in skipped after session connected: " +
                            focusMessage);
                    }
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
                    (attemptConnected ? " failed after connection: " : " failed before connection: ") +
                    session.message);
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

bool RdpSession::SendLocalPointer(const LocalPointerEvent& pointer, std::string& message)
{
    return impl_->SendLocalPointer(pointer, message);
}

bool RdpSession::SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
{
    return impl_->SendKey(rdpScancode, down, repeat, message);
}

bool RdpSession::SendPlatformKey(const OhosKeyEvent& event, std::string& message)
{
    return impl_->SendPlatformKey(event, message);
}

bool RdpSession::SendUnicode(uint32_t code, bool down, std::string& message)
{
    return impl_->SendUnicode(code, down, message);
}

bool RdpSession::SendCommittedText(const std::u16string& text, std::string& message)
{
    return impl_->SendCommittedText(text, message);
}

bool RdpSession::SendFocusIn(uint16_t toggleStates, std::string& message)
{
    return impl_->SendFocusIn(toggleStates, message);
}

bool RdpSession::ReleaseAllKeys(std::string& message)
{
    return impl_->ReleaseAllKeys(message);
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
