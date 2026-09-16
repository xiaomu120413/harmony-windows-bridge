#include "surface/avc420_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "freerdp/freerdp_runtime.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace rdp_bridge {
namespace {

bool RectsValid(const RECTANGLE_16* rects, uint32_t count, uint32_t width, uint32_t height)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string error;
    return EnsureFreerdpRuntimeLoaded(api, error) && api.ohosRdpgfxRectsValid != nullptr &&
        api.ohosRdpgfxRectsValid(rects, count, width, height) != FALSE;
}

std::string FormatRectText(const RECTANGLE_16* rect)
{
    if (rect == nullptr) {
        return "none";
    }
    return std::to_string(rect->left) + "," + std::to_string(rect->top) + "-" +
        std::to_string(rect->right) + "," + std::to_string(rect->bottom);
}

std::string RectsText(const RECTANGLE_16* rects, uint32_t count)
{
    if (rects == nullptr || count == 0) {
        return "rects:0,first:none,last:none,bounds:none,area:0";
    }

    uint32_t left = rects[0].left;
    uint32_t top = rects[0].top;
    uint32_t right = rects[0].right;
    uint32_t bottom = rects[0].bottom;
    uint64_t area = 0;
    for (uint32_t i = 0; i < count; ++i) {
        const RECTANGLE_16& rect = rects[i];
        left = std::min<uint32_t>(left, rect.left);
        top = std::min<uint32_t>(top, rect.top);
        right = std::max<uint32_t>(right, rect.right);
        bottom = std::max<uint32_t>(bottom, rect.bottom);
        area += static_cast<uint64_t>(rect.right - rect.left) * (rect.bottom - rect.top);
    }

    const RECTANGLE_16 bounds {
        static_cast<UINT16>(left),
        static_cast<UINT16>(top),
        static_cast<UINT16>(right),
        static_cast<UINT16>(bottom),
    };
    return "rects:" + std::to_string(count) +
        ",first:" + FormatRectText(rects) +
        ",last:" + FormatRectText(rects + count - 1U) +
        ",bounds:" + FormatRectText(&bounds) +
        ",area:" + std::to_string(area);
}

struct PreparedH264Packet {
    const uint8_t* data = nullptr;
    uint32_t size = 0;
    bool hadParameterSets = false;
    bool prependedParameterSets = false;
    std::string nalSummary;
    std::vector<uint8_t> storage;
};

PreparedH264Packet PrepareH264Packet(const uint8_t* data, uint32_t size, bool decoderStarted,
    std::vector<uint8_t>& parameterSets, std::vector<std::string>& logs)
{
    PreparedH264Packet packet;
    packet.data = data;
    packet.size = size;
    if (data == nullptr || size == 0) {
        packet.nalSummary = "empty";
        return packet;
    }

    std::vector<uint8_t> extracted;
    packet.hadParameterSets = ExtractH264ParameterSets(data, size, extracted, &packet.nalSummary);
    if (packet.hadParameterSets) {
        parameterSets = std::move(extracted);
    } else if (!decoderStarted && !parameterSets.empty()) {
        packet.storage.reserve(parameterSets.size() + size);
        packet.storage.insert(packet.storage.end(), parameterSets.begin(), parameterSets.end());
        packet.storage.insert(packet.storage.end(), data, data + size);
        packet.data = packet.storage.data();
        packet.size = static_cast<uint32_t>(packet.storage.size());
        packet.prependedParameterSets = true;
        logs.push_back("AVC420 GPU prepended H264 parameter sets: parameterSets=" +
            std::to_string(parameterSets.size()) + " payload=" + std::to_string(size) +
            " nalTypes=" + packet.nalSummary);
    }
    return packet;
}

} // namespace

void Avc420GpuCompositorImpl::State::Destroy()
{
    renderer.Destroy();
    avcDecoder.Close();
    streamParameterSets.clear();
    streamPts = 0;
    processedCommands = 0;
    decoded = 0;
    queuedPresents = 0;
    presented = 0;
    failures = 0;
    ignoredUpdates = 0;
    importFallbacks = 0;
    skippedWarmups = 0;
    prewarms = 0;
    prewarmFailures = 0;
    gdiBackgroundUpdates = 0;
    gdiBackgroundPresents = 0;
    gdiBackgroundPendingPresent = false;
    endFrameCallbacks = 0;
    endFrameSkipNoPending = 0;
    endFrameMismatches = 0;
    endFramePresentAttempts = 0;
    pendingPresentOverwrites = 0;
    pendingFrameId = 0;
    pendingSurfaceWidth = 0;
    pendingSurfaceHeight = 0;
    currentSurfaceWidth = 0;
    currentSurfaceHeight = 0;
    pendingPresent = false;
    resetDecoderBeforeNextDecode = false;
    nativeImportUnsupported = false;
    nativeImportUnsupportedWidth = 0;
    nativeImportUnsupportedHeight = 0;
    nativeImportUnsupportedFormat = 0;
    lastSampledProcessStartUs = 0;
    lastCommandStartUs = 0;
    lastEndFrameUs = 0;
    lastPresentUs = 0;
    maxCommandGapUs = 0;
    maxEndFrameGapUs = 0;
    maxPresentGapUs = 0;
    lastStatsUs = NowMicros();
    lastStatsPresented = 0;
    commandTiming.Reset();
    commandIntervalTiming.Reset();
    offscreenEnsureTiming.Reset();
    decodeTiming.Reset();
    windowEnsureTiming.Reset();
    presentTiming.Reset();
}

