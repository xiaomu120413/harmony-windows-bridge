#pragma once

#include "common/bridge_types.h"
#include "session/rdp_display_resize_types.h"
#include "surface/surface_bridge.h"

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#include "freerdp/freerdp_runtime.h"

#include <freerdp/client/channels.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/freerdp.h>

namespace rdp_bridge {

class RdpSessionChannels {
public:
    struct Callbacks {
        std::function<void(const std::string&)> log;
        std::function<SurfaceSnapshot()> surfaceSnapshot;
        std::function<bool(const RgbaFrame&, std::string&, bool)> queueFrame;
        std::function<void(const std::string&)> requestSurfaceRepaint;
    };

    void SetCallbacks(Callbacks callbacks);

    void SetActive(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context,
        freerdpOhosSession* ohosSession);
    void ClearActive(freerdp* instance);
    void RequestDisconnect();
    bool RequestCurrentFrameRender(const std::string& reason, std::string& message);
    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message);
    DisplayResizeResult RequestDynamicDesktopResizeEx(uint32_t width, uint32_t height,
        uint32_t orientation, const std::string& reason);

private:
    void EmitLog(const std::string& line);

    static std::mutex& RegistryMutex();
    static std::unordered_map<rdpContext*, RdpSessionChannels*>& Registry();
    static void RegisterSession(rdpContext* context, RdpSessionChannels* session);
    static void UnregisterSession(rdpContext* context);
    static RdpSessionChannels* FindSession(rdpContext* context);
    static bool IsDisplayControlChannel(const char* name);
    static bool IsGraphicsPipelineChannel(const char* name);
    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event);
    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event);

    void AttachDisplayControl(DispClientContext* disp);
    void DetachDisplayControl(DispClientContext* disp);
    void AttachGraphicsPipeline(RdpgfxClientContext* gfx);
    void DetachGraphicsPipeline(RdpgfxClientContext* gfx);
    bool DetachGraphicsPipelineLocked(RdpgfxClientContext* gfx);

    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
    freerdpOhosSession* activeOhosSession_ = nullptr;
    DispClientContext* activeDisp_ = nullptr;
    RdpgfxClientContext* activeGfx_ = nullptr;

    Callbacks callbacks_;
};

} // namespace rdp_bridge
