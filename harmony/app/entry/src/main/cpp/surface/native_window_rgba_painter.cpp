#include "surface/native_window_rgba_painter.h"

#include "frame_utils.h"
#include "net_utils.h"
#include "surface/native_rgba_copy.h"

#include <algorithm>
#include <cerrno>
#include <limits>
#include <poll.h>
#include <unistd.h>

namespace rdp_bridge {

void NativeWindowRgbaPainter::Destroy()
{
    configuredWindow_ = nullptr;
    configuredWidth_ = 0;
    configuredHeight_ = 0;
    configuredFormat_ = 0;
    configuredUsage_ = 0;
    bufferFrameSequences_.clear();
    dirtyHistory_.clear();
    dirtyHistoryWidth_ = 0;
    dirtyHistoryHeight_ = 0;
}

SurfacePaintResult NativeWindowRgbaPainter::Render(OHNativeWindow* nativeWindow,
    uint32_t targetWidth, uint32_t targetHeight, const RgbaFrame& frame, RenderViewport& renderedViewport)
{
    SurfacePaintResult result;
    if (nativeWindow == nullptr || targetWidth == 0 || targetHeight == 0) {
        result.message = "XComponent surface is not ready for render";
        result.logs.push_back(result.message);
        return result;
    }
    if (frame.data == nullptr || frame.width == 0 || frame.height == 0) {
        result.message = "RGBA frame is empty";
        result.logs.push_back(result.message);
        return result;
    }
    const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
        static_cast<int32_t>(frame.width * 4U);
    if (sourceStride < static_cast<int32_t>(frame.width * 4U)) {
        result.message = "RGBA frame stride is invalid";
        result.logs.push_back(result.message);
        return result;
    }

    constexpr uint32_t maxTargetSize = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
    if (targetWidth > maxTargetSize || targetHeight > maxTargetSize) {
        result.message = "NativeWindow target geometry exceeds int32 range";
        result.logs.push_back(result.message);
        return result;
    }
    const int32_t targetWidth32 = static_cast<int32_t>(targetWidth);
    const int32_t targetHeight32 = static_cast<int32_t>(targetHeight);

    if (!ConfigureNativeWindow(nativeWindow, targetWidth32, targetHeight32, result)) {
        return result;
    }

    OHNativeWindowBuffer* buffer = nullptr;
    int fenceFd = -1;
    int32_t rc = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &buffer, &fenceFd);
    if (rc != 0 || buffer == nullptr) {
        CloseFence(fenceFd);
        result.message = "NativeWindow request buffer failed: " + std::to_string(rc);
        result.logs.push_back(result.message);
        return result;
    }

