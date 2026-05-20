#include "surface/native_rgba_copy.h"

#include <algorithm>
#include <cstring>

namespace rdp_bridge {
namespace {

constexpr uint32_t kOneToOneFitTolerancePx = 16;

uint32_t DimensionDelta(uint32_t a, uint32_t b)
{
    return a > b ? a - b : b - a;
}

void CopyRgbaPixelToNative(uint8_t* pixel, int32_t format, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 || format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888) {
        pixel[0] = b;
        pixel[1] = g;
        pixel[2] = r;
        pixel[3] = a;
        return;
    }

    pixel[0] = r;
    pixel[1] = g;
    pixel[2] = b;
    pixel[3] = a;
}

void FillNativeRect(const BufferHandle& handle, int32_t rowBytes,
    uint32_t x, uint32_t y, uint32_t width, uint32_t height,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    if (width == 0 || height == 0) {
        return;
    }

    auto* target = static_cast<uint8_t*>(handle.virAddr);
    for (uint32_t row = 0; row < height; ++row) {
        uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (y + row) +
            static_cast<int64_t>(x) * 4;
        for (uint32_t column = 0; column < width; ++column) {
            CopyRgbaPixelToNative(targetRow + column * 4, handle.format, r, g, b, a);
        }
    }
}

void CopyRgbaToNative(const BufferHandle& handle, int32_t rowBytes,
    const uint8_t* source, int32_t sourceStride, uint32_t width, uint32_t height)
{
    auto* target = static_cast<uint8_t*>(handle.virAddr);
    if (handle.format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
        handle.format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888) {
        const size_t bytesPerRow = static_cast<size_t>(width) * 4U;
        for (uint32_t y = 0; y < height; ++y) {
            std::memcpy(target + static_cast<int64_t>(rowBytes) * y,
                source + static_cast<int64_t>(sourceStride) * y, bytesPerRow);
        }
        return;
    }

    for (uint32_t y = 0; y < height; ++y) {
        uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * y;
        const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * y;
        for (uint32_t x = 0; x < width; ++x) {
            const uint8_t* sourcePixel = sourceRow + x * 4;
            CopyRgbaPixelToNative(targetRow + x * 4, handle.format, sourcePixel[0],
                sourcePixel[1], sourcePixel[2], sourcePixel[3]);
        }
    }
}

} // namespace

bool IsSupportedFourByteFormat(int32_t format)
{
    return format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 ||
        format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888;
}

int32_t ResolveRowBytes(const BufferHandle& handle, uint32_t drawWidth, uint32_t drawHeight)
{
    if (drawWidth == 0 || drawHeight == 0) {
        return 0;
    }

    const int64_t tightRowBytes = static_cast<int64_t>(drawWidth) * 4;
    const int64_t stride = handle.stride > 0 ? handle.stride : handle.width;
    const int64_t pixelStrideRowBytes = stride * 4;
    const int64_t byteStrideRowBytes = stride;
    const int64_t size = handle.size;

    if (byteStrideRowBytes >= tightRowBytes && (size <= 0 || byteStrideRowBytes * drawHeight <= size)) {
        return static_cast<int32_t>(byteStrideRowBytes);
    }
    if (pixelStrideRowBytes >= tightRowBytes && (size <= 0 || pixelStrideRowBytes * drawHeight <= size)) {
        return static_cast<int32_t>(pixelStrideRowBytes);
    }
    if (size <= 0 || tightRowBytes * drawHeight <= size) {
        return static_cast<int32_t>(tightRowBytes);
    }
    return 0;
}

RenderViewport FitFrameIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
    uint32_t sourceWidth, uint32_t sourceHeight)
{
    RenderViewport viewport;
    if (targetWidth == 0 || targetHeight == 0 || sourceWidth == 0 || sourceHeight == 0) {
        return viewport;
    }

    if (sourceWidth <= targetWidth && sourceHeight <= targetHeight &&
        DimensionDelta(targetWidth, sourceWidth) <= kOneToOneFitTolerancePx &&
        DimensionDelta(targetHeight, sourceHeight) <= kOneToOneFitTolerancePx) {
        viewport.width = sourceWidth;
        viewport.height = sourceHeight;
        viewport.x = (targetWidth - viewport.width) / 2U;
        viewport.y = (targetHeight - viewport.height) / 2U;
        return viewport;
    }

    const uint64_t targetBySourceHeight = static_cast<uint64_t>(targetWidth) * sourceHeight;
    const uint64_t targetHeightBySourceWidth = static_cast<uint64_t>(targetHeight) * sourceWidth;
    if (targetBySourceHeight <= targetHeightBySourceWidth) {
        viewport.width = targetWidth;
        viewport.height = static_cast<uint32_t>(
            std::max<uint64_t>(1, targetBySourceHeight / sourceWidth));
    } else {
        viewport.height = targetHeight;
        viewport.width = static_cast<uint32_t>(
            std::max<uint64_t>(1, targetHeightBySourceWidth / sourceHeight));
    }

    viewport.width = std::min(viewport.width, targetWidth);
    viewport.height = std::min(viewport.height, targetHeight);
    viewport.x = (targetWidth - viewport.width) / 2U;
    viewport.y = (targetHeight - viewport.height) / 2U;
    return viewport;
}

