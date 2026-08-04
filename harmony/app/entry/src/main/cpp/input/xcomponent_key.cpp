#include "input/xcomponent_input_internal.h"
#include "input/remote_ime_client.h"
#include "input/remote_pointer_text_detector.h"

namespace rdp_bridge {
namespace {

bool IsModifierPressed(uint64_t modifiers, ArkUI_ModifierKeyName modifier)
{
    return (modifiers & static_cast<uint64_t>(modifier)) != 0;
}

} // namespace

void OnXComponentFocusEvent(OH_NativeXComponent*, void*)
{
    g_xcomponentFocused.store(true);
}

void OnXComponentBlurEvent(OH_NativeXComponent*, void*)
{
    g_xcomponentFocused.store(false);
    ResetRemotePointerTextDetector();
    if (g_remoteIme != nullptr) {
        std::string imeMessage;
        if (!g_remoteIme->Close(imeMessage)) {
            EmitInputLog("XComponent native IME close failed: " + imeMessage);
        }
    }
    g_nativeMouseButtons.store(0);
    {
        std::lock_guard<std::mutex> lock(g_nativeTouchMutex);
        g_nativeTouch = NativeTouchState{};
    }

    std::string message;
    if (g_inputSession != nullptr && !g_inputSession->ReleaseAllKeys(message) &&
        message != "no active FreeRDP session") {
        EmitInputLog("XComponent blurred; release keys skipped: " + message);
    }
}

bool OnXComponentKeyEvent(OH_NativeXComponent* component, void*)
{
    if (component == nullptr) {
        return false;
    }

    OH_NativeXComponent_KeyEvent* keyEvent = nullptr;
    if (OH_NativeXComponent_GetKeyEvent(component, &keyEvent) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        keyEvent == nullptr) {
        EmitInputLog("XComponent native key skipped: key event unavailable");
        return false;
    }

    OH_NativeXComponent_KeyAction action = OH_NATIVEXCOMPONENT_KEY_ACTION_UNKNOWN;
    OH_NativeXComponent_KeyCode keyCode = KEY_UNKNOWN;
    if (OH_NativeXComponent_GetKeyEventAction(keyEvent, &action) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        OH_NativeXComponent_GetKeyEventCode(keyEvent, &keyCode) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitInputLog("XComponent native key skipped: action/code unavailable");
        return false;
    }

    if (action != OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN &&
        action != OH_NATIVEXCOMPONENT_KEY_ACTION_UP) {
        return false;
    }

    uint64_t modifiers = 0;
    OH_NativeXComponent_GetKeyEventModifierKeyStates(keyEvent, &modifiers);
    const OhosKeyEvent input {
        static_cast<uint32_t>(keyCode),
        action == OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN,
        false,
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_CTRL),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_SHIFT),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_ALT),
        false,
    };

    std::string message;
    const bool ok = g_inputSession != nullptr && g_inputSession->SendPlatformKey(input, message);
    if (g_inputSession == nullptr) {
        message = "input bridge not configured";
    }

    static std::atomic_uint32_t keyLogCount{0};
    const uint32_t logIndex = keyLogCount.fetch_add(1);
    const bool importantKey = input.keyCode == KEY_ENTER || input.keyCode == KEY_NUMPAD_ENTER ||
        input.keyCode == KEY_DEL || input.keyCode == KEY_FORWARD_DEL || input.ctrl ||
        input.keyCode == KEY_CTRL_LEFT || input.keyCode == KEY_CTRL_RIGHT;
    if (importantKey || logIndex < 40 || (logIndex % 200) == 0 || !ok) {
        EmitInputLog("XComponent native key: keyCode=" + std::to_string(input.keyCode) +
            (input.down ? " down " : " up ") +
            "mods=" + std::to_string(modifiers) +
            " ctrl=" + std::to_string(input.ctrl ? 1 : 0) +
            " shift=" + std::to_string(input.shift ? 1 : 0) +
            " alt=" + std::to_string(input.alt ? 1 : 0) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
    return ok;
}

} // namespace rdp_bridge
