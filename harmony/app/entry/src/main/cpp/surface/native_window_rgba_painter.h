#pragma once

#include "bridge_types.h"
#include "surface/gpu_rgba_renderer.h"

#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>

#include <native_buffer/native_buffer.h>
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
    static bool IsSupportedFourByteFormat(int32_t format);
    static int32_t ResolveRowBytes(const BufferHandle& handle, uint32_t drawWidth, uint32_t drawHeight);
    static RenderViewport FitFrameIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
        uint32_t sourceWidth, uint32_t sourceHeight);
    static void CopyRgbaPixelToNative(uint8_t* pixel, int32_t format, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillNativeRect(const BufferHandle& handle, int32_t rowBytes, uint32_t x,
        uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void FillNativeLetterbox(const BufferHandle& handle, int32_t rowBytes,
        uint32_t width, uint32_t height, const RenderViewport& viewport,
        uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static void CopyRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t width, uint32_t height);
    static void CopyRgbaRectToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
    static void CopyScaledRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t sourceWidth, uint32_t sourceHeight, const RenderViewport& viewport);

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
