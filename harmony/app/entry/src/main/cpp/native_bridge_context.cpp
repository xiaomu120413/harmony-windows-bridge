#include "native_bridge_context.h"

#include "bridge_log.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp_runtime.h"
#include "string_utils.h"
#include "surface/latest_frame_renderer.h"

#include <atomic>
#include <cmath>
#include <cstdint>
#include <limits>
#include <mutex>
#include <string>

#include <arkui/ui_input_event.h>
#include <ace/xcomponent/native_interface_xcomponent.h>
#include <ace/xcomponent/native_xcomponent_key_event.h>

namespace rdp_bridge {
namespace {

std::atomic_uint64_t g_avc444SurfaceFrameCallbackCount{0};
std::atomic_uint32_t g_nativeMouseButtons{0};
SessionEventHub g_events;
SurfaceBridge g_surface;
LatestFrameRenderer g_frameRenderer;
RdpSession g_session;

void EmitNativeLog(const std::string& line)
{
    g_events.log.Emit(line);
}

class ResizeCoordinator {
public:
    void Reset(const std::string& reason)
    {
        bool cleared = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (pending_) {
                pending_ = false;
                skippedFrameCount_ = 0;
                cleared = true;
            }
        }
        if (cleared) {
            EmitNativeLog("resize target cleared after " + reason);
        }
    }

    void Begin(uint32_t width, uint32_t height, const std::string& reason)
    {
        if (width == 0 || height == 0) {
            return;
        }

        uint64_t generation = 0;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            pending_ = true;
            targetWidth_ = width;
            targetHeight_ = height;
            skippedFrameCount_ = 0;
            generation = ++generation_;
        }
        EmitNativeLog("resize target pending after " + reason + ": target=" +
            std::to_string(width) + "x" + std::to_string(height) +
            " generation=" + std::to_string(generation));
    }

    bool ShouldQueueFrame(const RgbaFrame& frame, std::string& message)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!pending_) {
            return true;
        }

        if (FrameMatchesTarget(frame.width, frame.height)) {
            pending_ = false;
            skippedFrameCount_ = 0;
            message = "resize target accepted by frame: target=" +
                std::to_string(targetWidth_) + "x" + std::to_string(targetHeight_) +
                " frame=" + std::to_string(frame.width) + "x" +
                std::to_string(frame.height) + " label=" + FrameLabel(frame);
            return true;
        }

        ++skippedFrameCount_;
        message = "resize pending target=" + std::to_string(targetWidth_) + "x" +
            std::to_string(targetHeight_) + " skipped frame=" +
            std::to_string(frame.width) + "x" + std::to_string(frame.height) +
            " label=" + FrameLabel(frame) + " skipped=" +
            std::to_string(skippedFrameCount_);
        return false;
    }

private:
    static constexpr uint32_t kFrameAlignmentTolerance = 16;

    static std::string FrameLabel(const RgbaFrame& frame)
    {
        return frame.label.empty() ? "frame" : frame.label;
    }

    bool FrameMatchesTarget(uint32_t width, uint32_t height) const
    {
        if (width == 0 || height == 0) {
            return false;
        }
        if (width == targetWidth_ && height == targetHeight_) {
            return true;
        }

        const uint32_t widthDelta = width > targetWidth_ ? width - targetWidth_ : targetWidth_ - width;
        const uint32_t heightDelta = height > targetHeight_ ? height - targetHeight_ : targetHeight_ - height;
        return widthDelta < kFrameAlignmentTolerance && heightDelta < kFrameAlignmentTolerance;
    }

    std::mutex mutex_;
    bool pending_ = false;
    uint32_t targetWidth_ = 0;
    uint32_t targetHeight_ = 0;
    uint64_t generation_ = 0;
    uint32_t skippedFrameCount_ = 0;
};

ResizeCoordinator g_resizeCoordinator;

SurfacePaintResult RenderSurfaceRgbaFrame(const RgbaFrame& frame)
{
    return g_surface.RenderRgbaFrame(frame);
}

