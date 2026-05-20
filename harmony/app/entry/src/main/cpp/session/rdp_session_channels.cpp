#include "session/rdp_session_channels.h"

#include "channels/rdpgfx_diagnostics.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <unordered_map>
#include <utility>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client/channels.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/rdpgfx.h>
#endif

namespace rdp_bridge {

void RdpSessionChannels::SetCallbacks(Callbacks callbacks)
{
    callbacks_ = std::move(callbacks);
}

void RdpSessionChannels::EmitLog(const std::string& line)
{
    if (callbacks_.log != nullptr) {
        callbacks_.log(line);
    }
}

void RdpSessionChannels::SetDynamicResizeAlignment(uint32_t alignment)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(activeMutex_);
    dynamicResizeAlignment_ = std::max<uint32_t>(1U, alignment);
    lastDynamicResizeWidth_ = 0;
    lastDynamicResizeHeight_ = 0;
#else
    (void)alignment;
#endif
}

void RdpSessionChannels::RequestDisconnect()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeApi_ != nullptr && activeContext_ != nullptr) {
        activeApi_->abortConnectContext(activeContext_);
    }
#endif
}

bool RdpSessionChannels::RequestCurrentFrameRender(const std::string& reason, std::string& message)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeContext_ == nullptr || activeContext_->gdi == nullptr) {
        message = "FreeRDP GDI context is not ready";
        return false;
    }
    if (callbacks_.queueFrame == nullptr) {
        message = "surface frame queue is not configured";
        return false;
    }

    rdpGdi* gdi = activeContext_->gdi;
    if (gdi->suppressOutput || gdi->primary_buffer == nullptr || gdi->width <= 0 ||
        gdi->height <= 0 || gdi->stride == 0) {
        message = "FreeRDP GDI primary buffer is not ready";
        return false;
    }
    if (!RdpPrimaryFrameReady()) {
        message.clear();
        return false;
    }

    RgbaFrame frame = {
        gdi->primary_buffer,
        static_cast<uint32_t>(gdi->width),
        static_cast<uint32_t>(gdi->height),
        static_cast<int32_t>(gdi->stride),
        reason.empty() ? "surface repaint" : reason,
        DirtyFrameStats{},
    };

    std::string queueMessage;
    if (!callbacks_.queueFrame(frame, queueMessage, true)) {
        message = queueMessage;
        return false;
    }

    message = std::to_string(frame.width) + "x" + std::to_string(frame.height) +
        " current-gdi";
    return true;
#else
    (void)reason;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

bool RdpSessionChannels::RequestDynamicDesktopResize(uint32_t width, uint32_t height,
    const std::string& reason, std::string& message)
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeApi_ == nullptr || activeApi_->ohosDisplayNormalizeSize == nullptr ||
        activeApi_->ohosDisplaySendMonitorLayout == nullptr) {
        message = "FreeRDP OHOS display-control helper symbols are not loaded";
        return false;
    }
    const uint32_t requestedWidth = width;
    const uint32_t requestedHeight = height;
    activeApi_->ohosDisplayNormalizeSize(width, height, dynamicResizeAlignment_, &width, &height);
    const std::string normalizeText =
        " requested=" + std::to_string(requestedWidth) + "x" +
        std::to_string(requestedHeight) +
        " normalized=" + std::to_string(width) + "x" + std::to_string(height) +
        " alignment=" + std::to_string(dynamicResizeAlignment_);
    if (activeDisp_ == nullptr || activeDisp_->SendMonitorLayout == nullptr) {
        message = "display-control channel is not ready;" + normalizeText;
        return false;
    }
    if (!displayControlCapsReady_) {
        message = "display-control caps are not ready;" + normalizeText;
        return false;
    }
    if (lastDynamicResizeWidth_ == width && lastDynamicResizeHeight_ == height) {
        message = "display-control resize unchanged: " + std::to_string(width) + "x" +
            std::to_string(height) + ";" + normalizeText;
        return true;
    }

    uint32_t sentWidth = 0;
    uint32_t sentHeight = 0;
    uint32_t channelStatus = 0;
    std::array<char, 192> detail {};
    if (activeApi_->ohosDisplaySendMonitorLayout(activeDisp_, width, height, 1U, &sentWidth,
        &sentHeight, &channelStatus, detail.data(), detail.size()) == 0) {
        if (detail[0] != '\0') {
            message = detail.data();
        } else {
            message = "display-control resize failed: " + std::to_string(channelStatus);
        }
        return false;
    }

    lastDynamicResizeWidth_ = sentWidth;
    lastDynamicResizeHeight_ = sentHeight;
    message = "display-control resize requested after " + reason + ": " +
        (detail[0] != '\0' ? detail.data() : (std::to_string(sentWidth) + "x" +
            std::to_string(sentHeight))) + ";" + normalizeText;
    return true;
#else
    (void)width;
    (void)height;
    (void)reason;
    message = "FreeRDP headers not found at build time";
    return false;
