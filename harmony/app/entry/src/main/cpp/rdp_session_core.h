#pragma once

#include "bridge_types.h"
#include "surface/surface_bridge.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

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

    bool SendPointer(uint16_t flags, uint16_t x, uint16_t y, std::string& message);
    bool SendKey(uint32_t rdpScancode, bool down, bool repeat, std::string& message);
    bool SendUnicode(uint32_t code, bool down, std::string& message);

    uint32_t InputQueueDepth() const;
    uint32_t InputQueuedCount() const;
    uint32_t InputSentCount() const;
    uint32_t InputDroppedCount() const;

    bool RequestCurrentFrameRender(const std::string& reason, std::string& message);
    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rdp_bridge