    std::string fenceError;
    if (!WaitFenceAndClose(fenceFd, fenceError)) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeWindow fence wait failed: " + fenceError;
        result.logs.push_back(result.message);
        return result;
    }
    fenceFd = -1;

    BufferHandle* handle = OH_NativeWindow_GetBufferHandleFromNative(buffer);
    if (handle == nullptr || handle->virAddr == nullptr) {
        result.logs.push_back("NativeWindow BufferHandle has no direct CPU address; using NativeBuffer map");
    }
    if (handle == nullptr) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeWindow buffer handle is null";
        result.logs.push_back(result.message);
        return result;
    }

    if (!IsSupportedFourByteFormat(handle->format)) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeWindow buffer format is not supported: " + std::to_string(handle->format);
        result.logs.push_back(result.message);
        return result;
    }

    const uint32_t bufferWidth = handle->width > 0 ? static_cast<uint32_t>(handle->width) : targetWidth;
    const uint32_t bufferHeight = handle->height > 0 ? static_cast<uint32_t>(handle->height) : targetHeight;
    const uint32_t targetAreaWidth = std::min(targetWidth, bufferWidth);
    const uint32_t targetAreaHeight = std::min(targetHeight, bufferHeight);
    const int32_t rowBytes = ResolveRowBytes(*handle, targetAreaWidth, targetAreaHeight);
    if (targetAreaWidth == 0 || targetAreaHeight == 0 || rowBytes <= 0) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeWindow buffer geometry is invalid";
        result.logs.push_back(result.message);
        return result;
    }
    const RenderViewport bufferViewport = FitFrameIntoTarget(
        targetAreaWidth, targetAreaHeight, frame.width, frame.height);
    if (bufferViewport.width == 0 || bufferViewport.height == 0) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "render viewport is invalid";
        result.logs.push_back(result.message);
        return result;
    }

    OH_NativeBuffer* nativeBuffer = nullptr;
    void* mappedAddress = handle->virAddr;
    int32_t mappedRowBytes = rowBytes;
    bool mappedNativeBuffer = false;
    if (mappedAddress == nullptr) {
        rc = OH_NativeBuffer_FromNativeWindowBuffer(buffer, &nativeBuffer);
        if (rc != 0 || nativeBuffer == nullptr) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer conversion failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            return result;
        }

        rc = OH_NativeBuffer_Map(nativeBuffer, &mappedAddress);
        if (rc != 0 || mappedAddress == nullptr) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeBuffer map failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            return result;
        }
        mappedNativeBuffer = true;

        OH_NativeBuffer_Config config = {};
        OH_NativeBuffer_GetConfig(nativeBuffer, &config);
        if (config.stride >= static_cast<int32_t>(targetAreaWidth * 4U)) {
            mappedRowBytes = config.stride;
        }
    }

    if (mappedRowBytes < static_cast<int32_t>(targetAreaWidth * 4U)) {
        if (mappedNativeBuffer) {
            OH_NativeBuffer_Unmap(nativeBuffer);
        }
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeBuffer row stride is invalid: " + std::to_string(mappedRowBytes);
        result.logs.push_back(result.message);
        return result;
    }

    BufferHandle mappedHandle = *handle;
    mappedHandle.virAddr = mappedAddress;
    const uintptr_t bufferKey = reinterpret_cast<uintptr_t>(buffer);
    DirtyFrameStats partialDirty;
    const bool canUsePartialDirty = CanUsePartialDirty(bufferKey, frame,
        targetAreaWidth, targetAreaHeight, bufferViewport, partialDirty);
    if (canUsePartialDirty) {
        CopyRgbaRectToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride,
            partialDirty.x, partialDirty.y, partialDirty.width, partialDirty.height);
    } else {
        FillNativeLetterbox(mappedHandle, mappedRowBytes, targetAreaWidth, targetAreaHeight,
            bufferViewport, 0, 0, 0, 0xFF);
        CopyScaledRgbaToNative(mappedHandle, mappedRowBytes, frame.data, sourceStride,
            frame.width, frame.height, bufferViewport);
    }
    if (mappedNativeBuffer) {
        OH_NativeBuffer_Unmap(nativeBuffer);
    }

    Region::Rect dirtyRect = canUsePartialDirty ?
        Region::Rect{static_cast<int32_t>(partialDirty.x), static_cast<int32_t>(partialDirty.y),
            partialDirty.width, partialDirty.height} :
        Region::Rect{0, 0, targetAreaWidth, targetAreaHeight};
    Region dirtyRegion = {&dirtyRect, 1};
    rc = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, buffer, -1, dirtyRegion);
    if (rc != 0) {
        OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
        result.message = "NativeWindow flush buffer failed: " + std::to_string(rc);
        result.logs.push_back(result.message);
        return result;
    }

    RecordBufferFrame(bufferKey, frame);
    renderedViewport = bufferViewport;
    result.partial = canUsePartialDirty;
    result.ok = true;
    const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
    result.message = "NativeWindow RGBA frame rendered: " + frameLabel + " " +
        std::to_string(bufferViewport.width) + "x" + std::to_string(bufferViewport.height) +
        " bufferViewport=" + std::to_string(bufferViewport.x) + "," +
        std::to_string(bufferViewport.y) + " displayViewport=" +
        std::to_string(renderedViewport.x) + "," + std::to_string(renderedViewport.y) + " " +
        std::to_string(renderedViewport.width) + "x" + std::to_string(renderedViewport.height) +
        (canUsePartialDirty ? " mode=dirty-bbox " + DescribeDirtyStats(partialDirty) : " mode=full");
    result.logs.push_back(result.message);
    result.logs.push_back("RGBA source=" + std::to_string(frame.width) + "x" +
        std::to_string(frame.height) + " stride=" + std::to_string(sourceStride));
    result.logs.push_back("NativeWindow format=" + std::to_string(handle->format) +
        " stride=" + std::to_string(handle->stride) +
        " rowBytes=" + std::to_string(mappedRowBytes) +
        " directVirAddr=" + std::string(mappedNativeBuffer ? "false" : "true"));
    return result;
}

bool NativeWindowRgbaPainter::ConfigureNativeWindow(OHNativeWindow* nativeWindow,
    int32_t targetWidth, int32_t targetHeight, SurfacePaintResult& result)
{
    if (configuredWindow_ != nativeWindow) {
        configuredWindow_ = nativeWindow;
        configuredWidth_ = 0;
        configuredHeight_ = 0;
        configuredFormat_ = 0;
        configuredUsage_ = 0;
    }

    if (configuredWidth_ != targetWidth || configuredHeight_ != targetHeight) {
        const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(
            nativeWindow, SET_BUFFER_GEOMETRY, targetWidth, targetHeight);
        if (rc != 0) {
            result.message = "NativeWindow SET_BUFFER_GEOMETRY failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            return false;
        }
        configuredWidth_ = targetWidth;
        configuredHeight_ = targetHeight;
    }

    constexpr int32_t format = static_cast<int32_t>(NATIVEBUFFER_PIXEL_FMT_RGBA_8888);
    if (configuredFormat_ != format) {
        const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, format);
        if (rc != 0) {
            result.logs.push_back("NativeWindow SET_FORMAT warning: " + std::to_string(rc));
        } else {
            configuredFormat_ = format;
        }
    }

    constexpr uint64_t usage = NATIVEBUFFER_USAGE_CPU_WRITE | NATIVEBUFFER_USAGE_MEM_DMA |
        NATIVEBUFFER_USAGE_HW_TEXTURE;
    if (configuredUsage_ != usage) {
        const int32_t rc = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_USAGE, usage);
        if (rc != 0) {
            result.logs.push_back("NativeWindow SET_USAGE warning: " + std::to_string(rc));
        } else {
            configuredUsage_ = usage;
        }
    }

    return true;
}

