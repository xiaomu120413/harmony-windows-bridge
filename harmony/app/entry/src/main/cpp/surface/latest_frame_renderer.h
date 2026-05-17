#pragma once

#include "bridge_types.h"

#include <functional>
#include <memory>
#include <string>

namespace rdp_bridge {

class LatestFrameRenderer {
public:
    using RenderFrameFn = std::function<SurfacePaintResult(const RgbaFrame&)>;
    using LogFn = std::function<void(const std::string&)>;

    LatestFrameRenderer();
    ~LatestFrameRenderer();

    void SetCallbacks(RenderFrameFn renderFrame, LogFn log);
    void Start();
    void Stop();
    bool Enqueue(const RgbaFrame& frame, std::string& message, bool forceRender);
    RenderStatsSnapshot Snapshot();
    std::string BuildStatsLog();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rdp_bridge
