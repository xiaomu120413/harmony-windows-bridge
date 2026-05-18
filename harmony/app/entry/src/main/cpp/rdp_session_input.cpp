#include "rdp_session_input.h"

#include <array>
#include <vector>

namespace rdp_bridge {
namespace {

constexpr size_t kMaxOhosKeyPackets = 96;

std::string HexInput(uint32_t value)
{
    constexpr char digits[] = "0123456789ABCDEF";
    std::string result = "0x";
    bool started = false;
    for (int shift = 28; shift >= 0; shift -= 4) {
        const uint32_t nibble = (value >> shift) & 0xFU;
        if (nibble != 0 || started || shift == 0) {
            result.push_back(digits[nibble]);
            started = true;
        }
    }
    return result;
}

} // namespace

RdpSessionInput::RdpSessionInput()
{
}

RdpSessionInput::~RdpSessionInput()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (keyboardState_ != nullptr && keyboardApi_ != nullptr &&
        keyboardApi_->ohosKeyboardStateFree != nullptr) {
        keyboardApi_->ohosKeyboardStateFree(keyboardState_);
    }
    keyboardState_ = nullptr;
    keyboardApi_ = nullptr;
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
#endif

} // namespace

bool RdpSessionInput::EnqueuePointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    QueuedInputEvent event;
    event.type = QueuedInputType::Pointer;
    event.flags = flags;
    event.x = x;
    event.y = y;
    return EnqueueInput(event, "pointer event queued", message, log);
