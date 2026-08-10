#pragma once

#include "common/bridge_types.h"
#include "input/ohos_keyboard_adapter.h"
#include "session/rdp_session_channels.h"
#include "surface/surface_bridge.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rdp_bridge {

struct RdpSessionCallbacks {
    std::function<void(const std::string&)> emitState;
    std::function<void(const std::string&)> emitLog;
    std::function<void(const std::string&)> emitError;
    std::function<SurfaceSnapshot()> surfaceSnapshot;
    std::function<bool(const RgbaFrame&, std::string&, bool)> queueSurfaceRgbaFrame;
    std::function<void()> startRenderPipeline;
    std::function<void()> stopRenderPipeline;
    std::function<void(const std::string&)> requestSurfaceRepaint;
    std::function<std::string()> renderStats;
};

class RdpSession {
public:
    RdpSession();
    ~RdpSession();

    RdpSession(const RdpSession&) = delete;
    RdpSession& operator=(const RdpSession&) = delete;

    void SetCallbacks(RdpSessionCallbacks callbacks);

    bool Connect(const ConnectParams& params, std::string& message);
    void Disconnect();
    bool RequestDisconnect();
    bool IsConnected() const;
    uint64_t DiagnosticSessionId() const;
    void SetDisplayOrientation(uint32_t orientation);
    uint32_t DisplayOrientation() const;
    bool SetMonitorLayout(std::vector<FREERDP_OHOS_MONITOR_LAYOUT> monitors,
        std::string& message);

    bool SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message);
    bool SendLocalPointer(const LocalPointerEvent& pointer, std::string& message);
    bool SendLocalPen(const LocalPenEvent& pen, std::string& message);
    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message);
    bool SendPlatformKey(const OhosKeyEvent& event, std::string& message);
    bool SendUnicode(uint32_t code, bool down, std::string& message);
    bool SendCommittedText(const std::u16string& text, std::string& message);
    bool SendFocusIn(uint16_t toggleStates, std::string& message);
    bool ReleaseAllKeys(std::string& message);

    uint32_t InputQueueDepth() const;
    uint32_t InputQueuedCount() const;
    uint32_t InputSentCount() const;
    uint32_t InputDroppedCount() const;

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message);
    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message);
    DisplayResizeResult RequestDynamicDesktopResizeEx(uint32_t width, uint32_t height,
        uint32_t orientation, const std::string& reason);
    DisplayResizeResult RequestDynamicDesktopResizeEx(const DisplayResizeRequest& request);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rdp_bridge
