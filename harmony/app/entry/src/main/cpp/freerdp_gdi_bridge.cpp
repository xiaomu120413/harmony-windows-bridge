#include "freerdp_gdi_bridge.h"

#include "bridge_log.h"
#include "freerdp_runtime.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <string>
#include <utility>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/codec/color.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
namespace {

std::mutex g_gdiCallbacksMutex;
GdiBridgeCallbacks g_gdiCallbacks;

void EmitGdiLog(const std::string& line);

GdiBridgeCallbacks SnapshotGdiBridgeCallbacks()
{
    std::lock_guard<std::mutex> lock(g_gdiCallbacksMutex);
    return g_gdiCallbacks;
}

bool IsAvc420SurfaceOutputEnabled()
{
    GdiBridgeCallbacks callbacks = SnapshotGdiBridgeCallbacks();
    return callbacks.isAvc420SurfaceOutputEnabled != nullptr && callbacks.isAvc420SurfaceOutputEnabled();
}

bool QueueGdiFrame(const RgbaFrame& frame, std::string& message, bool forceRender = false)
{
    GdiBridgeCallbacks callbacks = SnapshotGdiBridgeCallbacks();
    if (!callbacks.queueFrame) {
        message = "GDI frame queue callback is not configured";
        return false;
    }
    return callbacks.queueFrame(frame, message, forceRender);
}

void StartGdiRenderPipeline()
{
    static std::atomic_uint32_t skipLogCount{0};
    if (IsAvc444GpuRenderOutputOwner()) {
        const uint32_t count = ++skipLogCount;
        if (count <= 3 || count % 60 == 0) {
            EmitGdiLog("GDI render pipeline start skipped: outputOwner=" +
                CurrentRenderOutputOwnerName() + " count=" + std::to_string(count));
        }
        return;
    }
    GdiBridgeCallbacks callbacks = SnapshotGdiBridgeCallbacks();
    if (callbacks.startRenderPipeline) {
        callbacks.startRenderPipeline();
    }
}

void StopGdiRenderPipeline()
{
    GdiBridgeCallbacks callbacks = SnapshotGdiBridgeCallbacks();
    if (callbacks.stopRenderPipeline) {
        callbacks.stopRenderPipeline();
    }
}

void EmitGdiLog(const std::string& line)
{
    GdiBridgeCallbacks callbacks = SnapshotGdiBridgeCallbacks();
    if (callbacks.log) {
        callbacks.log(line);
    } else {
        EmitHilogInfo(line);
    }
}

void ClearGdiInvalidRegion(rdpGdi* gdi)
{
    if (gdi == nullptr || gdi->primary == nullptr || gdi->primary->hdc == nullptr ||
        gdi->primary->hdc->hwnd == nullptr) {
        return;
    }
    HGDI_WND hwnd = gdi->primary->hdc->hwnd;
    if (hwnd->invalid != nullptr) {
        hwnd->invalid->null = TRUE;
    }
    hwnd->ninvalid = 0;
}

} // namespace

void SetGdiBridgeCallbacks(GdiBridgeCallbacks callbacks)
{
    std::lock_guard<std::mutex> lock(g_gdiCallbacksMutex);
    g_gdiCallbacks = std::move(callbacks);
}
std::atomic_uint32_t g_freerdpRenderedFrameCount{0};
std::atomic_uint32_t g_freerdpRenderSkipCount{0};
std::atomic_uint32_t g_rdpDesktopWidth{0};
std::atomic_uint32_t g_rdpDesktopHeight{0};
std::atomic_bool g_rdpPrimaryFrameReady{false};

void SetRdpDesktopSize(uint32_t width, uint32_t height)
{
    g_rdpDesktopWidth.store(width);
    g_rdpDesktopHeight.store(height);
}

void ClearRdpDesktopSize()
{
    SetRdpDesktopSize(0, 0);
    g_rdpPrimaryFrameReady.store(false);
}

uint32_t RdpDesktopWidth()
{
    return g_rdpDesktopWidth.load();
}

uint32_t RdpDesktopHeight()
{
    return g_rdpDesktopHeight.load();
}

bool RdpPrimaryFrameReady()
{
    return g_rdpPrimaryFrameReady.load();
}

