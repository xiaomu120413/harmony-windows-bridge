#include "rdpgfx_diagnostics.h"

#include "bridge_log.h"
#include "freerdp_runtime.h"
#include "string_utils.h"

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
std::atomic_uint64_t g_rdpgfxStartFrameCount{0};
std::atomic_uint64_t g_rdpgfxEndFrameCount{0};
std::atomic_uint64_t g_rdpgfxSurfaceCommandCount{0};
std::atomic_uint64_t g_rdpgfxCodecUncompressedCount{0};
std::atomic_uint64_t g_rdpgfxCodecCavideoCount{0};
std::atomic_uint64_t g_rdpgfxCodecClearCodecCount{0};
std::atomic_uint64_t g_rdpgfxCodecPlanarCount{0};
std::atomic_uint64_t g_rdpgfxCodecProgressiveCount{0};
std::atomic_uint64_t g_rdpgfxCodecAvc420Count{0};
std::atomic_uint64_t g_rdpgfxCodecAlphaCount{0};
std::atomic_uint64_t g_rdpgfxCodecAvc444Count{0};
std::atomic_uint64_t g_rdpgfxCodecAvc444v2Count{0};
std::atomic_uint64_t g_rdpgfxCodecUnknownCount{0};
std::atomic_uint32_t g_rdpgfxCapsConfirmCount{0};
std::atomic_uint32_t g_rdpgfxConfirmedCapsMode{0};
std::atomic_uint32_t g_rdpgfxConfirmedCapsVersion{0};
std::atomic_uint32_t g_rdpgfxConfirmedCapsFlags{0};
std::atomic_uint32_t g_rdpgfxLastCodecId{0};
std::atomic_uint32_t g_rdpgfxLastSurfaceId{0};
std::atomic_uint32_t g_rdpgfxLastCommandWidth{0};
std::atomic_uint32_t g_rdpgfxLastCommandHeight{0};

constexpr uint32_t RDPEGFX_CONFIRMED_NONE = 0;
constexpr uint32_t RDPEGFX_CONFIRMED_AVC420 = 1;
constexpr uint32_t RDPEGFX_CONFIRMED_AVC444 = 2;
constexpr uint32_t RDPEGFX_CONFIRMED_NON_AVC = 3;

} // namespace

const char* RdpgfxCodecName(uint32_t codecId)
{
    switch (codecId) {
        case RDPGFX_CODECID_UNCOMPRESSED:
            return "UNCOMPRESSED";
        case RDPGFX_CODECID_CAVIDEO:
            return "CAVIDEO";
        case RDPGFX_CODECID_CLEARCODEC:
            return "CLEARCODEC";
        case RDPGFX_CODECID_PLANAR:
            return "PLANAR";
        case RDPGFX_CODECID_CAPROGRESSIVE:
            return "CAPROGRESSIVE";
        case RDPGFX_CODECID_CAPROGRESSIVE_V2:
            return "CAPROGRESSIVE_V2";
        case RDPGFX_CODECID_AVC420:
            return "AVC420";
        case RDPGFX_CODECID_ALPHA:
            return "ALPHA";
        case RDPGFX_CODECID_AVC444:
            return "AVC444";
        case RDPGFX_CODECID_AVC444v2:
            return "AVC444v2";
        default:
            return "UNKNOWN";
    }
}

const char* RdpgfxConfirmedModeName(uint32_t mode)
{
    switch (mode) {
        case RDPEGFX_CONFIRMED_AVC420:
            return "avc420";
        case RDPEGFX_CONFIRMED_AVC444:
            return "avc444";
        case RDPEGFX_CONFIRMED_NON_AVC:
            return "non-avc";
        case RDPEGFX_CONFIRMED_NONE:
        default:
            return "none";
    }
}

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

void RecordRdpgfxStartFrame()
{
    g_rdpgfxStartFrameCount.fetch_add(1);
}

void RecordRdpgfxEndFrame()
{
    g_rdpgfxEndFrameCount.fetch_add(1);
}

