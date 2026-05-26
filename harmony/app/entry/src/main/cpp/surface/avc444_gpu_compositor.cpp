#include "surface/avc444_gpu_compositor.h"

#include "surface/avc444_gpu_compositor_internal.h"

#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace rdp_bridge {
Avc444GpuCompositor::Avc444GpuCompositor() : impl_(std::make_unique<Avc444GpuCompositorImpl>()) {}

Avc444GpuCompositor::~Avc444GpuCompositor() = default;

void Avc444GpuCompositor::Configure(bool enabled, Avc444GpuLogFn log,
    Avc444GpuCompositorCallbacks callbacks)
{
    std::lock_guard<std::mutex> processLock(processingMutex_);
    if (impl_) {
        impl_->Destroy();
    }
    std::lock_guard<std::mutex> lock(mutex_);
    enabled_ = enabled;
    log_ = std::move(log);
    callbacks_ = std::move(callbacks);
    outputActive_ = false;
    candidates_ = 0;
    invalidLcRejects_ = 0;
    lastFrameId_ = 0;
    lastLC_ = 0;
    lastStream1Bytes_ = 0;
    lastStream2Bytes_ = 0;
    lastTargetWidth_ = 0;
    lastTargetHeight_ = 0;
    diagnostics_ = enabled ?
        "avc444 gpu compositor: configured mapped-plane compositor, gdi suppression follows per-command GPU success" :
        "avc444 gpu compositor: off";
}

void Avc444GpuCompositor::Reset()
{
    Configure(false, nullptr, {});
}

std::string Avc444GpuCompositor::Diagnostics() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return diagnostics_;
}

void Avc444GpuCompositor::SetOutputActive(bool active, const std::string& reason)
{
    bool changed = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        changed = outputActive_ != active;
        outputActive_ = active;
        diagnostics_ = "avc444 gpu compositor: enabled=" +
            std::string(enabled_ ? "yes" : "no") +
            " policyActive=" + std::string(outputActive_ ? "yes" : "no") +
            " reason=" + reason;
    }
    if (changed) {
        Log("AVC444 GPU compositor observed FreeRDP output policy: active=" +
            std::string(active ? "yes" : "no") + " reason=" + reason);
    }
}

bool Avc444GpuCompositor::OnSurfaceCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    if (command == nullptr) {
        return false;
    }

    bool shouldLogCommand = false;
    bool frameOpen = false;
    bool lcValid = false;
    uint64_t candidate = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!enabled_) {
            return false;
        }
        candidate = ++candidates_;
        shouldLogCommand = candidate <= 12 || (candidate % 120) == 0;

        frameOpen = command->frameOpen ? true : false;
        lcValid = Avc444GpuCompositorImpl::CommandLcIsValid(command);
        if (!lcValid) {
            invalidLcRejects_++;
        }
        lastFrameId_ = command->frameId;
        lastLC_ = command->LC;
        lastStream1Bytes_ = command->stream1.length;
        lastStream2Bytes_ = command->LC == 0 ? command->stream2.length : 0;
        lastTargetWidth_ = command->targetWidth;
        lastTargetHeight_ = command->targetHeight;
    }

    if (shouldLogCommand || !frameOpen || !lcValid) {
        const RECTANGLE_16* stream1FirstRect = command->stream1.numRegionRects == 0 ?
            nullptr : command->stream1.regionRects;
        const bool hasStream2 = command->LC == 0;
        const RECTANGLE_16* stream2FirstRect = !hasStream2 || command->stream2.numRegionRects == 0 ?
            nullptr : command->stream2.regionRects;
        const std::string stream2Text = hasStream2 ?
            "bytes:" + std::to_string(command->stream2.length) +
                ",rects:" + std::to_string(command->stream2.numRegionRects) +
                ",first:" + Avc444GpuCompositorImpl::RectText(stream2FirstRect) :
            "unused";
        Log("AVC444 GPU compositor candidate: index=" + std::to_string(candidate) +
            " codec=" + std::to_string(command->codecId) +
            " surface=" + std::to_string(command->surfaceId) +
            " frame=" + std::to_string(command->frameId) +
            " frameOpen=" + std::string(frameOpen ? "yes" : "no") +
            " LC=" + std::to_string(command->LC) +
            " lcValid=" + std::string(lcValid ? "yes" : "no") +
            " commandOrigin=" + std::to_string(command->left) + "," +
            std::to_string(command->top) +
            " surfaceSize=" +
            std::to_string(command->width) + "x" + std::to_string(command->height) +
            " target=" + std::to_string(command->targetWidth) + "x" +
            std::to_string(command->targetHeight) +
            " stream1=bytes:" + std::to_string(command->stream1.length) +
            ",rects:" + std::to_string(command->stream1.numRegionRects) +
            ",first:" + Avc444GpuCompositorImpl::RectText(stream1FirstRect) +
            " stream2=" + stream2Text);
    }

    Avc444GpuCompositorCallbacks callbacks;
    bool outputActive = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks = callbacks_;
        outputActive = outputActive_;
    }

    if (!lcValid) {
        return false;
    }

    std::vector<std::string> logs;
    bool consumed = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            outputActive = outputActive_;
        }
        consumed = impl_ != nullptr &&
            impl_->ProcessCommand(command, callbacks, outputActive, logs);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc444 gpu compositor: enabled=yes policyActive=" +
                std::string(outputActive_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " suppress=" + std::string(consumed ? "this-command" : "no");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return consumed;
}

bool Avc444GpuCompositor::OnEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame)
{
    if (frame == nullptr) {
        return false;
    }

    std::vector<std::string> logs;
    Avc444GpuCompositorCallbacks callbacks;
    bool handled = false;
    {
        std::lock_guard<std::mutex> processLock(processingMutex_);
        bool outputActive = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (!enabled_) {
                return false;
            }
            outputActive = outputActive_;
            callbacks = callbacks_;
        }

        handled = impl_ != nullptr &&
            impl_->PresentEndFrame(frame, callbacks, outputActive, logs);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            diagnostics_ = "avc444 gpu compositor: enabled=yes policyActive=" +
                std::string(outputActive_ ? "yes" : "no") +
                " candidates=" + std::to_string(candidates_) +
                " lastFrame=" + std::to_string(lastFrameId_) +
                " lastLC=" + std::to_string(lastLC_) +
                " endFrame=" + std::to_string(frame->frameId) +
                " endFramePresent=" + std::string(handled ? "handled" : "none");
        }
    }
    for (const std::string& line : logs) {
        Log(line);
    }
    return handled;
}

void Avc444GpuCompositor::Log(const std::string& message) const
{
    Avc444GpuLogFn log;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        log = log_;
    }
    if (log != nullptr) {
        log(message);
    }
}

Avc444GpuCompositor& SharedAvc444GpuCompositor()
{
    static Avc444GpuCompositor compositor;
    return compositor;
}

} // namespace rdp_bridge
