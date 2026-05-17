#include "rdp_session_input.h"

namespace rdp_bridge {

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

void RdpSessionInput::Clear()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(inputMutex_);
    inputQueue_.clear();
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

bool RdpSessionInput::EnqueueInput(const QueuedInputEvent& event, const char* okMessage, std::string& message,
    const std::function<void(const std::string&)>& log)
{
    constexpr size_t maxInputQueue = 4096;
    bool droppedOldPointer = false;
    bool droppedNewEvent = false;

    {
        std::lock_guard<std::mutex> lock(inputMutex_);

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
        } else {
            inputQueue_.push_back(event);
            inputQueueDepth_.store(static_cast<uint32_t>(inputQueue_.size()));
            inputQueuedCount_.fetch_add(1);
            message = okMessage;
        }
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

void RdpSessionInput::Drain(FreerdpRuntimeApi* api, rdpContext* context,
    const std::function<void(const std::string&)>& log)
{
    std::deque<QueuedInputEvent> pending;
    {
        std::lock_guard<std::mutex> lock(inputMutex_);
        if (inputQueue_.empty()) {
            inputQueueDepth_.store(0);
            return;
        }
        pending.swap(inputQueue_);
        inputQueueDepth_.store(0);
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
            if (api->inputSendKeyboardEventEx != nullptr) {
                ok = api->inputSendKeyboardEventEx(context->input, event.down ? TRUE : FALSE,
                    event.repeat ? TRUE : FALSE, event.scancode);
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