void ResetRdpgfxDiagnosticsStats()
{
    g_rdpgfxConnectedCount.store(0);
    g_rdpgfxDisconnectedCount.store(0);
    g_rdpgfxInitFailedCount.store(0);
    g_rdpgfxStartFrameCount.store(0);
    g_rdpgfxEndFrameCount.store(0);
    g_rdpgfxSurfaceCommandCount.store(0);
    g_rdpgfxCodecUncompressedCount.store(0);
    g_rdpgfxCodecCavideoCount.store(0);
    g_rdpgfxCodecClearCodecCount.store(0);
    g_rdpgfxCodecPlanarCount.store(0);
    g_rdpgfxCodecProgressiveCount.store(0);
    g_rdpgfxCodecAvc420Count.store(0);
    g_rdpgfxCodecAlphaCount.store(0);
    g_rdpgfxCodecAvc444Count.store(0);
    g_rdpgfxCodecAvc444v2Count.store(0);
    g_rdpgfxCodecUnknownCount.store(0);
    g_rdpgfxCapsConfirmCount.store(0);
    g_rdpgfxConfirmedCapsMode.store(RDPEGFX_CONFIRMED_NONE);
    g_rdpgfxConfirmedCapsVersion.store(0);
    g_rdpgfxConfirmedCapsFlags.store(0);
    g_rdpgfxLastCodecId.store(0);
    g_rdpgfxLastSurfaceId.store(0);
    g_rdpgfxLastCommandWidth.store(0);
    g_rdpgfxLastCommandHeight.store(0);
}

void RecordRdpgfxCapsConfirm(const RDPGFX_CAPS_CONFIRM_PDU* capsConfirm)
{
    g_rdpgfxCapsConfirmCount.fetch_add(1);
    if (capsConfirm == nullptr || capsConfirm->capsSet == nullptr) {
        g_rdpgfxConfirmedCapsMode.store(RDPEGFX_CONFIRMED_NONE);
        g_rdpgfxConfirmedCapsVersion.store(0);
        g_rdpgfxConfirmedCapsFlags.store(0);
        EmitHilogInfo("rdpgfx caps confirm: mode=none capsConfirm=null");
        return;
    }

    const RDPGFX_CAPSET* capsSet = capsConfirm->capsSet;
    const uint32_t version = capsSet->version;
    const uint32_t flags = capsSet->flags;
    const bool avc420 = version == RDPGFX_CAPVERSION_81 &&
        (flags & RDPGFX_CAPS_FLAG_AVC420_ENABLED) != 0;
    const bool avc444 = version == RDPGFX_CAPVERSION_101 ||
        (version >= RDPGFX_CAPVERSION_10 && (flags & RDPGFX_CAPS_FLAG_AVC_DISABLED) == 0);
    const uint32_t mode = avc420 ? RDPEGFX_CONFIRMED_AVC420 :
        (avc444 ? RDPEGFX_CONFIRMED_AVC444 : RDPEGFX_CONFIRMED_NON_AVC);

    g_rdpgfxConfirmedCapsMode.store(mode);
    g_rdpgfxConfirmedCapsVersion.store(version);
    g_rdpgfxConfirmedCapsFlags.store(flags);
    EmitHilogInfo("rdpgfx caps confirm: mode=" + std::string(RdpgfxConfirmedModeName(mode)) +
        " version=" + Hex32(version) +
        " flags=" + Hex32(flags) +
        " confirms=" + std::to_string(g_rdpgfxCapsConfirmCount.load()));
}

