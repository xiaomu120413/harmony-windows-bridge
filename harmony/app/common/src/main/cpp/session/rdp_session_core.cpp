#include "session/rdp_session_core.h"

#include "channels/rdpgfx_pipeline.h"
#include "common/bridge_log.h"
#include "common/net_utils.h"
#include "common/string_utils.h"
#include "freerdp/freerdp_gdi_bridge.h"
#include "freerdp/graphics_config.h"
#include "session/freerdp_session_runner.h"
#include "session/rdp_session_channels.h"
#include "session/rdp_session_input.h"

#include <atomic>
#include <cctype>
#include <chrono>
#include <thread>
#include <utility>
#include <vector>

namespace rdp_bridge {
namespace {
std::atomic_uint64_t g_nextDiagnosticSessionId { 1 };

void EmitCallback(const std::function<void(const std::string&)>& callback, const std::string& line)
{
    if (callback != nullptr) {
        callback(line);
    }
}

uint64_t ElapsedMilliseconds(const std::chrono::steady_clock::time_point& startedAt)
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startedAt).count());
}

void EmitCoreDiagnostic(uint64_t sessionId, const std::string& fields,
    BridgeLogLevel level = BridgeLogLevel::Info)
{
    BridgeLogger::LogPublic(level,
        "RDP_CORE sid=" + std::to_string(sessionId) + " " + fields);
}

std::string AuthenticationForm(const std::string& username)
{
    if (username.find('\\') != std::string::npos) {
        return "domain";
    }
    if (username.find('@') != std::string::npos) {
        return "upn";
    }
    return "account";
}

bool ContainsNonAscii(const std::string& value)
{
    for (const unsigned char byte : value) {
        if (byte > 0x7FU) {
            return true;
        }
    }
    return false;
}

std::string FailureReason(const std::string& message)
{
    size_t start = message.find("ERRCONNECT_");
    if (start == std::string::npos) {
        start = 0;
    }

    std::string reason;
    for (size_t index = start; index < message.size() && reason.size() < 64U; ++index) {
        const unsigned char byte = static_cast<unsigned char>(message[index]);
        if (std::isalnum(byte) != 0 || byte == '_') {
            reason.push_back(static_cast<char>(byte));
            continue;
        }
        if (byte == ' ' || byte == '-') {
            if (!reason.empty() && reason.back() != '_') {
                reason.push_back('_');
            }
            continue;
        }
        break;
    }

    const size_t codeStart = message.find("0x");
    if (codeStart != std::string::npos && codeStart + 2U < message.size()) {
        std::string code;
        for (size_t index = codeStart; index < message.size() && code.size() < 10U; ++index) {
            const unsigned char byte = static_cast<unsigned char>(message[index]);
            if (index < codeStart + 2U || std::isxdigit(byte) != 0) {
                code.push_back(static_cast<char>(byte));
            } else {
                break;
            }
        }
        if (code.size() > 2U) {
            reason += " code=" + code;
        }
    }
    return reason.empty() ? "unclassified" : reason;
}

SurfaceSnapshot EmptySurfaceSnapshot()
{
    return {};
}

} // namespace

struct RdpSession::Impl {
    static constexpr uint32_t kPointerInputDiagnostic = 1U << 0U;
    static constexpr uint32_t kKeyboardInputDiagnostic = 1U << 1U;
    static constexpr uint32_t kUnicodeInputDiagnostic = 1U << 2U;
    static constexpr uint32_t kImeInputDiagnostic = 1U << 3U;

