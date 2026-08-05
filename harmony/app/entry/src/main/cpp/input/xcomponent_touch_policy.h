#pragma once

#include <cstdint>
#include <vector>

namespace rdp_bridge {

constexpr uint64_t kNativeLongPressIntervalMs = 500;
constexpr float kNativeTouchDragThresholdVp = 5.0f;
constexpr float kNativeTouchWheelQuantumVp = 12.0f;

struct NativeAxisSteps {
    int32_t horizontal = 0;
    int32_t vertical = 0;
};

class NativeAxisScrollPolicy {
public:
    void Begin(int32_t source, double quantum);
    NativeAxisSteps Update(int32_t source, double quantum, double deltaX, double deltaY);
    void End();

private:
    int32_t source_ = -1;
    double quantum_ = 1.0;
    double remainderX_ = 0.0;
    double remainderY_ = 0.0;
};

enum class NativeGestureActionType : uint8_t {
    Move,
    LeftDown,
    LeftUp,
    LeftClick,
    RightClick,
    WheelVertical,
    WheelHorizontal,
};

struct NativeGestureAction {
    NativeGestureActionType type = NativeGestureActionType::Move;
    float x = 0.0f;
    float y = 0.0f;
    int32_t steps = 0;
    bool held = false;
};

std::vector<NativeGestureAction> BuildNativeClickActions(
    uint32_t clickCount, float x, float y);
std::vector<NativeGestureAction> BuildNativeRightClickActions(float x, float y);

class NativeSystemTapPolicy {
public:
    std::vector<NativeGestureAction> SingleAccepted(float x, float y);
    std::vector<NativeGestureAction> DoubleAccepted(float x, float y);
    void Cancel();

private:
    bool firstAccepted_ = false;
    float firstX_ = 0.0f;
    float firstY_ = 0.0f;
};

class NativeSystemPanPolicy {
public:
    std::vector<NativeGestureAction> Accept(
        float startX, float startY, float currentX, float currentY);
    std::vector<NativeGestureAction> Update(float x, float y);
    std::vector<NativeGestureAction> End(float x, float y);
    std::vector<NativeGestureAction> Cancel();

private:
    bool active_ = false;
    float lastX_ = 0.0f;
    float lastY_ = 0.0f;
};

class NativeSystemScrollPolicy {
public:
    void Begin(float offsetX, float offsetY, float quantumPx);
    std::vector<NativeGestureAction> Update(
        float x, float y, float offsetX, float offsetY, float quantumPx);
    void End();

private:
    bool active_ = false;
    float lastOffsetX_ = 0.0f;
    float lastOffsetY_ = 0.0f;
    NativeAxisScrollPolicy steps_;
};

} // namespace rdp_bridge