void Avc420GpuCompositorImpl::State::RecordCommandGap(uint64_t nowUs)
{
    if (lastCommandStartUs != 0 && nowUs >= lastCommandStartUs) {
        maxCommandGapUs = std::max(maxCommandGapUs, nowUs - lastCommandStartUs);
    }
    lastCommandStartUs = nowUs;
}

void Avc420GpuCompositorImpl::State::RecordEndFrameGap(uint64_t nowUs)
{
    if (lastEndFrameUs != 0 && nowUs >= lastEndFrameUs) {
        maxEndFrameGapUs = std::max(maxEndFrameGapUs, nowUs - lastEndFrameUs);
    }
    lastEndFrameUs = nowUs;
}

void Avc420GpuCompositorImpl::State::RecordPresentGap(uint64_t nowUs)
{
    if (lastPresentUs != 0 && nowUs >= lastPresentUs) {
        maxPresentGapUs = std::max(maxPresentGapUs, nowUs - lastPresentUs);
    }
    lastPresentUs = nowUs;
}

void Avc420GpuCompositorImpl::State::ClearPendingPresent()
{
    pendingPresent = false;
    pendingFrameId = 0;
    pendingSurfaceWidth = 0;
    pendingSurfaceHeight = 0;
}

void Avc420GpuCompositorImpl::State::OnSurfaceTargetChanged(const std::string& reason,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (!outputActive) {
        renderer.DetachWindowSurface(reason, logs);
        logs.push_back("AVC420 native-buffer GPU noted target change while inactive after " +
            reason + "; decoder preserved with SPS/PPS bootstrap parameter sets");
        return;
    }

    DecoderSurfaceTarget target {};
    if (callbacks.decoderSurfaceTarget != nullptr) {
        target = callbacks.decoderSurfaceTarget();
    }
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        renderer.DetachWindowSurface(reason, logs);
        logs.push_back("AVC420 native-buffer GPU detached missing target after " + reason +
            "; decoder preserved, waiting for next target");
        return;
    }

    const uint32_t surfaceWidth =
        pendingSurfaceWidth != 0 ? pendingSurfaceWidth : currentSurfaceWidth;
    const uint32_t surfaceHeight =
        pendingSurfaceHeight != 0 ? pendingSurfaceHeight : currentSurfaceHeight;
    if (surfaceWidth == 0 || surfaceHeight == 0) {
        renderer.DetachWindowSurface(reason, logs);
        logs.push_back("AVC420 native-buffer GPU target changed after " + reason +
            " with no retained surface size; decoder preserved, waiting for next AVC420 frame");
        return;
    }

    ++endFramePresentAttempts;
    const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
    bool windowReady = false;
    {
        ScopedTiming timing(windowEnsureTiming, sampleTiming);
        windowReady = renderer.Ensure(target.window, target.width, target.height,
            surfaceWidth, surfaceHeight, logs);
    }
    if (!windowReady) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU target change window attach failed after " +
            reason + " failures=" + std::to_string(failures));
        return;
    }

    bool presentOk = false;
    {
        ScopedTiming timing(presentTiming, sampleTiming);
        presentOk = renderer.PresentComposite(logs, true);
    }
    if (!presentOk) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU target change pending present failed after " +
            reason + " failures=" + std::to_string(failures));
        return;
    }

    const bool includedGdiBackground = gdiBackgroundPendingPresent;
    if (includedGdiBackground) {
        ++gdiBackgroundPresents;
        gdiBackgroundPendingPresent = false;
    }
    const uint32_t frameId = pendingFrameId;
    ClearPendingPresent();
    ++presented;
    logs.push_back("AVC420 native-buffer GPU re-presented retained composite after " +
        reason + ": frame=" + std::to_string(frameId) +
        " presented=" + std::to_string(presented) +
        " gdiBgIncluded=" + std::string(includedGdiBackground ? "yes" : "no") +
        " decoder=preserved retainFrames=yes");
}

