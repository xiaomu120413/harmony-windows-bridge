#pragma once

#include <atomic>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <string>

#include "input/ohos_keyboard_adapter.h"
#include "client/OHOS/ohos_ime.h"
#include "client/OHOS/ohos_keyboard.h"

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include "freerdp_runtime.h"
#endif

namespace rdp_bridge {

class RdpSessionInput {
public:
    RdpSessionInput();
    ~RdpSessionInput();

    bool EnqueuePointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueuePlatformKey(const OhosKeyEvent& event, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueUnicode(uint32_t code, bool down, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueCommittedText(const std::u16string& text, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueReleaseAllKeys(std::string& message, const std::function<void(const std::string&)>& log);

    void Clear();
    void Reset();

    uint32_t QueueDepth() const;
    uint32_t QueuedCount() const;
    uint32_t SentCount() const;
    uint32_t DroppedCount() const;

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    void Drain(FreerdpRuntimeApi* api, rdpContext* context,
        const std::function<void(const std::string&)>& log);
#endif

private:
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    enum class QueuedInputType {
        Pointer,
        Key,
        PlatformKey,
        PlatformKeyPacket,
        Unicode,
    };

    struct QueuedInputEvent {
        QueuedInputType type = QueuedInputType::Pointer;
        uint16_t flags = 0;
        uint16_t x = 0;
        uint16_t y = 0;
        uint32_t scancode = 0;
        uint32_t keyCode = 0;
        uint32_t vk = 0;
        uint32_t code = 0;
        bool ctrl = false;
        bool shift = false;
        bool alt = false;
        bool meta = false;
        bool down = false;
        bool repeat = false;
        bool extended = false;
        bool synthetic = false;
    };

    const char* InputTypeName(const QueuedInputEvent& event) const;
    bool IsPointerWheelEvent(const QueuedInputEvent& event) const;
    bool IsPointerMotionEvent(const QueuedInputEvent& event) const;
    bool HasSamePointerMotionClass(const QueuedInputEvent& lhs, const QueuedInputEvent& rhs) const;
    bool IsDroppablePointerEvent(const QueuedInputEvent& event) const;
    bool DropOldestDroppablePointerEventLocked();
    void AppendPlatformKeyPacketLocked(const FREERDP_OHOS_KEY_PACKET& packet,
        std::deque<QueuedInputEvent>& pending);
    bool AppendPlatformKeyPacketsLocked(const QueuedInputEvent& event,
        std::deque<QueuedInputEvent>& pending, const std::function<void(const std::string&)>& log);
    void AppendDueRepeatPacketsLocked(std::deque<QueuedInputEvent>& pending,
        const std::function<void(const std::string&)>& log);
    bool EnqueueInput(const QueuedInputEvent& event, const char* okMessage, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueInputLocked(const QueuedInputEvent& event, const char* okMessage, std::string& message,
        bool& droppedOldPointer, bool& droppedNewEvent);
    void LogInputFailure(const std::string& message, const std::function<void(const std::string&)>& log);
    void LogInputBackpressure(const std::string& message, const std::function<void(const std::string&)>& log);
    void LogKeyDispatch(const QueuedInputEvent& event, uint16_t flags, bool ok,
        const std::function<void(const std::string&)>& log);
    void LogPlatformKeyDispatch(const QueuedInputEvent& event, uint32_t scancode, bool ok,
        const std::function<void(const std::string&)>& log);

    std::mutex inputMutex_;
    std::deque<QueuedInputEvent> inputQueue_;
    FREERDP_OHOS_KEYBOARD_STATE* keyboardState_ = nullptr;
#endif

    std::atomic_uint32_t inputQueueDepth_{0};
    std::atomic_uint32_t inputQueuedCount_{0};
    std::atomic_uint32_t inputSentCount_{0};
    std::atomic_uint32_t inputDroppedCount_{0};
    std::atomic_uint32_t inputDispatchLogCount_{0};
    std::atomic_uint32_t inputKeyDispatchLogCount_{0};
    std::atomic_uint32_t inputFailureLogCount_{0};
    std::atomic_uint32_t inputBackpressureLogCount_{0};
};

} // namespace rdp_bridge