bool QueueSurfaceRgbaFrame(const RgbaFrame& frame, std::string& message, bool forceRender)
{
    std::string resizeMessage;
    if (!g_resizeCoordinator.ShouldQueueFrame(frame, resizeMessage)) {
        message = resizeMessage;
        return false;
    }
    if (!resizeMessage.empty()) {
        EmitNativeLog(resizeMessage);
    }
    return g_frameRenderer.Enqueue(frame, message, forceRender);
}

void DropPendingRenderFrame(const std::string& reason)
{
    std::string message;
    if (g_frameRenderer.DropPending(reason, message)) {
        EmitNativeLog(message);
    }
}

void StartRenderPipeline()
{
    g_frameRenderer.SetCallbacks(RenderSurfaceRgbaFrame, EmitNativeLog);
    g_frameRenderer.Start();
}

void StopRenderPipeline()
{
    g_frameRenderer.Stop();
}

DecoderSurfaceTarget SnapshotDecoderSurfaceTarget()
{
    return g_surface.DecoderSurface();
}

bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
    std::string& error)
{
    return g_surface.EnsureAvc444SurfaceTargets(width, height, targets, error);
}

void OnAvc444SurfaceFrameDecoded(uint32_t surfaceId, uint32_t width, uint32_t height,
    uint32_t op, uint32_t codecId, void*)
{
    const uint64_t count = ++g_avc444SurfaceFrameCallbackCount;
    if (count <= 3 || (count % 120) == 0) {
        EmitNativeLog("OHOS AVC444 surface frame callback: count=" + std::to_string(count) +
            " surfaceId=" + std::to_string(surfaceId) +
            " size=" + std::to_string(width) + "x" + std::to_string(height) +
            " op=" + std::to_string(op) +
            " codec=" + Hex32(codecId));
    }
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool RegisterAvc444DecodeSurfaces(FreerdpRuntimeApi& api, uint32_t width, uint32_t height,
    const FreerdpLogFn& log)
{
    if (api.ohosAvcodecSetAvc444OutputSurfaces == nullptr) {
        log("OHOS AVC444 NativeImage surface registration skipped: FreeRDP symbol unavailable");
        return false;
    }

    Avc444SurfaceTargets targets;
    std::string error;
    if (!EnsureAvc444SurfaceTargets(width, height, targets, error)) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        log("OHOS AVC444 NativeImage surface registration failed: " + error);
        return false;
    }

    api.ohosAvcodecSetAvc444OutputSurfaces(
        targets.lumaWindow, targets.chromaWindow, targets.width, targets.height, TRUE);
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(OnAvc444SurfaceFrameDecoded, nullptr);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    log("OHOS AVC444 NativeImage decode surfaces registered: " +
        std::to_string(targets.width) + "x" + std::to_string(targets.height) +
        " lumaTex=" + std::to_string(targets.lumaTexture) +
        " chromaTex=" + std::to_string(targets.chromaTexture) +
        " lumaSurface=" + std::to_string(targets.lumaSurfaceId) +
        " chromaSurface=" + std::to_string(targets.chromaSurfaceId) +
        " route=disabled-until-compositor");
    return true;
}
#endif

void RequestSurfaceRepaint(const std::string& reason)
{
    static std::atomic_uint32_t repaintLogCount{0};
    static std::atomic_uint32_t repaintSkipLogCount{0};
    std::string message;
    if (g_session.RequestCurrentFrameRender(reason, message)) {
        const uint32_t count = ++repaintLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog("Surface repaint queued after " + reason + ": " + message +
                " count=" + std::to_string(count));
        }
        return;
    }

    const uint32_t skipCount = ++repaintSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("Surface repaint skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

void RequestRemoteDesktopResize(uint32_t width, uint32_t height, const std::string& reason)
{
    static std::atomic_uint32_t resizeLogCount{0};
    static std::atomic_uint32_t resizeSkipLogCount{0};
    std::string message;
    if (g_session.RequestDynamicDesktopResize(width, height, reason, message)) {
        const uint32_t count = ++resizeLogCount;
        if (count <= 3 || count % 30 == 0) {
            EmitNativeLog(message + " count=" + std::to_string(count));
        }
        return;
    }

    const uint32_t skipCount = ++resizeSkipLogCount;
    if (skipCount <= 3 || skipCount % 30 == 0) {
        EmitNativeLog("display-control resize skipped after " + reason + ": " + message +
            " count=" + std::to_string(skipCount));
    }
}

void ConfigureRdpgfxPipelineCallbacks()
{
    SetRdpgfxPipelineCallbacks({
        SnapshotDecoderSurfaceTarget,
#if defined(HARMONY_HAS_FREERDP_HEADERS)
        RegisterAvc444DecodeSurfaces,
#endif
        StartRenderPipeline,
        StopRenderPipeline,
        EmitNativeLog,
    });
}

void ConfigureRdpSessionCallbacks()
{
    g_session.SetCallbacks({
        [](const std::string& state) {
            g_events.state.Emit(state);
        },
        [](const std::string& line) {
            g_events.log.Emit(line);
        },
        [](const std::string& message) {
            g_events.error.Emit(message);
        },
        []() {
            return g_surface.Snapshot();
        },
        QueueSurfaceRgbaFrame,
        StartRenderPipeline,
        StopRenderPipeline,
        RequestSurfaceRepaint,
    });
}

void OnXComponentSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceCreated(component, window);
    g_resizeCoordinator.Reset("surface created");
    UpdateAvc420SurfaceOutputIfActive("surface created");
    RequestSurfaceRepaint("surface created");
}

void OnXComponentSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceChanged(component, window);
    UpdateAvc420SurfaceOutputIfActive("surface changed");
    const SurfaceSnapshot snapshot = g_surface.Snapshot();
    g_resizeCoordinator.Begin(snapshot.width, snapshot.height, "surface changed");
    DropPendingRenderFrame("surface changed");
    RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface changed");
}

void OnXComponentSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    g_surface.OnSurfaceDestroyed(component, window);
    g_resizeCoordinator.Reset("surface destroyed");
    UpdateAvc420SurfaceOutputIfActive("surface destroyed");
}

void OnXComponentTouchEvent(OH_NativeXComponent*, void*)
{
    g_surface.OnTouchEvent();
}

uint32_t RoundSurfaceCoordinate(float value)
{
    if (value <= 0.0f) {
        return 0;
    }
    const float maxValue = static_cast<float>(std::numeric_limits<uint32_t>::max());
    if (value >= maxValue) {
        return std::numeric_limits<uint32_t>::max();
    }
    return static_cast<uint32_t>(std::lround(value));
}

uint32_t NativeMouseButtonMask(OH_NativeXComponent_MouseEventButton button)
{
    switch (button) {
        case OH_NATIVEXCOMPONENT_LEFT_BUTTON:
            return LocalPointerButtonLeft;
        case OH_NATIVEXCOMPONENT_RIGHT_BUTTON:
            return LocalPointerButtonRight;
        case OH_NATIVEXCOMPONENT_MIDDLE_BUTTON:
            return LocalPointerButtonMiddle;
        default:
            return LocalPointerButtonNone;
    }
}

const char* NativeMouseActionName(OH_NativeXComponent_MouseEventAction action)
{
    switch (action) {
        case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
            return "press";
        case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
            return "release";
        case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
            return "move";
        default:
            return "none";
    }
}