bool Avc420GpuCompositorImpl::State::Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    if (surfaceWidth == 0 || surfaceHeight == 0) {
        logs.push_back("AVC420 native-buffer GPU prewarm skipped: invalid surface size");
        return false;
    }

    ++prewarms;
    const bool ready = renderer.Ensure(nullptr, 0, 0, surfaceWidth, surfaceHeight, logs);
    if (!ready) {
        ++prewarmFailures;
        logs.push_back("AVC420 native-buffer GPU prewarm failed: surface=" +
            std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
            " prewarms=" + std::to_string(prewarms) +
            " failures=" + std::to_string(prewarmFailures));
        return false;
    }
    if (ShouldLogFrequent(prewarms)) {
        logs.push_back("AVC420 native-buffer GPU prewarmed: surface=" +
            std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
            " prewarms=" + std::to_string(prewarms) +
            " " + renderer.DebugState());
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::ProcessGdiFrame(const RgbaFrame& frame, bool outputActive,
    std::vector<std::string>& logs)
{
    if (!outputActive) {
        return false;
    }
    const int32_t sourceStride = frame.strideBytes > 0 ? frame.strideBytes :
        static_cast<int32_t>(frame.width * 4U);
    if (frame.data == nullptr || frame.width == 0 || frame.height == 0 ||
        sourceStride < static_cast<int32_t>(frame.width * 4U)) {
        logs.push_back("AVC420 native-buffer GPU rejected invalid GDI background task");
        return false;
    }

    const bool sampleTiming = ShouldSampleTiming(gdiBackgroundUpdates + 1);
    bool offscreenReady = false;
    {
        ScopedTiming timing(offscreenEnsureTiming, sampleTiming);
        offscreenReady = renderer.Ensure(nullptr, 0, 0, frame.width, frame.height, logs);
    }
    if (!offscreenReady) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU GDI background renderer init failed: " +
            renderer.DebugState());
        return false;
    }

    const bool logSummary = ShouldLogFrequent(gdiBackgroundUpdates + 1);
    if (!renderer.CompositeRgbaFrame(frame, logs, logSummary)) {
        ++failures;
        return false;
    }
    ++gdiBackgroundUpdates;
    gdiBackgroundPendingPresent = true;
    currentSurfaceWidth = frame.width;
    currentSurfaceHeight = frame.height;

    if (logSummary) {
        logs.push_back("AVC420 native-buffer GPU retained GDI background composite: "
            "updates=" + std::to_string(gdiBackgroundUpdates) +
            " presents=" + std::to_string(gdiBackgroundPresents) +
            " pendingPresent=yes"
            " presentDeferred=next-retained-present"
            " retainFrames=yes");
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::PresentGdiBackgroundNow(const std::string& trigger,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (!outputActive || !gdiBackgroundPendingPresent || pendingPresent) {
        return false;
    }
    if (currentSurfaceWidth == 0 || currentSurfaceHeight == 0) {
        logs.push_back("AVC420 native-buffer GPU immediate GDI present skipped: "
            "retained surface size missing trigger=" + trigger +
            " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
            std::to_string(gdiBackgroundPresents));
        return false;
    }

    DecoderSurfaceTarget target {};
    if (callbacks.decoderSurfaceTarget != nullptr) {
        target = callbacks.decoderSurfaceTarget();
    }
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        ++ignoredUpdates;
        if (ShouldLogFrequent(ignoredUpdates)) {
            logs.push_back("AVC420 native-buffer GPU immediate GDI present skipped: "
                "target unavailable trigger=" + trigger +
                " ignoredUpdates=" + std::to_string(ignoredUpdates));
        }
        return false;
    }

    ++endFramePresentAttempts;
    const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
    bool windowReady = false;
    {
        ScopedTiming timing(windowEnsureTiming, sampleTiming);
        windowReady = renderer.Ensure(target.window, target.width, target.height,
            currentSurfaceWidth, currentSurfaceHeight, logs);
    }
    if (!windowReady) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU immediate GDI present window attach failed "
            "trigger=" + trigger + " failures=" + std::to_string(failures));
        return false;
    }

    const bool logPresentSummary = ShouldLogFrequent(gdiBackgroundPresents + 1) ||
        ShouldLogFrequent(endFramePresentAttempts);
    bool presentOk = false;
    {
        ScopedTiming timing(presentTiming, sampleTiming);
        presentOk = renderer.PresentComposite(logs, logPresentSummary);
    }
    if (!presentOk) {
        ++failures;
        ++ignoredUpdates;
        logs.push_back("AVC420 native-buffer GPU immediate GDI present failed trigger=" +
            trigger + " failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return false;
    }

    ++gdiBackgroundPresents;
    gdiBackgroundPendingPresent = false;
    RecordPresentGap(NowMicros());
    ++presented;
    if (ShouldLogFrequent(gdiBackgroundPresents) || ShouldLogFrequent(presented)) {
        logs.push_back("AVC420 native-buffer GPU presented retained GDI background "
            "immediately: trigger=" + trigger +
            " presented=" + std::to_string(presented) +
            " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
            std::to_string(gdiBackgroundPresents) +
            " surface=" + std::to_string(currentSurfaceWidth) + "x" +
            std::to_string(currentSurfaceHeight) +
            " policyActive=yes retainFrames=yes");
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::ProcessCommand(const FREERDP_OHOS_RDPGFX_AVC420_COMMAND_INFO* command,
    const Avc420GpuCompositorCallbacks&, bool outputActive, std::vector<std::string>& logs)
{
    const uint64_t commandIndex = ++processedCommands;
    const bool sampleTiming = ShouldSampleTiming(commandIndex);
    const uint64_t processStartUs = NowMicros();
    RecordCommandGap(processStartUs);
    ScopedTiming commandTimer(commandTiming, processStartUs, sampleTiming);
    if (sampleTiming) {
        if (lastSampledProcessStartUs != 0 && processStartUs >= lastSampledProcessStartUs) {
            commandIntervalTiming.Add(processStartUs - lastSampledProcessStartUs);
        }
        lastSampledProcessStartUs = processStartUs;
    }

    if (command == nullptr || command->codecId != RDPGFX_CODECID_AVC420) {
        logs.push_back("AVC420 native-buffer GPU supports AVC420 only");
        return false;
    }
    if (command->stream.data == nullptr || command->stream.length == 0) {
        logs.push_back("AVC420 native-buffer GPU rejected empty H264 stream");
        return false;
    }
    if (!RectsValid(command->stream.regionRects, command->stream.numRegionRects,
            command->width, command->height)) {
        logs.push_back("AVC420 native-buffer GPU rejected invalid dirty rects");
        return false;
    }
    if (nativeImportUnsupported && (nativeImportUnsupportedWidth != command->width ||
            nativeImportUnsupportedHeight != command->height)) {
        logs.push_back("AVC420 native-buffer GPU clears native import fallback after surface "
            "size changed: old=" + std::to_string(nativeImportUnsupportedWidth) + "x" +
            std::to_string(nativeImportUnsupportedHeight) + " new=" +
            std::to_string(command->width) + "x" + std::to_string(command->height));
        nativeImportUnsupported = false;
        nativeImportUnsupportedWidth = 0;
        nativeImportUnsupportedHeight = 0;
        nativeImportUnsupportedFormat = 0;
    }
    if (nativeImportUnsupported && !outputActive) {
        ++skippedWarmups;
        if (ShouldLogFrequent(skippedWarmups)) {
            logs.push_back("AVC420 native-buffer GPU skipped warm-up after native import "
                "fallback: surface=" + std::to_string(command->width) + "x" +
                std::to_string(command->height) +
                " nativeFormat=" +
                NativeBufferFormatName(
                    static_cast<OH_NativeBuffer_Format>(nativeImportUnsupportedFormat)) +
                " skippedWarmups=" + std::to_string(skippedWarmups) +
                "; preserving FreeRDP GDI path without duplicate decoder work");
        }
        return false;
    }

    auto applyActiveUpdatePolicy = [&](const std::string& reason,
        ActiveAvc420UpdatePolicy policy) {
        ++ignoredUpdates;
        ClearPendingPresent();
        if (policy == ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner) {
            resetDecoderBeforeNextDecode = true;
            logs.push_back("AVC420 native-buffer GPU active update policy=" +
                std::string(ActiveAvc420UpdatePolicyName(policy)) +
                " scheduled decoder reset for stream resync; SPS/PPS bootstrap parameter "
                "sets are preserved");
            if (ignoredUpdates >= kActiveResetIgnoreFallbackThreshold) {
                renderer.Destroy();
                avcDecoder.Close();
                resetDecoderBeforeNextDecode = false;
                logs.push_back("AVC420 native-buffer GPU active update policy=" +
                    std::string(ActiveAvc420UpdatePolicyName(
                        ActiveAvc420UpdatePolicy::ReleaseOwner)) +
                    " after repeated reset-required updates: ignoredUpdates=" +
                    std::to_string(ignoredUpdates) +
                    " threshold=" + std::to_string(kActiveResetIgnoreFallbackThreshold) +
                    "; GDI may recover on the next decodable frame");
                return false;
            }
        }
        logs.push_back("AVC420 native-buffer GPU active update policy=" +
            std::string(ActiveAvc420UpdatePolicyName(policy)) +
            " reason=" + reason +
            "; GDI remains suppressed so its H264 context is not re-entered mid-stream "
            "ignoredUpdates=" + std::to_string(ignoredUpdates));
        return true;
    };

    auto fail = [&](const std::string& reason) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU failed: " + reason +
            " failures=" + std::to_string(failures));
        if (outputActive) {
            if (failures >= kActiveFailureFallbackThreshold) {
                ClearPendingPresent();
                renderer.Destroy();
                avcDecoder.Close();
                resetDecoderBeforeNextDecode = false;
                logs.push_back("AVC420 native-buffer GPU releasing output after repeated "
                    "active failures: failures=" + std::to_string(failures) +
                    " threshold=" + std::to_string(kActiveFailureFallbackThreshold) +
                    "; GDI may recover on the next decodable frame");
                return false;
            }
            return applyActiveUpdatePolicy(
                reason, ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner);
        }
        ClearPendingPresent();
        renderer.Destroy();
        return false;
    };

    if (resetDecoderBeforeNextDecode) {
        avcDecoder.Close();
        resetDecoderBeforeNextDecode = false;
        logs.push_back("AVC420 native-buffer GPU reset hardware decoder before decode; "
            "waiting for stored SPS/PPS or a fresh parameter set to bootstrap");
    }

    bool offscreenReady = false;
    {
        ScopedTiming timing(offscreenEnsureTiming, sampleTiming);
        offscreenReady = renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs);
    }
    if (!offscreenReady) {
        if (!outputActive) {
            logs.push_back("AVC420 native-buffer GPU offscreen EGL context unavailable before "
                "decode; keeping GDI");
            return false;
        }
        return fail("offscreen renderer init");
    }

    PreparedH264Packet packet = PrepareH264Packet(command->stream.data,
        command->stream.length, avcDecoder.Started(), streamParameterSets, logs);
    if (!avcDecoder.Started() && !packet.hadParameterSets &&
        !packet.prependedParameterSets) {
        if (outputActive) {
            return applyActiveUpdatePolicy(
                "missing initial SPS/PPS before AVC420 stream",
                ActiveAvc420UpdatePolicy::ResetDecoderAndPreserveOwner);
        }
        logs.push_back("AVC420 native-buffer GPU waits for SPS/PPS before hardware decode; "
            "keeping GDI nalTypes=" + packet.nalSummary);
        return false;
    }

    if (pendingPresent && (pendingSurfaceWidth != command->width ||
            pendingSurfaceHeight != command->height)) {
        ClearPendingPresent();
    }

    if (!avcDecoder.Ensure(command->width, command->height, logs)) {
        return fail("decoder init");
    }

    NativeDecodedFrame frame;
    DecodeResult decode = DecodeResult::Failed;
    {
        ScopedTiming timing(decodeTiming, sampleTiming);
        const int64_t pts = MakeDecoderPts(command->frameId, ++streamPts);
        decode = avcDecoder.Decode(packet.data, packet.size, pts, frame, logs);
    }
    if (decode != DecodeResult::Decoded) {
        if (!outputActive) {
            logs.push_back("AVC420 native-buffer GPU warm-up decode " +
                std::string(decode == DecodeResult::NoOutput ? "has no output yet" : "failed") +
                "; keeping GDI");
            return false;
        }
        if (decode == DecodeResult::NoOutput) {
            return applyActiveUpdatePolicy(
                "decode output not ready", ActiveAvc420UpdatePolicy::PreserveOwner);
        }
        return fail("decode failed");
    }
    ++decoded;

    nativeImportUnsupported = false;
    nativeImportUnsupportedWidth = 0;
    nativeImportUnsupportedHeight = 0;
    nativeImportUnsupportedFormat = 0;

    const bool logCompositeSummary = ShouldLogFrequent(queuedPresents + 1);
    if (!renderer.CompositeFrame(frame, command->stream.regionRects,
            command->stream.numRegionRects, logs, logCompositeSummary)) {
        if (!outputActive) {
            ++importFallbacks;
            nativeImportUnsupported = true;
            nativeImportUnsupportedWidth = command->width;
            nativeImportUnsupportedHeight = command->height;
            nativeImportUnsupportedFormat = frame.nativeFormat;
            frame.Release();
            ClearPendingPresent();
            avcDecoder.Close();
            renderer.Destroy();
            logs.push_back("AVC420 native-buffer GPU fallback before takeover: "
                "native buffer composite/import is unavailable for " +
                std::to_string(command->width) + "x" + std::to_string(command->height) +
                " nativeFormat=" +
                NativeBufferFormatName(
                    static_cast<OH_NativeBuffer_Format>(nativeImportUnsupportedFormat)) +
                " importFallbacks=" + std::to_string(importFallbacks) +
                "; preserving FreeRDP GDI path and disabling duplicate warm-up until "
                "reset/resize");
            return false;
        }
        frame.Release();
        return fail("retained dirty-rect composite");
    }

    if (pendingPresent) {
        ++pendingPresentOverwrites;
        if (ShouldLogFrequent(pendingPresentOverwrites)) {
            logs.push_back("AVC420 native-buffer GPU overwriting pending EndFrame present: "
                "oldFrame=" + std::to_string(pendingFrameId) + " newFrame=" +
                std::to_string(command->frameId) +
                " overwrites=" + std::to_string(pendingPresentOverwrites));
        }
    }

    pendingPresent = true;
    pendingFrameId = command->frameId;
    pendingSurfaceWidth = command->width;
    pendingSurfaceHeight = command->height;
    currentSurfaceWidth = command->width;
    currentSurfaceHeight = command->height;
    ++queuedPresents;
    if (ShouldLogFrequent(queuedPresents)) {
        logs.push_back("AVC420 native-buffer GPU queued EndFrame present: frame=" +
            std::to_string(command->frameId) +
            " surface=" + std::to_string(command->width) + "x" +
            std::to_string(command->height) +
            " targetHint=" + std::to_string(command->targetWidth) + "x" +
            std::to_string(command->targetHeight) +
            " fullSurface=" + std::string(command->fullSurface ? "yes" : "no") +
            " stream=bytes:" + std::to_string(command->stream.length) +
            "," + RectsText(command->stream.regionRects, command->stream.numRegionRects) +
            " route=hardware-decode+native-buffer-eglimage-gles-avc420-retained-composite "
            "retainFrames=yes " + NativeFrameText(frame));
    }
    if (!command->frameOpen) {
        logs.push_back("AVC420 native-buffer GPU queued inter-frame update; bridge will trigger "
            "GPU present without entering FreeRDP dirty state frame=" +
            std::to_string(command->frameId));
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::PresentQueuedUpdate(const std::string& trigger, uint32_t frameId,
    uint32_t activeFrameId, bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks,
    bool outputActive, std::vector<std::string>& logs)
{
    if (!pendingPresent) {
        logs.push_back("AVC420 native-buffer GPU " + trigger +
            " present skipped: pending=no policyActive=" +
            std::string(outputActive ? "yes" : "no") + " " + renderer.DebugState());
        return false;
    }

    if (!matchedFrame || frameId != pendingFrameId) {
        ++endFrameMismatches;
        ++ignoredUpdates;
        const uint32_t queuedFrameId = pendingFrameId;
        ClearPendingPresent();
        logs.push_back("AVC420 native-buffer GPU dropped pending present at " + trigger +
            " mismatch: frame=" + std::to_string(frameId) +
            " queuedFrame=" + std::to_string(queuedFrameId) +
            " activeFrame=" + std::to_string(activeFrameId) +
            " matched=" + std::string(matchedFrame ? "yes" : "no") +
            " mismatches=" + std::to_string(endFrameMismatches) +
            " queuedPresents=" + std::to_string(queuedPresents) +
            " presented=" + std::to_string(presented) +
            " endCallbacks=" + std::to_string(endFrameCallbacks));
        return outputActive;
    }

    DecoderSurfaceTarget target {};
    if (callbacks.decoderSurfaceTarget != nullptr) {
        target = callbacks.decoderSurfaceTarget();
    }
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        ++ignoredUpdates;
        if (outputActive) {
            logs.push_back("AVC420 native-buffer GPU " + trigger +
                " target unavailable; retained pending present while AVC420 output is "
                "target-paused policyActive=yes ignoredUpdates=" +
                std::to_string(ignoredUpdates));
            return true;
        }
        ++failures;
        ClearPendingPresent();
        logs.push_back("AVC420 native-buffer GPU " + trigger +
            " target unavailable; policyActive=" +
            std::string(outputActive ? "yes" : "no") +
            " failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return false;
    }

    ++endFramePresentAttempts;
    const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
    bool windowReady = false;
    {
        ScopedTiming timing(windowEnsureTiming, sampleTiming);
        windowReady = renderer.Ensure(target.window, target.width, target.height,
            pendingSurfaceWidth, pendingSurfaceHeight, logs);
    }
    if (!windowReady) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU " + trigger +
            " renderer window attach failed failures=" + std::to_string(failures));
        return false;
    }

    const bool logPresentSummary = ShouldLogFrequent(endFramePresentAttempts);
    bool presentOk = false;
    {
        ScopedTiming timing(presentTiming, sampleTiming);
        presentOk = renderer.PresentComposite(logs, logPresentSummary);
    }
    if (!presentOk) {
        ++failures;
        ++ignoredUpdates;
        ClearPendingPresent();
        logs.push_back("AVC420 native-buffer GPU " + trigger +
            " present failed failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return false;
    }

    const bool includedGdiBackground = gdiBackgroundPendingPresent;
    if (includedGdiBackground) {
        ++gdiBackgroundPresents;
        gdiBackgroundPendingPresent = false;
    }
    RecordPresentGap(NowMicros());
    ClearPendingPresent();
    ++presented;
    if (ShouldLogFrequent(presented)) {
        logs.push_back("AVC420 native-buffer GPU presented at " + trigger + ": frame=" +
            std::to_string(frameId) + " presented=" + std::to_string(presented) +
            " attempts=" + std::to_string(endFramePresentAttempts) +
            " queued=" + std::to_string(queuedPresents) +
            " policyActive=" + std::string(outputActive ? "yes" : "no") +
            " gdiBgIncluded=" + std::string(includedGdiBackground ? "yes" : "no") +
            " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
            std::to_string(gdiBackgroundPresents) +
            " retainFrames=yes");
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::PresentGdiBackgroundAtEndFrame(uint32_t frameId, uint32_t activeFrameId,
    bool matchedFrame, const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (!outputActive || !gdiBackgroundPendingPresent) {
        return false;
    }
    if (!matchedFrame) {
        if (ShouldLogFrequent(endFrameCallbacks)) {
            logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
                "endFrame=" + std::to_string(frameId) +
                " activeFrame=" + std::to_string(activeFrameId) +
                " matched=no gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
                std::to_string(gdiBackgroundPresents));
        }
        return false;
    }
    if (currentSurfaceWidth == 0 || currentSurfaceHeight == 0) {
        logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
            "retained surface size missing gdiBg=" +
            std::to_string(gdiBackgroundUpdates) + "/" +
            std::to_string(gdiBackgroundPresents));
        return false;
    }

    DecoderSurfaceTarget target {};
    if (callbacks.decoderSurfaceTarget != nullptr) {
        target = callbacks.decoderSurfaceTarget();
    }
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        ++ignoredUpdates;
        if (ShouldLogFrequent(ignoredUpdates)) {
            logs.push_back("AVC420 native-buffer GPU deferred GDI-only present: "
                "target unavailable while output active ignoredUpdates=" +
                std::to_string(ignoredUpdates));
        }
        return false;
    }

    ++endFramePresentAttempts;
    const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
    bool windowReady = false;
    {
        ScopedTiming timing(windowEnsureTiming, sampleTiming);
        windowReady = renderer.Ensure(target.window, target.width, target.height,
            currentSurfaceWidth, currentSurfaceHeight, logs);
    }
    if (!windowReady) {
        ++failures;
        logs.push_back("AVC420 native-buffer GPU EndFrame GDI-only renderer window attach "
            "failed failures=" + std::to_string(failures));
        return false;
    }

    const bool logPresentSummary = ShouldLogFrequent(gdiBackgroundPresents + 1) ||
        ShouldLogFrequent(endFramePresentAttempts);
    bool presentOk = false;
    {
        ScopedTiming timing(presentTiming, sampleTiming);
        presentOk = renderer.PresentComposite(logs, logPresentSummary);
    }
    if (!presentOk) {
        ++failures;
        ++ignoredUpdates;
        logs.push_back("AVC420 native-buffer GPU EndFrame GDI-only present failed "
            "failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return false;
    }

    ++gdiBackgroundPresents;
    gdiBackgroundPendingPresent = false;
    RecordPresentGap(NowMicros());
    ++presented;
    if (ShouldLogFrequent(gdiBackgroundPresents) || ShouldLogFrequent(presented)) {
        logs.push_back("AVC420 native-buffer GPU presented retained GDI background at "
            "EndFrame: frame=" + std::to_string(frameId) +
            " activeFrame=" + std::to_string(activeFrameId) +
            " presented=" + std::to_string(presented) +
            " gdiBg=" + std::to_string(gdiBackgroundUpdates) + "/" +
            std::to_string(gdiBackgroundPresents) +
            " surface=" + std::to_string(currentSurfaceWidth) + "x" +
            std::to_string(currentSurfaceHeight) +
            " policyActive=yes retainFrames=yes");
    }
    return true;
}

