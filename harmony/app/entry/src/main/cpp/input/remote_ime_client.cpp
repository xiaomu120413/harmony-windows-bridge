#include "input/remote_ime_client.h"

#include "common/bridge_log.h"
#include "input/ohos_keyboard_adapter.h"
#include "session/rdp_session_core.h"

#include <algorithm>
#include <atomic>

#include <ace/xcomponent/native_xcomponent_key_event.h>
#include <inputmethod/inputmethod_attach_options_capi.h>
#include <inputmethod/inputmethod_controller_capi.h>
#include <inputmethod/inputmethod_inputmethod_proxy_capi.h>
#include <inputmethod/inputmethod_text_config_capi.h>
#include <inputmethod/inputmethod_text_editor_proxy_capi.h>

namespace rdp_bridge {
namespace {

std::atomic<RemoteImeClient*> g_remoteImeClient{nullptr};

RemoteImeClient* ActiveClient()
{
    return g_remoteImeClient.load();
}

void OnGetTextConfig(InputMethod_TextEditorProxy*, InputMethod_TextConfig* config)
{
    RemoteImeClient* client = ActiveClient();
    if (client == nullptr || config == nullptr) {
        return;
    }
    client->FillTextConfig(config);
}

void OnInsertText(InputMethod_TextEditorProxy*, const char16_t* text, size_t length)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->InsertText(text, length);
    }
}

void OnDeleteForward(InputMethod_TextEditorProxy*, int32_t length)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->DeleteText(KEY_FORWARD_DEL, length);
    }
}

void OnDeleteBackward(InputMethod_TextEditorProxy*, int32_t length)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->DeleteText(KEY_DEL, length);
    }
}

void OnSendKeyboardStatus(InputMethod_TextEditorProxy*, InputMethod_KeyboardStatus status)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->UpdateKeyboardVisibility(status == IME_KEYBOARD_STATUS_SHOW);
    }
    BridgeLogger::Debug("Native remote IME keyboard status=" +
        std::to_string(static_cast<int32_t>(status)));
}

void OnSendEnter(InputMethod_TextEditorProxy*, InputMethod_EnterKeyType)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->SendEnter();
    }
}

void OnMoveCursor(InputMethod_TextEditorProxy*, InputMethod_Direction direction)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->MoveCursor(static_cast<int32_t>(direction));
    }
}

void HandleSetSelection(InputMethod_TextEditorProxy*, int32_t, int32_t)
{
}

void HandleExtendAction(InputMethod_TextEditorProxy*, InputMethod_ExtendAction)
{
}

void GetEmptyText(InputMethod_TextEditorProxy*, int32_t, char16_t[], size_t* length)
{
    if (length != nullptr) {
        *length = 0;
    }
}

int32_t GetTextIndexAtCursor(InputMethod_TextEditorProxy*)
{
    return 0;
}

int32_t ReceivePrivateCommand(InputMethod_TextEditorProxy*, InputMethod_PrivateCommand*[], size_t)
{
    return 0;
}

int32_t OnSetPreviewText(InputMethod_TextEditorProxy*, const char16_t text[], size_t length,
    int32_t, int32_t)
{
    RemoteImeClient* client = ActiveClient();
    if (client == nullptr) {
        return -1;
    }
    client->SetPreviewText(text, length);
    return 0;
}

void OnFinishTextPreview(InputMethod_TextEditorProxy*)
{
    if (RemoteImeClient* client = ActiveClient(); client != nullptr) {
        client->FinishPreviewText();
    }
}

} // namespace

RemoteImeClient::RemoteImeClient()
{
    g_remoteImeClient.store(this);
}

RemoteImeClient::~RemoteImeClient()
{
    std::string message;
    (void)Close(message);
    if (editorProxy_ != nullptr) {
        OH_TextEditorProxy_Destroy(editorProxy_);
        editorProxy_ = nullptr;
    }
    if (g_remoteImeClient.load() == this) {
        g_remoteImeClient.store(nullptr);
    }
}

void RemoteImeClient::Configure(RdpSession* session, uint32_t windowId)
{
    session_.store(session);
    windowId_.store(windowId);
}