void DispatchNativeMousePointer(const OH_NativeXComponent_MouseEvent& mouseEvent)
{
    LocalPointerEvent event;
    const uint32_t buttonMask = NativeMouseButtonMask(mouseEvent.button);
    const uint32_t currentButtons = g_nativeMouseButtons.load();

    event.x = RoundSurfaceCoordinate(mouseEvent.x);
    event.y = RoundSurfaceCoordinate(mouseEvent.y);
    event.allowClamp = true;

    switch (mouseEvent.action) {
        case OH_NATIVEXCOMPONENT_MOUSE_PRESS:
            if (buttonMask == LocalPointerButtonNone) {
                return;
            }
            event.action = LocalPointerAction::ButtonDown;
            event.buttons = buttonMask;
            g_nativeMouseButtons.fetch_or(buttonMask);
            break;
        case OH_NATIVEXCOMPONENT_MOUSE_RELEASE:
            event.action = LocalPointerAction::ButtonUp;
            event.buttons = buttonMask != LocalPointerButtonNone ? buttonMask : currentButtons;
            if (event.buttons == LocalPointerButtonNone) {
                return;
            }
            g_nativeMouseButtons.fetch_and(~event.buttons);
            break;
        case OH_NATIVEXCOMPONENT_MOUSE_MOVE:
            event.action = LocalPointerAction::Move;
            event.buttons = currentButtons;
            break;
        default:
            return;
    }

    std::string message;
    const bool ok = g_session.SendLocalPointer(event, message);
    static std::atomic_uint32_t mouseLogCount{0};
    const uint32_t logIndex = mouseLogCount.fetch_add(1);
    if (!ok || logIndex < 60 || (logIndex % 200) == 0 ||
        mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_PRESS ||
        mouseEvent.action == OH_NATIVEXCOMPONENT_MOUSE_RELEASE) {
        EmitNativeLog("XComponent native mouse: action=" +
            std::string(NativeMouseActionName(mouseEvent.action)) +
            " button=" + std::to_string(static_cast<int32_t>(mouseEvent.button)) +
            " buttons=" + std::to_string(event.buttons) +
            " x=" + std::to_string(event.x) +
            " y=" + std::to_string(event.y) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
}

void OnXComponentMouseEvent(OH_NativeXComponent* component, void* window)
{
    if (component == nullptr) {
        return;
    }

    OH_NativeXComponent_MouseEvent mouseEvent{};
    const int32_t rc = OH_NativeXComponent_GetMouseEvent(component, window, &mouseEvent);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitNativeLog("XComponent native mouse skipped: get mouse event failed rc=" +
            std::to_string(rc));
        return;
    }

    DispatchNativeMousePointer(mouseEvent);
}

void OnXComponentHoverEvent(OH_NativeXComponent*, bool)
{
}

void OnXComponentFocusEvent(OH_NativeXComponent*, void*)
{
    EmitNativeLog("XComponent focused for native input");
}

void OnXComponentBlurEvent(OH_NativeXComponent*, void*)
{
    std::string message;
    if (g_session.ReleaseAllKeys(message)) {
        EmitNativeLog("XComponent blurred; " + message);
    } else {
        EmitNativeLog("XComponent blurred; release keys skipped: " + message);
    }
}

bool IsModifierPressed(uint64_t modifiers, ArkUI_ModifierKeyName modifier)
{
    return (modifiers & static_cast<uint64_t>(modifier)) != 0;
}

bool OnXComponentKeyEvent(OH_NativeXComponent* component, void*)
{
    if (component == nullptr) {
        return false;
    }

    OH_NativeXComponent_KeyEvent* keyEvent = nullptr;
    if (OH_NativeXComponent_GetKeyEvent(component, &keyEvent) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        keyEvent == nullptr) {
        EmitNativeLog("XComponent native key skipped: key event unavailable");
        return false;
    }

    OH_NativeXComponent_KeyAction action = OH_NATIVEXCOMPONENT_KEY_ACTION_UNKNOWN;
    OH_NativeXComponent_KeyCode keyCode = KEY_UNKNOWN;
    if (OH_NativeXComponent_GetKeyEventAction(keyEvent, &action) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS ||
        OH_NativeXComponent_GetKeyEventCode(keyEvent, &keyCode) != OH_NATIVEXCOMPONENT_RESULT_SUCCESS) {
        EmitNativeLog("XComponent native key skipped: action/code unavailable");
        return false;
    }

    if (action != OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN &&
        action != OH_NATIVEXCOMPONENT_KEY_ACTION_UP) {
        return false;
    }

    uint64_t modifiers = 0;
    OH_NativeXComponent_GetKeyEventModifierKeyStates(keyEvent, &modifiers);
    const OhosKeyEvent event {
        static_cast<uint32_t>(keyCode),
        action == OH_NATIVEXCOMPONENT_KEY_ACTION_DOWN,
        false,
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_CTRL),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_SHIFT),
        IsModifierPressed(modifiers, ARKUI_MODIFIER_KEY_ALT),
        false,
    };

    std::string message;
    const bool ok = g_session.SendPlatformKey(event, message);
    static std::atomic_uint32_t keyLogCount{0};
    const uint32_t logIndex = keyLogCount.fetch_add(1);
    const bool importantKey = event.keyCode == KEY_ENTER || event.keyCode == KEY_NUMPAD_ENTER ||
        event.keyCode == KEY_DEL || event.keyCode == KEY_FORWARD_DEL || event.ctrl ||
        event.keyCode == KEY_CTRL_LEFT || event.keyCode == KEY_CTRL_RIGHT;
    if (importantKey || logIndex < 40 || (logIndex % 200) == 0 || !ok) {
        EmitNativeLog("XComponent native key: keyCode=" + std::to_string(event.keyCode) +
            (event.down ? " down " : " up ") +
            "mods=" + std::to_string(modifiers) +
            " ctrl=" + std::to_string(event.ctrl ? 1 : 0) +
            " shift=" + std::to_string(event.shift ? 1 : 0) +
            " alt=" + std::to_string(event.alt ? 1 : 0) +
            " result=" + (ok ? "ok " : "failed ") + message);
    }
    return ok;
}

} // namespace

