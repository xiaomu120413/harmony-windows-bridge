#pragma once

#include <cstdint>
#include <chrono>
#include <functional>
#include <mutex>
#include <string>

#include <database/pasteboard/oh_pasteboard.h>

namespace rdp_bridge {

class ClipboardPasteboard {
public:
    using LogFn = std::function<void(const std::string&)>;
    using ChangeFn = std::function<void()>;

    ~ClipboardPasteboard();

    bool Initialize(const LogFn& log, const ChangeFn& onLocalChange);
    void Uninitialize();

    bool ReadPlainText(std::string& text, std::string& error);
    bool WritePlainText(const std::string& text, std::string& error);

private:
    static void OnPasteboardChanged(void* context, Pasteboard_NotifyType type);
    static void OnPasteboardFinalize(void*);

    void HandlePasteboardChanged(Pasteboard_NotifyType type);
    void Log(const std::string& line);

    std::mutex mutex_;
    LogFn log_;
    ChangeFn onLocalChange_;
    OH_Pasteboard* pasteboard_ = nullptr;
    OH_PasteboardObserver* observer_ = nullptr;
    bool subscribed_ = false;
    uint32_t ignoreLocalChanges_ = 0;
    std::chrono::steady_clock::time_point ignoreLocalChangesUntil_{};
};

} // namespace rdp_bridge