bool NativeWindowRgbaPainter::CanUsePartialDirty(uintptr_t bufferKey, const RgbaFrame& frame,
    uint32_t targetAreaWidth, uint32_t targetAreaHeight, const RenderViewport& viewport,
    DirtyFrameStats& dirty) const
{
    constexpr uint32_t kMaxPartialDirtyAreaPermille = 650;
    if (bufferKey == 0 || frame.sequence == 0 || frame.dirtySequenceStart == 0 || !frame.dirty.valid) {
        return false;
    }
    if (frame.width != targetAreaWidth || frame.height != targetAreaHeight ||
        viewport.x != 0 || viewport.y != 0 ||
        viewport.width != frame.width || viewport.height != frame.height) {
        return false;
    }
    if (dirtyHistoryWidth_ != frame.width || dirtyHistoryHeight_ != frame.height) {
        return false;
    }

    const auto bufferIt = bufferFrameSequences_.find(bufferKey);
    if (bufferIt == bufferFrameSequences_.end() || bufferIt->second >= frame.sequence) {
        return false;
    }

    const uint64_t lastBufferSequence = bufferIt->second;
    uint64_t expectedSequence = lastBufferSequence + 1U;
    DirtyFrameStats accumulated;
    for (const DirtyHistoryEntry& entry : dirtyHistory_) {
        if (entry.toSequence <= lastBufferSequence) {
            continue;
        }
        if (entry.fromSequence > expectedSequence) {
            return false;
        }
        accumulated = MergeDirtyStats(accumulated, entry.dirty, frame.width, frame.height);
        expectedSequence = entry.toSequence + 1U;
    }

    if (frame.dirtySequenceStart > expectedSequence) {
        return false;
    }
    accumulated = MergeDirtyStats(accumulated, frame.dirty, frame.width, frame.height);
    if (!accumulated.valid || accumulated.width == 0 || accumulated.height == 0) {
        return false;
    }
    if (accumulated.x + accumulated.width > targetAreaWidth ||
        accumulated.y + accumulated.height > targetAreaHeight) {
        return false;
    }
    if (accumulated.areaPermille > kMaxPartialDirtyAreaPermille) {
        return false;
    }

    dirty = accumulated;
    return true;
}

void NativeWindowRgbaPainter::RecordBufferFrame(uintptr_t bufferKey, const RgbaFrame& frame)
{
    if (frame.sequence == 0) {
        return;
    }
    if (dirtyHistoryWidth_ != frame.width || dirtyHistoryHeight_ != frame.height) {
        dirtyHistory_.clear();
        bufferFrameSequences_.clear();
        dirtyHistoryWidth_ = frame.width;
        dirtyHistoryHeight_ = frame.height;
    }

    DirtyHistoryEntry entry;
    entry.fromSequence = frame.dirtySequenceStart == 0 ? frame.sequence : frame.dirtySequenceStart;
    entry.toSequence = frame.sequence;
    entry.dirty = frame.dirty;
    dirtyHistory_.push_back(entry);
    while (dirtyHistory_.size() > kDirtyHistoryLimit) {
        dirtyHistory_.pop_front();
    }
    if (bufferKey != 0) {
        bufferFrameSequences_[bufferKey] = frame.sequence;
    }
}

void NativeWindowRgbaPainter::CloseFence(int fenceFd)
{
    if (fenceFd >= 0) {
        ::close(fenceFd);
    }
}

bool NativeWindowRgbaPainter::WaitFenceAndClose(int fenceFd, std::string& error)
{
    if (fenceFd < 0) {
        return true;
    }

    pollfd fence = {fenceFd, POLLIN, 0};
    int rc = ::poll(&fence, 1, 3000);
    int savedErrno = errno;
    CloseFence(fenceFd);
    if (rc > 0) {
        return true;
    }
    if (rc == 0) {
        error = "timeout";
        return false;
    }
    error = SystemErrorMessage(savedErrno);
    return false;
}

} // namespace rdp_bridge