DirtyFrameStats CaptureGdiDirtyStats(const rdpGdi* gdi)
{
    DirtyFrameStats stats;
    if (gdi == nullptr || gdi->width <= 0 || gdi->height <= 0 ||
        gdi->primary == nullptr || gdi->primary->hdc == nullptr ||
        gdi->primary->hdc->hwnd == nullptr) {
        return stats;
    }

    HGDI_WND hwnd = gdi->primary->hdc->hwnd;
    const uint32_t frameWidth = static_cast<uint32_t>(gdi->width);
    const uint32_t frameHeight = static_cast<uint32_t>(gdi->height);
    const uint64_t frameArea = static_cast<uint64_t>(frameWidth) * frameHeight;
    if (frameArea == 0) {
        return stats;
    }

    uint32_t minX = frameWidth;
    uint32_t minY = frameHeight;
    uint32_t maxX = 0;
    uint32_t maxY = 0;
    uint64_t dirtyArea = 0;

    auto addRegion = [&](const GDI_RGN* region) {
        if (region == nullptr || region->null || region->w <= 0 || region->h <= 0) {
            return;
        }

        const int64_t left = std::max<int64_t>(0, region->x);
        const int64_t top = std::max<int64_t>(0, region->y);
        const int64_t right = std::min<int64_t>(frameWidth, static_cast<int64_t>(region->x) + region->w);
        const int64_t bottom = std::min<int64_t>(frameHeight, static_cast<int64_t>(region->y) + region->h);
        if (right <= left || bottom <= top) {
            return;
        }

        const uint32_t clampedLeft = static_cast<uint32_t>(left);
        const uint32_t clampedTop = static_cast<uint32_t>(top);
        const uint32_t clampedRight = static_cast<uint32_t>(right);
        const uint32_t clampedBottom = static_cast<uint32_t>(bottom);
        minX = std::min(minX, clampedLeft);
        minY = std::min(minY, clampedTop);
        maxX = std::max(maxX, clampedRight);
        maxY = std::max(maxY, clampedBottom);
        dirtyArea += static_cast<uint64_t>(clampedRight - clampedLeft) *
            static_cast<uint64_t>(clampedBottom - clampedTop);
        ++stats.rectCount;
    };

    if (hwnd->ninvalid > 0 && hwnd->cinvalid != nullptr) {
        for (INT32 i = 0; i < hwnd->ninvalid; ++i) {
            addRegion(&hwnd->cinvalid[i]);
        }
    } else {
        addRegion(hwnd->invalid);
    }

    if (stats.rectCount == 0) {
        return stats;
    }

    stats.valid = true;
    stats.x = minX;
    stats.y = minY;
    stats.width = maxX > minX ? maxX - minX : 0;
    stats.height = maxY > minY ? maxY - minY : 0;
    const uint64_t cappedArea = std::min(dirtyArea, frameArea);
    stats.areaPermille = static_cast<uint32_t>((cappedArea * 1000U + frameArea / 2U) / frameArea);
    return stats;
}

BOOL HarmonyBeginPaint(rdpContext* context)
{
    if (context == nullptr || context->gdi == nullptr || context->gdi->primary == nullptr ||
        context->gdi->primary->hdc == nullptr || context->gdi->primary->hdc->hwnd == nullptr ||
        context->gdi->primary->hdc->hwnd->invalid == nullptr) {
        return TRUE;
    }

    context->gdi->primary->hdc->hwnd->invalid->null = TRUE;
    return TRUE;
}