#endif
}

#if defined(HARMONY_HAS_FREERDP_HEADERS)
void RdpSessionChannels::SetActive(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context)
{
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        activeApi_ = api;
        activeInstance_ = instance;
        activeContext_ = context;
        activeDisp_ = nullptr;
        activeGfx_ = nullptr;
        displayControlCapsReady_ = false;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
    }

    RegisterSession(context, this);
    if (api != nullptr && api->pubSubSubscribe != nullptr && context != nullptr &&
        context->pubSub != nullptr) {
        (void)api->pubSubSubscribe(context->pubSub, "ChannelConnected", OnChannelConnected);
        (void)api->pubSubSubscribe(context->pubSub, "ChannelDisconnected", OnChannelDisconnected);
    }
}

void RdpSessionChannels::ClearActive(freerdp* instance)
{
    rdpContext* oldContext = nullptr;
    FreerdpRuntimeApi* oldApi = nullptr;
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeInstance_ != instance) {
        return;
    }

    oldContext = activeContext_;
    oldApi = activeApi_;
    DetachGraphicsPipelineLocked(activeGfx_);
    if (oldApi != nullptr && oldApi->pubSubUnsubscribe != nullptr && oldContext != nullptr &&
        oldContext->pubSub != nullptr) {
        (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelConnected", OnChannelConnected);
        (void)oldApi->pubSubUnsubscribe(oldContext->pubSub, "ChannelDisconnected", OnChannelDisconnected);
    }
    UnregisterSession(oldContext);
    activeApi_ = nullptr;
    activeInstance_ = nullptr;
    activeContext_ = nullptr;
    activeDisp_ = nullptr;
    activeGfx_ = nullptr;
    displayControlCapsReady_ = false;
    dynamicResizeAlignment_ = 1;
    lastDynamicResizeWidth_ = 0;
    lastDynamicResizeHeight_ = 0;
}

std::mutex& RdpSessionChannels::RegistryMutex()
{
    static std::mutex mutex;
    return mutex;
}

std::unordered_map<rdpContext*, RdpSessionChannels*>& RdpSessionChannels::Registry()
{
    static std::unordered_map<rdpContext*, RdpSessionChannels*> registry;
    return registry;
}

void RdpSessionChannels::RegisterSession(rdpContext* context, RdpSessionChannels* session)
{
    if (context == nullptr || session == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(RegistryMutex());
    Registry()[context] = session;
}

void RdpSessionChannels::UnregisterSession(rdpContext* context)
{
    if (context == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(RegistryMutex());
    Registry().erase(context);
}

RdpSessionChannels* RdpSessionChannels::FindSession(rdpContext* context)
{
    std::lock_guard<std::mutex> lock(RegistryMutex());
    auto iter = Registry().find(context);
    return iter == Registry().end() ? nullptr : iter->second;
}

bool RdpSessionChannels::IsDisplayControlChannel(const char* name)
{
    return name != nullptr &&
        (std::strcmp(name, DISP_CHANNEL_NAME) == 0 ||
            std::strcmp(name, DISP_DVC_CHANNEL_NAME) == 0);
}

bool RdpSessionChannels::IsGraphicsPipelineChannel(const char* name)
{
    return name != nullptr && std::strcmp(name, RDPGFX_DVC_CHANNEL_NAME) == 0;
}

void RdpSessionChannels::OnChannelConnected(void* context, const ChannelConnectedEventArgs* event)
{
    if (context == nullptr || event == nullptr || event->name == nullptr) {
        return;
    }

    RdpSessionChannels* session = FindSession(static_cast<rdpContext*>(context));
    if (session == nullptr) {
        return;
    }
    if (IsDisplayControlChannel(event->name)) {
        session->AttachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
    } else if (IsGraphicsPipelineChannel(event->name)) {
        session->AttachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
    }
}

void RdpSessionChannels::OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event)
{
    if (context == nullptr || event == nullptr || event->name == nullptr) {
        return;
    }

    RdpSessionChannels* session = FindSession(static_cast<rdpContext*>(context));
    if (session == nullptr) {
        return;
    }
    if (IsDisplayControlChannel(event->name)) {
        session->DetachDisplayControl(static_cast<DispClientContext*>(event->pInterface));
    } else if (IsGraphicsPipelineChannel(event->name)) {
        session->DetachGraphicsPipeline(static_cast<RdpgfxClientContext*>(event->pInterface));
    }
}

UINT RdpSessionChannels::DisplayControlCaps(DispClientContext* disp, UINT32 maxNumMonitors,
    UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB)
{
    auto* session = disp == nullptr ? nullptr : static_cast<RdpSessionChannels*>(disp->custom);
    if (session != nullptr) {
        session->HandleDisplayControlCaps(maxNumMonitors, maxMonitorAreaFactorA,
            maxMonitorAreaFactorB);
    }
    return CHANNEL_RC_OK;
}

void RdpSessionChannels::HandleDisplayControlCaps(UINT32 maxNumMonitors,
    UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB)
{
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        displayControlCapsReady_ = true;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
    }

    EmitLog("display-control caps: maxMonitors=" + std::to_string(maxNumMonitors) +
        " areaFactor=" + std::to_string(maxMonitorAreaFactorA) + "/" +
        std::to_string(maxMonitorAreaFactorB));

    if (callbacks_.surfaceSnapshot == nullptr) {
        return;
    }
    const SurfaceSnapshot snapshot = callbacks_.surfaceSnapshot();
    if (snapshot.width > 0 && snapshot.height > 0) {
        std::string resizeMessage;
        if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
            "display-control caps", resizeMessage)) {
            EmitLog(resizeMessage);
        } else {
            EmitLog("display-control resize skipped after display-control caps: " +
                resizeMessage);
        }
    }
}