    RdpSessionCallbacks callbacks;
    std::atomic_bool running = false;
    std::atomic_bool connected = false;
    std::atomic_uint64_t activeDiagnosticSessionId = 0;
    std::atomic_uint32_t acceptedInputDiagnosticMask = 0;
    std::atomic_uint32_t rejectedInputDiagnosticMask = 0;
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
        const uint64_t diagnosticSessionId = g_nextDiagnosticSessionId.fetch_add(1);
        activeDiagnosticSessionId.store(diagnosticSessionId, std::memory_order_relaxed);
        acceptedInputDiagnosticMask.store(0, std::memory_order_relaxed);
        rejectedInputDiagnosticMask.store(0, std::memory_order_relaxed);
        worker = std::thread([this, params, diagnosticSessionId]() {
            WorkerMain(params, diagnosticSessionId);
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
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueuePointer(flags, x, y, message, [this](const std::string& line) {
            EmitLog(line);
        });
        EmitInputDiagnostic(kPointerInputDiagnostic, "pointer", accepted);
        return accepted;
    }

    bool SendLocalPointer(const LocalPointerEvent& pointer, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueueLocalPointer(pointer, SurfaceSnapshotValue(), RdpDesktopWidth(),
            RdpDesktopHeight(), message, [this](const std::string& line) {
                EmitLog(line);
            });
        EmitInputDiagnostic(kPointerInputDiagnostic, "pointer", accepted);
        return accepted;
    }

    bool SendLocalPen(const LocalPenEvent& pen, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        uint32_t action = FREERDP_OHOS_PEN_ACTION_MOVE;
        switch (pen.action) {
            case LocalPenAction::Down:
                action = FREERDP_OHOS_PEN_ACTION_DOWN;
                break;
            case LocalPenAction::Up:
                action = FREERDP_OHOS_PEN_ACTION_UP;
                break;
            case LocalPenAction::Cancel:
                action = FREERDP_OHOS_PEN_ACTION_CANCEL;
                break;
            case LocalPenAction::Move:
            default:
                break;
        }
        uint32_t flags = 0;
        if ((pen.flags & LocalPenFlagEraser) != 0) {
            flags |= FREERDP_OHOS_PEN_FLAG_ERASER;
        }
        if ((pen.flags & LocalPenFlagInverted) != 0) {
            flags |= FREERDP_OHOS_PEN_FLAG_INVERTED;
        }
        if ((pen.flags & LocalPenFlagBarrel) != 0) {
            flags |= FREERDP_OHOS_PEN_FLAG_BARREL;
        }
        const FREERDP_OHOS_PEN_EVENT event {
            sizeof(FREERDP_OHOS_PEN_EVENT), FREERDP_OHOS_PEN_EVENT_VERSION, action,
            pen.deviceId, pen.x, pen.y, pen.pressure, pen.tiltX, pen.tiltY, 0,
            flags, pen.allowClamp ? TRUE : FALSE,
        };
        return channels.SendPen(BuildOhosPointerViewport(SurfaceSnapshotValue(),
            RdpDesktopWidth(), RdpDesktopHeight()), event, message);
    }

    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueueKey(rdpScancode, down, repeat, message,
            [this](const std::string& line) {
            EmitLog(line);
        });
        EmitInputDiagnostic(kKeyboardInputDiagnostic, "keyboard", accepted);
        return accepted;
    }

    bool SendPlatformKey(const OhosKeyEvent& event, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueuePlatformKey(event, message, [this](const std::string& line) {
            EmitLog(line);
        });
        EmitInputDiagnostic(kKeyboardInputDiagnostic, "keyboard", accepted);
        return accepted;
    }

    bool SendUnicode(uint32_t code, bool down, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueueUnicode(code, down, message, [this](const std::string& line) {
            EmitLog(line);
        });
        EmitInputDiagnostic(kUnicodeInputDiagnostic, "unicode", accepted);
        return accepted;
    }

    bool SendCommittedText(const std::u16string& text, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        const bool accepted = input.EnqueueCommittedText(text, message, [this](const std::string& line) {
            EmitLog(line);
        });
        EmitInputDiagnostic(kImeInputDiagnostic, "ime_commit", accepted);
        return accepted;
    }

    bool SendFocusIn(uint16_t toggleStates, std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueFocusIn(toggleStates, message, [this](const std::string& line) {
            EmitLog(line);
        });
    }

