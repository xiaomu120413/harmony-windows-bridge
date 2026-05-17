#include "channels/rdpgfx_pipeline.h"

#include "channels/rdpgfx_diagnostics.h"
#include "freerdp_runtime.h"
#include "string_utils.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <unordered_map>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/channels/rdpgfx.h>
#include <freerdp/client/channels.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#endif

namespace rdp_bridge {
namespace {

std::atomic_bool g_avc420SurfaceOutputConfigured{false};
std::atomic_bool g_avc420SurfaceOutputActive{false};
std::atomic<uint64_t> g_avcSurfaceSubrectSkipCount{0};
std::atomic<uint64_t> g_avcSurfaceNoDirectCount{0};
std::mutex g_callbacksMutex;
RdpgfxPipelineCallbacks g_callbacks;

RdpgfxPipelineCallbacks SnapshotCallbacks()
{
    std::lock_guard<std::mutex> lock(g_callbacksMutex);
    return g_callbacks;
}

void LogThroughCallbacks(const std::string& line)
{
    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.log != nullptr) {
        callbacks.log(line);
    }
}

bool ShouldLogAvcSurfaceCounter(uint64_t count)
{
    return count == 1 || (count % 300U) == 0;
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
struct RdpgfxDiagnosticsHookState {
    pcRdpgfxStartFrame startFrame = nullptr;
    pcRdpgfxEndFrame endFrame = nullptr;
    pcRdpgfxSurfaceCommand surfaceCommand = nullptr;
    pcRdpgfxCapsConfirm capsConfirm = nullptr;
};

std::mutex g_rdpgfxHooksMutex;
std::unordered_map<RdpgfxClientContext*, RdpgfxDiagnosticsHookState> g_rdpgfxHooks;

struct RdpgfxPluginCapsSnapshot {
    GENERIC_DYNVC_PLUGIN base;
    void* zgfx;
    UINT32 unacknowledgedFrames;
    UINT32 totalDecodedFrames;
    UINT64 startDecodingTime;
    BOOL suspendFrameAcks;
    BOOL sendFrameAcks;
    void* surfaceTable;
    UINT16 maxCacheSlots;
    void* cacheSlots[25600];
    void* persistent;
    rdpContext* rdpcontext;
    RDPGFX_CAPSET connectionCaps;
    RdpgfxClientContext* context;
};

void RecordRdpgfxConnectionCapsSnapshot(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr || gfx->handle == nullptr) {
        return;
    }

    const auto* plugin = static_cast<const RdpgfxPluginCapsSnapshot*>(gfx->handle);
    if (plugin->connectionCaps.version == 0) {
        LogThroughCallbacks("RDPGFX connection caps snapshot unavailable: version=0");
        return;
    }

    RecordRdpgfxCapsConfirmValues(plugin->connectionCaps.version, plugin->connectionCaps.flags,
        "connection-caps-snapshot");
}

bool RdpgfxCapsConfirmAvc420(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    return capsConfirm != nullptr && capsConfirm->capsSet != nullptr &&
        capsConfirm->capsSet->version == RDPGFX_CAPVERSION_81 &&
        (capsConfirm->capsSet->flags & RDPGFX_CAPS_FLAG_AVC420_ENABLED) != 0;
}

bool RdpgfxCapsConfirmAvc444(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    if (capsConfirm == nullptr || capsConfirm->capsSet == nullptr) {
        return false;
    }

    const uint32_t version = capsConfirm->capsSet->version;
    const uint32_t flags = capsConfirm->capsSet->flags;
    return version == RDPGFX_CAPVERSION_101 ||
        (version >= RDPGFX_CAPVERSION_10 && (flags & RDPGFX_CAPS_FLAG_AVC_DISABLED) == 0);
}

bool IsH264SurfaceCodec(uint32_t codecId)
{
    return codecId == RDPGFX_CODECID_AVC420 ||
        codecId == RDPGFX_CODECID_AVC444 ||
        codecId == RDPGFX_CODECID_AVC444v2;
}

std::string RdpgfxCapsConfirmSummary(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    if (capsConfirm == nullptr || capsConfirm->capsSet == nullptr) {
        return "capsConfirm=null";
    }

    const RDPGFX_CAPSET* capsSet = capsConfirm->capsSet;
    return "version=" + Hex32(capsSet->version) + " flags=" + Hex32(capsSet->flags);
}

bool IsFullWindowAvcCommand(const RDPGFX_SURFACE_COMMAND* command, const DecoderSurfaceTarget& target)
{
    if (command == nullptr) {
        return false;
    }
    if (command->left != 0 || command->top != 0 || command->width == 0 || command->height == 0) {
        return false;
    }
    if (target.width == 0 || target.height == 0 || command->width != target.width) {
        return false;
    }

    const uint32_t heightDelta = target.height > command->height ?
        target.height - command->height : command->height - target.height;
    return heightDelta <= 16U;
}

bool BindAvcSurfaceOutput(const std::string& reason, const RDPGFX_SURFACE_COMMAND* command)
{
    if (!g_avc420SurfaceOutputConfigured.load()) {
        return false;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return false;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": decoder surface callback is not configured");
        return false;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        LogThroughCallbacks("AVC surface output activation skipped after " + reason +
            ": XComponent surface unavailable");
        return false;
    }
    if (!IsFullWindowAvcCommand(command, target)) {
        const uint64_t skipCount =
            g_avcSurfaceSubrectSkipCount.fetch_add(1, std::memory_order_relaxed) + 1;
        if (ShouldLogAvcSurfaceCounter(skipCount)) {
            LogThroughCallbacks("AVC surface output activation skipped after " + reason +
                ": command is a sub-rectangle surface update, not a full-window frame; command=" +
                std::to_string(command == nullptr ? 0 : command->width) + "x" +
                std::to_string(command == nullptr ? 0 : command->height) + " at " +
                std::to_string(command == nullptr ? 0 : command->left) + "," +
                std::to_string(command == nullptr ? 0 : command->top) +
                " target=" + std::to_string(target.width) + "x" + std::to_string(target.height) +
                " count=" + std::to_string(skipCount));
        }
        return false;
    }

