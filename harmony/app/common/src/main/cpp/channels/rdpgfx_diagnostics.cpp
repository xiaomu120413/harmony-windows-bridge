#include "channels/rdpgfx_diagnostics.h"

#include "channels/rdpgfx_pipeline.h"
#include "freerdp/freerdp_runtime.h"
#include "surface/avc444_gpu_compositor.h"

#include <atomic>
#include <cctype>
#include <sstream>

namespace rdp_bridge {

namespace {

std::atomic_bool g_rdpgfxRuntimeRequested{false};
std::atomic_bool g_rdpgfxH264Requested{false};
std::atomic_bool g_rdpgfxBridgeAttached{false};
std::atomic_uint32_t g_rdpgfxConnectedCount{0};
std::atomic_uint32_t g_rdpgfxDisconnectedCount{0};
std::atomic_uint32_t g_rdpgfxInitFailedCount{0};

bool ParseUintAfter(const std::string& text, const std::string& key, uint64_t& value)
{
    const size_t keyOffset = text.find(key);
    if (keyOffset == std::string::npos) {
        return false;
    }

    size_t offset = keyOffset + key.size();
    if (offset >= text.size() || !std::isdigit(static_cast<unsigned char>(text[offset]))) {
        return false;
    }

    uint64_t parsed = 0;
    while (offset < text.size() && std::isdigit(static_cast<unsigned char>(text[offset]))) {
        parsed = (parsed * 10U) + static_cast<uint64_t>(text[offset] - '0');
        ++offset;
    }
    value = parsed;
    return true;
}

bool ParseFrames(const std::string& text, uint64_t& startFrames, uint64_t& endFrames)
{
    const size_t keyOffset = text.find("frames=");
    if (keyOffset == std::string::npos) {
        return false;
    }

    size_t offset = keyOffset + 7U;
    if (offset >= text.size() || !std::isdigit(static_cast<unsigned char>(text[offset]))) {
        return false;
    }

    uint64_t start = 0;
    while (offset < text.size() && std::isdigit(static_cast<unsigned char>(text[offset]))) {
        start = (start * 10U) + static_cast<uint64_t>(text[offset] - '0');
        ++offset;
    }
    if (offset >= text.size() || text[offset] != '/') {
        return false;
    }
    ++offset;
    if (offset >= text.size() || !std::isdigit(static_cast<unsigned char>(text[offset]))) {
        return false;
    }

    uint64_t end = 0;
    while (offset < text.size() && std::isdigit(static_cast<unsigned char>(text[offset]))) {
        end = (end * 10U) + static_cast<uint64_t>(text[offset] - '0');
        ++offset;
    }

    startFrames = start;
    endFrames = end;
    return true;
}

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

RdpgfxFrameProgress SnapshotRdpgfxFrameProgress()
{
    RdpgfxFrameProgress progress;
    progress.bridgeAttached = g_rdpgfxBridgeAttached.load();

    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return progress;
    }

    const std::string bridgeDiagnostics = OhosRdpgfxBridgeDiagnostics(api);
    if (bridgeDiagnostics.empty()) {
        return progress;
    }

    progress.bridgeAttached =
        progress.bridgeAttached || bridgeDiagnostics.find("bridge=attached") != std::string::npos;
    progress.available =
        ParseFrames(bridgeDiagnostics, progress.startFrames, progress.endFrames) &&
        ParseUintAfter(bridgeDiagnostics, "surfaceCommands=", progress.surfaceCommands);
    return progress;
}

std::string BuildGraphicsPipelineStatsLog()
{
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
}

} // namespace rdp_bridge
