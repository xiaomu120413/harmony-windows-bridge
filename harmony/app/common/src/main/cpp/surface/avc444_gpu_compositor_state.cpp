#include "surface/avc444_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

namespace rdp_bridge {

// ---------------------------------------------------------------------------
// Avc444GpuCompositorImpl static methods.
// ---------------------------------------------------------------------------

bool Avc444GpuCompositorImpl::CommandLcIsValid(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command)
{
    return IsValidLcForCommand(command);
}

std::string Avc444GpuCompositorImpl::RectText(const RECTANGLE_16* rect)
{
    return FormatRectText(rect);
}

// ---------------------------------------------------------------------------
// Avc444GpuCompositorImpl::State method bodies.
// ---------------------------------------------------------------------------

void Avc444GpuCompositorImpl::State::Destroy()
{
    renderer.Destroy();
    avcDecoder.Close();
    streamParameterSets.clear();
    queuedPresents = 0;
    presented = 0;
    failures = 0;
    ignoredUpdates = 0;
    endFrameCallbacks = 0;
    endFrameSkipInactive = 0;
    endFrameSkipNoPending = 0;
    endFrameMismatches = 0;
    endFramePresentAttempts = 0;
    pendingPresentOverwrites = 0;
    prewarms = 0;
    prewarmFailures = 0;
    pendingPresent = false;
    resetDecodersBeforeNextDecode = false;
    pendingFrameId = 0;
    pendingSurfaceWidth = 0;
    pendingSurfaceHeight = 0;
    processedCommands = 0;
    lastSampledProcessStartUs = 0;
    commandTiming.Reset();
    commandIntervalTiming.Reset();
    offscreenEnsureTiming.Reset();
    lumaDecodeTiming.Reset();
    chromaDecodeTiming.Reset();
    lumaApplyTiming.Reset();
    chromaApplyTiming.Reset();
    windowEnsureTiming.Reset();
    presentTiming.Reset();
}

bool Avc444GpuCompositorImpl::State::Prewarm(uint32_t surfaceWidth, uint32_t surfaceHeight,
    std::vector<std::string>& logs)
{
    if (surfaceWidth == 0 || surfaceHeight == 0) {
        logs.push_back("AVC444 GPU renderer prewarm skipped: invalid surface size");
        return false;
    }

    ++prewarms;
    const bool ready = renderer.Ensure(nullptr, 0, 0, surfaceWidth, surfaceHeight, logs);
    if (!ready) {
        ++prewarmFailures;
        logs.push_back("AVC444 GPU renderer prewarm failed: surface=" +
            std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
            " prewarms=" + std::to_string(prewarms) +
            " failures=" + std::to_string(prewarmFailures));
        return false;
    }

    if (ShouldLogFrequent(prewarms)) {
        logs.push_back("AVC444 GPU renderer prewarmed: surface=" +
            std::to_string(surfaceWidth) + "x" + std::to_string(surfaceHeight) +
            " prewarms=" + std::to_string(prewarms) +
            " " + renderer.DebugState());
    }
    return true;
}

bool Avc444GpuCompositorImpl::State::ProcessCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    const uint64_t commandIndex = ++processedCommands;
    const bool sampleTiming = ShouldSampleTiming(commandIndex);
    const uint64_t processStartUs = sampleTiming ? NowMicros() : 0;
    ScopedTiming commandTimer(commandTiming, processStartUs, sampleTiming);
    if (sampleTiming) {
        if (lastSampledProcessStartUs != 0 &&
            processStartUs >= lastSampledProcessStartUs) {
            commandIntervalTiming.Add(processStartUs - lastSampledProcessStartUs);
        }
        lastSampledProcessStartUs = processStartUs;
    }

    const bool codecV1 = command->codecId == RDPGFX_CODECID_AVC444;
    const bool codecV2 = command->codecId == RDPGFX_CODECID_AVC444v2;
    if (!codecV1 && !codecV2) {
        logs.push_back("AVC444 GPU compositor supports AVC444/AVC444v2 only; codec=" +
            std::to_string(command->codecId) + " stays on GDI");
        return false;
    }
    if (!RectsValid(command->stream1.regionRects, command->stream1.numRegionRects,
            command->width, command->height) ||
        (command->LC == 0 && !RectsValid(command->stream2.regionRects,
            command->stream2.numRegionRects, command->width, command->height))) {
        logs.push_back("AVC444 GPU compositor rejected invalid dirty rects");
        return false;
    }
    const bool wasOutputActive = outputActive;
    bool rendererStateTouched = false;

