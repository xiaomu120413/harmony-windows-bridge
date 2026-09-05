#include "surface/surface_bridge.h"

#include "common/bridge_log.h"
#include "surface/gpu_rgba_renderer.h"
#include "surface/native_rgba_copy.h"
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
        (void)OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

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
        }
    }

    void OnSurfaceChanged(OH_NativeXComponent* component, void* window)
    {
        uint64_t width = 0;
        uint64_t height = 0;
        (void)OH_NativeXComponent_GetXComponentSize(component, window, &width, &height);

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
        }
    }

    bool OnSurfaceLayout(uint32_t width, uint32_t height, std::string& message)
    {
        if (width == 0 || height == 0) {
            message = "XComponent layout size is invalid";
            return false;
        }

        SurfaceSnapshot snapshot;
        uint32_t targetWidth = width;
        uint32_t targetHeight = height;
        bool resolvedNativeSize = false;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            resolvedNativeSize = ResolveNativeSurfaceSizeLocked(targetWidth, targetHeight);
            if (width_ == targetWidth && height_ == targetHeight) {
                message = "XComponent layout unchanged: " + std::to_string(targetWidth) + "x" +
                    std::to_string(targetHeight);
                if (resolvedNativeSize && (targetWidth != width || targetHeight != height)) {
                    message += " requested=" + std::to_string(width) + "x" + std::to_string(height);
                }
                return false;
            }

            width_ = targetWidth;
            height_ = targetHeight;
            ClearNativeWindowConfigLocked();
            ClearViewportLocked();
            ++changedCount_;
            snapshot = SnapshotLocked();
        }

        message = "XComponent layout changed: " + snapshot.id + " " +
            std::to_string(snapshot.width) + "x" + std::to_string(snapshot.height);
        if (resolvedNativeSize && (snapshot.width != width || snapshot.height != height)) {
            message += " requested=" + std::to_string(width) + "x" + std::to_string(height);
        }
        return true;
    }

    void OnSurfaceDestroyed(OH_NativeXComponent* component, void*)
    {
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
        }
    }

    SurfacePaintResult RenderRgbaFrame(const RgbaFrame& frame)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return RenderRgbaFrameLocked(frame);
    }

    void ReleaseRenderTarget(const std::string& reason)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ClearNativeWindowConfigLocked();
        (void)reason;
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

#if defined(RDP_BRIDGE_CPU_ONLY)
        if (!cpuOnlyLogged_) {
            Log("CPU-only recording mode: bypassing EGL/GLES RGBA renderer");
            cpuOnlyLogged_ = true;
        }
        return RenderRgbaFrameCpuLocked(frame);
#else
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
            viewportRemoteWidth_ = frame.width;
            viewportRemoteHeight_ = frame.height;
            lastPaintMessage_ = result.message;
            return result;
        }

        if (!gpuFallbackLogged_ && !result.logs.empty()) {
            Log("GLES render fallback to CPU: " + result.logs.back());
            gpuFallbackLogged_ = true;
        }
        return RenderRgbaFrameCpuLocked(frame);
#endif
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
            viewportRemoteWidth_ = frame.width;
            viewportRemoteHeight_ = frame.height;
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
        viewportRemoteWidth_ = 0;
        viewportRemoteHeight_ = 0;
    }

    void ClearNativeWindowConfigLocked()
    {
        gpuRenderer_.Destroy();
        nativePainter_.Destroy();
        gpuFallbackLogged_ = false;
    }

    bool ResolveNativeSurfaceSizeLocked(uint32_t& width, uint32_t& height) const
    {
        if (component_ == nullptr || window_ == nullptr) {
            return false;
        }

        uint64_t nativeWidth = 0;
        uint64_t nativeHeight = 0;
        const int32_t rc = OH_NativeXComponent_GetXComponentSize(component_, window_, &nativeWidth, &nativeHeight);
        if (rc != OH_NATIVEXCOMPONENT_RESULT_SUCCESS || nativeWidth == 0 || nativeHeight == 0) {
            return false;
        }

        constexpr uint64_t maxDimension = std::numeric_limits<uint32_t>::max();
        width = static_cast<uint32_t>(std::min(nativeWidth, maxDimension));
        height = static_cast<uint32_t>(std::min(nativeHeight, maxDimension));
        return true;
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
            viewportRemoteWidth_,
            viewportRemoteHeight_,
            createdCount_,
            changedCount_,
            destroyedCount_,
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
            BridgeLogger::Debug(line);
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
    uint32_t viewportRemoteWidth_ = 0;
    uint32_t viewportRemoteHeight_ = 0;
    uint32_t createdCount_ = 0;
    uint32_t changedCount_ = 0;
    uint32_t destroyedCount_ = 0;
    uint32_t paintCount_ = 0;
    GpuRgbaRenderer gpuRenderer_;
    NativeWindowRgbaPainter nativePainter_;
    bool gpuFallbackLogged_ = false;
    bool cpuOnlyLogged_ = false;
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

SurfacePaintResult SurfaceBridge::RenderRgbaFrame(const RgbaFrame& frame)
{
    return impl_->RenderRgbaFrame(frame);
}

void SurfaceBridge::ReleaseRenderTarget(const std::string& reason)
{
    impl_->ReleaseRenderTarget(reason);
}

SurfaceSnapshot SurfaceBridge::Snapshot()
{
    return impl_->Snapshot();
}

DecoderSurfaceTarget SurfaceBridge::DecoderSurface()
{
    return impl_->DecoderSurface();
}

} // namespace rdp_bridge
