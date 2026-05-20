#pragma once

#include <functional>
#include <string>

#include "common/bridge_types.h"
#include "freerdp/freerdp_runtime.h"

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
enum class CertificatePolicy {
    Tofu,
    Strict,
    Ignore,
};

using CertificatePolicyLogFn = std::function<void(const std::string&)>;

const char* CertificatePolicyName(CertificatePolicy policy);
void SetCertificatePolicyLogSink(CertificatePolicyLogFn log);
void ClearCertificatePolicyLogSink();

bool ConfigureFreerdpStoragePaths(FreerdpRuntimeApi& api, rdpSettings* settings,
    const ConnectParams& params, const CertificatePolicyLogFn& log, std::string& error);
CertificatePolicy ParseCertificatePolicy(const std::string& value);
void RegisterCertificatePolicy(freerdp* instance, CertificatePolicy policy);
void UnregisterCertificatePolicy(freerdp* instance);

DWORD HarmonyVerifyCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    DWORD flags);
DWORD HarmonyVerifyChangedCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    const char* oldSubject, const char* oldIssuer, const char* oldFingerprint, DWORD flags);
#endif

} // namespace rdp_bridge
