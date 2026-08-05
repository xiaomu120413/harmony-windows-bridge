#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

#include "input/ohos_keyboard_adapter.h"
#include "surface/surface_bridge.h"

#include "freerdp/freerdp_runtime.h"
#include "client/OHOS/ohos_input_queue.h"
#include "client/OHOS/ohos_pointer.h"

namespace rdp_bridge {

FREERDP_OHOS_POINTER_VIEWPORT BuildOhosPointerViewport(
    const SurfaceSnapshot& surface, uint32_t desktopWidth, uint32_t desktopHeight);

class RdpSessionInput {
public:
    RdpSessionInput();
    ~RdpSessionInput();

    bool EnqueuePointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueLocalPointer(const LocalPointerEvent& pointer, const SurfaceSnapshot& surface,
        uint32_t desktopWidth, uint32_t desktopHeight, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueuePlatformKey(const OhosKeyEvent& event, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueUnicode(uint32_t code, bool down, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueCommittedText(const std::u16string& text, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueFocusIn(uint16_t toggleStates, std::string& message,
        const std::function<void(const std::string&)>& log);
    bool EnqueueReleaseAllKeys(std::string& message,
        const std::function<void(const std::string&)>& log);

    void Clear();
    void Reset();

    uint32_t QueueDepth() const;
    uint32_t QueuedCount() const;
    uint32_t SentCount() const;
    uint32_t DroppedCount() const;

    void Drain(FreerdpRuntimeApi* api, rdpContext* context,
        const std::function<void(const std::string&)>& log);

private:
    bool EnsureQueue(std::string& message, const std::function<void(const std::string&)>& log);
    FREERDP_OHOS_INPUT_QUEUE_DIAGNOSTICS Diagnostics() const;

    FreerdpRuntimeApi* queueApi_ = nullptr;
    freerdpOhosInputQueue* queue_ = nullptr;

    void LogInputFailure(const std::string& message,
        const std::function<void(const std::string&)>& log);

    std::atomic_uint32_t inputFailureLogCount_{0};
};

} // namespace rdp_bridge
