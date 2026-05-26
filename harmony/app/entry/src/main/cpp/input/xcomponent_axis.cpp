#include "input/xcomponent_input_internal.h"

namespace rdp_bridge {
namespace {

bool ShouldSkipDuplicateAxisEvent(int32_t action, float x, float y, double deltaX, double deltaY)
{
    const uint64_t now = NowMs();
    std::lock_guard<std::mutex> lock(g_nativeAxisMutex);
    const bool duplicate = now - g_nativeAxis.lastAtMs <= kAxisEventDedupMs &&
        g_nativeAxis.lastAction == action &&
        std::fabs(g_nativeAxis.lastX - x) < 0.01f &&
        std::fabs(g_nativeAxis.lastY - y) < 0.01f &&
        std::fabs(g_nativeAxis.lastDeltaX - deltaX) < 0.01 &&
        std::fabs(g_nativeAxis.lastDeltaY - deltaY) < 0.01;
    g_nativeAxis.lastAtMs = now;
    g_nativeAxis.lastAction = action;
    g_nativeAxis.lastX = x;
    g_nativeAxis.lastY = y;
    g_nativeAxis.lastDeltaX = deltaX;
    g_nativeAxis.lastDeltaY = deltaY;
    return duplicate;
}

void DispatchNativeAxisPointer(const ArkUI_UIInputEvent* event)
{
    const int32_t action = OH_ArkUI_UIInputEvent_GetAction(event);
    const double deltaY = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
    const double deltaX = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
    const float x = OH_ArkUI_PointerEvent_GetX(event);
    const float y = OH_ArkUI_PointerEvent_GetY(event);
    if (ShouldSkipDuplicateAxisEvent(action, x, y, deltaX, deltaY)) {
        return;
    }
    if (std::fabs(deltaY) < 0.01 && std::fabs(deltaX) < 0.01) {
        return;
    }
    if (action == UI_AXIS_EVENT_ACTION_BEGIN ||
        action == UI_AXIS_EVENT_ACTION_END ||
        action == UI_AXIS_EVENT_ACTION_CANCEL) {
        return;
    }

    const bool vertical = std::fabs(deltaY) >= std::fabs(deltaX);
    const double delta = vertical ? deltaY : deltaX;
    const LocalPointerAction pointerAction =
        vertical ? LocalPointerAction::WheelVertical : LocalPointerAction::WheelHorizontal;
    SendNativePointer(MakeNativePointer(pointerAction, x, y, LocalPointerButtonNone,
        static_cast<int32_t>(std::lround(delta))),
        vertical ? "axis.wheel.vertical" : "axis.wheel.horizontal");
}

} // namespace

void OnXComponentAxisEvent(OH_NativeXComponent*, ArkUI_UIInputEvent* event,
    ArkUI_UIInputEvent_Type type)
{
    if (event == nullptr || type != ARKUI_UIINPUTEVENT_TYPE_AXIS) {
        return;
    }
    DispatchNativeAxisPointer(event);
}

} // namespace rdp_bridge