bool Avc420GpuCompositorImpl::State::PresentEndFrame(const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc420GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    RecordEndFrameGap(NowMicros());
    ++endFrameCallbacks;
    const uint32_t frameId = frame == nullptr ? 0 : frame->frameId;
    const uint32_t activeFrameId = frame == nullptr ? 0 : frame->activeFrameId;
    const bool matchedFrame = frame != nullptr && frame->matchedFrame;
    if (!pendingPresent) {
        if (PresentGdiBackgroundAtEndFrame(frameId, activeFrameId, matchedFrame,
                callbacks, outputActive, logs)) {
            return true;
        }
        ++endFrameSkipNoPending;
        if (ShouldLogFrequent(endFrameSkipNoPending) ||
            ShouldLogFrequent(endFrameCallbacks)) {
            logs.push_back("AVC420 native-buffer GPU EndFrame callback skipped: endFrame=" +
                std::to_string(frameId) + " activeFrame=" + std::to_string(activeFrameId) +
                " matched=" + std::string(matchedFrame ? "yes" : "no") +
                " pending=no callbacks=" + std::to_string(endFrameCallbacks) +
                " skipNoPending=" + std::to_string(endFrameSkipNoPending) +
                " queued=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented) +
                " decoded=" + std::to_string(decoded) +
                " gdiBgPending=" +
                std::string(gdiBackgroundPendingPresent ? "yes" : "no") +
                " " + renderer.DebugState());
        }
        return false;
    }
    return PresentQueuedUpdate("EndFrame", frameId, activeFrameId, matchedFrame,
        callbacks, outputActive, logs);
}

