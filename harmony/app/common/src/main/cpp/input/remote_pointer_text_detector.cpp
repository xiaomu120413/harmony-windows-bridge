#include "input/remote_pointer_text_detector.h"

#include "freerdp/freerdp_runtime.h"
#include "input/remote_pointer_text_policy.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include <freerdp/codec/color.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/graphics.h>

namespace rdp_bridge {
namespace {

constexpr uint64_t kDirectTouchIntentMs = 900;
constexpr uint64_t kPointerShapeFallbackMs = 120;

struct TextAwarePointer {
    rdpPointer pointer;
    BOOL textCandidate = FALSE;
};

std::mutex g_callbacksMutex;
RemotePointerImeVisibilityRequest g_requestImeVisibility;
RemotePointerTextLog g_log;
std::atomic<FreerdpRuntimeApi*> g_api{nullptr};
std::atomic_bool g_textCandidate{false};
std::atomic_uint64_t g_touchIntentDeadlineMs{0};

uint64_t MonotonicMs()
{
    using Clock = std::chrono::steady_clock;
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        Clock::now().time_since_epoch()).count());
}

void EmitLog(const std::string& line)
{
    RemotePointerTextLog log;
    {
        std::lock_guard<std::mutex> lock(g_callbacksMutex);
        log = g_log;
    }
    if (log != nullptr) {
        log(line);
    }
}

bool ConsumeTouchIntentForIme(bool textCandidate)
{
    const uint64_t now = MonotonicMs();
    uint64_t deadline = g_touchIntentDeadlineMs.load();
    if (deadline == 0 || now > deadline ||
        !g_touchIntentDeadlineMs.compare_exchange_strong(deadline, 0)) {
        return false;
    }

    RemotePointerImeVisibilityRequest request;
    {
        std::lock_guard<std::mutex> lock(g_callbacksMutex);
        request = g_requestImeVisibility;
    }
    if (request == nullptr || !request(textCandidate)) {
        return false;
    }
    return true;
}

BOOL PointerNew(rdpContext* context, rdpPointer* pointer)
{
    auto* textPointer = reinterpret_cast<TextAwarePointer*>(pointer);
    FreerdpRuntimeApi* api = g_api.load();
    if (context == nullptr || context->gdi == nullptr || pointer == nullptr || api == nullptr ||
        api->imageCopyFromPointerData == nullptr || pointer->width == 0 || pointer->height == 0 ||
        pointer->width > 256 || pointer->height > 256) {
        return FALSE;
    }

    const uint32_t stride = pointer->width * 4;
    std::vector<uint8_t> rgba(static_cast<size_t>(stride) * pointer->height);
    if (!api->imageCopyFromPointerData(rgba.data(), PIXEL_FORMAT_RGBA32, stride, 0, 0,
        pointer->width, pointer->height, pointer->xorMaskData, pointer->lengthXorMask,
        pointer->andMaskData, pointer->lengthAndMask, pointer->xorBpp, &context->gdi->palette)) {
        return FALSE;
    }

    RemotePointerTextMetrics metrics;
    textPointer->textCandidate = IsRemoteTextPointerCandidate(rgba.data(), pointer->width,
        pointer->height, stride, pointer->xPos, pointer->yPos, &metrics) ? TRUE : FALSE;
    if (textPointer->textCandidate) {
        EmitLog("RDP_IME event=text_pointer_candidate bounds=" +
            std::to_string(metrics.boundsWidth) + "x" + std::to_string(metrics.boundsHeight) +
            " opaque=" + std::to_string(metrics.opaquePixels));
    }
    return TRUE;
}

void PointerFree(rdpContext*, rdpPointer*)
{
}

BOOL PointerSet(rdpContext*, rdpPointer* pointer)
{
    if (pointer == nullptr) {
        return FALSE;
    }
    auto* textPointer = reinterpret_cast<TextAwarePointer*>(pointer);
    g_textCandidate.store(textPointer->textCandidate == TRUE);
    (void)ConsumeTouchIntentForIme(textPointer->textCandidate == TRUE);
    return TRUE;
}

BOOL PointerSetNull(rdpContext*)
{
    g_textCandidate.store(false);
    return TRUE;
}

BOOL PointerSetDefault(rdpContext*)
{
    g_textCandidate.store(false);
    return TRUE;
}

BOOL PointerSetPosition(rdpContext*, UINT32, UINT32)
{
    return TRUE;
}

} // namespace

void ConfigureRemotePointerTextDetector(RemotePointerImeVisibilityRequest requestImeVisibility,
    RemotePointerTextLog log)
{
    std::lock_guard<std::mutex> lock(g_callbacksMutex);
    g_requestImeVisibility = std::move(requestImeVisibility);
    g_log = std::move(log);
}

bool RegisterRemotePointerTextDetector(FreerdpRuntimeApi& api, rdpContext* context,
    std::string& message)
{
    if (context == nullptr || context->graphics == nullptr || api.graphicsRegisterPointer == nullptr ||
        api.imageCopyFromPointerData == nullptr) {
        message = "remote pointer text detector dependencies unavailable";
        return false;
    }

    rdpPointer prototype{};
    prototype.size = sizeof(TextAwarePointer);
    prototype.New = PointerNew;
    prototype.Free = PointerFree;
    prototype.Set = PointerSet;
    prototype.SetNull = PointerSetNull;
    prototype.SetDefault = PointerSetDefault;
    prototype.SetPosition = PointerSetPosition;
    g_api.store(&api);
    api.graphicsRegisterPointer(context->graphics, &prototype);
    message = "remote pointer text detector registered";
    return true;
}

void NotifyRemotePointerDirectTouch(uint64_t nowMs)
{
    g_touchIntentDeadlineMs.store(nowMs + kDirectTouchIntentMs);
    const bool fallbackTextCandidate = g_textCandidate.load();
    std::thread([fallbackTextCandidate]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(kPointerShapeFallbackMs));
        (void)ConsumeTouchIntentForIme(fallbackTextCandidate);
    }).detach();
}

void ResetRemotePointerTextDetector()
{
    g_textCandidate.store(false);
    g_touchIntentDeadlineMs.store(0);
}

} // namespace rdp_bridge
