#include "rdp_session_input.h"

#include <array>
#include <vector>

namespace rdp_bridge {
namespace {

constexpr uint32_t kWinprKeyboardTypeIbmEnhanced = 0x00000004U;
constexpr uint32_t kWinprKeyboardExtendedFlag = 0x0100U;
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
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    keyboardState_ = freerdp_ohos_keyboard_state_new();
#endif
}

RdpSessionInput::~RdpSessionInput()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    freerdp_ohos_keyboard_state_free(keyboardState_);
    keyboardState_ = nullptr;
#endif
}

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
    if (freerdp_ohos_ime_build_committed_text_packets(
        reinterpret_cast<const uint16_t*>(text.data()), text.size(), packets.data(), packets.size(),
        &packetCount, &skipped) == 0) {
        message = "OHOS IME committed text conversion failed";
        LogInputFailure(message, log);
        return false;
    }

    std::array<char, 160> formatted {};
    if (freerdp_ohos_ime_format_committed_text_result(text.size(), packetCount, skipped,
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
    freerdp_ohos_keyboard_state_reset(keyboardState_);
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
    event.down = packet.down != 0;
    event.repeat = packet.repeat != 0;
    event.extended = packet.extended != 0;
    event.synthetic = packet.synthetic != 0;
    pending.push_back(event);
}

bool RdpSessionInput::AppendPlatformKeyPacketsLocked(const QueuedInputEvent& event,
    std::deque<QueuedInputEvent>& pending, const std::function<void(const std::string&)>& log)
{
    FREERDP_OHOS_KEY_PACKET packets[kMaxOhosKeyPackets] = {};
    size_t packetCount = 0;
    bool ok = false;

    if (keyboardState_ == nullptr) {
        LogInputFailure("OHOS keyboard state backend is not initialized", log);
        return false;
    }

    if (event.synthetic && event.keyCode == 0) {
        ok = freerdp_ohos_keyboard_state_release_all(keyboardState_, packets,
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
        ok = freerdp_ohos_keyboard_state_handle_event(keyboardState_, &nativeEvent, packets,
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

void RdpSessionInput::AppendDueRepeatPacketsLocked(std::deque<QueuedInputEvent>& pending,
    const std::function<void(const std::string&)>& log)
{
    FREERDP_OHOS_KEY_PACKET packets[kMaxOhosKeyPackets] = {};
    size_t packetCount = 0;

    if (keyboardState_ == nullptr) {
        return;
    }

    if (freerdp_ohos_keyboard_state_collect_due_repeats(keyboardState_, packets,
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
                AppendPlatformKeyPacketsLocked(event, pending, log);
            } else {
                pending.push_back(event);
            }
        }
        AppendDueRepeatPacketsLocked(pending, log);

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
            if (api->inputSendKeyboardEventEx != nullptr &&
                api->getVirtualScanCodeFromVirtualKeyCode != nullptr) {
                const uint32_t vkForScancode = event.vk |
                    (event.extended ? kWinprKeyboardExtendedFlag : 0U);
                const uint32_t scancode = api->getVirtualScanCodeFromVirtualKeyCode(
                    vkForScancode, kWinprKeyboardTypeIbmEnhanced);
                if (scancode == 0) {
                    LogInputFailure("FreeRDP platform key scancode mapping failed: keyCode=" +
                        std::to_string(event.keyCode) + " vk=" + HexInput(event.vk) +
                        (event.extended ? " extended" : ""), log);
                } else {
                    ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE,
                        event.repeat ? TRUE : FALSE, scancode);
                    LogPlatformKeyDispatch(event, scancode, ok == TRUE, log);
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
