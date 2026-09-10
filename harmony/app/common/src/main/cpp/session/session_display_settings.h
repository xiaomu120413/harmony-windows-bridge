#pragma once

#include "session/rdp_display_resize_types.h"
#include <chrono>
#include <mutex>

namespace rdp_bridge {

struct SessionDisplaySnapshot {
    bool fixed = false;
    bool connected = false;
    bool multimon = false;
    bool pending = false;
    uint32_t requestedWidth = 0;
    uint32_t requestedHeight = 0;
    uint32_t actualWidth = 0;
    uint32_t actualHeight = 0;
    uint64_t generation = 0;
    std::string status = "Idle";
};

// Pure state policy. Never holds a lock while calling the protocol or renderer.
class SessionDisplaySettings {
public:
    explicit SessionDisplaySettings(std::chrono::milliseconds timeout = std::chrono::seconds(8))
        : timeout_(timeout) {}
    void Reset(bool connected);
    void InvalidatePresentation();
    void SetConnected(bool connected);
    void SetMultimon(bool multimon);
    bool Select(bool fixed, uint32_t width, uint32_t height);
    bool IsFixed();
    uint64_t Begin(uint32_t width, uint32_t height);
    void Result(uint64_t generation, const DisplayResizeResult& result);
    uint64_t Generation();
    void Presented(uint64_t generation, uint32_t width, uint32_t height);
    SessionDisplaySnapshot Snapshot();
    bool BlocksPointer();
private:
    void ExpireLocked();
    void ConfirmLocked();
    std::mutex mutex_;
    SessionDisplaySnapshot state_;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    bool resultAccepted_ = false;
    bool presentedSinceBegin_ = false;
    std::chrono::steady_clock::time_point deadline_;
    const std::chrono::milliseconds timeout_;
};

SessionDisplaySettings& DisplaySettings();

} // namespace rdp_bridge
