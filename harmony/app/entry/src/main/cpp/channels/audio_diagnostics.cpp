#include "channels/audio_diagnostics.h"

#include "freerdp/freerdp_runtime.h"

#include <sstream>

namespace rdp_bridge {

std::string BuildOHAudioStatsLog()
{
#if defined(HARMONY_HAS_FREERDP_HEADERS)
    std::string error;
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (!EnsureFreerdpRuntimeLoaded(api, error)) {
        return "OHAudio stats unavailable: " + error;
    }
    std::string rdpsndClientDiagnostics;
    if (api.rdpsndClientGetDiagnostics != nullptr) {
        const char* diagnostics = api.rdpsndClientGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            rdpsndClientDiagnostics = " | ";
            rdpsndClientDiagnostics += diagnostics;
        }
    }
    std::string audinDiagnostics;
    if (api.audinOhosGetDiagnostics != nullptr) {
        const char* diagnostics = api.audinOhosGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            audinDiagnostics = " | ";
            audinDiagnostics += diagnostics;
        }
    }
    if (api.rdpsndOhosGetDiagnostics != nullptr) {
        const char* diagnostics = api.rdpsndOhosGetDiagnostics();
        if (diagnostics != nullptr && diagnostics[0] != '\0') {
            return std::string(diagnostics) + rdpsndClientDiagnostics + audinDiagnostics;
        }
    }
    if (api.rdpsndOhosGetStats == nullptr) {
        return "OHAudio stats unavailable: backend symbol not exported" + rdpsndClientDiagnostics +
            audinDiagnostics;
    }

    UINT64 registeredCount = 0;
    UINT64 openCount = 0;
    UINT64 closeCount = 0;
    UINT64 playCount = 0;
    UINT64 playBytes = 0;
    UINT64 callbackCount = 0;
    UINT64 renderedBytes = 0;
    UINT64 underrunBytes = 0;
    UINT32 lastRate = 0;
    UINT16 lastChannels = 0;
    UINT16 lastBits = 0;
    UINT32 lastLatencyMs = 0;
    if (!api.rdpsndOhosGetStats(&registeredCount, &openCount, &closeCount, &playCount,
        &playBytes, &callbackCount, &renderedBytes, &underrunBytes, &lastRate, &lastChannels,
        &lastBits, &lastLatencyMs)) {
        return "OHAudio stats unavailable: backend query failed";
    }

    std::ostringstream out;
    out << "OHAudio stats: registered=" << registeredCount
        << " open=" << openCount
        << " close=" << closeCount
        << " playCalls=" << playCount
        << " playBytes=" << playBytes
        << " callbacks=" << callbackCount
        << " renderedBytes=" << renderedBytes
        << " underrunBytes=" << underrunBytes
        << " lastFormat=" << lastRate << "Hz/" << lastChannels << "ch/" << lastBits
        << "bit latency=" << lastLatencyMs << "ms";
    return out.str() + rdpsndClientDiagnostics + audinDiagnostics;
#else
    return "OHAudio stats unavailable: FreeRDP headers not found at build time";
#endif
}

} // namespace rdp_bridge