void RdpSessionChannels::AttachDisplayControl(DispClientContext* disp)
{
    if (disp == nullptr) {
        EmitLog("display-control connected without client context");
        return;
    }

    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        activeDisp_ = disp;
        activeDisp_->custom = this;
        activeDisp_->DisplayControlCaps = DisplayControlCaps;
        displayControlCapsReady_ = false;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
    }
    EmitLog("display-control connected to HarmonyOS window resize bridge");

    if (callbacks_.surfaceSnapshot == nullptr) {
        return;
    }
    const SurfaceSnapshot snapshot = callbacks_.surfaceSnapshot();
    if (snapshot.width > 0 && snapshot.height > 0) {
        std::string resizeMessage;
        if (RequestDynamicDesktopResize(snapshot.width, snapshot.height,
            "display-control connected", resizeMessage)) {
            EmitLog(resizeMessage);
        } else {
            EmitLog("display-control resize skipped after display-control connected: " +
                resizeMessage);
        }
    }
}

void RdpSessionChannels::DetachDisplayControl(DispClientContext* disp)
{
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeDisp_ != nullptr && activeDisp_ == disp) {
        activeDisp_->custom = nullptr;
        activeDisp_ = nullptr;
        displayControlCapsReady_ = false;
        lastDynamicResizeWidth_ = 0;
        lastDynamicResizeHeight_ = 0;
        EmitLog("display-control disconnected from HarmonyOS window resize bridge");
    }
}

void RdpSessionChannels::AttachGraphicsPipeline(RdpgfxClientContext* gfx)
{
    std::string message;
    bool attached = false;
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (gfx == nullptr) {
            message = "rdpgfx connected without client context";
        } else if (activeApi_ == nullptr || activeContext_ == nullptr || activeContext_->gdi == nullptr) {
            IncrementRdpgfxInitFailed();
            message = "rdpgfx connected before GDI context was ready";
        } else if (activeApi_->gdiGraphicsPipelineInit == nullptr) {
            IncrementRdpgfxInitFailed();
            message = "rdpgfx GDI pipeline init symbol unavailable";
        } else {
            if (activeGfx_ != nullptr && activeGfx_ != gfx) {
                DetachGraphicsPipelineLocked(activeGfx_);
            }
            if (activeApi_->gdiGraphicsPipelineInit(activeContext_->gdi, gfx)) {
                InstallRdpgfxDiagnosticsHooks(gfx);
                activeGfx_ = gfx;
                SetRdpgfxBridgeAttached(true);
                IncrementRdpgfxConnected();
                attached = true;
                message = "rdpgfx connected to FreeRDP GDI graphics pipeline";
            } else {
                IncrementRdpgfxInitFailed();
                message = "rdpgfx GDI graphics pipeline init failed";
            }
        }
    }

    EmitLog(message);
    if (attached && callbacks_.requestSurfaceRepaint != nullptr) {
        callbacks_.requestSurfaceRepaint("rdpgfx connected");
    }
}

void RdpSessionChannels::DetachGraphicsPipeline(RdpgfxClientContext* gfx)
{
    bool detached = false;
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        detached = DetachGraphicsPipelineLocked(gfx);
    }
    if (detached) {
        EmitLog("rdpgfx disconnected from FreeRDP GDI graphics pipeline");
    }
}

bool RdpSessionChannels::DetachGraphicsPipelineLocked(RdpgfxClientContext* gfx)
{
    if (activeGfx_ == nullptr || activeGfx_ != gfx) {
        return false;
    }
    RestoreRdpgfxDiagnosticsHooks(activeGfx_);
    if (activeApi_ != nullptr && activeApi_->gdiGraphicsPipelineUninit != nullptr &&
        activeContext_ != nullptr && activeContext_->gdi != nullptr) {
        activeApi_->gdiGraphicsPipelineUninit(activeContext_->gdi, activeGfx_);
    }
    activeGfx_ = nullptr;
    SetRdpgfxBridgeAttached(false);
    IncrementRdpgfxDisconnected();
    return true;
}
#endif

} // namespace rdp_bridge