    auto ignoreActiveUpdate = [&](const std::string& reason, bool resetRenderer,
                                  bool resetDecoders) {
        ++ignoredUpdates;
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        if (resetRenderer) {
            logs.push_back(
                "AVC444 GPU compositor preserved the last composed state after an ignored "
                "update, matching FreeRDP's ignore-this-update behavior");
        }
        if (resetDecoders) {
            resetDecodersBeforeNextDecode = true;
            logs.push_back(
                "AVC444 GPU compositor scheduled hardware decoder reset for stream resync; "
                "cached H264 parameter sets are preserved");
        }
        logs.push_back(
            "AVC444 GPU compositor ignored update while preserving GPU ownership: " + reason +
            "; GDI remains suppressed so its H264 context is not re-entered mid-stream"
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return true;
    };

    auto dropActiveSynchronousTimeout = [&](const std::string& reason) {
        ++ignoredUpdates;
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        logs.push_back(
            "AVC444 GPU compositor dropped active update after bounded synchronous decode wait: " +
            reason + "; preserving last presented GPU frame and trying the next command"
            " because FreeRDP native GDI H264 state is behind the suppressed GPU stream"
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return true;
    };

    auto fail = [&](const std::string& reason) {
        ++failures;
        logs.push_back("AVC444 GPU compositor failed: " + reason +
            " failures=" + std::to_string(failures));
        if (wasOutputActive) {
            return ignoreActiveUpdate(reason, true, false);
        }
        if (rendererStateTouched) {
            renderer.Destroy();
            logs.push_back(
                "AVC444 GPU compositor reset warm-up renderer state after failed update; "
                "FreeRDP native GDI remains active");
        }
        return false;
    };

    const bool needsLuma = command->LC == 0 || command->LC == 1;
    const bool needsChroma = command->LC == 0 || command->LC == 2;
    DecodedFrame lumaFrame;
    DecodedFrame chromaFrame;
    bool lumaUpdated = false;
    bool chromaUpdated = false;

    if (resetDecodersBeforeNextDecode) {
        avcDecoder.Close();
        resetDecodersBeforeNextDecode = false;
        logs.push_back(
            "AVC444 GPU compositor reset the single hardware decoder before decode; "
            "waiting for cached SPS/PPS or fresh parameter sets to bootstrap");
    }

    bool offscreenReady = true;
    if (needsLuma || needsChroma) {
        ScopedTiming timing(offscreenEnsureTiming, sampleTiming);
        offscreenReady = renderer.Ensure(nullptr, 0, 0, command->width, command->height, logs);
    }
    if (!offscreenReady) {
        if (!outputActive) {
            logs.push_back("AVC444 GPU compositor offscreen renderer unavailable before decode; keeping GDI");
            return false;
        }
        rendererStateTouched = true;
        return fail("offscreen renderer init");
    }
    if (needsLuma) {
        PreparedH264Packet packet = PrepareH264Packet(command->stream1.data,
            command->stream1.length, avcDecoder.Started(), streamParameterSets,
            streamParameterSets, "luma", logs);
        if (!avcDecoder.Started() && !packet.hadParameterSets &&
            !packet.prependedParameterSets) {
            if (outputActive) {
                return ignoreActiveUpdate("single decoder missing initial SPS/PPS before luma stream",
                    true, true);
            }
            logs.push_back("AVC444 GPU compositor waits for luma SPS/PPS before single "
                "hardware decode; keeping GDI nalTypes=" + packet.nalSummary);
            return false;
        }
        if (!avcDecoder.Ensure(command->width, command->height, "avc444", logs)) {
            return fail("single avc444 decoder init before luma");
        }
        const int64_t pts = static_cast<int64_t>(++streamPts);
        DecodeResult decode = DecodeResult::Failed;
        {
            ScopedTiming timing(lumaDecodeTiming, sampleTiming);
            decode = avcDecoder.Decode(packet.data, packet.size, pts, lumaFrame, logs);
        }
        if (decode != DecodeResult::Decoded) {
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor luma warm-up decode " +
                    std::string(decode == DecodeResult::NoOutput ? "has no output yet" :
                        "failed") + "; keeping GDI");
                return false;
            }
            if (decode == DecodeResult::NoOutput) {
                return dropActiveSynchronousTimeout("luma decode output not ready");
            }
            return ignoreActiveUpdate("luma decode failed", true, true);
        }
        lumaUpdated = true;
    }