void RecordRdpgfxSurfaceCommand(const RDPGFX_SURFACE_COMMAND& command)
{
    const uint64_t total = g_rdpgfxSurfaceCommandCount.fetch_add(1) + 1;
    g_rdpgfxLastCodecId.store(command.codecId);
    g_rdpgfxLastSurfaceId.store(command.surfaceId);
    g_rdpgfxLastCommandWidth.store(command.width);
    g_rdpgfxLastCommandHeight.store(command.height);

    switch (command.codecId) {
        case RDPGFX_CODECID_UNCOMPRESSED:
            g_rdpgfxCodecUncompressedCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CAVIDEO:
            g_rdpgfxCodecCavideoCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CLEARCODEC:
            g_rdpgfxCodecClearCodecCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_PLANAR:
            g_rdpgfxCodecPlanarCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_CAPROGRESSIVE:
        case RDPGFX_CODECID_CAPROGRESSIVE_V2:
            g_rdpgfxCodecProgressiveCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC420:
            g_rdpgfxCodecAvc420Count.fetch_add(1);
            break;
        case RDPGFX_CODECID_ALPHA:
            g_rdpgfxCodecAlphaCount.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC444:
            g_rdpgfxCodecAvc444Count.fetch_add(1);
            break;
        case RDPGFX_CODECID_AVC444v2:
            g_rdpgfxCodecAvc444v2Count.fetch_add(1);
            break;
        default:
            g_rdpgfxCodecUnknownCount.fetch_add(1);
            break;
    }

    if (total <= 5 || total % 120 == 0) {
        EmitHilogInfo("rdpgfx surface command: total=" + std::to_string(total) +
            " codec=" + RdpgfxCodecName(command.codecId) +
            "(" + std::to_string(command.codecId) + ")" +
            " surface=" + std::to_string(command.surfaceId) +
            " rect=" + std::to_string(command.left) + "," + std::to_string(command.top) +
            " " + std::to_string(command.width) + "x" + std::to_string(command.height) +
            " counts=clear:" + std::to_string(g_rdpgfxCodecClearCodecCount.load()) +
            ",progressive:" + std::to_string(g_rdpgfxCodecProgressiveCount.load()) +
            ",avc420:" + std::to_string(g_rdpgfxCodecAvc420Count.load()) +
            ",avc444:" + std::to_string(g_rdpgfxCodecAvc444Count.load()) +
            ",raw:" + std::to_string(g_rdpgfxCodecUncompressedCount.load()) +
            ",unknown:" + std::to_string(g_rdpgfxCodecUnknownCount.load()));
    }
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
        << " frames=" << g_rdpgfxStartFrameCount.load() << "/" << g_rdpgfxEndFrameCount.load()
        << " surfaceCommands=" << g_rdpgfxSurfaceCommandCount.load()
        << " confirmed=" << RdpgfxConfirmedModeName(g_rdpgfxConfirmedCapsMode.load())
        << " capsVersion=" << Hex32(g_rdpgfxConfirmedCapsVersion.load())
        << " capsFlags=" << Hex32(g_rdpgfxConfirmedCapsFlags.load())
        << " capsConfirms=" << g_rdpgfxCapsConfirmCount.load()
        << " codecs=raw:" << g_rdpgfxCodecUncompressedCount.load()
        << ",progressive:" << g_rdpgfxCodecProgressiveCount.load()
        << ",cavideo:" << g_rdpgfxCodecCavideoCount.load()
        << ",clear:" << g_rdpgfxCodecClearCodecCount.load()
        << ",planar:" << g_rdpgfxCodecPlanarCount.load()
        << ",avc420:" << g_rdpgfxCodecAvc420Count.load()
        << ",avc444:" << g_rdpgfxCodecAvc444Count.load()
        << ",avc444v2:" << g_rdpgfxCodecAvc444v2Count.load()
        << ",alpha:" << g_rdpgfxCodecAlphaCount.load()
        << ",unknown:" << g_rdpgfxCodecUnknownCount.load()
        << " lastCodec=" << RdpgfxCodecName(g_rdpgfxLastCodecId.load())
        << "(" << g_rdpgfxLastCodecId.load() << ")"
        << " lastSurface=" << g_rdpgfxLastSurfaceId.load()
        << " lastSize=" << g_rdpgfxLastCommandWidth.load() << "x" << g_rdpgfxLastCommandHeight.load()
        << " symbols=gdiInit:" << (api.gdiGraphicsPipelineInit != nullptr ? "yes" : "no")
        << ",gdiUninit:" << (api.gdiGraphicsPipelineUninit != nullptr ? "yes" : "no")
        << ",ctxNew:" << (api.rdpgfxClientContextNew != nullptr ? "yes" : "no")
        << ",ctxFree:" << (api.rdpgfxClientContextFree != nullptr ? "yes" : "no")
        << ",ohosAvcodec:" << (api.ohosAvcodecGetDiagnostics != nullptr ? "yes" : "no");
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
