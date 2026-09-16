#include "freerdp/freerdp_gdi_bridge.h"

#include "common/bridge_log.h"
#include "freerdp/freerdp_runtime.h"
#include "input/remote_pointer_text_detector.h"
#include "surface/render_output_owner.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <utility>

#include <freerdp/codec/color.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/settings.h>
#include <freerdp/settings_keys.h>

namespace rdp_bridge {
bool UpdateAvc420CompositeWithGdiFrame(const RgbaFrame& frame);
void UpdateAvc420SurfaceOutputIfActive(const std::string& reason);
} // namespace rdp_bridge

namespace rdp_bridge {

namespace {

std::mutex g_gdiCallbacksMutex;
GdiBridgeCallbacks g_gdiCallbacks;
std::mutex g_gdiPrimaryFrameMutex;
RgbaFrame g_lastGdiPrimaryFrame;
std::chrono::steady_clock::time_point g_lastGdiPrimaryFrameAt {};
uint64_t g_gdiPrimaryFrameSequence = 0;

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
    if (CurrentRenderOutputOwner() != RenderOutputOwner::Gdi) {
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
        BridgeLogger::Debug(line);
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

DirtyFrameStats FullDirtyStats(uint32_t width, uint32_t height)
{
    DirtyFrameStats stats;
    if (width == 0 || height == 0) {
        return stats;
    }
    stats.valid = true;
    stats.rectCount = 1;
    stats.x = 0;
    stats.y = 0;
    stats.width = width;
    stats.height = height;
    stats.areaPermille = 1000;
    return stats;
}

bool DirtyCoversFullFrame(const DirtyFrameStats& dirty, uint32_t width, uint32_t height)
{
    return dirty.valid && width != 0 && height != 0 &&
        dirty.x == 0 && dirty.y == 0 &&
        dirty.width >= width && dirty.height >= height;
}

void RememberGdiPrimaryFrame(RgbaFrame frame)
{
    if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
        frame.strideBytes == 0) {
        return;
    }
    std::lock_guard<std::mutex> lock(g_gdiPrimaryFrameMutex);
    frame.sequence = ++g_gdiPrimaryFrameSequence;
    g_lastGdiPrimaryFrame = std::move(frame);
    g_lastGdiPrimaryFrameAt = std::chrono::steady_clock::now();
}

void ClearRememberedGdiPrimaryFrame()
{
    std::lock_guard<std::mutex> lock(g_gdiPrimaryFrameMutex);
    g_lastGdiPrimaryFrame = {};
    g_lastGdiPrimaryFrameAt = {};
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

bool SnapshotRdpPrimaryFrame(RgbaFrame& frame, bool forceFullDirty,
    const std::string& label, uint64_t maxAgeMs)
{
    std::lock_guard<std::mutex> lock(g_gdiPrimaryFrameMutex);
    if (!g_rdpPrimaryFrameReady.load() || g_lastGdiPrimaryFrame.data == nullptr ||
        g_lastGdiPrimaryFrame.width == 0 || g_lastGdiPrimaryFrame.height == 0 ||
        g_lastGdiPrimaryFrame.strideBytes == 0 ||
        g_lastGdiPrimaryFrameAt == std::chrono::steady_clock::time_point{}) {
        return false;
    }
    if (!DirtyCoversFullFrame(g_lastGdiPrimaryFrame.dirty,
            g_lastGdiPrimaryFrame.width, g_lastGdiPrimaryFrame.height)) {
        return false;
    }
    if (maxAgeMs != 0) {
        const uint64_t ageMs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - g_lastGdiPrimaryFrameAt).count());
        if (ageMs > maxAgeMs) {
            return false;
        }
    }

    frame = g_lastGdiPrimaryFrame;
    if (!label.empty()) {
        frame.label = label;
    }
    if (forceFullDirty) {
        frame.dirty = FullDirtyStats(frame.width, frame.height);
        frame.dirtySequenceStart = frame.sequence;
    }
    return true;
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
    rdpGdi* gdi = context->gdi;
    if (IsAvc420SurfaceOutputEnabled()) {
        if (gdi->primary_buffer == nullptr || gdi->width <= 0 ||
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
            "freerdp gdi background",
            CaptureGdiDirtyStats(gdi),
        };
        RememberGdiPrimaryFrame(frame);
        g_rdpPrimaryFrameReady.store(true);
        if (!UpdateAvc420CompositeWithGdiFrame(frame)) {
            const uint32_t skipCount = ++g_freerdpRenderSkipCount;
            if (skipCount == 1 || skipCount % 300 == 0) {
                EmitGdiLog("FreeRDP GDI background composite skipped for AVC420 owner");
            }
        } else {
            g_freerdpRenderSkipCount.store(0);
        }
        ClearGdiInvalidRegion(gdi);
        return TRUE;
    }

    if (IsAvc444GpuRenderOutputOwner()) {
        ClearGdiInvalidRegion(gdi);
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
    RememberGdiPrimaryFrame(frame);
    g_rdpPrimaryFrameReady.store(true);
    ++g_freerdpRenderedFrameCount;
    std::string queueMessage;
    if (!QueueGdiFrame(frame, queueMessage)) {
        const uint32_t skipCount = ++g_freerdpRenderSkipCount;
        if (skipCount == 1 || skipCount % 300 == 0) {
            EmitGdiLog("FreeRDP GDI frame queue skipped: " + queueMessage);
        }
    } else {
        g_freerdpRenderSkipCount.store(0);
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
    const uint16_t orientation = api.settingsGetUint16(
        context->settings, FreeRDP_DesktopOrientation);
    const uint32_t desktopScaleFactor = api.settingsGetUint32(
        context->settings, FreeRDP_DesktopScaleFactor);
    const uint32_t deviceScaleFactor = api.settingsGetUint32(
        context->settings, FreeRDP_DeviceScaleFactor);
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
        ClearRememberedGdiPrimaryFrame();
    }
    if (IsAvc420SurfaceOutputEnabled()) {
        UpdateAvc420SurfaceOutputIfActive("desktop resize " + std::to_string(width) + "x" +
            std::to_string(height));
    } else {
        StartGdiRenderPipeline();
    }
    EmitGdiLog("FreeRDP desktop resize applied: " + std::to_string(width) + "x" +
        std::to_string(height) + " orientation=" + std::to_string(orientation) +
        " scale=" + std::to_string(desktopScaleFactor) + "/" +
        std::to_string(deviceScaleFactor));
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

    std::string pointerMessage;
    if (!RegisterRemotePointerTextDetector(api, instance->context, pointerMessage)) {
        EmitGdiLog(pointerMessage);
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
    ClearRememberedGdiPrimaryFrame();
    if (instance->context->settings != nullptr) {
        const uint32_t width = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopWidth);
        const uint32_t height = api.settingsGetUint32(instance->context->settings, FreeRDP_DesktopHeight);
        if (width > 0 && height > 0) {
            SetRdpDesktopSize(width, height);
        }
    }
    return TRUE;
}

void HarmonyPostDisconnect(freerdp* instance)
{
    ResetRemotePointerTextDetector();
    StopGdiRenderPipeline();
    if (instance == nullptr || instance->context == nullptr || instance->context->gdi == nullptr) {
        ClearRdpDesktopSize();
        ClearRememberedGdiPrimaryFrame();
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.gdiFree != nullptr) {
        api.gdiFree(instance);
    }
    ClearRdpDesktopSize();
    ClearRememberedGdiPrimaryFrame();
}


} // namespace rdp_bridge