    if (needsChroma) {
        const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
            command->LC == 0 ? &command->stream2 : &command->stream1;
        PreparedH264Packet packet = PrepareH264Packet(chromaStream->data,
            chromaStream->length, avcDecoder.Started(), streamParameterSets,
            streamParameterSets, "chroma", logs);
        if (!avcDecoder.Started() && !packet.hadParameterSets &&
            !packet.prependedParameterSets) {
            if (outputActive) {
                return ignoreActiveUpdate("single decoder missing initial SPS/PPS before chroma stream",
                    true, true);
            }
            logs.push_back("AVC444 GPU compositor waits for chroma SPS/PPS before single "
                "hardware decode; keeping GDI nalTypes=" + packet.nalSummary);
            return false;
        }
        if (!avcDecoder.Ensure(command->width, command->height, "avc444", logs)) {
            return fail("single avc444 decoder init before chroma");
        }
        const int64_t pts = static_cast<int64_t>(++streamPts);
        DecodeResult decode = DecodeResult::Failed;
        {
            ScopedTiming timing(chromaDecodeTiming, sampleTiming);
            decode = avcDecoder.Decode(packet.data, packet.size, pts, chromaFrame, logs);
        }
        if (decode != DecodeResult::Decoded) {
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor chroma warm-up decode " +
                    std::string(decode == DecodeResult::NoOutput ? "has no output yet" :
                        "failed") + "; keeping GDI");
                return false;
            }
            if (decode == DecodeResult::NoOutput) {
                return dropActiveSynchronousTimeout("chroma decode output not ready");
            }
            return ignoreActiveUpdate("chroma decode failed", true, true);
        }
        chromaUpdated = true;
    }

    const uint32_t dirtyRectCount = command->stream1.numRegionRects +
        (command->LC == 0 ? command->stream2.numRegionRects : 0U);
    if (dirtyRectCount == 0) {
        logs.push_back("AVC444 GPU compositor decoded command with no dirty rects; "
            "matching FreeRDP UpdateSurfaceArea no-op frame=" +
            std::to_string(command->frameId) + " LC=" + std::to_string(command->LC));
        return outputActive;
    }

    if (needsLuma) {
        if (!lumaFrame.PlanesValid() && !avcDecoder.MapDecodedFrame(lumaFrame, logs)) {
            rendererStateTouched = true;
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor luma mapped-plane output failed; keeping GDI");
                renderer.Destroy();
                return false;
            }
            return fail("luma mapped-plane output");
        }
        bool lumaApplied = false;
        {
            ScopedTiming timing(lumaApplyTiming, sampleTiming);
            lumaApplied = renderer.ApplyLuma(lumaFrame, command->stream1.regionRects,
                command->stream1.numRegionRects, logs);
        }
        if (!lumaApplied) {
            rendererStateTouched = true;
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor luma offscreen update failed; keeping GDI");
                renderer.Destroy();
                return false;
            }
            return fail("luma shader update");
        }
        rendererStateTouched = true;
    }

    if (needsChroma) {
        const FREERDP_OHOS_RDPGFX_AVC444_STREAM_INFO* chromaStream =
            command->LC == 0 ? &command->stream2 : &command->stream1;
        if (!chromaFrame.PlanesValid() && !avcDecoder.MapDecodedFrame(chromaFrame, logs)) {
            rendererStateTouched = true;
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor chroma mapped-plane output failed; keeping GDI");
                renderer.Destroy();
                return false;
            }
            return fail("chroma mapped-plane output");
        }
        bool chromaApplied = false;
        {
            ScopedTiming timing(chromaApplyTiming, sampleTiming);
            chromaApplied = codecV1 ?
                renderer.ApplyChromaV1(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs) :
                renderer.ApplyChromaV2(chromaFrame, chromaStream->regionRects,
                    chromaStream->numRegionRects, logs);
        }
        if (!chromaApplied) {
            rendererStateTouched = true;
            if (!outputActive) {
                logs.push_back("AVC444 GPU compositor chroma offscreen update failed; keeping GDI");
                renderer.Destroy();
                return false;
            }
            return fail(codecV1 ? "chroma-v1 shader update" : "chroma-v2 shader update");
        }
        rendererStateTouched = true;
    }

    if (!renderer.ReadyToPresent()) {
        logs.push_back("AVC444 GPU compositor warmed " +
            std::string(lumaUpdated ? "luma" : "-") + "/" +
            std::string(chromaUpdated ? "chroma" : "-") +
            " state; waiting for complete luma/base-chroma state before suppressing GDI");
        if (outputActive) {
            ++ignoredUpdates;
            pendingPresent = false;
            pendingFrameId = 0;
            pendingSurfaceWidth = 0;
            pendingSurfaceHeight = 0;
            logs.push_back(
                "AVC444 GPU compositor ignored not-ready composed state while active; "
                "GDI remains suppressed and the next command will continue the stream "
                "ignoredUpdates=" + std::to_string(ignoredUpdates));
            return true;
        }
        return false;
    }

    if (pendingPresent) {
        ++pendingPresentOverwrites;
        if (ShouldLogFrequent(pendingPresentOverwrites)) {
            logs.push_back("AVC444 GPU compositor overwriting pending EndFrame present before "
                "previous one was presented: oldFrame=" + std::to_string(pendingFrameId) +
                " newFrame=" + std::to_string(command->frameId) +
                " overwrites=" + std::to_string(pendingPresentOverwrites) +
                " queued=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented));
        }
    }
    const std::string route = std::string("hardware-decode+mapped-plane-gpu-combine+") +
        (codecV1 ? "avc444v1" : "avc444v2");
    if (ShouldLogFrequent(queuedPresents + 1U)) {
        const std::string stream2Text = command->LC == 0 ? StreamText(command->stream2) : "unused";
        logs.push_back("AVC444 GPU compositor update detail: frame=" +
            std::to_string(command->frameId) +
            " LC=" + std::to_string(command->LC) +
            " lumaUpdated=" + std::string(lumaUpdated ? "yes" : "no") +
            " chromaUpdated=" + std::string(chromaUpdated ? "yes" : "no") +
            " targetHint=" + std::to_string(command->targetWidth) + "x" +
                std::to_string(command->targetHeight) +
            " remoteSurface=" + std::to_string(command->width) + "x" +
                std::to_string(command->height) +
            " targetMinusSurface=" +
                std::to_string(static_cast<int64_t>(command->targetWidth) -
                    static_cast<int64_t>(command->width)) + "x" +
                std::to_string(static_cast<int64_t>(command->targetHeight) -
                    static_cast<int64_t>(command->height)) +
            " route=" + route +
            " stream1=" + StreamText(command->stream1) +
            " stream2=" + stream2Text);
        if (lumaUpdated) {
            logs.push_back("AVC444 GPU compositor luma frame layout: " +
                FramePlaneText(lumaFrame));
        }
        if (chromaUpdated) {
            logs.push_back("AVC444 GPU compositor chroma frame layout: " +
                FramePlaneText(chromaFrame));
        }
    }
    pendingPresent = true;
    pendingFrameId = command->frameId;
    pendingSurfaceWidth = command->width;
    pendingSurfaceHeight = command->height;
    ++queuedPresents;
    if (ShouldLogFrequent(queuedPresents)) {
        logs.push_back("AVC444 GPU compositor queued EndFrame present: frame=" +
            std::to_string(command->frameId) + " LC=" + std::to_string(command->LC) +
            " queued=" + std::to_string(queuedPresents) +
            " route=" + route +
            " suppress=this-command");
    }
    if (!command->frameOpen) {
        logs.push_back("AVC444 GPU compositor queued inter-frame AVC444 update; "
            "the bridge will skip FreeRDP dirty state and trigger GPU present: frame=" +
            std::to_string(command->frameId) + " LC=" + std::to_string(command->LC));
    }
    if (!outputActive) {
        logs.push_back("AVC444 GPU compositor queued update; FreeRDP policy will decide "
            "whether GPU output becomes active before the matching frame boundary");
    }
    return true;
}

