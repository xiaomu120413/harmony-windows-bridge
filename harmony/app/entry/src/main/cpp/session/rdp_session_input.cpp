#include "session/rdp_session_input.h"

#include <array>

namespace rdp_bridge {

RdpSessionInput::RdpSessionInput() = default;

RdpSessionInput::~RdpSessionInput()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (queue_ != nullptr && queueApi_ != nullptr && queueApi_->ohosInputQueueFree != nullptr) {
        queueApi_->ohosInputQueueFree(queue_);
    }
#endif
}

namespace {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
uint32_t ToOhosPointerAction(LocalPointerAction action)
{
    switch (action) {
        case LocalPointerAction::ButtonDown:
            return FREERDP_OHOS_POINTER_ACTION_BUTTON_DOWN;
        case LocalPointerAction::ButtonUp:
            return FREERDP_OHOS_POINTER_ACTION_BUTTON_UP;
        case LocalPointerAction::WheelVertical:
            return FREERDP_OHOS_POINTER_ACTION_WHEEL_VERTICAL;
        case LocalPointerAction::WheelHorizontal:
            return FREERDP_OHOS_POINTER_ACTION_WHEEL_HORIZONTAL;
        case LocalPointerAction::Move:
        default:
            return FREERDP_OHOS_POINTER_ACTION_MOVE;
    }
}

uint32_t ToOhosPointerButtons(uint32_t buttons)
{
    uint32_t result = FREERDP_OHOS_POINTER_BUTTON_NONE;
    if ((buttons & LocalPointerButtonLeft) != 0) {
        result |= FREERDP_OHOS_POINTER_BUTTON_LEFT;
    }
    if ((buttons & LocalPointerButtonRight) != 0) {
        result |= FREERDP_OHOS_POINTER_BUTTON_RIGHT;
    }
    if ((buttons & LocalPointerButtonMiddle) != 0) {
        result |= FREERDP_OHOS_POINTER_BUTTON_MIDDLE;
    }
    return result;
}

FREERDP_OHOS_POINTER_VIEWPORT BuildOhosPointerViewport(
    const SurfaceSnapshot& surface, uint32_t desktopWidth, uint32_t desktopHeight)
{
    FREERDP_OHOS_POINTER_VIEWPORT viewport = {};
    viewport.surfaceWidth = surface.width;
    viewport.surfaceHeight = surface.height;
    viewport.viewportX = surface.viewportX;
    viewport.viewportY = surface.viewportY;
    viewport.viewportWidth = surface.viewportWidth;
    viewport.viewportHeight = surface.viewportHeight;
    viewport.desktopWidth = desktopWidth;
    viewport.desktopHeight = desktopHeight;
    return viewport;
}

FREERDP_OHOS_KEY_EVENT ToOhosKeyEvent(const OhosKeyEvent& event)
{
    return FREERDP_OHOS_KEY_EVENT {
        event.keyCode,
        event.down ? 1 : 0,
        event.repeat ? 1 : 0,
        event.ctrl ? 1 : 0,
        event.shift ? 1 : 0,
        event.alt ? 1 : 0,
        event.meta ? 1 : 0,
    };
}
#endif

} // namespace

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool RdpSessionInput::EnsureQueue(
    std::string& message, const std::function<void(const std::string&)>& log)
{
    auto& api = SharedFreerdpRuntimeApi();
    std::string runtimeError;
    if (!EnsureFreerdpRuntimeLoaded(api, runtimeError)) {
        message = runtimeError;
        LogInputFailure(message, log);
        return false;
    }

    const bool complete = api.ohosInputQueueNew != nullptr &&
        api.ohosInputQueueFree != nullptr &&
        api.ohosInputQueueClear != nullptr &&
        api.ohosInputQueueReset != nullptr &&
        api.ohosInputQueueEnqueuePointer != nullptr &&
        api.ohosInputQueueEnqueuePointerPacket != nullptr &&
        api.ohosInputQueueEnqueueKeyScancode != nullptr &&
        api.ohosInputQueueEnqueueKey != nullptr &&
        api.ohosInputQueueEnqueueUnicode != nullptr &&
        api.ohosInputQueueEnqueueText != nullptr &&
        api.ohosInputQueueEnqueueFocusIn != nullptr &&
        api.ohosInputQueueEnqueueReleaseAllKeys != nullptr &&
        api.ohosInputQueueDrain != nullptr &&
        api.ohosInputQueueGetDiagnostics != nullptr;
    if (!complete) {
        message = "FreeRDP OHOS input queue symbols are not loaded";
        LogInputFailure(message, log);
        return false;
    }

    if (queue_ == nullptr) {
        queue_ = api.ohosInputQueueNew();
        queueApi_ = &api;
        if (queue_ == nullptr) {
            queueApi_ = nullptr;
            message = "FreeRDP OHOS input queue allocation failed";
            LogInputFailure(message, log);
            return false;
        }
    }
    return true;
}

