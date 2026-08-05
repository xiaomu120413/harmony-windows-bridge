#include "input/xcomponent_native_gesture.h"

#include "common/bridge_log.h"
#include "input/xcomponent_input_internal.h"

#include <arkui/native_gesture.h>
#include <arkui/native_interface.h>
#include <arkui/ui_input_event.h>

#include <algorithm>
#include <mutex>

namespace rdp_bridge {
namespace {

enum class SystemGestureKind : uint8_t {
    SingleTap,
    DoubleTap,
    LongPress,
    OneFingerPan,
    TwoFingerPan,
};

class NativeGestureBinding;

struct GestureCallbackContext {
    NativeGestureBinding* owner = nullptr;
    SystemGestureKind kind = SystemGestureKind::SingleTap;
};

struct GesturePoint {
    float x = 0.0f;
    float y = 0.0f;
};

GesturePoint GestureCenter(const ArkUI_UIInputEvent* event, uint32_t maxPointers)
{
    if (event == nullptr) {
        return {};
    }
    const uint32_t count = std::min(
        OH_ArkUI_PointerEvent_GetPointerCount(event), maxPointers);
    if (count == 0) {
        return {OH_ArkUI_PointerEvent_GetX(event), OH_ArkUI_PointerEvent_GetY(event)};
    }
    GesturePoint center;
    for (uint32_t i = 0; i < count; ++i) {
        center.x += OH_ArkUI_PointerEvent_GetXByIndex(event, i);
        center.y += OH_ArkUI_PointerEvent_GetYByIndex(event, i);
    }
    center.x /= static_cast<float>(count);
    center.y /= static_cast<float>(count);
    return center;
}

class NativeGestureBinding {
public:
    bool Bind(ArkUI_NodeHandle node, std::string& message)
    {
        if (node == nullptr) {
            message = "XComponent native node is invalid";
            BridgeLogger::Error(message);
            return false;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        if (node_ == node && rootGroup_ != nullptr && attached_) {
            message = "XComponent native gestures already bound";
            return true;
        }
        UnbindLocked();

        OH_ArkUI_GetModuleInterface(ARKUI_NATIVE_GESTURE, ArkUI_NativeGestureAPI_1, api_);
        if (api_ == nullptr) {
            message = "ArkUI Native Gesture API 1 is unavailable";
            BridgeLogger::Error(message);
            return false;
        }
        node_ = node;
        rootGroup_ = api_->createGroupGesture(EXCLUSIVE_GROUP);
        tapGroup_ = api_->createGroupGesture(PARALLEL_GROUP);
        singleTap_ = api_->createTapGesture(1, 1);
        doubleTap_ = api_->createTapGesture(2, 1);
        longPress_ = api_->createLongPressGesture(1, false, kNativeLongPressIntervalMs);
        oneFingerPan_ = api_->createPanGesture(
            1, GESTURE_DIRECTION_ALL, kNativeTouchDragThresholdVp);
        twoFingerPan_ = api_->createPanGesture(
            2, GESTURE_DIRECTION_ALL, kNativeTouchWheelQuantumVp);
        if (!AllRecognizersCreated()) {
            return FailLocked("failed to create native system gesture recognizers", message);
        }

        singleContext_ = {this, SystemGestureKind::SingleTap};
        doubleContext_ = {this, SystemGestureKind::DoubleTap};
        longContext_ = {this, SystemGestureKind::LongPress};
        onePanContext_ = {this, SystemGestureKind::OneFingerPan};
        twoPanContext_ = {this, SystemGestureKind::TwoFingerPan};
        if (!SetExactFingerCounts() || !RegisterCallbacks() || !BuildGestureTree() ||
            api_->addGestureToNode(node_, rootGroup_, PRIORITY, NORMAL_GESTURE_MASK) != 0) {
            return FailLocked("failed to attach native system gestures to XComponent", message);
        }
        attached_ = true;
        message = "native tap, long-press, drag, and two-finger scroll gestures bound";
        BridgeLogger::Info(message);
        return true;
    }

    void Cancel(const std::string& reason)
    {
        std::vector<NativeGestureAction> release;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            release = panPolicy_.Cancel();
            scrollPolicy_.End();
            tapPolicy_.Cancel();
        }
        DispatchNativeGestureActions(release, "touch.systemCancel." + reason);
    }

    void Unbind()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        UnbindLocked();
    }

