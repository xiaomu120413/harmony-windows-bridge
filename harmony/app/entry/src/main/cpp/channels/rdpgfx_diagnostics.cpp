#include "rdpgfx_diagnostics.h"

#include "channels/rdpgfx_pipeline.h"
#include "freerdp_runtime.h"
#include "surface/avc444_gpu_compositor.h"

#include <atomic>
#include <sstream>

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
namespace {

std::atomic_bool g_rdpgfxRuntimeRequested{false};
std::atomic_bool g_rdpgfxH264Requested{false};
std::atomic_bool g_rdpgfxBridgeAttached{false};
std::atomic_uint32_t g_rdpgfxConnectedCount{0};
std::atomic_uint32_t g_rdpgfxDisconnectedCount{0};
std::atomic_uint32_t g_rdpgfxInitFailedCount{0};

} // namespace

void SetRdpgfxRuntimeRequest(bool requested, bool h264Requested)
{
    g_rdpgfxRuntimeRequested.store(requested);
    g_rdpgfxH264Requested.store(h264Requested);
}

void SetRdpgfxBridgeAttached(bool attached)
{
    g_rdpgfxBridgeAttached.store(attached);
}

void IncrementRdpgfxConnected()
{
    g_rdpgfxConnectedCount.fetch_add(1);
}

void IncrementRdpgfxDisconnected()
{
    g_rdpgfxDisconnectedCount.fetch_add(1);
}

void IncrementRdpgfxInitFailed()
{
    g_rdpgfxInitFailedCount.fetch_add(1);
}

void ResetRdpgfxDiagnosticsStats()
{
    g_rdpgfxConnectedCount.store(0);
    g_rdpgfxDisconnectedCount.store(0);
    g_rdpgfxInitFailedCount.store(0);
}
#endif

std::string BuildGraphicsPipelineStatsLog()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return "rdpgfx stats unavailable: " + error;
    }

    std::ostringstream out;
    out << "rdpgfx stats: compiled=yes"
        << " runtime=" << (g_rdpgfxRuntimeRequested.load() ? "requested" : "off")
        << " h264=" << (g_rdpgfxH264Requested.load() ? "requested" : "off")
        << " bridge=" << (g_rdpgfxBridgeAttached.load() ? "attached" : "detached")
        << " connected=" << g_rdpgfxConnectedCount.load()
        << " disconnected=" << g_rdpgfxDisconnectedCount.load()
        << " initFailed=" << g_rdpgfxInitFailedCount.load()
        << " symbols=gdiInit:" << (api.gdiGraphicsPipelineInit != nullptr ? "yes" : "no")
        << ",gdiUninit:" << (api.gdiGraphicsPipelineUninit != nullptr ? "yes" : "no")
        << ",ctxNew:" << (api.rdpgfxClientContextNew != nullptr ? "yes" : "no")
        << ",ctxFree:" << (api.rdpgfxClientContextFree != nullptr ? "yes" : "no")
        << ",ohosBridge:" << (api.ohosRdpgfxBridgeGetDiagnostics != nullptr ? "yes" : "no")
        << ",ohosAvcodec:" << (api.ohosAvcodecGetDiagnostics != nullptr ? "yes" : "no");
    const std::string bridgeDiagnostics = OhosRdpgfxBridgeDiagnostics(api);
    if (!bridgeDiagnostics.empty()) {
        out << " | " << bridgeDiagnostics;
    }
    const std::string avc420RouteDiagnostics = OhosAvc420RouteDiagnostics(api);
    if (!avc420RouteDiagnostics.empty()) {
        out << " | " << avc420RouteDiagnostics;
    }
    out << " | " << SharedAvc444GpuCompositor().Diagnostics();
    if (api.ohosAvcodecGetDiagnostics != nullptr) {
        const char* diagnostics = api.ohosAvcodecGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            out << " | " << diagnostics;
        }
    }
    return out.str();
#else
    return "rdpgfx stats unavailable: FreeRDP headers not found at build time";
#endif
}

} // namespace rdp_bridge