void FillNativeLetterbox(const BufferHandle& handle, int32_t rowBytes,
    uint32_t width, uint32_t height, const RenderViewport& viewport,
    uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    FillNativeRect(handle, rowBytes, 0, 0, width, viewport.y, r, g, b, a);
    const uint32_t bottomY = viewport.y + viewport.height;
    if (bottomY < height) {
        FillNativeRect(handle, rowBytes, 0, bottomY, width, height - bottomY, r, g, b, a);
    }
    FillNativeRect(handle, rowBytes, 0, viewport.y, viewport.x, viewport.height, r, g, b, a);
    const uint32_t rightX = viewport.x + viewport.width;
    if (rightX < width) {
        FillNativeRect(handle, rowBytes, rightX, viewport.y, width - rightX,
            viewport.height, r, g, b, a);
    }
}

void CopyRgbaRectToNative(const BufferHandle& handle, int32_t rowBytes,
    const uint8_t* source, int32_t sourceStride, uint32_t x, uint32_t y,
    uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0) {
        return;
    }

    auto* target = static_cast<uint8_t*>(handle.virAddr);
    if (handle.format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
        handle.format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888) {
        const size_t bytesPerRow = static_cast<size_t>(width) * 4U;
        for (uint32_t row = 0; row < height; ++row) {
            std::memcpy(target + static_cast<int64_t>(rowBytes) * (y + row) +
                static_cast<int64_t>(x) * 4,
                source + static_cast<int64_t>(sourceStride) * (y + row) +
                static_cast<int64_t>(x) * 4,
                bytesPerRow);
        }
        return;
    }

    for (uint32_t row = 0; row < height; ++row) {
        uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (y + row) +
            static_cast<int64_t>(x) * 4;
        const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * (y + row) +
            static_cast<int64_t>(x) * 4;
        for (uint32_t column = 0; column < width; ++column) {
            const uint8_t* sourcePixel = sourceRow + column * 4;
            CopyRgbaPixelToNative(targetRow + column * 4, handle.format, sourcePixel[0],
                sourcePixel[1], sourcePixel[2], sourcePixel[3]);
        }
    }
}

void CopyScaledRgbaToNative(const BufferHandle& handle, int32_t rowBytes,
    const uint8_t* source, int32_t sourceStride, uint32_t sourceWidth,
    uint32_t sourceHeight, const RenderViewport& viewport)
{
    if (sourceWidth == viewport.width && sourceHeight == viewport.height &&
        viewport.x == 0 && viewport.y == 0) {
        CopyRgbaToNative(handle, rowBytes, source, sourceStride, sourceWidth, sourceHeight);
        return;
    }

    auto* target = static_cast<uint8_t*>(handle.virAddr);
    for (uint32_t y = 0; y < viewport.height; ++y) {
        const uint32_t sourceY = static_cast<uint32_t>(
            (static_cast<uint64_t>(y) * sourceHeight) / viewport.height);
        uint8_t* targetRow = target + static_cast<int64_t>(rowBytes) * (viewport.y + y) +
            static_cast<int64_t>(viewport.x) * 4;
        const uint8_t* sourceRow = source + static_cast<int64_t>(sourceStride) * sourceY;
        for (uint32_t x = 0; x < viewport.width; ++x) {
            const uint32_t sourceX = static_cast<uint32_t>(
                (static_cast<uint64_t>(x) * sourceWidth) / viewport.width);
            const uint8_t* sourcePixel = sourceRow + sourceX * 4;
            CopyRgbaPixelToNative(targetRow + x * 4, handle.format, sourcePixel[0],
                sourcePixel[1], sourcePixel[2], sourcePixel[3]);
        }
    }
}

} // namespace rdp_bridge
