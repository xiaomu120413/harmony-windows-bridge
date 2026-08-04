#include "session/rdp_session_channels.h"

#include "channels/rdpgfx_diagnostics.h"
#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_gdi_bridge.h"

#include <array>
#include <cstring>
#include <unordered_map>
#include <utility>

#include <freerdp/client/channels.h>
#include <freerdp/channels/disp.h>
#include <freerdp/channels/rdpgfx.h>

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

void RdpSessionChannels::RequestDisconnect()
{
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeApi_ != nullptr && activeContext_ != nullptr) {
        activeApi_->abortConnectContext(activeContext_);
    }
}

void RdpSessionChannels::SetDisplayOrientation(uint32_t orientation)
{
    displayOrientation_.store(orientation);
}

uint32_t RdpSessionChannels::DisplayOrientation() const
{
    return displayOrientation_.load();
}

bool RdpSessionChannels::RequestCurrentFrameRender(const std::string& reason, std::string& message)
{
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
}

bool RdpSessionChannels::RequestDynamicDesktopResize(uint32_t width, uint32_t height,
    const std::string& reason, std::string& message)
{
    const DisplayResizeResult result = RequestDynamicDesktopResizeEx(
        width, height, DisplayOrientation(), reason);
    message = result.message;
    return result.status != DisplayResizeStatus::Failed &&
        result.status != DisplayResizeStatus::Unsupported;
}

DisplayResizeResult RdpSessionChannels::RequestDynamicDesktopResizeEx(uint32_t width,
    uint32_t height, uint32_t orientation, const std::string& reason)
{
    DisplayResizeResult result;
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeApi_ == nullptr) {
        result.status = DisplayResizeStatus::Unsupported;
        result.message = "FreeRDP OHOS session runtime is not active";
        return result;
    }
    if (activeOhosSession_ == nullptr) {
        result.status = DisplayResizeStatus::Failed;
        result.message = "FreeRDP OHOS session resize target is not active";
        return result;
    }

    std::array<char, 192> detail {};
    if (activeApi_->ohosSessionResizeEx != nullptr) {
        FREERDP_OHOS_SESSION_RESIZE_REQUEST request {
            sizeof(FREERDP_OHOS_SESSION_RESIZE_REQUEST),
            FREERDP_OHOS_SESSION_RESIZE_VERSION,
            width,
            height,
            orientation,
        };
        FREERDP_OHOS_SESSION_RESIZE_RESULT nativeResult {};
        nativeResult.structSize = sizeof(nativeResult);
        if (!activeApi_->ohosSessionResizeEx(activeOhosSession_, &request, &nativeResult,
            detail.data(), detail.size())) {
            result.status = DisplayResizeStatus::Failed;
            result.message = detail[0] != '\0' ? detail.data() :
                "FreeRDP OHOS session resize_ex call failed";
            return result;
        }

        switch (nativeResult.status) {
            case FREERDP_OHOS_SESSION_RESIZE_SENT:
                result.status = DisplayResizeStatus::Sent;
                break;
            case FREERDP_OHOS_SESSION_RESIZE_DEFERRED:
                result.status = DisplayResizeStatus::Deferred;
                break;
            case FREERDP_OHOS_SESSION_RESIZE_UNCHANGED:
                result.status = DisplayResizeStatus::Unchanged;
                break;
            case FREERDP_OHOS_SESSION_RESIZE_UNSUPPORTED:
                result.status = DisplayResizeStatus::Unsupported;
                break;
            case FREERDP_OHOS_SESSION_RESIZE_FAILED:
            default:
                result.status = DisplayResizeStatus::Failed;
                break;
        }
        result.normalizedWidth = nativeResult.normalizedWidth;
        result.normalizedHeight = nativeResult.normalizedHeight;
        result.sentWidth = nativeResult.sentWidth;
        result.sentHeight = nativeResult.sentHeight;
        result.orientation = nativeResult.orientation;
        result.message = detail[0] != '\0' ? detail.data() :
            (std::string("display-control resize_ex ") + DisplayResizeStatusName(result.status));
        return result;
    }

    result.status = DisplayResizeStatus::Unsupported;
    result.message = "FreeRDP OHOS session resize_ex symbol is not loaded";
    return result;
}

void RdpSessionChannels::SetActive(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context,
    freerdpOhosSession* ohosSession)
{
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        activeApi_ = api;
        activeInstance_ = instance;
        activeContext_ = context;
        activeOhosSession_ = ohosSession;
        activeDisp_ = nullptr;
        activeGfx_ = nullptr;
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
    activeOhosSession_ = nullptr;
    activeDisp_ = nullptr;
    activeGfx_ = nullptr;
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

void RdpSessionChannels::AttachDisplayControl(DispClientContext* disp)
{
    if (disp == nullptr) {
        EmitLog("display-control connected without client context");
        return;
    }

    std::array<char, 192> detail {};
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        if (activeApi_ == nullptr || activeApi_->ohosSessionAttachDisplayControl == nullptr ||
            activeOhosSession_ == nullptr) {
            EmitLog("display-control connected but FreeRDP OHOS session attach symbol is not ready");
            return;
        }
        if (!activeApi_->ohosSessionAttachDisplayControl(activeOhosSession_, disp, detail.data(),
            detail.size())) {
            EmitLog(detail[0] != '\0' ? detail.data() : "display-control attach failed");
            return;
        }
        activeDisp_ = disp;
    }
    if (callbacks_.surfaceSnapshot == nullptr) {
        return;
    }
    const SurfaceSnapshot snapshot = callbacks_.surfaceSnapshot();
    if (snapshot.width > 0 && snapshot.height > 0) {
        std::string resizeMessage;
        (void)RequestDynamicDesktopResize(snapshot.width, snapshot.height,
            "display-control connected", resizeMessage);
    }
}

void RdpSessionChannels::DetachDisplayControl(DispClientContext* disp)
{
    std::lock_guard<std::mutex> lock(activeMutex_);
    if (activeDisp_ != nullptr && activeDisp_ == disp) {
        if (activeApi_ != nullptr && activeApi_->ohosSessionDetachDisplayControl != nullptr &&
            activeOhosSession_ != nullptr) {
            activeApi_->ohosSessionDetachDisplayControl(activeOhosSession_, activeDisp_);
        }
        activeDisp_ = nullptr;
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

    if (!attached) {
        EmitLog(message);
    }
    if (attached && callbacks_.requestSurfaceRepaint != nullptr) {
        callbacks_.requestSurfaceRepaint("rdpgfx connected");
    }
}

void RdpSessionChannels::DetachGraphicsPipeline(RdpgfxClientContext* gfx)
{
    {
        std::lock_guard<std::mutex> lock(activeMutex_);
        (void)DetachGraphicsPipelineLocked(gfx);
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

} // namespace rdp_bridge
