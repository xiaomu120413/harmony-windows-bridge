#include "surface/surface_bridge.h"

#include "bridge_log.h"
#include "frame_utils.h"
#include "net_utils.h"
#include "string_utils.h"
#include "surface/avc444_surface_pool.h"
#include "surface/gpu_rgba_renderer.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <limits>
#include <mutex>
#include <poll.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <unistd.h>
#include <vector>

#include <native_buffer/native_buffer.h>
#include <native_window/external_window.h>

namespace rdp_bridge {

std::string ReadXComponentId(OH_NativeXComponent* component)
{
    char id[OH_XCOMPONENT_ID_LEN_MAX + 1] = {};
    uint64_t size = sizeof(id);
    int32_t rc = OH_NativeXComponent_GetXComponentId(component, id, &size);
    if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS || size == 0) {
        return "unknown";
    }
    return std::string(id);
}

class SurfaceBridge::Impl {
public:
    void SetLogSink(SurfaceBridge::LogFn log)
    {
        std::lock_guard<std::mutex> lock(logMutex_);
        log_ = std::move(log);
    }

    void Register(OH_NativeXComponent* component, bool ok)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        component_ = component;
        registered_ = ok;
        id_ = component == nullptr ? "" : ReadXComponentId(component);
    }

    void OnSurfaceCreated(OH_NativeXComponent* component, void* window)
    {
        uint64_t width = 0;
        uint64_t height = 0;
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = window;
            registered_ = true;
            ready_ = window != nullptr;
            id_ = ReadXComponentId(component);
            width_ = static_cast<uint32_t>(width);
            height_ = static_cast<uint32_t>(height);
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++createdCount_;
            snapshot = SnapshotLocked();
        }

        Log("XComponent surface created: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height));
    }

    void OnSurfaceChanged(OH_NativeXComponent* component, void* window)
    {
        uint64_t width = 0;
        uint64_t height = 0;
        OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = window;
            ready_ = window != nullptr;
            id_ = ReadXComponentId(component);
            width_ = static_cast<uint32_t>(width);
            height_ = static_cast<uint32_t>(height);
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        Log("XComponent surface changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height));
    }

    bool OnSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
    {
        if (width == 0 || height == 0) {
            message = "XComponent layout size is invalid";
            return false;
        }

        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (width_ == width && height_ == height) {
                message = "XComponent layout unchanged: " + std::to_string(width) + "x" +
                    std::to_string(height);
                return false;
            }

            width_ = width;
            height_ = height;
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        message = "XComponent layout changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height);
        return true;
    }

    void OnSurfaceDestroyed(OH_NativeXComponent* component, void*)
    {
        SurfaceSnapshot snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            component_ = component;
            window_ = nullptr;
            ready_ = false;
            id_ = ReadXComponentId(component);
            width_ = 0;
            height_ = 0;
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++destroyedCount_;
            snapshot = SnapshotLocked();
        }

        Log("XComponent surface destroyed: " + snapshot.id);
    }

    void OnTouchEvent()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++touchCount_;
    }

    SurfacePaintResult RenderRgbaFrame(const RgbaFrame& frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return RenderRgbaFrameLocked(frame);
    }

    SurfaceSnapshot Snapshot()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return SnapshotLocked();
    }

    DecoderSurfaceTarget DecoderSurface()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            return {};
        }
        return {static_cast<OHNativeWindow*>(window_), width_, height_};
    }

    bool EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height, Avc444SurfaceTargets& targets,
        std::string& error)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || window_ == nullptr) {
            error = "XComponent surface is not ready";
            return false;
        }
        return avc444Surfaces_.Ensure(width, height, targets, error);
    }