    bool ReleaseAllKeys(std::string& message)
    {
        if (!connected.load()) {
            input.Clear();
            message = "no active FreeRDP session";
            return false;
        }
        return input.EnqueueReleaseAllKeys(message, [this](const std::string& line) {
            EmitLog(line);
        });
    }

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message)
    {
        if (IsAvc420SurfaceOutputEnabled()) {
            message = "AVC420 surface output owns XComponent";
            return false;
        }
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        return channels.RequestCurrentFrameRender(reason, message);
    }

    void SetDisplayOrientation(uint32_t orientation)
    {
        channels.SetDisplayOrientation(orientation);
    }

    uint32_t DisplayOrientation() const
    {
        return channels.DisplayOrientation();
    }

    bool SetMonitorLayout(std::vector<FREERDP_OHOS_MONITOR_LAYOUT> monitors,
        std::string& message)
    {
        return channels.SetMonitorLayout(std::move(monitors), message);
    }

    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message)
    {
        if (!connected.load()) {
            message = "no active FreeRDP session";
            return false;
        }

        return channels.RequestDynamicDesktopResize(width, height, reason, message);
    }

    DisplayResizeResult RequestDynamicDesktopResizeEx(uint32_t width, uint32_t height,
        uint32_t orientation, const std::string& reason)
    {
        return RequestDynamicDesktopResizeEx({
            width, height, 0, 0, orientation, 100, 100, reason,
        });
    }

    DisplayResizeResult RequestDynamicDesktopResizeEx(const DisplayResizeRequest& request)
    {
        if (!connected.load()) {
            DisplayResizeResult result;
            result.status = DisplayResizeStatus::Failed;
            result.message = "no active FreeRDP session";
            return result;
        }

        return channels.RequestDynamicDesktopResizeEx(request);
    }

    void EmitState(const std::string& state)
    {
        EmitCallback(callbacks.emitState, state);
    }

    void EmitLog(const std::string& line)
    {
        EmitCallback(callbacks.emitLog, line);
    }

    void EmitInputDiagnostic(uint32_t kindMask, const char* kind, bool accepted)
    {
        std::atomic_uint32_t& emittedMask = accepted ? acceptedInputDiagnosticMask :
            rejectedInputDiagnosticMask;
        if ((emittedMask.load(std::memory_order_relaxed) & kindMask) != 0U) {
            return;
        }
        const uint32_t previous = emittedMask.fetch_or(kindMask, std::memory_order_relaxed);
        if ((previous & kindMask) != 0U) {
            return;
        }
        const uint64_t diagnosticSessionId =
            activeDiagnosticSessionId.load(std::memory_order_relaxed);
        if (diagnosticSessionId == 0) {
            return;
        }
        EmitCoreDiagnostic(diagnosticSessionId,
            std::string("event=") + (accepted ? "input_first_use" : "input_rejected") +
                " kind=" + kind,
            accepted ? BridgeLogLevel::Info : BridgeLogLevel::Warn);
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

    void SetActiveNative(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context,
        freerdpOhosSession* ohosSession)
    {
        channels.SetActive(api, instance, context, ohosSession);
    }

    void ClearActiveNative(freerdp* instance)
    {
        channels.ClearActive(instance);
    }

    bool WaitForAutoInitialResolution(ConnectParams& params)
    {
        if (!IsAutoResolution(params.resolution)) {
            return true;
        }

        constexpr int maxWaitMs = 3000;
        constexpr int stepMs = 50;
        int elapsed = 0;
        while (running.load()) {
            const SurfaceSnapshot snapshot = SurfaceSnapshotValue();
            if (snapshot.ready && snapshot.width >= 320 && snapshot.height >= 240) {
                params.resolution = std::to_string(snapshot.width) + "x" +
                    std::to_string(snapshot.height);
                return true;
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

    void WorkerMain(ConnectParams params, uint64_t diagnosticSessionId)
    {
        const auto workerStartedAt = std::chrono::steady_clock::now();
        EmitCoreDiagnostic(diagnosticSessionId,
            "event=connect_start requestedMode=" + params.graphicsMode +
                " requestedResolution=" + params.resolution +
                " authForm=" + AuthenticationForm(params.username) +
                " usernameNonAscii=" + (ContainsNonAscii(params.username) ? "yes" : "no"));
        if (!running.load()) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            EmitCoreDiagnostic(diagnosticSessionId, "event=session_end outcome=cancelled elapsedMs=" +
                std::to_string(ElapsedMilliseconds(workerStartedAt)));
            return;
        }

        EmitState("Resolving");

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
            EmitCoreDiagnostic(diagnosticSessionId,
                "event=connect_failed phase=tcp reason=" + FailureReason(tcp.message) +
                    " elapsedMs=" +
                    std::to_string(ElapsedMilliseconds(workerStartedAt)), BridgeLogLevel::Error);
            running.store(false);
            return;
        }

        EmitState("TCP connected");
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Negotiating");
        if (!SleepInterruptibly(250)) {
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            return;
        }

        EmitState("Authenticating");
        if (!WaitForAutoInitialResolution(params)) {
            EmitCoreDiagnostic(diagnosticSessionId,
                "event=connect_failed phase=surface_resolution elapsedMs=" +
                    std::to_string(ElapsedMilliseconds(workerStartedAt)), BridgeLogLevel::Error);
            return;
        }
        RdpSessionRunResult session;
        std::string lastAttemptMode = params.graphicsMode;
        const std::vector<std::string> graphicsModes = BuildGraphicsFallbackModes(params);
        if (graphicsModes.empty()) {
            const std::string message =
                "FreeRDP OHOS graphics fallback helper returned no modes";
            connected.store(false);
            EmitState("Failed");
            EmitLog(message);
            EmitError(message);
            EmitCoreDiagnostic(diagnosticSessionId,
                "event=connect_failed phase=graphics_config elapsedMs=" +
                    std::to_string(ElapsedMilliseconds(workerStartedAt)), BridgeLogLevel::Error);
            running.store(false);
            return;
        }
        for (size_t attempt = 0; attempt < graphicsModes.size(); ++attempt) {
            ConnectParams attemptParams = params;
            attemptParams.graphicsMode = graphicsModes[attempt];
            lastAttemptMode = attemptParams.graphicsMode;
            bool attemptConnected = false;
            session = RunFreerdpSession(attemptParams, diagnosticSessionId,
                channels.MonitorLayout(), running, callbacks,
                [this](FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context,
                    freerdpOhosSession* ohosSession) {
                    SetActiveNative(api, instance, context, ohosSession);
                },
                [this](freerdp* instance) {
                    ClearActiveNative(instance);
                },
                [this](const std::string& line) {
                    EmitLog(line);
                },
                [this, &attemptConnected, selectedMode = attemptParams.graphicsMode,
                    diagnosticSessionId, workerStartedAt]() {
                    attemptConnected = true;
                    connected.store(true);
                    EmitState("Connected");
                    EmitLog("graphics mode selected: " + selectedMode);
                    const SurfaceSnapshot diagnosticSurface = SurfaceSnapshotValue();
                    EmitCoreDiagnostic(diagnosticSessionId,
                        "event=login_success mode=" + selectedMode +
                            " elapsedMs=" + std::to_string(ElapsedMilliseconds(workerStartedAt)) +
                            " surface=" + std::to_string(diagnosticSurface.width) + "x" +
                            std::to_string(diagnosticSurface.height) +
                            " desktop=" + std::to_string(RdpDesktopWidth()) + "x" +
                            std::to_string(RdpDesktopHeight()));
                    std::string focusMessage;
                    if (!SendFocusIn(0, focusMessage)) {
                        EmitLog("FreeRDP focus-in skipped after session connected: " +
                            focusMessage);
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
                EmitCoreDiagnostic(diagnosticSessionId,
                    "event=graphics_fallback from=" + attemptParams.graphicsMode +
                        " to=" + graphicsModes[attempt + 1] +
                        " phase=" + (attemptConnected ? "post_connect" : "pre_connect") +
                        " elapsedMs=" + std::to_string(ElapsedMilliseconds(workerStartedAt)),
                    BridgeLogLevel::Warn);
                EmitState("Negotiating");
                continue;
            }

            if (session.failed && !attemptConnected && attempt + 1 < graphicsModes.size()) {
                EmitLog("graphics fallback skipped for non-graphics failure: " +
                    session.message);
            }
            break;
        }
        input.Clear();

        if (session.cancelled || !running.load()) {
            connected.store(false);
            EmitState("Disconnected");
            EmitLog("native worker cancelled");
            EmitCoreDiagnostic(diagnosticSessionId, "event=session_end outcome=cancelled elapsedMs=" +
                std::to_string(ElapsedMilliseconds(workerStartedAt)));
            return;
        }

        if (session.failed) {
            std::string message = session.available ? session.message : "FreeRDP runtime unavailable: " + session.message;
            connected.store(false);
            EmitState("Failed");
            EmitLog(message);
            EmitError(message);
            EmitCoreDiagnostic(diagnosticSessionId,
                "event=connect_failed phase=freerdp mode=" + lastAttemptMode +
                    " connected=" + (session.connected ? "yes" : "no") +
                    " reason=" + FailureReason(session.message) +
                    " elapsedMs=" + std::to_string(ElapsedMilliseconds(workerStartedAt)),
                BridgeLogLevel::Error);
            running.store(false);
            return;
        }

        connected.store(false);
        EmitState("Disconnected");
        EmitLog(session.message);
        EmitCoreDiagnostic(diagnosticSessionId, "event=session_end outcome=disconnected mode=" +
            lastAttemptMode + " elapsedMs=" +
            std::to_string(ElapsedMilliseconds(workerStartedAt)));
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

void RdpSession::SetDisplayOrientation(uint32_t orientation)
{
    impl_->SetDisplayOrientation(orientation);
}

uint32_t RdpSession::DisplayOrientation() const
{
    return impl_->DisplayOrientation();
}

bool RdpSession::SetMonitorLayout(std::vector<FREERDP_OHOS_MONITOR_LAYOUT> monitors,
    std::string& message)
{
    return impl_->SetMonitorLayout(std::move(monitors), message);
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

bool RdpSession::SendLocalPen(const LocalPenEvent& pen, std::string& message)
{
    return impl_->SendLocalPen(pen, message);
}

DisplayResizeResult RdpSession::RequestDynamicDesktopResizeEx(uint32_t width, uint32_t height,
    uint32_t orientation, const std::string& reason)
{
    return impl_->RequestDynamicDesktopResizeEx(width, height, orientation, reason);
}

DisplayResizeResult RdpSession::RequestDynamicDesktopResizeEx(const DisplayResizeRequest& request)
{
    return impl_->RequestDynamicDesktopResizeEx(request);
}

} // namespace rdp_bridge