BOOL HarmonyEndPaint(rdpContext* context)
{
    if (context == nullptr || context->gdi == nullptr) {
        return TRUE;
    }
    if (IsAvc420SurfaceOutputEnabled()) {
        return TRUE;
    }

    rdpGdi* gdi = context->gdi;
    if (IsAvc444GpuRenderOutputOwner()) {
        static std::atomic_uint32_t skipLogCount{0};
        ClearGdiInvalidRegion(gdi);
        const uint32_t count = ++skipLogCount;
        if (count <= 3 || count % 120 == 0) {
            EmitGdiLog("FreeRDP GDI EndPaint skipped: outputOwner=" +
                CurrentRenderOutputOwnerName() +
                " because AVC444 GPU compositor owns the XComponent count=" +
                std::to_string(count));
        }
        return TRUE;
    }
    if (gdi->suppressOutput || gdi->primary_buffer == nullptr || gdi->width <= 0 ||
        gdi->height <= 0 || gdi->stride == 0) {
        return TRUE;
    }

    if (gdi->primary != nullptr && gdi->primary->hdc != nullptr &&
        gdi->primary->hdc->hwnd != nullptr) {
        HGDI_WND hwnd = gdi->primary->hdc->hwnd;
        if (hwnd->invalid != nullptr && hwnd->invalid->null) {
            return TRUE;
        }
    }

    RgbaFrame frame = {
        gdi->primary_buffer,
        static_cast<uint32_t>(gdi->width),
        static_cast<uint32_t>(gdi->height),
        static_cast<int32_t>(gdi->stride),
        "freerdp gdi",
        CaptureGdiDirtyStats(gdi),
    };
    g_rdpPrimaryFrameReady.store(true);
    const uint32_t frameCount = ++g_freerdpRenderedFrameCount;
    std::string queueMessage;
    if (!QueueGdiFrame(frame, queueMessage)) {
        const uint32_t skipCount = ++g_freerdpRenderSkipCount;
        if (skipCount <= 3 || skipCount % 120 == 0) {
            EmitGdiLog("FreeRDP GDI frame queue skipped: " + queueMessage);
        }
    } else {
        g_freerdpRenderSkipCount.store(0);
        if (frameCount <= 3 || frameCount % 60 == 0) {
            EmitGdiLog("FreeRDP GDI frame queued: " + queueMessage);
        }
    }

    ClearGdiInvalidRegion(gdi);
    return TRUE;
}

BOOL HarmonyDesktopResize(rdpContext* context)
{
    if (context == nullptr || context->settings == nullptr || context->gdi == nullptr) {
        return FALSE;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    const uint32_t width = api.settingsGetUint32(context->settings, FreeRDP_DesktopWidth);
    const uint32_t height = api.settingsGetUint32(context->settings, FreeRDP_DesktopHeight);
    const bool sizeChanged = RdpDesktopWidth() != width || RdpDesktopHeight() != height;
    StopGdiRenderPipeline();
    if (width == 0 || height == 0 || !api.gdiResize(context->gdi, width, height)) {
        if (!IsAvc420SurfaceOutputEnabled()) {
            StartGdiRenderPipeline();
        }
        EmitGdiLog("FreeRDP desktop resize failed");
        return FALSE;
    }

    SetRdpDesktopSize(width, height);
    if (sizeChanged) {
        g_rdpPrimaryFrameReady.store(false);
    }
    if (!IsAvc420SurfaceOutputEnabled()) {
        StartGdiRenderPipeline();
    }
    EmitGdiLog("FreeRDP desktop resized: " + std::to_string(width) + "x" + std::to_string(height));
    return TRUE;
}

BOOL HarmonyPostConnect(freerdp* instance)
{
    if (instance == nullptr || instance->context == nullptr || instance->context->update == nullptr) {
        return FALSE;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiInit == nullptr || !api.gdiInit(instance, PIXEL_FORMAT_RGBA32)) {
        EmitGdiLog("FreeRDP gdi_init failed");
        return FALSE;
    }

    rdpUpdate* update = instance->context->update;
    update->BeginPaint = HarmonyBeginPaint;
    update->EndPaint = HarmonyEndPaint;
    update->DesktopResize = HarmonyDesktopResize;
    if (IsAvc420SurfaceOutputEnabled()) {
        StopGdiRenderPipeline();
    } else {
        StartGdiRenderPipeline();
    }
    g_freerdpRenderedFrameCount.store(0);
    g_freerdpRenderSkipCount.store(0);
    g_rdpPrimaryFrameReady.store(false);
    if (instance->context->settings != nullptr) {
        const uint32_t width = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopWidth);
        const uint32_t height = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopHeight);
        if (width > 0 && height > 0) {
            SetRdpDesktopSize(width, height);
            EmitGdiLog("FreeRDP desktop size: " + std::to_string(width) + "x" + std::to_string(height));
        }
    }
    EmitGdiLog("FreeRDP GDI callbacks registered");
    return TRUE;
}

void HarmonyPostDisconnect(freerdp* instance)
{
    StopGdiRenderPipeline();
    if (instance == nullptr || instance->context == nullptr || instance->context->gdi == nullptr) {
        ClearRdpDesktopSize();
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiFree != nullptr) {
        api.gdiFree(instance);
        EmitGdiLog("FreeRDP GDI resources released");
    }
    ClearRdpDesktopSize();
}

#endif

} // namespace rdp_bridge
