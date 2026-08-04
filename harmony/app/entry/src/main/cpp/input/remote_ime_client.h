#pragma once

#include <cstddef>
#include <cstdint>
#include <atomic>
#include <mutex>
#include <string>

struct InputMethod_InputMethodProxy;
struct InputMethod_TextConfig;
struct InputMethod_TextEditorProxy;

namespace rdp_bridge {

class RdpSession;

class RemoteImeClient {
public:
    RemoteImeClient();
    ~RemoteImeClient();

    RemoteImeClient(const RemoteImeClient&) = delete;
    RemoteImeClient& operator=(const RemoteImeClient&) = delete;

    void Configure(RdpSession* session, uint32_t windowId);
    bool Open(std::string& message);
    bool Close(std::string& message);
    bool IsOpen();
    bool IsKeyboardVisible() const;
    void UpdateKeyboardVisibility(bool visible);

    void FillTextConfig(InputMethod_TextConfig* config);
    void InsertText(const char16_t* text, size_t length);
    void DeleteText(uint32_t keyCode, int32_t length);
    void SendEnter();
    void MoveCursor(int32_t direction);
    void SetPreviewText(const char16_t* text, size_t length);
    void FinishPreviewText();

private:
    bool EnsureEditorProxy(std::string& message);
    bool SendKeyPress(uint32_t keyCode, std::string& message);

    std::mutex mutex_;
    std::atomic<RdpSession*> session_{nullptr};
    std::atomic_uint32_t windowId_{0};
    std::atomic_bool keyboardVisible_{false};
    InputMethod_TextEditorProxy* editorProxy_ = nullptr;
    InputMethod_InputMethodProxy* inputMethodProxy_ = nullptr;
    std::mutex previewMutex_;
    std::u16string previewText_;
};

} // namespace rdp_bridge