void RemoteImeClient::FillTextConfig(InputMethod_TextConfig* config)
{
    if (config == nullptr) {
        return;
    }
    (void)OH_TextConfig_SetInputType(config, IME_TEXT_INPUT_TYPE_TEXT);
    (void)OH_TextConfig_SetEnterKeyType(config, IME_ENTER_KEY_DONE);
    (void)OH_TextConfig_SetPreviewTextSupport(config, true);
    (void)OH_TextConfig_SetSelection(config, 0, 0);
    (void)OH_TextConfig_SetWindowId(config, static_cast<int32_t>(windowId_.load()));
}

bool RemoteImeClient::EnsureEditorProxy(std::string& message)
{
    if (editorProxy_ != nullptr) {
        return true;
    }
    editorProxy_ = OH_TextEditorProxy_Create();
    if (editorProxy_ == nullptr) {
        message = "native remote IME editor proxy creation failed";
        return false;
    }

    const InputMethod_ErrorCode results[] = {
        OH_TextEditorProxy_SetGetTextConfigFunc(editorProxy_, OnGetTextConfig),
        OH_TextEditorProxy_SetInsertTextFunc(editorProxy_, OnInsertText),
        OH_TextEditorProxy_SetDeleteForwardFunc(editorProxy_, OnDeleteForward),
        OH_TextEditorProxy_SetDeleteBackwardFunc(editorProxy_, OnDeleteBackward),
        OH_TextEditorProxy_SetSendKeyboardStatusFunc(editorProxy_, OnSendKeyboardStatus),
        OH_TextEditorProxy_SetSendEnterKeyFunc(editorProxy_, OnSendEnter),
        OH_TextEditorProxy_SetMoveCursorFunc(editorProxy_, OnMoveCursor),
        OH_TextEditorProxy_SetHandleSetSelectionFunc(editorProxy_, HandleSetSelection),
        OH_TextEditorProxy_SetHandleExtendActionFunc(editorProxy_, HandleExtendAction),
        OH_TextEditorProxy_SetGetLeftTextOfCursorFunc(editorProxy_, GetEmptyText),
        OH_TextEditorProxy_SetGetRightTextOfCursorFunc(editorProxy_, GetEmptyText),
        OH_TextEditorProxy_SetGetTextIndexAtCursorFunc(editorProxy_, GetTextIndexAtCursor),
        OH_TextEditorProxy_SetReceivePrivateCommandFunc(editorProxy_, ReceivePrivateCommand),
        OH_TextEditorProxy_SetSetPreviewTextFunc(editorProxy_, OnSetPreviewText),
        OH_TextEditorProxy_SetFinishTextPreviewFunc(editorProxy_, OnFinishTextPreview),
    };
    for (const InputMethod_ErrorCode result : results) {
        if (result != IME_ERR_OK) {
            message = "native remote IME callback registration failed: rc=" +
                std::to_string(static_cast<int32_t>(result));
            OH_TextEditorProxy_Destroy(editorProxy_);
            editorProxy_ = nullptr;
            return false;
        }
    }
    return true;
}

bool RemoteImeClient::Open(std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    RdpSession* session = session_.load();
    if (session == nullptr || windowId_.load() == 0) {
        message = "native remote IME host window is not configured";
        return false;
    }
    if (!session->IsConnected()) {
        message = "native remote IME requires an active RDP session";
        return false;
    }
    std::string releaseMessage;
    (void)session->ReleaseAllKeys(releaseMessage);
    if (inputMethodProxy_ != nullptr) {
        const InputMethod_ErrorCode showResult = OH_InputMethodProxy_ShowKeyboard(inputMethodProxy_);
        if (showResult == IME_ERR_OK) {
            keyboardVisible_.store(true);
        }
        message = "native remote IME show rc=" + std::to_string(static_cast<int32_t>(showResult));
        return showResult == IME_ERR_OK;
    }
    if (!EnsureEditorProxy(message)) {
        return false;
    }

    InputMethod_AttachOptions* options = OH_AttachOptions_Create(true);
    if (options == nullptr) {
        message = "native remote IME attach options creation failed";
        return false;
    }
    const InputMethod_ErrorCode attachResult =
        OH_InputMethodController_Attach(editorProxy_, options, &inputMethodProxy_);
    OH_AttachOptions_Destroy(options);
    if (attachResult == IME_ERR_OK) {
        keyboardVisible_.store(true);
    } else {
        inputMethodProxy_ = nullptr;
    }
    message = "native remote IME attach rc=" + std::to_string(static_cast<int32_t>(attachResult));
    return attachResult == IME_ERR_OK;
}