FREERDP_OHOS_INPUT_QUEUE_DIAGNOSTICS RdpSessionInput::Diagnostics() const
{
    FREERDP_OHOS_INPUT_QUEUE_DIAGNOSTICS diagnostics = {};
    if (queue_ != nullptr && queueApi_ != nullptr &&
        queueApi_->ohosInputQueueGetDiagnostics != nullptr) {
        queueApi_->ohosInputQueueGetDiagnostics(queue_, &diagnostics);
    }
    return diagnostics;
}
#endif

bool RdpSessionInput::EnqueuePointer(uint16_t flags, uint16_t x, uint16_t y,
    std::string& message, const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 256> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueuePointerPacket(
        queue_, flags, x, y, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)flags;
    (void)x;
    (void)y;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueLocalPointer(const LocalPointerEvent& pointer,
    const SurfaceSnapshot& surface, uint32_t desktopWidth, uint32_t desktopHeight,
    std::string& message, const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }

    FREERDP_OHOS_POINTER_VIEWPORT viewport =
        BuildOhosPointerViewport(surface, desktopWidth, desktopHeight);
    FREERDP_OHOS_POINTER_EVENT nativeEvent = {};
    nativeEvent.action = ToOhosPointerAction(pointer.action);
    nativeEvent.buttons = ToOhosPointerButtons(pointer.buttons);
    nativeEvent.x = pointer.x;
    nativeEvent.y = pointer.y;
    nativeEvent.delta = pointer.delta;
    nativeEvent.allowClamp = pointer.allowClamp ? TRUE : FALSE;

    std::array<char, 256> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueuePointer(
        queue_, &viewport, &nativeEvent, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)pointer;
    (void)surface;
    (void)desktopWidth;
    (void)desktopHeight;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueKey(uint32_t rdpScancode, bool down, bool repeat,
    std::string& message, const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 160> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueKeyScancode(
        queue_, rdpScancode, down ? TRUE : FALSE, repeat ? TRUE : FALSE,
        detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)rdpScancode;
    (void)down;
    (void)repeat;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueuePlatformKey(const OhosKeyEvent& event, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    const FREERDP_OHOS_KEY_EVENT nativeEvent = ToOhosKeyEvent(event);
    std::array<char, 256> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueKey(
        queue_, &nativeEvent, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)event;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueUnicode(uint32_t code, bool down, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 160> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueUnicode(
        queue_, code, down ? TRUE : FALSE, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)code;
    (void)down;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueCommittedText(const std::u16string& text, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 256> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueText(
        queue_, reinterpret_cast<const uint16_t*>(text.data()), text.size(),
        detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)text;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueFocusIn(uint16_t toggleStates, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 160> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueFocusIn(
        queue_, toggleStates, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)toggleStates;
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

bool RdpSessionInput::EnqueueReleaseAllKeys(
    std::string& message, const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!EnsureQueue(message, log)) {
        return false;
    }
    std::array<char, 160> detail {};
    const BOOL ok = queueApi_->ohosInputQueueEnqueueReleaseAllKeys(
        queue_, detail.data(), detail.size());
    message = detail.data();
    if (!ok) {
        LogInputFailure(message, log);
    }
    return ok == TRUE;
#else
    (void)log;
    message = "explicit FreeRDP demo build has no headers";
    return false;
#endif
}

void RdpSessionInput::Clear()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (queue_ != nullptr && queueApi_ != nullptr && queueApi_->ohosInputQueueClear != nullptr) {
        queueApi_->ohosInputQueueClear(queue_);
    }
#endif
}

void RdpSessionInput::Reset()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (queue_ != nullptr && queueApi_ != nullptr && queueApi_->ohosInputQueueReset != nullptr) {
        queueApi_->ohosInputQueueReset(queue_);
    }
#endif
    inputFailureLogCount_.store(0);
}

uint32_t RdpSessionInput::QueueDepth() const
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    return Diagnostics().depth;
#else
    return 0;
#endif
}

uint32_t RdpSessionInput::QueuedCount() const
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    return Diagnostics().queued;
#else
    return 0;
#endif
}

uint32_t RdpSessionInput::SentCount() const
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    return Diagnostics().sent;
#else
    return 0;
#endif
}

uint32_t RdpSessionInput::DroppedCount() const
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    return Diagnostics().dropped;
#else
    return 0;
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void RdpSessionInput::Drain(FreerdpRuntimeApi* api, rdpContext* context,
    const std::function<void(const std::string&)>& log)
{
    (void)api;
    if (queue_ == nullptr || queueApi_ == nullptr || queueApi_->ohosInputQueueDrain == nullptr) {
        return;
    }
    std::array<char, 256> detail {};
    const BOOL ok = queueApi_->ohosInputQueueDrain(queue_, context, detail.data(), detail.size());
    if (detail[0] != '\0' && log != nullptr) {
        log(detail.data());
    }
    if (!ok && detail[0] != '\0') {
        LogInputFailure(detail.data(), log);
    }
}
#endif

void RdpSessionInput::LogInputFailure(
    const std::string& message, const std::function<void(const std::string&)>& log)
{
    const uint32_t logIndex = inputFailureLogCount_.fetch_add(1);
    if (log != nullptr && (logIndex < 5 || logIndex % 100 == 0)) {
        log(message);
    }
}

} // namespace rdp_bridge