std::string Avc420GpuCompositorImpl::State::DebugSummary() const
{
    std::ostringstream out;
    out << "impl=queued:" << queuedPresents
        << ",decoded:" << decoded
        << ",presented:" << presented
        << ",failures:" << failures
        << ",prewarm:" << prewarms << "/" << prewarmFailures
        << ",ignored:" << ignoredUpdates
        << ",importFallbacks:" << importFallbacks
        << ",skippedWarmups:" << skippedWarmups
        << ",gdiBg:" << gdiBackgroundUpdates << "/" << gdiBackgroundPresents
        << ",gdiBgPending:" << (gdiBackgroundPendingPresent ? "yes" : "no")
        << ",nativeImportUnsupported:" << (nativeImportUnsupported ? "yes" : "no")
        << ",endCallbacks:" << endFrameCallbacks
        << ",skipNoPending:" << endFrameSkipNoPending
        << ",mismatch:" << endFrameMismatches
        << ",attempts:" << endFramePresentAttempts
        << ",commands:" << processedCommands
        << ",pending:" << (pendingPresent ? "yes" : "no")
        << ",pendingFrame:" << pendingFrameId
        << ",pendingSize:" << pendingSurfaceWidth << "x" << pendingSurfaceHeight
        << ",currentSurface:" << currentSurfaceWidth << "x" << currentSurfaceHeight
        << ",retainedComposite:yes"
        << ",lastSampleAgeUs:" << (lastSampledProcessStartUs == 0 ?
            0 : NowMicros() - lastSampledProcessStartUs)
        << ",timingSample=1/" << kTimingSampleInterval
        << ",timingUs(avg/max/count)="
        << commandTiming.Text("cmd") << ";"
        << commandIntervalTiming.Text("cmdGap") << ";"
        << offscreenEnsureTiming.Text("offEns") << ";"
        << decodeTiming.Text("dec") << ";"
        << windowEnsureTiming.Text("winEns") << ";"
        << presentTiming.Text("present")
        << "," << renderer.DebugState();
    return out.str();
}

