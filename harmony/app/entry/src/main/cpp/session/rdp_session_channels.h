#pragma once

#include "common/bridge_types.h"
#include "surface/surface_bridge.h"

#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include "freerdp/freerdp_runtime.h"

#include <freerdp/client/channels.h>
#include <freerdp/client/disp.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/freerdp.h>
#endif

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

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    void SetActive(FreerdpRuntimeApi* api, freerdp* instance, rdpContext* context);
    void ClearActive(freerdp* instance);
#endif
    void RequestDisconnect();
    void SetDynamicResizeAlignment(uint32_t alignment);
    bool RequestCurrentFrameRender(const std::string& reason, std::string& message);
    bool RequestDynamicDesktopResize(uint32_t width, uint32_t height, const std::string& reason,
        std::string& message);

private:
    void EmitLog(const std::string& line);

#if defined(HARMONY_HAS_FREERDP_HEADERS)
    static std::mutex& RegistryMutex();
    static std::unordered_map<rdpContext*, RdpSessionChannels*>& Registry();
    static void RegisterSession(rdpContext* context, RdpSessionChannels* session);
    static void UnregisterSession(rdpContext* context);
    static RdpSessionChannels* FindSession(rdpContext* context);
    static bool IsDisplayControlChannel(const char* name);
    static bool IsGraphicsPipelineChannel(const char* name);
    static void OnChannelConnected(void* context, const ChannelConnectedEventArgs* event);
    static void OnChannelDisconnected(void* context, const ChannelDisconnectedEventArgs* event);
    static UINT DisplayControlCaps(DispClientContext* disp, UINT32 maxNumMonitors,
        UINT32 maxMonitorAreaFactorA, UINT32 maxMonitorAreaFactorB);

    void HandleDisplayControlCaps(UINT32 maxNumMonitors, UINT32 maxMonitorAreaFactorA,
        UINT32 maxMonitorAreaFactorB);
    void AttachDisplayControl(DispClientContext* disp);
    void DetachDisplayControl(DispClientContext* disp);
    void AttachGraphicsPipeline(RdpgfxClientContext* gfx);
    void DetachGraphicsPipeline(RdpgfxClientContext* gfx);
    bool DetachGraphicsPipelineLocked(RdpgfxClientContext* gfx);

    std::mutex activeMutex_;
    FreerdpRuntimeApi* activeApi_ = nullptr;
    freerdp* activeInstance_ = nullptr;
    rdpContext* activeContext_ = nullptr;
    DispClientContext* activeDisp_ = nullptr;
    RdpgfxClientContext* activeGfx_ = nullptr;
    bool displayControlCapsReady_ = false;
    uint32_t dynamicResizeAlignment_ = 1;
    uint32_t lastDynamicResizeWidth_ = 0;
    uint32_t lastDynamicResizeHeight_ = 0;
#endif

    Callbacks callbacks_;
};

} // namespace rdp_bridge
