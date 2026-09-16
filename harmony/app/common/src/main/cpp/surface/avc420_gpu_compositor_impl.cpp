#include "surface/avc420_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"

#include <memory>
#include <string>
#include <vector>

namespace rdp_bridge {

Avc420GpuCompositorImpl::Avc420GpuCompositorImpl() : state_(std::make_unique<State>()) {}

Avc420GpuCompositorImpl::~Avc420GpuCompositorImpl() = default;

void Avc420GpuCompositorImpl::Destroy()
{
    if (state_) {
        state_->Destroy();
    }
}

void Avc420GpuCompositorImpl::OnSurfaceTargetChanged(const std::string& reason,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (state_) {
        state_->OnSurfaceTargetChanged(reason, callbacks, outputActive, logs);
    }
}

bool Avc420GpuCompositorImpl::Prewarm(
    uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->Prewarm(surfaceWidth, surfaceHeight, logs);
}

bool Avc420GpuCompositorImpl::ProcessGdiFrame(
    const RgbaFrame& frame, bool outputActive, std::vector<std::string>& logs)
{
    return state_ != nullptr &&
        state_->ProcessGdiFrame(frame, outputActive, logs);
}

bool Avc420GpuCompositorImpl::PresentGdiBackgroundNow(const std::string& trigger,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr &&
        state_->PresentGdiBackgroundNow(trigger, callbacks, outputActive, logs);
}

bool Avc420GpuCompositorImpl::ProcessCommand(
    const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->ProcessCommand(command, callbacks, outputActive, logs);
}

bool Avc420GpuCompositorImpl::PresentEndFrame(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->PresentEndFrame(frame, callbacks, outputActive, logs);
}

std::string Avc420GpuCompositorImpl::DebugSummary() const
{
    return state_ == nullptr ? "impl=null" : state_->DebugSummary();
}

std::string Avc420GpuCompositorImpl::StatsSummary()
{
    return state_ == nullptr ? "decoded=0 queuedPresents=0 presented=0 mismatch=0" :
        state_->StatsSummary();
}

} // namespace rdp_bridge