    if (!api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE)) {
        LogThroughCallbacks("AVC surface output activation failed after " + reason +
            ": OHOS AVCodec surface setup failed");
        return false;
    }

    if (!g_avc420SurfaceOutputActive.exchange(true)) {
        if (callbacks.stopRenderPipeline != nullptr) {
            callbacks.stopRenderPipeline();
        }
        std::string commandText;
        if (command != nullptr) {
            commandText = " surface=" + std::to_string(command->surfaceId) +
                " size=" + std::to_string(command->width) + "x" +
                std::to_string(command->height);
        }
        LogThroughCallbacks("AVC surface output activated after " + reason +
            ": target=" + std::to_string(target.width) + "x" + std::to_string(target.height) +
            commandText);
    } else {
        LogThroughCallbacks("AVC surface output updated after " + reason + ": target=" +
            std::to_string(target.width) + "x" + std::to_string(target.height));
    }
    return true;
}

void SwitchAvc420SurfaceToSoftwareFallback(const std::string& reason)
{
    if (!g_avc420SurfaceOutputConfigured.exchange(false)) {
        return;
    }
    g_avc420SurfaceOutputActive.store(false);

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
    }
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(nullptr, nullptr);
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.startRenderPipeline != nullptr) {
        callbacks.startRenderPipeline();
    }
    LogThroughCallbacks("AVC420 surface output disabled; using FreeRDP buffer/GLES fallback: " + reason);
}

void OnOhosAvcodecFallback(const char* reason, void*)
{
    SwitchAvc420SurfaceToSoftwareFallback(
        std::string("OHOS AVCodec runtime fallback: ") + SafeCString(reason));
}

UINT HarmonyRdpgfxStartFrame(RdpgfxClientContext* context, const RDPGFX_START_FRAME_PDU* startFrame)
{
    RecordRdpgfxStartFrame();
    pcRdpgfxStartFrame original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.startFrame;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, startFrame);
}

UINT HarmonyRdpgfxEndFrame(RdpgfxClientContext* context, const RDPGFX_END_FRAME_PDU* endFrame)
{
    RecordRdpgfxEndFrame();
    pcRdpgfxEndFrame original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.endFrame;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, endFrame);
}

UINT HarmonyRdpgfxCapsConfirm(RdpgfxClientContext* context, const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    RecordRdpgfxCapsConfirm(capsConfirm);

    if (g_avc420SurfaceOutputConfigured.load()) {
        const std::string summary = RdpgfxCapsConfirmSummary(capsConfirm);
        if (RdpgfxCapsConfirmAvc420(capsConfirm)) {
            LogThroughCallbacks("RDPGFX negotiated AVC420 surface mode: " + summary +
                "; GDI remains active until the first AVC420 surface command");
        } else if (RdpgfxCapsConfirmAvc444(capsConfirm)) {
            LogThroughCallbacks("RDPGFX negotiated AVC444 surface mode: " + summary +
                "; AVC444 primary stream will use the AVC420 output surface");
        } else {
            SwitchAvc420SurfaceToSoftwareFallback("server selected non-AVC graphics mode " + summary);
        }
    }

    pcRdpgfxCapsConfirm original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.capsConfirm;
        }
    }
    return original == nullptr ? CHANNEL_RC_OK : original(context, capsConfirm);
}

