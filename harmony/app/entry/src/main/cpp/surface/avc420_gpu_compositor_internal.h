#pragma once

#include "surface/avc420_gpu_compositor.h"

#include <memory>
#include <string>
#include <vector>

#include <client/OHOS/ohos_rdpgfx.h>

namespace rdp_bridge {

class Avc420GpuCompositorImpl {
public:
    Avc420GpuCompositorImpl();
    ~Avc420GpuCompositorImpl();
    Avc420GpuCompositorImpl(const Avc420GpuCompositorImpl&) = delete;
    Avc420GpuCompositorImpl& operator=(const Avc420GpuCompositorImpl&) = delete;

    void Destroy();
    void OnSurfaceTargetChanged(const std::string& reason,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);

    bool Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight,
        std::vector<std::string>& logs);
    bool ProcessGdiFrame(const RgbaFrame& frame,
        bool outputActive, std::vector<std::string>& logs);
    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
        std::vector<std::string>& logs);
    std::string DebugSummary() const;
    std::string StatsSummary();

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace rdp_bridge