bool Avc444GpuCompositorImpl::State::PresentQueuedUpdate(const std::string& trigger,
    uint32_t frameId, uint32_t activeFrameId, bool matchedFrame,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    if (!pendingPresent) {
        logs.push_back("AVC444 GPU compositor " + trigger +
            " present skipped: pending=no policyActive=" +
            std::string(outputActive ? "yes" : "no") + " " + renderer.DebugState());
        return false;
    }

    if (!matchedFrame || frameId != pendingFrameId) {
        ++endFrameMismatches;
        ++ignoredUpdates;
        pendingPresent = false;
        const uint32_t queuedFrameId = pendingFrameId;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        logs.push_back("AVC444 GPU compositor dropped pending present at " + trigger +
            " mismatch: frame=" + std::to_string(frameId) +
            " queuedFrame=" + std::to_string(queuedFrameId) +
            " activeFrame=" + std::to_string(activeFrameId) +
            " matched=" + std::string(matchedFrame ? "yes" : "no") +
            " mismatches=" + std::to_string(endFrameMismatches) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates) +
            " " + renderer.DebugState());
        return outputActive;
    }

    if (pendingSurfaceWidth == 0 || pendingSurfaceHeight == 0) {
        ++failures;
        ++ignoredUpdates;
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        logs.push_back("AVC444 GPU compositor dropped pending " + trigger +
            " present with invalid surface dimensions; policyActive=" +
            std::string(outputActive ? "yes" : "no") +
            " failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates));
        return outputActive;
    }

    DecoderSurfaceTarget target {};
    if (callbacks.decoderSurfaceTarget != nullptr) {
        target = callbacks.decoderSurfaceTarget();
    }
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        ++failures;
        ++ignoredUpdates;
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        logs.push_back("AVC444 GPU compositor " + trigger + " target unavailable; "
            "policyActive=" + std::string(outputActive ? "yes" : "no") +
            " failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates) +
            (outputActive ? "; preserving GPU output and continuing with the next command" :
                "; FreeRDP native GDI remains active"));
        return outputActive;
    }

    const bool attachingWindowTarget = !renderer.HasWindowTarget();
    if (attachingWindowTarget) {
        logs.push_back("AVC444 GPU compositor taking XComponent target at " + trigger +
            " after FreeRDP output policy callback: target=" +
            std::to_string(target.width) + "x" + std::to_string(target.height) +
            " surface=" + std::to_string(pendingSurfaceWidth) + "x" +
            std::to_string(pendingSurfaceHeight));
    }

    auto failPresent = [&](const std::string& reason) {
        ++failures;
        ++ignoredUpdates;
        pendingPresent = false;
        pendingFrameId = 0;
        pendingSurfaceWidth = 0;
        pendingSurfaceHeight = 0;
        logs.push_back("AVC444 GPU compositor " + trigger + " present failed: " + reason +
            " policyActive=" + std::string(outputActive ? "yes" : "no") +
            " failures=" + std::to_string(failures) +
            " ignoredUpdates=" + std::to_string(ignoredUpdates) +
            (outputActive ? "; preserving GPU output and continuing with the next command" :
                "; FreeRDP native GDI remains active"));
        return outputActive;
    };

    ++endFramePresentAttempts;
    const bool sampleTiming = ShouldSampleTiming(endFramePresentAttempts);
    bool windowReady = false;
    {
        ScopedTiming timing(windowEnsureTiming, sampleTiming);
        windowReady = renderer.Ensure(target.window, target.width, target.height,
            pendingSurfaceWidth, pendingSurfaceHeight, logs);
    }
    if (!windowReady) {
        return failPresent("renderer window attach");
    }

    const bool logPresentSummary = ShouldLogFrequent(endFramePresentAttempts);
    if (logPresentSummary) {
        logs.push_back("AVC444 GPU compositor " + trigger + " present attempt: "
            "frame=" + std::to_string(frameId) +
            " pendingFrame=" + std::to_string(pendingFrameId) +
            " attempts=" + std::to_string(endFramePresentAttempts) +
            " queued=" + std::to_string(queuedPresents) +
            " presented=" + std::to_string(presented) +
            " policyActive=" + std::string(outputActive ? "yes" : "no") +
            " " + renderer.DebugState());
    }

    bool presentOk = false;
    {
        ScopedTiming timing(presentTiming, sampleTiming);
        presentOk = renderer.Present(logs, logPresentSummary);
    }
    if (!presentOk) {
        return failPresent("draw/swap");
    }

    pendingPresent = false;
    pendingFrameId = 0;
    pendingSurfaceWidth = 0;
    pendingSurfaceHeight = 0;
    ++presented;
    if (ShouldLogFrequent(presented)) {
        logs.push_back("AVC444 GPU compositor presented at " + trigger + ": frame=" +
            std::to_string(frameId) + " presented=" + std::to_string(presented) +
            " attempts=" + std::to_string(endFramePresentAttempts) +
            " queued=" + std::to_string(queuedPresents) +
            " policyActive=" + std::string(outputActive ? "yes" : "no"));
    }
    return true;
}