    void Handle(ArkUI_GestureEvent* event, SystemGestureKind kind)
    {
        if (event == nullptr) {
            return;
        }
        const ArkUI_UIInputEvent* raw = OH_ArkUI_GestureEvent_GetRawInputEvent(event);
        if (raw == nullptr) {
            BridgeLogger::Error("native gesture callback has no raw input event");
            return;
        }
        const ArkUI_GestureEventActionType action = OH_ArkUI_GestureEvent_GetActionType(event);
        const GesturePoint point = GestureCenter(raw,
            kind == SystemGestureKind::TwoFingerPan ? 2U : 1U);
        std::vector<NativeGestureAction> actions;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            switch (kind) {
                case SystemGestureKind::SingleTap:
                    if (action == GESTURE_EVENT_ACTION_ACCEPT) {
                        actions = tapPolicy_.SingleAccepted(point.x, point.y);
                    }
                    break;
                case SystemGestureKind::DoubleTap:
                    if (action == GESTURE_EVENT_ACTION_ACCEPT) {
                        actions = tapPolicy_.DoubleAccepted(point.x, point.y);
                    }
                    break;
                case SystemGestureKind::LongPress:
                    if (action == GESTURE_EVENT_ACTION_ACCEPT) {
                        tapPolicy_.Cancel();
                        actions = BuildNativeRightClickActions(point.x, point.y);
                    }
                    break;
                case SystemGestureKind::OneFingerPan:
                    actions = HandleOneFingerPan(event, action, point);
                    break;
                case SystemGestureKind::TwoFingerPan:
                    actions = HandleTwoFingerPan(event, action, point);
                    break;
            }
        }
        DispatchNativeGestureActions(actions, Label(kind));
    }

private:
    static void OnGestureEvent(ArkUI_GestureEvent* event, void* extraParams)
    {
        auto* context = static_cast<GestureCallbackContext*>(extraParams);
        if (context != nullptr && context->owner != nullptr) {
            context->owner->Handle(event, context->kind);
        }
    }

    static const char* Label(SystemGestureKind kind)
    {
        switch (kind) {
            case SystemGestureKind::SingleTap:
                return "touch.systemSingleTap";
            case SystemGestureKind::DoubleTap:
                return "touch.systemDoubleTapSecond";
            case SystemGestureKind::LongPress:
                return "touch.systemLongPress";
            case SystemGestureKind::OneFingerPan:
                return "touch.systemDrag";
            case SystemGestureKind::TwoFingerPan:
                return "touch.systemTwoFingerScroll";
        }
        return "touch.systemGesture";
    }

    bool AllRecognizersCreated() const
    {
        return rootGroup_ != nullptr && tapGroup_ != nullptr && singleTap_ != nullptr &&
            doubleTap_ != nullptr && longPress_ != nullptr && oneFingerPan_ != nullptr &&
            twoFingerPan_ != nullptr;
    }

    bool SetExactFingerCounts()
    {
        for (ArkUI_GestureRecognizer* recognizer : {
            singleTap_, doubleTap_, longPress_, oneFingerPan_, twoFingerPan_}) {
            if (OH_ArkUI_SetGestureRecognizerLimitFingerCount(recognizer, true) != 0) {
                return false;
            }
        }
        return true;
    }

    bool RegisterCallbacks()
    {
        const ArkUI_GestureEventActionTypeMask panActions = GESTURE_EVENT_ACTION_ACCEPT |
            GESTURE_EVENT_ACTION_UPDATE | GESTURE_EVENT_ACTION_END |
            GESTURE_EVENT_ACTION_CANCEL;
        return api_->setGestureEventTarget(singleTap_, GESTURE_EVENT_ACTION_ACCEPT,
                   &singleContext_, OnGestureEvent) == 0 &&
            api_->setGestureEventTarget(doubleTap_, GESTURE_EVENT_ACTION_ACCEPT,
                &doubleContext_, OnGestureEvent) == 0 &&
            api_->setGestureEventTarget(longPress_, GESTURE_EVENT_ACTION_ACCEPT,
                &longContext_, OnGestureEvent) == 0 &&
            api_->setGestureEventTarget(oneFingerPan_, panActions,
                &onePanContext_, OnGestureEvent) == 0 &&
            api_->setGestureEventTarget(twoFingerPan_, panActions,
                &twoPanContext_, OnGestureEvent) == 0;
    }

    bool BuildGestureTree()
    {
        return api_->addChildGesture(tapGroup_, singleTap_) == 0 &&
            api_->addChildGesture(tapGroup_, doubleTap_) == 0 &&
            api_->addChildGesture(rootGroup_, tapGroup_) == 0 &&
            api_->addChildGesture(rootGroup_, longPress_) == 0 &&
            api_->addChildGesture(rootGroup_, oneFingerPan_) == 0 &&
            api_->addChildGesture(rootGroup_, twoFingerPan_) == 0;
    }

