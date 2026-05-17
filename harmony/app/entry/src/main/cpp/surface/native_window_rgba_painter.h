#pragma once

#include "bridge_types.h"
#include "surface/gpu_rgba_renderer.h"

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

#include <native_window/external_window.h>

namespace rdp_bridge {

class NativeWindowRgbaPainter {
public:
    void Destroy();
    SurfacePaintResult Render(OHNativeWindow* nativeWindow, uint32_t targetWidth,
        uint32_t targetHeight, const RgbaFrame& frame, RenderViewport& viewport);

private:
    struct DirtyHistoryEntry {
        uint64_t fromSequence = 0;
        uint64_t toSequence = 0;
        DirtyFrameStats dirty;
    };

    bool ConfigureNativeWindow(OHNativeWindow* nativeWindow, int32_t targetWidth,
        int32_t targetHeight, SurfacePaintResult& result);
    bool CanUsePartialDirty(uintptr_t bufferKey, const RgbaFrame& frame,
        uint32_t targetAreaWidth, uint32_t targetAreaHeight, const RenderViewport& viewport,
        DirtyFrameStats& dirty) const;
    void RecordBufferFrame(uintptr_t bufferKey, const RgbaFrame& frame);

    static void CloseFence(int fenceFd);
    static bool WaitFenceAndClose(int fenceFd, std::string& error);

    static constexpr size_t kDirtyHistoryLimit = 240;

    void* configuredWindow_ = nullptr;
    int32_t configuredWidth_ = 0;
    int32_t configuredHeight_ = 0;
    int32_t configuredFormat_ = 0;
    uint64_t configuredUsage_ = 0;
    std::unordered_map<uintptr_t, uint64_t> bufferFrameSequences_;
    std::deque<DirtyHistoryEntry> dirtyHistory_;
    uint32_t dirtyHistoryWidth_ = 0;
    uint32_t dirtyHistoryHeight_ = 0;
};

} // namespace rdp_bridge
