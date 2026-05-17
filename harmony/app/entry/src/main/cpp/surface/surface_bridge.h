#pragma once

#include "bridge_types.h"

#include <functional>
#include <memory>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {

struct SurfaceSnapshot {
    bool registered = false;
    bool ready = false;
    std::string id;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t viewportX = 0;
    uint32_t viewportY = 0;
    uint32_t viewportWidth = 0;
    uint32_t viewportHeight = 0;
    uint32_t createdCount = 0;
    uint32_t changedCount = 0;
    uint32_t destroyedCount = 0;
    uint32_t touchCount = 0;
    uint32_t paintCount = 0;
    std::string lastPaintMessage;
};

class SurfaceBridge {
public:
    using LogFn = std::function<void(const std::string&)>;

    SurfaceBridge();
    ~SurfaceBridge();

    void SetLogSink(LogFn log);
    void Register(OH_NativeXComponent* component, bool ok);
    void OnSurfaceCreated(OH_NativeXComponent* component, void* window);
    void OnSurfaceChanged(OH_NativeXComponent* component, void* window);
    bool OnSurfaceLayout(uint32_t width, uint32_t height, std::string& message);
    void OnSurfaceDestroyed(OH_NativeXComponent* component, void* window);
    void OnTouchEvent();
    SurfacePaintResult RenderRgbaFrame(const RgbaFrame& frame);
    SurfaceSnapshot Snapshot();
    DecoderSurfaceTarget DecoderSurface();
    bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
        std::string& error);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rdp_bridge
