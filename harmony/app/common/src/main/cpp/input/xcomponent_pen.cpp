#include "input/xcomponent_pen.h"

#include "input/xcomponent_input_internal.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <mutex>
#include <unordered_map>

namespace rdp_bridge {
namespace {

struct PenState {
    LocalPenEvent last;
    bool pointerFallback = false;
};

std::mutex g_penMutex;
std::unordered_map<int32_t, PenState> g_activePens;

bool IsPenTool(OH_NativeXComponent_TouchPointToolType tool)
{
    return tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_PEN ||
        tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_RUBBER ||
        tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_BRUSH ||
        tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_PENCIL ||
        tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_AIRBRUSH;
}

int32_t DeviceId(int64_t value)
{
    return static_cast<int32_t>(std::clamp(value,
        static_cast<int64_t>(std::numeric_limits<int32_t>::min()),
        static_cast<int64_t>(std::numeric_limits<int32_t>::max())));
}

LocalPenAction ToPenAction(OH_NativeXComponent_TouchEventType type)
{
    switch (type) {
        case OH_NATIVEXCOMPONENT_DOWN:
            return LocalPenAction::Down;
        case OH_NATIVEXCOMPONENT_UP:
            return LocalPenAction::Up;
        case OH_NATIVEXCOMPONENT_CANCEL:
            return LocalPenAction::Cancel;
        case OH_NATIVEXCOMPONENT_MOVE:
        case OH_NATIVEXCOMPONENT_UNKNOWN:
        default:
            return LocalPenAction::Move;
    }
}

bool SendPointerFallback(const LocalPenEvent& pen, bool wasFallback, std::string& message)
{
    LocalPointerAction action = LocalPointerAction::Move;
    uint32_t buttons = LocalPointerButtonNone;
    if (pen.action == LocalPenAction::Down) {
        action = LocalPointerAction::ButtonDown;
        buttons = LocalPointerButtonLeft;
    } else if (pen.action == LocalPenAction::Up || pen.action == LocalPenAction::Cancel) {
        action = LocalPointerAction::ButtonUp;
        buttons = LocalPointerButtonLeft;
    } else if (wasFallback) {
        buttons = LocalPointerButtonLeft;
    }
    return g_inputSession != nullptr && g_inputSession->SendLocalPointer(
        MakeNativePointer(action, static_cast<float>(pen.x), static_cast<float>(pen.y),
            buttons, 0, pen.action != LocalPenAction::Down), message);
}

} // namespace

bool TryHandleXComponentPenEvent(OH_NativeXComponent* component,
    const OH_NativeXComponent_TouchEvent& event)
{
    if (component == nullptr || event.numPoints == 0) {
        return false;
    }
    uint32_t pointIndex = 0;
    for (uint32_t index = 0; index < event.numPoints; ++index) {
        if (event.touchPoints[index].id == event.id) {
            pointIndex = index;
            break;
        }
    }
    OH_NativeXComponent_TouchPointToolType tool = OH_NATIVEXCOMPONENT_TOOL_TYPE_UNKNOWN;
    if (OH_NativeXComponent_GetTouchPointToolType(component, pointIndex, &tool) !=
        OH_NATIVEXCOMPONENT_RESULT_SUCCESS || !IsPenTool(tool)) {
        return false;
    }

    const OH_NativeXComponent_TouchPoint& point = event.touchPoints[pointIndex];
    float tiltX = 0.0f;
    float tiltY = 0.0f;
    (void)OH_NativeXComponent_GetTouchPointTiltX(component, pointIndex, &tiltX);
    (void)OH_NativeXComponent_GetTouchPointTiltY(component, pointIndex, &tiltY);
    LocalPenEvent pen;
    pen.action = ToPenAction(event.type);
    pen.deviceId = DeviceId(event.deviceId);
    pen.x = RoundSurfaceCoordinate(point.x);
    pen.y = RoundSurfaceCoordinate(point.y);
    const float rawPressure = std::isfinite(point.force) && point.force > 0.0f ? point.force :
        (std::isfinite(event.force) ? event.force : 0.0f);
    pen.pressure = std::clamp(rawPressure, 0.0f, 1.0f);
    pen.tiltX = static_cast<int16_t>(std::lround(std::clamp(tiltX, -90.0f, 90.0f)));
    pen.tiltY = static_cast<int16_t>(std::lround(std::clamp(tiltY, -90.0f, 90.0f)));
    pen.flags = tool == OH_NATIVEXCOMPONENT_TOOL_TYPE_RUBBER
        ? LocalPenFlagEraser : LocalPenFlagNone;
    pen.allowClamp = pen.action != LocalPenAction::Down;

    std::lock_guard<std::mutex> lock(g_penMutex);
    PenState& state = g_activePens[event.id];
    std::string message;
    bool ok = false;
    if (!state.pointerFallback) {
        ok = g_inputSession != nullptr && g_inputSession->SendLocalPen(pen, message);
        if (!ok && pen.action == LocalPenAction::Down) {
            state.pointerFallback = true;
        }
    }
    if (state.pointerFallback) {
        ok = SendPointerFallback(pen, true, message);
    }
    state.last = pen;
    if (pen.action == LocalPenAction::Up || pen.action == LocalPenAction::Cancel) {
        g_activePens.erase(event.id);
    }
    if (!ok || pen.action != LocalPenAction::Move) {
        EmitInputLog("XComponent pen: action=" + std::to_string(static_cast<int>(pen.action)) +
            " tool=" + std::to_string(static_cast<int>(tool)) + " result=" +
            (ok ? "ok " : "failed ") + message);
    }
    return true;
}

void CancelXComponentPenInput(const std::string& reason)
{
    std::lock_guard<std::mutex> lock(g_penMutex);
    for (auto& entry : g_activePens) {
        PenState& state = entry.second;
        state.last.action = LocalPenAction::Cancel;
        state.last.allowClamp = true;
        std::string message;
        if (state.pointerFallback) {
            (void)SendPointerFallback(state.last, true, message);
        } else if (g_inputSession != nullptr) {
            (void)g_inputSession->SendLocalPen(state.last, message);
        }
    }
    if (!g_activePens.empty()) {
        EmitInputLog("XComponent pen cancelled: reason=" + reason + " count=" +
            std::to_string(g_activePens.size()));
    }
    g_activePens.clear();
}

} // namespace rdp_bridge
