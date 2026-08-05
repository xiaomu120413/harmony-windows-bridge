#pragma once

#include <cstdint>
#include <vector>

namespace rdp_bridge {

constexpr uint64_t kNativeDoubleTapIntervalMs = 300;
constexpr uint64_t kNativeLongPressIntervalMs = 500;
constexpr float kNativeDoubleTapDistanceVp = 60.0f;
constexpr float kNativeTouchDragThresholdVp = 5.0f;
constexpr float kNativeTouchWheelQuantumVp = 12.0f;

struct NativeTouchThresholds {
    float doubleTapDistancePx = kNativeDoubleTapDistanceVp;
    float dragDistancePx = kNativeTouchDragThresholdVp;
    float wheelQuantumPx = kNativeTouchWheelQuantumVp;
};

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

NativeTouchThresholds NativeTouchThresholdsForDensity(float density);

enum class NativeTouchAction : uint8_t {
    Down,
    Move,
    Up,
    Cancel,
};

struct NativeTouchPoint {
    int32_t id = -1;
    float x = 0.0f;
    float y = 0.0f;
};

struct NativeTouchSample {
    NativeTouchAction action = NativeTouchAction::Cancel;
    uint64_t atMs = 0;
    std::vector<NativeTouchPoint> points;
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

class NativeTouchGesturePolicy {
public:
    explicit NativeTouchGesturePolicy(NativeTouchThresholds thresholds = {});

    std::vector<NativeGestureAction> Handle(const NativeTouchSample& sample);
    std::vector<NativeGestureAction> HandleLongPressTimeout(uint64_t atMs);
    std::vector<NativeGestureAction> Cancel();
    void SetThresholds(NativeTouchThresholds thresholds);
    bool IsLongPressPending() const;
    uint64_t LongPressDeadlineMs() const;

private:
    enum class State : uint8_t {
        Idle,
        Pressed,
        Dragging,
        DoubleSecondDown,
        LongPressRecognized,
        Scrolling,
    };

    const NativeTouchPoint* FindPoint(const NativeTouchSample& sample, int32_t id) const;
    void ResetActive();

    NativeTouchThresholds thresholds_;
    State state_ = State::Idle;
    int32_t primaryId_ = -1;
    int32_t secondaryId_ = -1;
    uint64_t downAtMs_ = 0;
    float startX_ = 0.0f;
    float startY_ = 0.0f;
    float lastX_ = 0.0f;
    float lastY_ = 0.0f;
    float scrollLastX_ = 0.0f;
    float scrollLastY_ = 0.0f;
    float scrollRemainderX_ = 0.0f;
    float scrollRemainderY_ = 0.0f;
    bool remoteLeftDown_ = false;
    bool lastTapValid_ = false;
    uint64_t lastTapAtMs_ = 0;
    float lastTapX_ = 0.0f;
    float lastTapY_ = 0.0f;
};

} // namespace rdp_bridge
