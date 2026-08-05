#include "input/xcomponent_input_internal.h"

#include <cstdlib>

namespace rdp_bridge {
namespace {

constexpr double kMouseWheelQuantumDegrees = 15.0;

bool IsDuplicateAxisEvent(int32_t action, float x, float y, double deltaX, double deltaY)
{
    const uint64_t now = NowMs();
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

void SendAxisSteps(LocalPointerAction action, float x, float y, int32_t steps,
    const std::string& label)
{
    const int32_t direction = steps < 0 ? -1 : 1;
    for (int32_t index = 0; index < std::abs(steps); ++index) {
        SendNativePointer(MakeNativePointer(action, x, y,
            LocalPointerButtonNone, direction), label);
    }
}

void DispatchNativeAxisPointer(const ArkUI_UIInputEvent* event)
{
    const int32_t action = OH_ArkUI_AxisEvent_GetAxisAction(event);
    const int32_t source = OH_ArkUI_UIInputEvent_GetSourceType(event);
    const int32_t tool = OH_ArkUI_UIInputEvent_GetToolType(event);
    const double deltaY = OH_ArkUI_AxisEvent_GetVerticalAxisValue(event);
    const double deltaX = OH_ArkUI_AxisEvent_GetHorizontalAxisValue(event);
    const float x = OH_ArkUI_PointerEvent_GetX(event);
    const float y = OH_ArkUI_PointerEvent_GetY(event);

    if (source != UI_INPUT_EVENT_SOURCE_TYPE_MOUSE ||
        (tool != UI_INPUT_EVENT_TOOL_TYPE_MOUSE && tool != UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD)) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_nativeAxisMutex);
    if (IsDuplicateAxisEvent(action, x, y, deltaX, deltaY)) {
        return;
    }
    if (OH_ArkUI_AxisEvent_HasAxis(event, UI_AXIS_TYPE_PINCH_AXIS)) {
        g_nativeAxis.scrollPolicy.End();
        return;
    }
    const double quantum = tool == UI_INPUT_EVENT_TOOL_TYPE_TOUCHPAD
        ? static_cast<double>(kNativeTouchWheelQuantumVp * g_inputDensity.load())
        : kMouseWheelQuantumDegrees;
    if (action == UI_AXIS_EVENT_ACTION_BEGIN) {
        g_nativeAxis.scrollPolicy.Begin(tool, quantum);
        return;
    }
    if (action == UI_AXIS_EVENT_ACTION_END || action == UI_AXIS_EVENT_ACTION_CANCEL) {
        g_nativeAxis.scrollPolicy.End();
        return;
    }
    if (action != UI_AXIS_EVENT_ACTION_UPDATE) {
        return;
    }
    const NativeAxisSteps steps = g_nativeAxis.scrollPolicy.Update(
        tool, quantum, deltaX, deltaY);
    SendAxisSteps(LocalPointerAction::WheelVertical, x, y, steps.vertical, "axis.wheel.vertical");
    SendAxisSteps(LocalPointerAction::WheelHorizontal, x, y, steps.horizontal, "axis.wheel.horizontal");
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