private:
    struct DirtyHistoryEntry {
        uint64_t fromSequence = 0;
        uint64_t toSequence = 0;
        DirtyFrameStats dirty;
    };



    static constexpr size_t kDirtyHistoryLimit = 240;

    SurfacePaintResult RenderRgbaFrameLocked(const RgbaFrame& frame)
    {
        SurfacePaintResult result;
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            result.message = "XComponent surface is not ready for render";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0) {
            result.message = "RGBA frame is empty";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            result.message = "RGBA frame stride is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        constexpr uint32_t maxTargetSize = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
        if (width_ > maxTargetSize || height_ > maxTargetSize) {
            result.message = "NativeWindow target geometry exceeds int32 range";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        const RenderViewport viewport = FitFrameIntoTarget(width_, height_, frame.width, frame.height);
        if (viewport.width == 0 || viewport.height == 0) {
            result.message = "render viewport is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        if (gpuRenderer_.Render(nativeWindow, width_, height_, frame, sourceStride, viewport, result)) {
            ++paintCount_;
            viewportX_ = viewport.x;
            viewportY_ = viewport.y;
            viewportWidth_ = viewport.width;
            viewportHeight_ = viewport.height;
            lastPaintMessage_ = result.message;
            return result;
        }

        if (!gpuFallbackLogged_ && !result.logs.empty()) {
            Log("GLES render fallback to CPU: " + result.logs.back());
            gpuFallbackLogged_ = true;
        }
        return RenderRgbaFrameCpuLocked(frame);
    }

    SurfacePaintResult RenderRgbaFrameCpuLocked(const RgbaFrame& frame)
    {
        SurfacePaintResult result;
        if (!ready_ || window_ == nullptr || width_ == 0 || height_ == 0) {
            result.message = "XComponent surface is not ready for render";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        if (frame.data == nullptr || frame.width == 0 || frame.height == 0) {
            result.message = "RGBA frame is empty";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
            static_cast<int32_t>(frame.width * 4U);
        if (sourceStride < static_cast<int32_t>(frame.width * 4U)) {
            result.message = "RGBA frame stride is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        constexpr uint32_t maxTargetSize = static_cast<uint32_t>(std::numeric_limits<int32_t>::max());
        if (width_ > maxTargetSize || height_ > maxTargetSize) {
            result.message = "NativeWindow target geometry exceeds int32 range";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const int32_t targetWidth = static_cast<int32_t>(width_);
        const int32_t targetHeight = static_cast<int32_t>(height_);

        if (!ConfigureNativeWindowLocked(nativeWindow, targetWidth, targetHeight, result)) {
            return result;
        }

        OHNativeWindowBuffer* buffer = nullptr;
        int fenceFd = -1;
        int32_t rc = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &buffer, &fenceFd);
        if (rc != 0 || buffer == nullptr) {
            CloseFence(fenceFd);
            result.message = "NativeWindow request buffer failed: " + std::to_string(rc);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        std::string fenceError;
        if (!WaitFenceAndClose(fenceFd, fenceError)) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow fence wait failed: " + fenceError;
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
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
            lastPaintMessage_ = result.message;
            return result;
        }

        if (!IsSupportedFourByteFormat(handle->format)) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer format is not supported: " + std::to_string(handle->format);
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }

        const uint32_t bufferWidth = handle->width > 0 ? static_cast<uint32_t>(handle->width) : width_;
        const uint32_t bufferHeight = handle->height > 0 ? static_cast<uint32_t>(handle->height) : height_;
        const uint32_t targetAreaWidth = std::min(width_, bufferWidth);
        const uint32_t targetAreaHeight = std::min(height_, bufferHeight);
        const int32_t rowBytes = ResolveRowBytes(*handle, targetAreaWidth, targetAreaHeight);
        if (targetAreaWidth == 0 || targetAreaHeight == 0 || rowBytes <= 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "NativeWindow buffer geometry is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
            return result;
        }
        const RenderViewport bufferViewport = FitFrameIntoTarget(
            targetAreaWidth, targetAreaHeight, frame.width, frame.height);
        if (bufferViewport.width == 0 || bufferViewport.height == 0) {
            OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
            result.message = "render viewport is invalid";
            result.logs.push_back(result.message);
            lastPaintMessage_ = result.message;
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
                lastPaintMessage_ = result.message;
                return result;
            }

            rc = OH_NativeBuffer_Map(nativeBuffer, &mappedAddress);
            if (rc != 0 || mappedAddress == nullptr) {
                OH_NativeWindow_NativeWindowAbortBuffer(nativeWindow, buffer);
                result.message = "NativeBuffer map failed: " + std::to_string(rc);
                result.logs.push_back(result.message);
                lastPaintMessage_ = result.message;
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
            lastPaintMessage_ = result.message;
            return result;
        }

        BufferHandle mappedHandle = *handle;
        mappedHandle.virAddr = mappedAddress;
        const uintptr_t bufferKey = reinterpret_cast<uintptr_t>(buffer);
        DirtyFrameStats partialDirty;
        const bool canUsePartialDirty = CanUsePartialDirtyLocked(bufferKey, frame,
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
            lastPaintMessage_ = result.message;
            return result;
        }

        ++paintCount_;
        RecordBufferFrameLocked(bufferKey, frame);
        result.partial = canUsePartialDirty;
        viewportX_ = bufferViewport.x;
        viewportY_ = bufferViewport.y;
        viewportWidth_ = bufferViewport.width;
        viewportHeight_ = bufferViewport.height;
        result.ok = true;
        const std::string frameLabel = frame.label.empty() ? "frame" : frame.label;
        result.message = "NativeWindow RGBA frame rendered: " + frameLabel + " " +
            std::to_string(bufferViewport.width) + "x" + std::to_string(bufferViewport.height) +
            " bufferViewport=" + std::to_string(bufferViewport.x) + "," +
            std::to_string(bufferViewport.y) + " displayViewport=" +
            std::to_string(viewportX_) + "," + std::to_string(viewportY_) + " " +
            std::to_string(viewportWidth_) + "x" + std::to_string(viewportHeight_) +
            (canUsePartialDirty ? " mode=dirty-bbox " + DescribeDirtyStats(partialDirty) : " mode=full");
        result.logs.push_back(result.message);
        result.logs.push_back("RGBA source=" + std::to_string(frame.width) + "x" +
            std::to_string(frame.height) + " stride=" + std::to_string(sourceStride));
        result.logs.push_back("NativeWindow format=" + std::to_string(handle->format) +
            " stride=" + std::to_string(handle->stride) +
            " rowBytes=" + std::to_string(mappedRowBytes) +
            " directVirAddr=" + std::string(mappedNativeBuffer ? "false" : "true"));
        lastPaintMessage_ = result.message;
        return result;
    }

    void ClearViewportLocked()
    {
        viewportX_ = 0;
        viewportY_ = 0;
        viewportWidth_ = 0;
        viewportHeight_ = 0;
    }

    void ClearNativeWindowConfigLocked()
    {
        gpuRenderer_.Destroy();
        avc444Surfaces_.Destroy();
        gpuFallbackLogged_ = false;
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

    bool CanUsePartialDirtyLocked(uintptr_t bufferKey, const RgbaFrame& frame,
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

    void RecordBufferFrameLocked(uintptr_t bufferKey, const RgbaFrame& frame)
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

    bool ConfigureNativeWindowLocked(OHNativeWindow* nativeWindow, int32_t targetWidth,
        int32_t targetHeight, SurfacePaintResult& result)
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
                lastPaintMessage_ = result.message;
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

    static void CloseFence(int fenceFd)
    {
        if (fenceFd >= 0) {
            ::close(fenceFd);
        }
    }

    static bool WaitFenceAndClose(int fenceFd, std::string& error)
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

    static bool IsSupportedFourByteFormat(int32_t format)
    {
        return format == NATIVEBUFFER_PIXEL_FMT_RGBA_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_RGBX_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_BGRA_8888 ||
            format == NATIVEBUFFER_PIXEL_FMT_BGRX_8888;
    }

    static int32_t ResolveRowBytes(const BufferHandle& handle, uint32_t drawWidth, uint32_t drawHeight)
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

    static RenderViewport FitFrameIntoTarget(uint32_t targetWidth, uint32_t targetHeight,
        uint32_t sourceWidth, uint32_t sourceHeight)
    {
        RenderViewport viewport;
        if (targetWidth == 0 || targetHeight == 0 || sourceWidth == 0 || sourceHeight == 0) {
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

    static void WriteRgbaPixel(uint8_t* pixel, uint8_t r, uint8_t g, uint8_t b)
    {
        pixel[0] = r;
        pixel[1] = g;
        pixel[2] = b;
        pixel[3] = 0xFF;
    }

    static void CopyRgbaPixelToNative(uint8_t* pixel, int32_t format, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
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

    static void FillNativeRect(const BufferHandle& handle, int32_t rowBytes, uint32_t x,
        uint32_t y, uint32_t width, uint32_t height, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
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

    static void FillNativeLetterbox(const BufferHandle& handle, int32_t rowBytes,
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

    static void CopyRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t width, uint32_t height)
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

    static void CopyRgbaRectToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
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

    static void CopyScaledRgbaToNative(const BufferHandle& handle, int32_t rowBytes, const uint8_t* source,
        int32_t sourceStride, uint32_t sourceWidth, uint32_t sourceHeight, const RenderViewport& viewport)
    {
        auto* target = static_cast<uint8_t*>(handle.virAddr);
        if (sourceWidth == viewport.width && sourceHeight == viewport.height &&
            viewport.x == 0 && viewport.y == 0) {
            CopyRgbaToNative(handle, rowBytes, source, sourceStride, sourceWidth, sourceHeight);
            return;
        }

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

    SurfaceSnapshot SnapshotLocked() const
    {
        return SurfaceSnapshot{
            registered_,
            ready_,
            id_.empty() ? "unknown" : id_,
            width_,
            height_,
            viewportX_,
            viewportY_,
            viewportWidth_,
            viewportHeight_,
            createdCount_,
            changedCount_,
            destroyedCount_,
            touchCount_,
            paintCount_,
            lastPaintMessage_,
        };
    }

    void Log(const std::string& line)
    {
        SurfaceBridge::LogFn sink;
        {
            std::lock_guard<std::mutex> lock(logMutex_);
            sink = log_;
        }
        if (sink) {
            sink(line);
        } else {
            EmitHilogInfo(line);
        }
    }

    std::mutex logMutex_;
    SurfaceBridge::LogFn log_;    std::mutex mutex_;
    OH_NativeXComponent* component_ = nullptr;
    void* window_ = nullptr;
    bool registered_ = false;
    bool ready_ = false;
    std::string id_;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
    uint32_t viewportX_ = 0;
    uint32_t viewportY_ = 0;
    uint32_t viewportWidth_ = 0;
    uint32_t viewportHeight_ = 0;
    uint32_t createdCount_ = 0;
    uint32_t changedCount_ = 0;
    uint32_t destroyedCount_ = 0;
    uint32_t touchCount_ = 0;
    uint32_t paintCount_ = 0;
    void* configuredWindow_ = nullptr;
    int32_t configuredWidth_ = 0;
    int32_t configuredHeight_ = 0;
    int32_t configuredFormat_ = 0;
    uint64_t configuredUsage_ = 0;
    GpuRgbaRenderer gpuRenderer_;
    GpuAvc444SurfacePool avc444Surfaces_;
    bool gpuFallbackLogged_ = false;
    std::unordered_map<uintptr_t, uint64_t> bufferFrameSequences_;
    std::deque<DirtyHistoryEntry> dirtyHistory_;
    uint32_t dirtyHistoryWidth_ = 0;
    uint32_t dirtyHistoryHeight_ = 0;
    std::string lastPaintMessage_;
};
SurfaceBridge::SurfaceBridge() : impl_(std::make_unique<Impl>()) {}

SurfaceBridge::~SurfaceBridge() = default;

void SurfaceBridge::SetLogSink(LogFn log)
{
    impl_->SetLogSink(std::move(log));
}

void SurfaceBridge::Register(OH_NativeXComponent* component, bool ok)
{
    impl_->Register(component, ok);
}

void SurfaceBridge::OnSurfaceCreated(OH_NativeXComponent* component, void* window)
{
    impl_->OnSurfaceCreated(component, window);
}

void SurfaceBridge::OnSurfaceChanged(OH_NativeXComponent* component, void* window)
{
    impl_->OnSurfaceChanged(component, window);
}

bool SurfaceBridge::OnSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
{
    return impl_->OnSurfaceLayout(width, height, message);
}

void SurfaceBridge::OnSurfaceDestroyed(OH_NativeXComponent* component, void* window)
{
    impl_->OnSurfaceDestroyed(component, window);
}

void SurfaceBridge::OnTouchEvent()
{
    impl_->OnTouchEvent();
}

SurfacePaintResult SurfaceBridge::RenderRgbaFrame(const RgbaFrame& frame)
{
    return impl_->RenderRgbaFrame(frame);
}

SurfaceSnapshot SurfaceBridge::Snapshot()
{
    return impl_->Snapshot();
}

DecoderSurfaceTarget SurfaceBridge::DecoderSurface()
{
    return impl_->DecoderSurface();
}

bool SurfaceBridge::EnsureAvc444SurfaceTargets(uint32_t width, uint32_t height,
    Avc444SurfaceTargets& targets, std::string& error)
{
    return impl_->EnsureAvc444SurfaceTargets(width, height, targets, error);
}

} // namespace rdp_bridge