UINT HarmonyRdpgfxSurfaceCommand(RdpgfxClientContext* context, const RDPGFX_SURFACE_COMMAND* command)
{
    if (command != nullptr) {
        RecordRdpgfxSurfaceCommand(*command);
        if (IsH264SurfaceCodec(command->codecId) && !g_avc420SurfaceOutputActive.load()) {
            if (!BindAvcSurfaceOutput(std::string(RdpgfxCodecName(command->codecId)) +
                " surface command", command)) {
                const uint64_t noDirectCount =
                    g_avcSurfaceNoDirectCount.fetch_add(1, std::memory_order_relaxed) + 1;
                if (ShouldLogAvcSurfaceCounter(noDirectCount)) {
                    LogThroughCallbacks("AVC surface command reached FreeRDP without direct surface output: codec=" +
                        std::string(RdpgfxCodecName(command->codecId)) +
                        " surface=" + std::to_string(command->surfaceId) +
                        " size=" + std::to_string(command->width) + "x" +
                        std::to_string(command->height) +
                        " count=" + std::to_string(noDirectCount));
                }
            }
        }
    }

    pcRdpgfxSurfaceCommand original = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
        auto iter = g_rdpgfxHooks.find(context);
        if (iter != g_rdpgfxHooks.end()) {
            original = iter->second.surfaceCommand;
        }
    }
    return original == nullptr ? ERROR_INTERNAL_ERROR : original(context, command);
}
#endif

} // namespace

void SetRdpgfxPipelineCallbacks(RdpgfxPipelineCallbacks callbacks)
{
    std::lock_guard<std::mutex> lock(g_callbacksMutex);
    g_callbacks = std::move(callbacks);
}

bool IsAvc420SurfaceOutputEnabled()
{
    return g_avc420SurfaceOutputActive.load();
}

void UpdateAvc420SurfaceOutputIfActive(const std::string& reason)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    if (!g_avc420SurfaceOutputConfigured.load() || !g_avc420SurfaceOutputActive.load()) {
        return;
    }

    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        LogThroughCallbacks("AVC420 surface update skipped after " + reason +
            ": OHOS AVCodec surface symbol is not loaded");
        return;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        LogThroughCallbacks("AVC420 surface update skipped after " + reason +
            ": decoder surface callback is not configured");
        return;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        g_avc420SurfaceOutputActive.store(false);
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
        if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
            api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
        }
        if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
            api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
        }
        if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
            api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
        }
        if (callbacks.startRenderPipeline != nullptr) {
            callbacks.startRenderPipeline();
        }
        LogThroughCallbacks("AVC420 surface output disabled after " + reason +
            ": XComponent surface unavailable");
        return;
    }

    api.ohosAvcodecSetOutputSurface(target.window, target.width, target.height, TRUE);
    LogThroughCallbacks("AVC surface output updated after " + reason + ": " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " avc444=primary-avc420-surface");
#else
    (void)reason;
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void ResetAvcSurfaceOutput(FreerdpRuntimeApi& api)
{
    g_avc420SurfaceOutputConfigured.store(false);
    g_avc420SurfaceOutputActive.store(false);
    g_avcSurfaceSubrectSkipCount.store(0);
    g_avcSurfaceNoDirectCount.store(0);
    if (api.ohosAvcodecSetOutputSurface != nullptr) {
        api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444OutputSurfaces != nullptr) {
        api.ohosAvcodecSetAvc444OutputSurfaces(nullptr, nullptr, 0, 0, FALSE);
    }
    if (api.ohosAvcodecSetAvc444SurfaceRouteEnabled != nullptr) {
        api.ohosAvcodecSetAvc444SurfaceRouteEnabled(FALSE);
    }
    if (api.ohosAvcodecSetAvc444FrameCallback != nullptr) {
        api.ohosAvcodecSetAvc444FrameCallback(nullptr, nullptr);
    }
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(nullptr, nullptr);
    }
}

