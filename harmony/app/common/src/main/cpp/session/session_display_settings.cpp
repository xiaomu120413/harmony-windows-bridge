#include "session/session_display_settings.h"

namespace rdp_bridge {
namespace {
bool Near(uint32_t a, uint32_t b) { return (a > b ? a - b : b - a) < 16; }
}

void SessionDisplaySettings::Reset(bool connected)
{
    std::lock_guard<std::mutex> lock(mutex_);
    const auto generation = state_.generation + 1;
    const bool multimon = state_.multimon;
    state_ = {};
    state_.generation = generation;
    state_.connected = connected;
    state_.multimon = multimon;
    targetWidth_ = targetHeight_ = 0;
    resultAccepted_ = presentedSinceBegin_ = false;
}
void SessionDisplaySettings::InvalidatePresentation()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++state_.generation;
    state_.actualWidth = state_.actualHeight = 0;
    state_.pending = false;
    state_.status = "Idle";
    resultAccepted_ = presentedSinceBegin_ = false;
}
void SessionDisplaySettings::SetConnected(bool connected)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_.connected = connected;
}
void SessionDisplaySettings::SetMultimon(bool multimon)
{
    std::lock_guard<std::mutex> lock(mutex_);
    state_.multimon = multimon;
    if (multimon) {
        state_.fixed = state_.pending = false;
        state_.requestedWidth = state_.requestedHeight = 0;
        state_.status = "Multimon";
        ++state_.generation;
        resultAccepted_ = presentedSinceBegin_ = false;
    }
}
bool SessionDisplaySettings::Select(bool fixed, uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (!state_.connected || state_.multimon ||
        (fixed && (width < 200 || height < 200 || width > 4096 || height > 4096))) {
        return false;
    }
    state_.fixed = fixed;
    state_.requestedWidth = fixed ? width : 0;
    state_.requestedHeight = fixed ? height : 0;
    return true;
}
bool SessionDisplaySettings::IsFixed()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.fixed;
}
uint64_t SessionDisplaySettings::Begin(uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    ++state_.generation;
    targetWidth_ = width;
    targetHeight_ = height;
    state_.status = "Applying";
    state_.pending = true;
    resultAccepted_ = presentedSinceBegin_ = false;
    deadline_ = std::chrono::steady_clock::now() + timeout_;
    return state_.generation;
}
void SessionDisplaySettings::Result(uint64_t generation, const DisplayResizeResult& result)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generation != state_.generation) { return; }
    if (result.status == DisplayResizeStatus::Failed || result.status == DisplayResizeStatus::Unsupported) {
        state_.pending = false;
        state_.status = DisplayResizeStatusName(result.status);
        return;
    }
    targetWidth_ = result.sentWidth ? result.sentWidth :
        (result.normalizedWidth ? result.normalizedWidth : targetWidth_);
    targetHeight_ = result.sentHeight ? result.sentHeight :
        (result.normalizedHeight ? result.normalizedHeight : targetHeight_);
    resultAccepted_ = true;
    state_.status = result.status == DisplayResizeStatus::Deferred ? "Deferred" : "WaitingFrame";
    // Unchanged is already displayed only if we have a matching successful presentation.
    if (result.status == DisplayResizeStatus::Unchanged && state_.actualWidth && state_.actualHeight) {
        presentedSinceBegin_ = true;
    }
    ConfirmLocked();
}
void SessionDisplaySettings::ConfirmLocked()
{
    if (resultAccepted_ && presentedSinceBegin_ && Near(targetWidth_, state_.actualWidth) &&
        Near(targetHeight_, state_.actualHeight)) {
        state_.pending = false;
        state_.status = "Applied";
    }
}
uint64_t SessionDisplaySettings::Generation()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return state_.generation;
}
void SessionDisplaySettings::Presented(uint64_t generation, uint32_t width, uint32_t height)
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (generation != state_.generation || !width || !height) { return; }
    state_.actualWidth = width;
    state_.actualHeight = height;
    presentedSinceBegin_ = true;
    ConfirmLocked();
}
void SessionDisplaySettings::ExpireLocked()
{
    if (state_.pending && std::chrono::steady_clock::now() >= deadline_) {
        state_.pending = false;
        state_.status = "Timeout";
    }
}
SessionDisplaySnapshot SessionDisplaySettings::Snapshot()
{
    std::lock_guard<std::mutex> lock(mutex_);
    ExpireLocked();
    return state_;
}
bool SessionDisplaySettings::BlocksPointer() { return Snapshot().pending; }
SessionDisplaySettings& DisplaySettings()
{
    static auto* settings = new SessionDisplaySettings();
    return *settings;
}
} // namespace rdp_bridge
