#pragma once

#include "surface/gpu_rgba_renderer.h"

#include <cstdint>

#include <native_buffer/native_buffer.h>

namespace rdp_bridge {

bool IsSupportedFourByteFormat(int32_t format);
int32_t ResolveRowBytes(const BufferHandle& handle, uint32_t drawWidth, uint32_t drawHeight);
RenderViewport FitFrameIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t sourceWidth, uint32_t sourceHeight);
void FillNativeLetterbox(const BufferHandle& handle, int32_t rowBytes,
    uint32_t width, uint32_t height, const RenderViewport& viewport,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a);
void CopyRgbaRectToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
    int32_t sourceStride, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void CopyScaledRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
    int32_t sourceStride, uint32_t sourceWidth, uint32_t sourceHeight, const RenderViewport& viewport);

} // namespace rdp_bridge