bool ConfigureAvc420SurfaceOutput(FreerdpRuntimeApi& api, const GraphicsPipelineConfig& graphicsConfig,
    const FreerdpLogFn& log, std::string& error)
{
    if (!graphicsConfig.enabled || !graphicsConfig.h264) {
        ResetAvcSurfaceOutput(api);
        return true;
    }

    if (api.ohosAvcodecSetOutputSurface == nullptr) {
        error = "OHOS AVCodec surface output symbol is not loaded";
        return false;
    }

    RdpgfxPipelineCallbacks callbacks = SnapshotCallbacks();
    if (callbacks.decoderSurfaceTarget == nullptr) {
        error = "AVC420 surface output requires a decoder surface callback";
        return false;
    }

    const DecoderSurfaceTarget target = callbacks.decoderSurfaceTarget();
    if (target.window == nullptr || target.width == 0 || target.height == 0) {
        error = "AVC420 surface output requires a ready XComponent NativeWindow";
        return false;
    }

    api.ohosAvcodecSetOutputSurface(nullptr, 0, 0, FALSE);
    if (api.ohosAvcodecSetFallbackCallback != nullptr) {
        api.ohosAvcodecSetFallbackCallback(OnOhosAvcodecFallback, nullptr);
    }

    g_avc420SurfaceOutputConfigured.store(true);
    g_avc420SurfaceOutputActive.store(false);
    g_avcSurfaceSubrectSkipCount.store(0);
    g_avcSurfaceNoDirectCount.store(0);
    log("OHOS AVCodec output surface armed: XComponent NativeWindow " +
        std::to_string(target.width) + "x" + std::to_string(target.height) +
        " mode=deferred-until-avc-surface-command avc444=primary-avc420-surface gdi=active-before-h264");
    return true;
}

bool ConfigureGraphicsPipelineChannel(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig, const FreerdpLogFn& log, std::string& error)
{
    SetRdpgfxRuntimeRequest(graphicsConfig.enabled, graphicsConfig.enabled && graphicsConfig.h264);
    SetRdpgfxBridgeAttached(false);
    ResetRdpgfxDiagnosticsStats();

    if (!graphicsConfig.enabled) {
        log("FreeRDP rdpgfx dynamic channel not requested: graphicsMode=gdi");
        log(BuildGraphicsPipelineStatsLog());
        return true;
    }

    if (api.clientAddDynamicChannel == nullptr) {
        error = "FreeRDP rdpgfx dynamic channel helper is not loaded";
        return false;
    }
    if (api.gdiGraphicsPipelineInit == nullptr || api.gdiGraphicsPipelineUninit == nullptr) {
        error = "FreeRDP GDI graphics pipeline symbols are not loaded";
        return false;
    }

    const char* params[] = {RDPGFX_CHANNEL_NAME};
    if (!api.clientAddDynamicChannel(settings, sizeof(params) / sizeof(params[0]), params)) {
        error = "set rdpgfx dynamic channel failed";
        return false;
    }

    log("FreeRDP rdpgfx requested: dynamic channel + GDI graphics pipeline bridge");
    log(BuildGraphicsPipelineStatsLog());
    return true;
}

void InstallRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
    if (g_rdpgfxHooks.find(gfx) != g_rdpgfxHooks.end()) {
        return;
    }

    RdpgfxDiagnosticsHookState state;
    state.startFrame = gfx->StartFrame;
    state.endFrame = gfx->EndFrame;
    state.surfaceCommand = gfx->SurfaceCommand;
    state.capsConfirm = gfx->CapsConfirm;
    g_rdpgfxHooks[gfx] = state;
    if (state.startFrame != nullptr) {
        gfx->StartFrame = HarmonyRdpgfxStartFrame;
    }
    if (state.endFrame != nullptr) {
        gfx->EndFrame = HarmonyRdpgfxEndFrame;
    }
    if (state.surfaceCommand != nullptr) {
        gfx->SurfaceCommand = HarmonyRdpgfxSurfaceCommand;
    }
    gfx->CapsConfirm = HarmonyRdpgfxCapsConfirm;
    RecordRdpgfxConnectionCapsSnapshot(gfx);
}

void RestoreRdpgfxDiagnosticsHooks(RdpgfxClientContext* gfx)
{
    if (gfx == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> lock(g_rdpgfxHooksMutex);
    auto iter = g_rdpgfxHooks.find(gfx);
    if (iter == g_rdpgfxHooks.end()) {
        return;
    }

    gfx->StartFrame = iter->second.startFrame;
    gfx->EndFrame = iter->second.endFrame;
    gfx->SurfaceCommand = iter->second.surfaceCommand;
    gfx->CapsConfirm = iter->second.capsConfirm;
    g_rdpgfxHooks.erase(iter);
}
#endif

} // namespace rdp_bridge
