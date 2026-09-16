#pragma once

#include "common/bridge_types.h"

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
    bool DropPending(const std::string& reason, std::string& message);
    // 生存期契约：Enqueue 不拷贝帧数据，worker 异步从 frame.data 裸指针渲染。
    // 调用方释放或重写底层 buffer（如 FreeRDP gdi->primary_buffer）前，
    // 必须先 Stop() join worker；否则 worker 访问已释放内存即 UAF。
    bool Enqueue(const RgbaFrame& frame, std::string& message, bool forceRender);
    RenderStatsSnapshot Snapshot();
    std::string BuildStatsLog();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace rdp_bridge
