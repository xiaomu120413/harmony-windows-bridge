#include "surface/surface_bridge.h"

#include "bridge_log.h"
#include "surface/avc444_surface_pool.h"
#include "surface/gpu_rgba_renderer.h"
#include "surface/native_window_rgba_painter.h"

#include <algorithm>
#include <limits>
#include <mutex>
#include <string>
#include <utility>

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
        RenderViewport viewport;
        auto* nativeWindow = static_cast<OHNativeWindow*>(window_);
        SurfacePaintResult result = nativePainter_.Render(nativeWindow, width_, height_, frame, viewport);
        if (result.ok) {
            ++paintCount_;
            viewportX_ = viewport.x;
            viewportY_ = viewport.y;
            viewportWidth_ = viewport.width;
            viewportHeight_ = viewport.height;
        }
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
        nativePainter_.Destroy();
        gpuFallbackLogged_ = false;
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
    GpuRgbaRenderer gpuRenderer_;
    NativeWindowRgbaPainter nativePainter_;
    GpuAvc444SurfacePool avc444Surfaces_;
    bool gpuFallbackLogged_ = false;
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