bool Avc444GpuCompositorImpl::State::PresentEndFrame(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    ++endFrameCallbacks;
    const uint32_t frameId = frame == nullptr ? 0 : frame->frameId;
    const uint32_t activeFrameId = frame == nullptr ? 0 : frame->activeFrameId;
    const bool matchedFrame = frame != nullptr && frame->matchedFrame;

    if (!pendingPresent) {
        ++endFrameSkipNoPending;
        if (ShouldLogFrequent(endFrameSkipNoPending) ||
            ShouldLogFrequent(endFrameCallbacks)) {
            logs.push_back("AVC444 GPU compositor EndFrame callback skipped: "
                "endFrame=" + std::to_string(frameId) +
                " activeFrame=" + std::to_string(activeFrameId) +
                " matched=" + std::string(matchedFrame ? "yes" : "no") +
                " policyActive=" + std::string(outputActive ? "yes" : "no") +
                " pending=no "
                " pendingFrame=" + std::to_string(pendingFrameId) +
                " callbacks=" + std::to_string(endFrameCallbacks) +
                " skipInactive=" + std::to_string(endFrameSkipInactive) +
                " skipNoPending=" + std::to_string(endFrameSkipNoPending) +
                " queued=" + std::to_string(queuedPresents) +
                " presented=" + std::to_string(presented) +
                " " + renderer.DebugState());
        }
        return false;
    }

    return PresentQueuedUpdate("EndFrame", frameId, activeFrameId, matchedFrame,
        callbacks, outputActive, logs);
}

