#pragma once

#include "surface/avc444_gpu_compositor.h"

#include <memory>
#include <string>
#include <vector>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <client/OHOS/ohos_rdpgfx.h>
#endif

namespace rdp_bridge {

class Avc444GpuCompositorImpl {
public:
    Avc444GpuCompositorImpl();
    ~Avc444GpuCompositorImpl();
    Avc444GpuCompositorImpl(const Avc444GpuCompositorImpl&) = delete;
    Avc444GpuCompositorImpl& operator=(const Avc444GpuCompositorImpl&) = delete;

    void Destroy();

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    bool ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
        const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
        std::vector<std::string>& logs);
    bool PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
        const Avc444GpuCompositorCallbacks& callbacks, bool& authoritative,
        std::vector<std::string>& logs);

    static bool CommandLcIsValid(const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command);
    static std::string RectText(const RECTANGLE_16* rect);
#endif

private:
    struct State;
    std::unique_ptr<State> state_;
};

} // namespace rdp_bridge