#else
    (void)flags;
    (void)x;
    (void)y;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueLocalPointer(const LocalPointerEvent& pointer,
    const SurfaceSnapshot& surface, uint32_t desktopWidth, uint32_t desktopHeight,
    std::string& message, const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    auto& api = SharedFreerdpRuntimeApi();
    std::string runtimeError;
    if (!EnsureFreerdpRuntimeLoaded(api, runtimeError) || api.ohosPointerBuildEvent == nullptr) {
        message = runtimeError.empty() ? "FreeRDP OHOS pointer backend unavailable" : runtimeError;
        LogInputFailure(message, log);
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

    FREERDP_OHOS_POINTER_PACKET packet = {};
    std::array<char, 256> detail {};
    if (!api.ohosPointerBuildEvent(&viewport, &nativeEvent, &packet, detail.data(), detail.size()) ||
        !packet.ok) {
        message = detail[0] == '\0' ? "OHOS pointer event mapping failed" : detail.data();
        LogInputFailure(message, log);
        return false;
    }

    QueuedInputEvent event;
    event.type = QueuedInputType::Pointer;
    event.flags = packet.flags;
    event.x = packet.x;
    event.y = packet.y;
    return EnqueueInput(event, detail[0] == '\0' ? "pointer event queued" : detail.data(),
        message, log);
#else
    (void)pointer;
    (void)surface;
    (void)desktopWidth;
    (void)desktopHeight;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    QueuedInputEvent event;
    event.type = QueuedInputType::Key;
    event.scancode = rdpScancode;
    event.down = down;
    event.repeat = repeat;
    return EnqueueInput(event, down ? (repeat ? "key down queued repeat" : "key down queued") : "key up queued",
        message, log);
#else
    (void)rdpScancode;
    (void)down;
    (void)repeat;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueuePlatformKey(const OhosKeyEvent& key, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    const uint32_t vk = MapOhosKeyCodeToWindowsVk(key.keyCode);
    if (vk == 0) {
        message = FormatOhosKeyEvent(key) + " not mapped";
        LogInputFailure(message, log);
        return false;
    }

    QueuedInputEvent event;
    event.type = QueuedInputType::PlatformKey;
    event.keyCode = key.keyCode;
    event.vk = vk;
    event.down = key.down;
    event.repeat = key.repeat;
    event.ctrl = key.ctrl;
    event.shift = key.shift;
    event.alt = key.alt;
    event.meta = key.meta;
    event.extended = OhosKeyCodeRequiresExtendedScancode(key.keyCode);
    return EnqueueInput(event, key.down ? (key.repeat ? "platform key down queued repeat" :
        "platform key down queued") : "platform key up queued", message, log);
#else
    (void)key;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueUnicode(uint32_t code, bool down, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (code == 0 || code > 0xFFFFU) {
        message = "unicode input requires a BMP UTF-16 code unit";
        return false;
    }

    QueuedInputEvent event;
    event.type = QueuedInputType::Unicode;
    event.code = code;
    event.down = down;
    return EnqueueInput(event, down ? "unicode key down queued" : "unicode key up queued", message, log);
#else
    (void)code;
    (void)down;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueCommittedText(const std::u16string& text, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (text.empty()) {
        message = "committed text input is empty";
        return false;
    }

    std::vector<FREERDP_OHOS_IME_PACKET> packets(text.size() * 2U);
    size_t packetCount = 0;
    size_t skipped = 0;
    auto& api = SharedFreerdpRuntimeApi();
    std::string runtimeError;
    if (!EnsureFreerdpRuntimeLoaded(api, runtimeError) ||
        api.ohosImeBuildCommittedTextPackets == nullptr ||
        api.ohosImeFormatCommittedTextResult == nullptr) {
        message = runtimeError.empty() ? "FreeRDP OHOS IME backend unavailable" : runtimeError;
        LogInputFailure(message, log);
        return false;
    }

    if (api.ohosImeBuildCommittedTextPackets(
        reinterpret_cast<const uint16_t*>(text.data()), text.size(), packets.data(), packets.size(),
        &packetCount, &skipped) == 0) {
        message = "OHOS IME committed text conversion failed";
        LogInputFailure(message, log);
        return false;
    }

    std::array<char, 160> formatted {};
    if (api.ohosImeFormatCommittedTextResult(text.size(), packetCount, skipped,
        formatted.data(), formatted.size()) != 0) {
        message = formatted.data();
    } else {
        message = "OHOS IME committed text queued";
    }

    if (packetCount == 0) {
        LogInputFailure(message, log);
        return skipped == 0;
    }

    bool failed = false;
    bool droppedOldPointer = false;
    bool droppedNewEvent = false;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        for (size_t index = 0; index < packetCount; ++index) {
            QueuedInputEvent event;
            event.type = QueuedInputType::Unicode;
            event.code = packets[index].codeUnit;
            event.down = packets[index].down != 0;

            std::string packetMessage;
            bool oldPointer = false;
            bool newEvent = false;
            if (!EnqueueInputLocked(event, "unicode text packet queued", packetMessage,
                oldPointer, newEvent)) {
                failed = true;
                message = packetMessage;
            }
            droppedOldPointer = droppedOldPointer || oldPointer;
            droppedNewEvent = droppedNewEvent || newEvent;
            if (newEvent) {
                break;
            }
        }
    }

    if (droppedOldPointer) {
        LogInputBackpressure("FreeRDP input queue protected committed text by dropping pending pointer motion",
            log);
    }
    if (failed || droppedNewEvent) {
        LogInputFailure(message, log);
        return false;
    }
    if (skipped > 0) {
        LogInputFailure(message, log);
    }
    return true;
#else
    (void)text;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueFocusIn(uint16_t toggleStates, std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    QueuedInputEvent event;
    event.type = QueuedInputType::FocusIn;
    event.flags = toggleStates;
    return EnqueueInput(event, "focus-in event queued", message, log);
#else
    (void)toggleStates;
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionInput::EnqueueReleaseAllKeys(std::string& message,
    const std::function<void(const std::string&)>& log)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    QueuedInputEvent event;
    event.type = QueuedInputType::PlatformKey;
    event.keyCode = 0;
    event.synthetic = true;
    return EnqueueInput(event, "platform key release-all queued", message, log);
#else
    (void)log;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

void RdpSessionInput::Clear()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(inputMutex_);
    inputQueue_.clear();
    if (keyboardState_ != nullptr && keyboardApi_ != nullptr &&
        keyboardApi_->ohosKeyboardStateReset != nullptr) {
        keyboardApi_->ohosKeyboardStateReset(keyboardState_);
    }
#endif
    inputQueueDepth_.store(0);
}

void RdpSessionInput::Reset()
{
    Clear();
    inputQueuedCount_.store(0);
    inputSentCount_.store(0);
    inputDroppedCount_.store(0);
    inputDispatchLogCount_.store(0);
    inputKeyDispatchLogCount_.store(0);
    inputFailureLogCount_.store(0);
    inputBackpressureLogCount_.store(0);
}

uint32_t RdpSessionInput::QueueDepth() const
{
    return inputQueueDepth_.load();
}

uint32_t RdpSessionInput::QueuedCount() const
{
    return inputQueuedCount_.load();
}

uint32_t RdpSessionInput::SentCount() const
{
    return inputSentCount_.load();
}

uint32_t RdpSessionInput::DroppedCount() const
{
    return inputDroppedCount_.load();
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
const char* RdpSessionInput::InputTypeName(const QueuedInputEvent& event) const
{
    if (event.type == QueuedInputType::Pointer) {
        return "pointer";
    }
    if (event.type == QueuedInputType::Key) {
        return "key";
    }
    if (event.type == QueuedInputType::PlatformKey) {
        return "platform-key";
    }
    if (event.type == QueuedInputType::PlatformKeyPacket) {
        return "platform-key-packet";
    }
    if (event.type == QueuedInputType::FocusIn) {
        return "focus-in";
    }
    return "unicode";
}

bool RdpSessionInput::IsPointerWheelEvent(const QueuedInputEvent& event) const
{
    return event.type == QueuedInputType::Pointer &&
        (event.flags & (PTR_FLAGS_WHEEL | PTR_FLAGS_HWHEEL)) != 0;
}

bool RdpSessionInput::IsPointerMotionEvent(const QueuedInputEvent& event) const
{
    return event.type == QueuedInputType::Pointer &&
        (event.flags & PTR_FLAGS_MOVE) != 0 &&
        !IsPointerWheelEvent(event);
}

bool RdpSessionInput::HasSamePointerMotionClass(const QueuedInputEvent& lhs,
    const QueuedInputEvent& rhs) const
{
    constexpr uint16_t pointerStateMask = PTR_FLAGS_BUTTON1 | PTR_FLAGS_BUTTON2 | PTR_FLAGS_BUTTON3 | PTR_FLAGS_DOWN;
    return (lhs.flags & pointerStateMask) == (rhs.flags & pointerStateMask);
}

bool RdpSessionInput::IsDroppablePointerEvent(const QueuedInputEvent& event) const
{
    return IsPointerMotionEvent(event) || IsPointerWheelEvent(event);
}

bool RdpSessionInput::DropOldestDroppablePointerEventLocked()
{
    for (auto iter = inputQueue_.begin(); iter != inputQueue_.end(); ++iter) {
        if (IsDroppablePointerEvent(*iter)) {
            inputQueue_.erase(iter);
            inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
            return true;
        }
    }
    return false;
}

bool RdpSessionInput::EnsureKeyboardBackendLocked(FreerdpRuntimeApi* api,
    const std::function<void(const std::string&)>& log)
{
    if (api == nullptr) {
        LogInputFailure("FreeRDP runtime is unavailable for OHOS keyboard backend", log);
        return false;
    }
    if (api->ohosKeyboardStateNew == nullptr || api->ohosKeyboardStateFree == nullptr ||
        api->ohosKeyboardStateReset == nullptr || api->ohosKeyboardStateHandleEvent == nullptr ||
        api->ohosKeyboardStateCollectDueRepeats == nullptr ||
        api->ohosKeyboardStateReleaseAll == nullptr) {
        LogInputFailure("FreeRDP OHOS keyboard symbols are not loaded", log);
        return false;
    }
    if (keyboardState_ == nullptr) {
        keyboardState_ = api->ohosKeyboardStateNew();
        keyboardApi_ = api;
        if (keyboardState_ == nullptr) {
            keyboardApi_ = nullptr;
            LogInputFailure("FreeRDP OHOS keyboard state allocation failed", log);
            return false;
        }
    }
    return true;
}

bool RdpSessionInput::EnqueueInputLocked(const QueuedInputEvent& event, const char* okMessage,
    std::string& message, bool& droppedOldPointer, bool& droppedNewEvent)
{
    constexpr size_t maxInputQueue = 4096;

    droppedOldPointer = false;
    droppedNewEvent = false;

    if (IsPointerMotionEvent(event) && !inputQueue_.empty() &&
        IsPointerMotionEvent(inputQueue_.back()) &&
        HasSamePointerMotionClass(event, inputQueue_.back())) {
        inputQueue_.back() = event;
        inputQueuedCount_.fetch_add(1);
        message = okMessage;
        return true;
    }

    if (inputQueue_.size() >= maxInputQueue) {
        const bool mustProtectNewEvent = event.type != QueuedInputType::Pointer || !IsDroppablePointerEvent(event);
        if (mustProtectNewEvent && DropOldestDroppablePointerEventLocked()) {
            inputDroppedCount_.fetch_add(1);
            droppedOldPointer = true;
        }
    }

    if (inputQueue_.size() >= maxInputQueue) {
        inputDroppedCount_.fetch_add(1);
        message = std::string("FreeRDP input queue is full; dropped ") + InputTypeName(event) + " event";
        droppedNewEvent = true;
        return false;
    }

    inputQueue_.push_back(event);
    inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
    inputQueuedCount_.fetch_add(1);
    message = okMessage;
    return true;
}

bool RdpSessionInput::EnqueueInput(const QueuedInputEvent& event, const char* okMessage, std::string& message,
    const std::function<void(const std::string&)>& log)
{
    bool droppedOldPointer = false;
    bool droppedNewEvent = false;

    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        EnqueueInputLocked(event, okMessage, message, droppedOldPointer, droppedNewEvent);
    }

    if (droppedOldPointer) {
        LogInputBackpressure(std::string("FreeRDP input queue protected ") + InputTypeName(event) +
            " event by dropping pending pointer motion", log);
    }
    if (droppedNewEvent) {
        LogInputFailure(message, log);
        return false;
    }
    return true;
}

void RdpSessionInput::AppendPlatformKeyPacketLocked(const FREERDP_OHOS_KEY_PACKET& packet,
    std::deque<QueuedInputEvent>& pending)
{
    QueuedInputEvent event;
    event.type = QueuedInputType::PlatformKeyPacket;
    event.keyCode = packet.keyCode;
    event.vk = packet.windowsVk;
    event.scancode = packet.rdpScancode;
    event.down = packet.down != 0;
    event.repeat = packet.repeat != 0;
    event.extended = packet.extended != 0;
    event.synthetic = packet.synthetic != 0;
    pending.push_back(event);
}

bool RdpSessionInput::AppendPlatformKeyPacketsLocked(FreerdpRuntimeApi* api,
    const QueuedInputEvent& event,
    std::deque<QueuedInputEvent>& pending, const std::function<void(const std::string&)>& log)
{
    FREERDP_OHOS_KEY_PACKET packets[kMaxOhosKeyPackets] = {};
    size_t packetCount = 0;
    bool ok = false;

    if (!EnsureKeyboardBackendLocked(api, log)) {
        return false;
    }

    if (event.synthetic && event.keyCode == 0) {
        ok = api->ohosKeyboardStateReleaseAll(keyboardState_, packets,
            kMaxOhosKeyPackets, &packetCount) != 0;
    } else {
        FREERDP_OHOS_KEY_EVENT nativeEvent {
            event.keyCode,
            event.down ? 1 : 0,
            event.repeat ? 1 : 0,
            event.ctrl ? 1 : 0,
            event.shift ? 1 : 0,
            event.alt ? 1 : 0,
            event.meta ? 1 : 0,
        };
        ok = api->ohosKeyboardStateHandleEvent(keyboardState_, &nativeEvent, packets,
            kMaxOhosKeyPackets, &packetCount) != 0;
    }

    if (!ok) {
        LogInputFailure("OHOS keyboard state backend failed for keyCode=" +
            std::to_string(event.keyCode), log);
        return false;
    }

    for (size_t index = 0; index < packetCount; ++index) {
        AppendPlatformKeyPacketLocked(packets[index], pending);
    }
    return true;
}

void RdpSessionInput::AppendDueRepeatPacketsLocked(FreerdpRuntimeApi* api,
    std::deque<QueuedInputEvent>& pending,
    const std::function<void(const std::string&)>& log)
{
    FREERDP_OHOS_KEY_PACKET packets[kMaxOhosKeyPackets] = {};
    size_t packetCount = 0;

    if (!EnsureKeyboardBackendLocked(api, log)) {
        return;
    }

    if (api->ohosKeyboardStateCollectDueRepeats(keyboardState_, packets,
        kMaxOhosKeyPackets, &packetCount) == 0) {
        LogInputFailure("OHOS keyboard repeat collection failed", log);
        return;
    }

    for (size_t index = 0; index < packetCount; ++index) {
        AppendPlatformKeyPacketLocked(packets[index], pending);
    }
}

void RdpSessionInput::Drain(FreerdpRuntimeApi* api, rdpContext* context,
    const std::function<void(const std::string&)>& log)
{
    std::deque<QueuedInputEvent> pending;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        std::deque<QueuedInputEvent> queued;
        queued.swap(inputQueue_);
        inputQueueDepth_.store(0);

        for (const QueuedInputEvent& event : queued) {
            if (event.type == QueuedInputType::PlatformKey) {
                AppendPlatformKeyPacketsLocked(api, event, pending, log);
            } else {
                pending.push_back(event);
            }
        }
        AppendDueRepeatPacketsLocked(api, pending, log);

        if (pending.empty()) {
            inputQueueDepth_.store(0);
            return;
        }
    }

    if (api == nullptr || context == nullptr || context->input == nullptr) {
        inputDroppedCount_.fetch_add(static_cast<uint32_t>(pending.size()));
        LogInputFailure("FreeRDP input context is not ready; queued input dropped", log);
        return;
    }

    uint32_t sent = 0;
    for (const QueuedInputEvent& event : pending) {
        BOOL ok = FALSE;
        if (event.type == QueuedInputType::Pointer) {
            if (api->inputSendMouseEvent != nullptr) {
                ok = api->inputSendMouseEvent(context->input, event.flags, event.x, event.y);
            }
        } else if (event.type == QueuedInputType::Key) {
            if (api->inputSendKeyboardEvent != nullptr) {
                uint16_t flags = RDP_SCANCODE_EXTENDED(event.scancode) ? KBD_FLAGS_EXTENDED : 0;
                if (event.down) {
                    flags |= KBD_FLAGS_DOWN;
                } else if (!event.down) {
                    flags |= KBD_FLAGS_RELEASE;
                }
                ok = api->inputSendKeyboardEvent(context->input, flags, RDP_SCANCODE_CODE(event.scancode));
                LogKeyDispatch(event, flags, ok == TRUE, log);
            } else if (api->inputSendKeyboardEventEx != nullptr) {
                ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE,
                    event.repeat ? TRUE : FALSE, event.scancode);
            }
        } else if (event.type == QueuedInputType::PlatformKeyPacket) {
            if (api->inputSendKeyboardEventEx != nullptr) {
                if (event.scancode == 0) {
                    LogInputFailure("FreeRDP OHOS platform key has no RDP scancode: keyCode=" +
                        std::to_string(event.keyCode) + " vk=" + HexInput(event.vk) +
                        (event.extended ? " extended" : ""), log);
                } else {
                    ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE,
                        event.repeat ? TRUE : FALSE, event.scancode);
                    LogPlatformKeyDispatch(event, event.scancode, ok == TRUE, log);
                }
            }
        } else if (event.type == QueuedInputType::FocusIn) {
            if (api->inputSendFocusInEvent != nullptr) {
                ok = api->inputSendFocusInEvent(context->input, event.flags);
                if (log != nullptr) {
                    log("FreeRDP focus-in dispatch: toggleStates=" +
                        HexInput(event.flags) + (ok ? " ok" : " failed"));
                }
            }
        } else {
            if (api->inputSendUnicodeKeyboardEvent != nullptr) {
                const UINT16 flags = event.down ? 0 : KBD_FLAGS_RELEASE;
                ok = api->inputSendUnicodeKeyboardEvent(context->input, flags, static_cast<UINT16>(event.code));
            }
        }

        if (ok) {
            ++sent;
        } else {
            inputDroppedCount_.fetch_add(1);
            LogInputFailure("FreeRDP input dispatch failed on worker thread", log);
        }
    }

    if (sent == 0) {
        return;
    }

    const uint32_t totalSent = inputSentCount_.fetch_add(sent) + sent;
    const uint32_t logIndex = inputDispatchLogCount_.fetch_add(1);
    if (log != nullptr && (logIndex < 5 || totalSent % 200 == 0)) {
        log("FreeRDP input dispatched on worker thread: " + std::to_string(sent) +
            " event(s), total=" + std::to_string(totalSent));
    }
}

void RdpSessionInput::LogInputFailure(const std::string& message,
    const std::function<void(const std::string&)>& log)
{
    const uint32_t logIndex = inputFailureLogCount_.fetch_add(1);
    if (log != nullptr && (logIndex < 5 || logIndex % 100 == 0)) {
        log(message);
    }
}

void RdpSessionInput::LogKeyDispatch(const QueuedInputEvent& event, uint16_t flags, bool ok,
    const std::function<void(const std::string&)>& log)
{
    const uint32_t logIndex = inputKeyDispatchLogCount_.fetch_add(1);
    if (log == nullptr || (logIndex >= 80 && logIndex % 200 != 0)) {
        return;
    }
    log("FreeRDP key dispatch: scancode=" + std::to_string(event.scancode) +
        " code=" + std::to_string(static_cast<uint32_t>(RDP_SCANCODE_CODE(event.scancode))) +
        " flags=" + std::to_string(static_cast<uint32_t>(flags)) +
        (event.down ? " down" : " up") +
        (event.repeat ? " repeat" : "") +
        (ok ? " ok" : " failed"));
}

void RdpSessionInput::LogPlatformKeyDispatch(const QueuedInputEvent& event, uint32_t scancode, bool ok,
    const std::function<void(const std::string&)>& log)
{
    const uint32_t logIndex = inputKeyDispatchLogCount_.fetch_add(1);
    if (log == nullptr || (logIndex >= 80 && logIndex % 200 != 0)) {
        return;
    }
    log("FreeRDP platform key dispatch: keyCode=" + std::to_string(event.keyCode) +
        " vk=" + HexInput(event.vk) +
        " scancode=" + std::to_string(scancode) +
        (event.extended ? " extended" : "") +
        (event.down ? " down" : " up") +
        (event.repeat ? " repeat" : "") +
        (event.synthetic ? " synthetic" : "") +
        (ok ? " ok" : " failed"));
}

void RdpSessionInput::LogInputBackpressure(const std::string& message,
    const std::function<void(const std::string&)>& log)
{
    const uint32_t logIndex = inputBackpressureLogCount_.fetch_add(1);
    if (log != nullptr && (logIndex < 5 || logIndex % 100 == 0)) {
        log(message);
    }
}
#endif

} // namespace rdp_bridge