std::string Avc444GpuCompositorImpl::State::DebugSummary() const
{
    std::ostringstream out;
    out << "impl=queued:" << queuedPresents
        << ",presented:" << presented
        << ",failures:" << failures
        << ",prewarm:" << prewarms << "/" << prewarmFailures
        << ",ignored:" << ignoredUpdates
        << ",endCallbacks:" << endFrameCallbacks
        << ",skipNoPending:" << endFrameSkipNoPending
        << ",mismatch:" << endFrameMismatches
        << ",attempts:" << endFramePresentAttempts
        << ",commands:" << processedCommands
        << ",pending:" << (pendingPresent ? "yes" : "no")
        << ",pendingFrame:" << pendingFrameId
        << ",pendingSize:" << pendingSurfaceWidth << "x" << pendingSurfaceHeight
        << ",lastSampleAgeUs:" << (lastSampledProcessStartUs == 0 ?
            0 : NowMicros() - lastSampledProcessStartUs)
        << ",timingSample=1/" << kTimingSampleInterval
        << ",timingUs(avg/max/count)="
        << commandTiming.Text("cmd") << ";"
        << commandIntervalTiming.Text("cmdGap") << ";"
        << offscreenEnsureTiming.Text("offEns") << ";"
        << lumaDecodeTiming.Text("lDec") << ";"
        << chromaDecodeTiming.Text("cDec") << ";"
        << lumaApplyTiming.Text("lApply") << ";"
        << chromaApplyTiming.Text("cApply") << ";"
        << windowEnsureTiming.Text("winEns") << ";"
        << presentTiming.Text("present")
        << "," << renderer.DebugState();
    return out.str();
}

// ---------------------------------------------------------------------------
// Avc444GpuCompositorImpl outer member definitions (forwarding to State).
// ---------------------------------------------------------------------------

Avc444GpuCompositorImpl::Avc444GpuCompositorImpl() : state_(std::make_unique<State>()) {}

Avc444GpuCompositorImpl::~Avc444GpuCompositorImpl() = default;

void Avc444GpuCompositorImpl::Destroy()
{
    if (state_) {
        state_->Destroy();
    }
}

bool Avc444GpuCompositorImpl::Prewarm(
    uint32_t surfaceWidth, uint32_t surfaceHeight, std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->Prewarm(surfaceWidth, surfaceHeight, logs);
}

std::string Avc444GpuCompositorImpl::DebugSummary() const
{
    return state_ == nullptr ? "impl=null" : state_->DebugSummary();
}

bool Avc444GpuCompositorImpl::ProcessCommand(
    const FREERDP_OHOS_RDPGFX_AVC444_COMMAND_INFO* command,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->ProcessCommand(command, callbacks, outputActive, logs);
}

bool Avc444GpuCompositorImpl::PresentEndFrame(
    const FREERDP_OHOS_RDPGFX_FRAME_INFO* frame,
    const Avc444GpuCompositorCallbacks& callbacks, bool outputActive,
    std::vector<std::string>& logs)
{
    return state_ != nullptr && state_->PresentEndFrame(frame, callbacks, outputActive, logs);
}

} // namespace rdp_bridge