    std::vector<NativeGestureAction> HandleOneFingerPan(
        ArkUI_GestureEvent* event, ArkUI_GestureEventActionType action,
        const GesturePoint& point)
    {
        tapPolicy_.Cancel();
        if (action == GESTURE_EVENT_ACTION_ACCEPT) {
            const float offsetX = OH_ArkUI_PanGesture_GetOffsetX(event);
            const float offsetY = OH_ArkUI_PanGesture_GetOffsetY(event);
            return panPolicy_.Accept(
                point.x - offsetX, point.y - offsetY, point.x, point.y);
        }
        if (action == GESTURE_EVENT_ACTION_UPDATE) {
            return panPolicy_.Update(point.x, point.y);
        }
        if (action == GESTURE_EVENT_ACTION_END) {
            return panPolicy_.End(point.x, point.y);
        }
        return panPolicy_.Cancel();
    }

    std::vector<NativeGestureAction> HandleTwoFingerPan(
        ArkUI_GestureEvent* event, ArkUI_GestureEventActionType action,
        const GesturePoint& point)
    {
        tapPolicy_.Cancel();
        const float offsetX = OH_ArkUI_PanGesture_GetOffsetX(event);
        const float offsetY = OH_ArkUI_PanGesture_GetOffsetY(event);
        const float quantumPx = kNativeTouchWheelQuantumVp * g_inputDensity.load();
        if (action == GESTURE_EVENT_ACTION_ACCEPT) {
            scrollPolicy_.Begin(offsetX, offsetY, quantumPx);
            return {};
        }
        if (action == GESTURE_EVENT_ACTION_UPDATE) {
            return scrollPolicy_.Update(
                point.x, point.y, offsetX, offsetY, quantumPx);
        }
        scrollPolicy_.End();
        return {};
    }

    bool FailLocked(const std::string& failure, std::string& message)
    {
        message = failure;
        BridgeLogger::Error(message);
        UnbindLocked();
        return false;
    }

    void RemoveChildrenLocked()
    {
        if (api_ == nullptr) {
            return;
        }
        if (tapGroup_ != nullptr) {
            if (singleTap_ != nullptr) {
                (void)api_->removeChildGesture(tapGroup_, singleTap_);
            }
            if (doubleTap_ != nullptr) {
                (void)api_->removeChildGesture(tapGroup_, doubleTap_);
            }
        }
        if (rootGroup_ != nullptr) {
            for (ArkUI_GestureRecognizer* child : {
                tapGroup_, longPress_, oneFingerPan_, twoFingerPan_}) {
                if (child != nullptr) {
                    (void)api_->removeChildGesture(rootGroup_, child);
                }
            }
        }
    }

    void DisposeLocked(ArkUI_GestureRecognizer*& recognizer)
    {
        if (api_ != nullptr && recognizer != nullptr) {
            api_->dispose(recognizer);
        }
        recognizer = nullptr;
    }

    void UnbindLocked()
    {
        DispatchNativeGestureActions(panPolicy_.Cancel(), "touch.systemUnbind");
        scrollPolicy_.End();
        tapPolicy_.Cancel();
        if (api_ != nullptr && attached_ && node_ != nullptr && rootGroup_ != nullptr) {
            (void)api_->removeGestureFromNode(node_, rootGroup_);
        }
        RemoveChildrenLocked();
        DisposeLocked(singleTap_);
        DisposeLocked(doubleTap_);
        DisposeLocked(longPress_);
        DisposeLocked(oneFingerPan_);
        DisposeLocked(twoFingerPan_);
        DisposeLocked(tapGroup_);
        DisposeLocked(rootGroup_);
        node_ = nullptr;
        attached_ = false;
    }

    std::mutex mutex_;
    ArkUI_NativeGestureAPI_1* api_ = nullptr;
    ArkUI_NodeHandle node_ = nullptr;
    ArkUI_GestureRecognizer* rootGroup_ = nullptr;
    ArkUI_GestureRecognizer* tapGroup_ = nullptr;
    ArkUI_GestureRecognizer* singleTap_ = nullptr;
    ArkUI_GestureRecognizer* doubleTap_ = nullptr;
    ArkUI_GestureRecognizer* longPress_ = nullptr;
    ArkUI_GestureRecognizer* oneFingerPan_ = nullptr;
    ArkUI_GestureRecognizer* twoFingerPan_ = nullptr;
    GestureCallbackContext singleContext_;
    GestureCallbackContext doubleContext_;
    GestureCallbackContext longContext_;
    GestureCallbackContext onePanContext_;
    GestureCallbackContext twoPanContext_;
    NativeSystemTapPolicy tapPolicy_;
    NativeSystemPanPolicy panPolicy_;
    NativeSystemScrollPolicy scrollPolicy_;
    bool attached_ = false;
};

NativeGestureBinding& GestureBinding()
{
    static NativeGestureBinding binding;
    return binding;
}

} // namespace

bool BindXComponentNativeGestures(ArkUI_NodeHandle node, std::string& message)
{
    return GestureBinding().Bind(node, message);
}

void CancelXComponentNativeGestures(const std::string& reason)
{
    GestureBinding().Cancel(reason);
}

void UnbindXComponentNativeGestures()
{
    GestureBinding().Unbind();
}

} // namespace rdp_bridge