bool RemoteImeClient::Close(std::string& message)
{
    std::lock_guard<std::mutex> lock(mutex_);
    FinishPreviewText();
    if (inputMethodProxy_ == nullptr) {
        keyboardVisible_.store(false);
        message = "native remote IME already closed";
        return true;
    }
    const InputMethod_ErrorCode hideResult = OH_InputMethodProxy_HideKeyboard(inputMethodProxy_);
    const InputMethod_ErrorCode detachResult = OH_InputMethodController_Detach(inputMethodProxy_);
    inputMethodProxy_ = nullptr;
    keyboardVisible_.store(false);
    message = "native remote IME close hideRc=" + std::to_string(static_cast<int32_t>(hideResult)) +
        " detachRc=" + std::to_string(static_cast<int32_t>(detachResult));
    return hideResult == IME_ERR_OK && detachResult == IME_ERR_OK;
}

bool RemoteImeClient::IsOpen()
{
    std::lock_guard<std::mutex> lock(mutex_);
    return inputMethodProxy_ != nullptr;
}

bool RemoteImeClient::IsKeyboardVisible() const
{
    return keyboardVisible_.load();
}

void RemoteImeClient::UpdateKeyboardVisibility(bool visible)
{
    keyboardVisible_.store(visible);
}

bool RemoteImeClient::SendKeyPress(uint32_t keyCode, std::string& message)
{
    RdpSession* session = session_.load();
    if (session == nullptr) {
        message = "native remote IME session unavailable";
        return false;
    }
    OhosKeyEvent down {keyCode, true, false, false, false, false, false};
    if (!session->SendPlatformKey(down, message)) {
        return false;
    }
    OhosKeyEvent up {keyCode, false, false, false, false, false, false};
    return session->SendPlatformKey(up, message);
}

void RemoteImeClient::InsertText(const char16_t* text, size_t length)
{
    RdpSession* session = session_.load();
    if (session == nullptr || text == nullptr || length == 0) {
        return;
    }
    std::string message;
    if (!session->SendCommittedText(std::u16string(text, length), message)) {
        BridgeLogger::Error("Native remote IME committed text failed: " + message);
    }
    FinishPreviewText();
}

void RemoteImeClient::DeleteText(uint32_t keyCode, int32_t length)
{
    const int32_t count = std::clamp(length, 1, 64);
    for (int32_t index = 0; index < count; ++index) {
        std::string message;
        if (!SendKeyPress(keyCode, message)) {
            BridgeLogger::Error("Native remote IME delete failed: " + message);
            return;
        }
    }
}

void RemoteImeClient::SendEnter()
{
    std::string message;
    if (!SendKeyPress(KEY_ENTER, message)) {
        BridgeLogger::Error("Native remote IME enter failed: " + message);
    }
}

void RemoteImeClient::MoveCursor(int32_t direction)
{
    uint32_t keyCode = 0;
    switch (direction) {
        case IME_DIRECTION_UP:
            keyCode = KEY_DPAD_UP;
            break;
        case IME_DIRECTION_DOWN:
            keyCode = KEY_DPAD_DOWN;
            break;
        case IME_DIRECTION_LEFT:
            keyCode = KEY_DPAD_LEFT;
            break;
        case IME_DIRECTION_RIGHT:
            keyCode = KEY_DPAD_RIGHT;
            break;
        default:
            return;
    }
    std::string message;
    if (!SendKeyPress(keyCode, message)) {
        BridgeLogger::Error("Native remote IME cursor move failed: " + message);
    }
}

void RemoteImeClient::SetPreviewText(const char16_t* text, size_t length)
{
    std::lock_guard<std::mutex> lock(previewMutex_);
    if (text == nullptr) {
        previewText_.clear();
        return;
    }
    previewText_.assign(text, length);
}

void RemoteImeClient::FinishPreviewText()
{
    std::lock_guard<std::mutex> lock(previewMutex_);
    previewText_.clear();
}

} // namespace rdp_bridge