std::string Avc420GpuCompositorImpl::State::StatsSummary()
{
    const uint64_t nowUs = NowMicros();
    const uint64_t intervalUs = nowUs >= lastStatsUs ? nowUs - lastStatsUs : 0;
    const uint64_t presentDelta =
        presented >= lastStatsPresented ? presented - lastStatsPresented : 0;
    const double fps = intervalUs == 0 ? 0.0 :
        (static_cast<double>(presentDelta) * 1000000.0) /
            static_cast<double>(intervalUs);
    const uint64_t maxCommandGapSnapshotUs = maxCommandGapUs;
    const uint64_t maxEndFrameGapSnapshotUs = maxEndFrameGapUs;
    const uint64_t maxPresentGapSnapshotUs = maxPresentGapUs;
    const uint64_t presentGap = decoded > presented ? decoded - presented : 0;
    const uint64_t callbackGap =
        endFrameCallbacks > presented ? endFrameCallbacks - presented : 0;
    lastStatsUs = nowUs;
    lastStatsPresented = presented;
    maxCommandGapUs = 0;
    maxEndFrameGapUs = 0;
    maxPresentGapUs = 0;

    std::ostringstream out;
    out << "decoded=" << decoded
        << " queuedPresents=" << queuedPresents
        << " presented=" << presented
        << " presentDelta=" << presentDelta
        << " fps=" << FormatFixed(fps, 1)
        << " maxCommandGapMs=" << FormatMs(maxCommandGapSnapshotUs)
        << " maxEndFrameGapMs=" << FormatMs(maxEndFrameGapSnapshotUs)
        << " maxPresentGapMs=" << FormatMs(maxPresentGapSnapshotUs)
        << " presentGap=" << presentGap
        << " pending=" << (pendingPresent ? "yes" : "no")
        << " pendingFrame=" << pendingFrameId
        << " endCallbacks=" << endFrameCallbacks
        << " callbackGap=" << callbackGap
        << " skipNoPending=" << endFrameSkipNoPending
        << " mismatch=" << endFrameMismatches
        << " overwrites=" << pendingPresentOverwrites
        << " failures=" << failures
        << " ignored=" << ignoredUpdates
        << " importFallbacks=" << importFallbacks
        << " skippedWarmups=" << skippedWarmups
        << " gdiBg=" << gdiBackgroundUpdates << "/" << gdiBackgroundPresents
        << " gdiBgPending=" << (gdiBackgroundPendingPresent ? "yes" : "no")
        << " nativeImportUnsupported=" << (nativeImportUnsupported ? "yes" : "no")
        << " timingUs="
        << commandTiming.Text("cmd") << ";"
        << commandIntervalTiming.Text("cmdGap") << ";"
        << offscreenEnsureTiming.Text("offEns") << ";"
        << decodeTiming.Text("dec") << ";"
        << windowEnsureTiming.Text("winEns") << ";"
        << presentTiming.Text("present")
        << " " << renderer.DebugState();
    return out.str();
}

} // namespace rdp_bridge