SessionEventHub& BridgeEvents()
{
    return g_events;
}

RdpSession& BridgeSession()
{
    return g_session;
}

SurfaceSnapshot BridgeSurfaceSnapshot()
{
    return g_surface.Snapshot();
}

std::string BridgeRenderStatsLog()
{
    return g_frameRenderer.BuildStatsLog();
}

void InitializeNativeBridgeContext()
{
    ConfigureRdpgfxPipelineCallbacks();
    ConfigureRdpSessionCallbacks();
}

bool RegisterNativeXComponent(napi_env env, napi_value exports)
{
    g_surface.SetLogSink(EmitNativeLog);

    napi_value nativeXComponentValue = nullptr;
    napi_status status = napi_get_named_property(env, exports, OH_NATIVE_XCOMPONENT_OBJ, &nativeXComponentValue);
    if (status != napi_ok || nativeXComponentValue == nullptr) {
        return false;
    }

    OH_NativeXComponent* component = nullptr;
    status = napi_unwrap(env, nativeXComponentValue, reinterpret_cast<void**>(&component));
    if (status != napi_ok || component == nullptr) {
        return false;
    }

    static OH_NativeXComponent_Callback callback = {
        OnXComponentSurfaceCreated,
        OnXComponentSurfaceChanged,
        OnXComponentSurfaceDestroyed,
        OnXComponentTouchEvent,
    };
    static OH_NativeXComponent_MouseEvent_Callback mouseCallback = {
        OnXComponentMouseEvent,
        OnXComponentHoverEvent,
    };

    int32_t rc = OH_NativeXComponent_RegisterCallback(component, &callback);
    const bool ok = rc == OH_NATIVEXCOMPONENT_RESULT_SUCCESS;
    const int32_t mouseRc = OH_NativeXComponent_RegisterMouseEventCallback(component, &mouseCallback);
    const int32_t focusRc = OH_NativeXComponent_RegisterFocusEventCallback(component, OnXComponentFocusEvent);
    const int32_t blurRc = OH_NativeXComponent_RegisterBlurEventCallback(component, OnXComponentBlurEvent);
    const int32_t keyRc = OH_NativeXComponent_RegisterKeyEventCallbackWithResult(component, OnXComponentKeyEvent);
    const int32_t softKeyboardRc = OH_NativeXComponent_SetNeedSoftKeyboard(component, true);
    g_surface.Register(component, ok);
    if (ok) {
        g_events.log.Emit("XComponent callback registered: " + g_surface.Snapshot().id +
            " mouseRc=" + std::to_string(mouseRc) +
            " focusRc=" + std::to_string(focusRc) +
            " blurRc=" + std::to_string(blurRc) +
            " keyRc=" + std::to_string(keyRc) +
            " softKeyboardRc=" + std::to_string(softKeyboardRc));
    }
    return ok;
}

bool NotifyBridgeSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
{
    const bool changed = g_surface.OnSurfaceLayout(width, height, message);
    if (changed) {
        EmitNativeLog(message);
        UpdateAvc420SurfaceOutputIfActive("surface layout changed");
        const SurfaceSnapshot snapshot = g_surface.Snapshot();
        g_resizeCoordinator.Begin(snapshot.width, snapshot.height, "surface layout changed");
        DropPendingRenderFrame("surface layout changed");
        RequestRemoteDesktopResize(snapshot.width, snapshot.height, "surface layout changed");
    }
    return changed;
}

} // namespace rdp_bridge
