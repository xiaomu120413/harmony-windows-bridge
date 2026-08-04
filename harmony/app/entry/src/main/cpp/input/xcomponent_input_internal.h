#pragma once

#include "input/ohos_keyboard_adapter.h"
#include "input/xcomponent_input_bridge.h"

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <mutex>
#include <string>

#include <arkui/ui_input_event.h>
#include <ace/xcomponent/native_xcomponent_key_event.h>

namespace rdp_bridge {

constexpr uint64_t kAxisEventDedupMs = 12;

struct NativeTouchState {
    bool singleActive = false;
    bool leftDragActive = false;
    bool longPressSent = false;
    bool scrollActive = false;
    uint64_t downAtMs = 0;
    float startX = 0.0f;
    float startY = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool lastTapValid = false;
    uint64_t lastTapAtMs = 0;
    float lastTapX = 0.0f;
    float lastTapY = 0.0f;
    float scrollLastX = 0.0f;
    float scrollLastY = 0.0f;
};

struct NativeAxisState {
    uint64_t lastAtMs = 0;
    int32_t lastAction = -1;
    float lastX = 0.0f;
    float lastY = 0.0f;
    double lastDeltaX = 0.0;
    double lastDeltaY = 0.0;
};

extern RdpSession* g_inputSession;
extern RemoteImeClient* g_remoteIme;
extern std::function<void(const std::string&)> g_inputLog;
extern std::atomic_uint32_t g_nativeMouseButtons;
extern std::atomic_bool g_xcomponentFocused;
extern std::mutex g_nativeTouchMutex;
extern NativeTouchState g_nativeTouch;
extern std::mutex g_nativeAxisMutex;
extern NativeAxisState g_nativeAxis;

void EmitInputLog(const std::string& line);
uint32_t RoundSurfaceCoordinate(float value);
uint64_t NowMs();
const char* LocalPointerActionName(LocalPointerAction action);
LocalPointerEvent MakeNativePointer(LocalPointerAction action, float x, float y,
    uint32_t buttons = LocalPointerButtonNone, int32_t delta = 0);
bool SendNativePointer(const LocalPointerEvent& event, const std::string& label,
    bool forceLog = false);

void OnXComponentMouseEvent(OH_NativeXComponent* component, void* window);
void OnXComponentHoverEvent(OH_NativeXComponent* component, bool hover);
void OnXComponentAxisEvent(OH_NativeXComponent* component, ArkUI_UIInputEvent* event,
    ArkUI_UIInputEvent_Type type);
void OnXComponentFocusEvent(OH_NativeXComponent* component, void* window);
void OnXComponentBlurEvent(OH_NativeXComponent* component, void* window);
bool OnXComponentKeyEvent(OH_NativeXComponent* component, void* window);

} // namespace rdp_bridge
