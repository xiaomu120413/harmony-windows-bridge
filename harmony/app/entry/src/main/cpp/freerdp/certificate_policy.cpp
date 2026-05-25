#include "freerdp/certificate_policy.h"

#include "common/bridge_log.h"

#include <array>
#include <mutex>
#include <unordered_map>
#include <utility>

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::mutex g_certificatePolicyMutex;
std::unordered_map<freerdp*, CertificatePolicy> g_certificatePolicies;

std::mutex g_certificateLogMutex;
CertificatePolicyLogFn g_certificateLogSink;

const char* CertificatePolicyName(CertificatePolicy policy)
{
    switch (policy) {
        case CertificatePolicy::Strict:
            return "strict";
        case CertificatePolicy::Ignore:
            return "ignore";
        case CertificatePolicy::Tofu:
        default:
            return "tofu";
    }
}

void SetCertificatePolicyLogSink(CertificatePolicyLogFn log)
{
    std::lock_guard<std::mutex> lock(g_certificateLogMutex);
    g_certificateLogSink = std::move(log);
}

void ClearCertificatePolicyLogSink()
{
    SetCertificatePolicyLogSink({});
}

void EmitCertificatePolicyLog(const std::string& line)
{
    CertificatePolicyLogFn sink;
    {
        std::lock_guard<std::mutex> lock(g_certificateLogMutex);
        sink = g_certificateLogSink;
    }
    if (sink) {
        sink(line);
    } else {
        EmitHilogInfo(line);
    }
}

std::string CertificatePolicyDecisionLog(CertificatePolicy policy, bool changed, DWORD rc, UINT16 port)
{
    const char* action = rc == 0 ? "rejected" : "accepted";
    return std::string(changed ? "Changed certificate " : "Certificate ") + action +
        " by " + CertificatePolicyName(policy) + " policy: target=<redacted>:" +
        std::to_string(port);
}

uint32_t ToOhosCertificatePolicy(CertificatePolicy policy)
{
    switch (policy) {
        case CertificatePolicy::Strict:
            return FREERDP_OHOS_CERTIFICATE_POLICY_STRICT;
        case CertificatePolicy::Ignore:
            return FREERDP_OHOS_CERTIFICATE_POLICY_IGNORE;
        case CertificatePolicy::Tofu:
        default:
            return FREERDP_OHOS_CERTIFICATE_POLICY_TOFU;
    }
}

CertificatePolicy FromOhosCertificatePolicy(uint32_t policy)
{
    switch (policy) {
        case FREERDP_OHOS_CERTIFICATE_POLICY_STRICT:
            return CertificatePolicy::Strict;
        case FREERDP_OHOS_CERTIFICATE_POLICY_IGNORE:
            return CertificatePolicy::Ignore;
        case FREERDP_OHOS_CERTIFICATE_POLICY_TOFU:
        default:
            return CertificatePolicy::Tofu;
    }
}

void RegisterCertificatePolicy(freerdp* instance, CertificatePolicy policy)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    if (instance == nullptr) {
        return;
    }
    g_certificatePolicies[instance] = policy;
}

void UnregisterCertificatePolicy(freerdp* instance)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    g_certificatePolicies.erase(instance);
}

CertificatePolicy LookupCertificatePolicy(freerdp* instance)
{
    std::lock_guard<std::mutex> lock(g_certificatePolicyMutex);
    auto it = g_certificatePolicies.find(instance);
    if (it == g_certificatePolicies.end()) {
        return CertificatePolicy::Tofu;
    }
    return it->second;
}

DWORD HarmonyVerifyCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    DWORD)
{
    CertificatePolicy policy = LookupCertificatePolicy(instance);
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosCertificateVerify != nullptr) {
        FREERDP_OHOS_CERTIFICATE_VERIFY_INFO info = {};
        info.host = host;
        info.port = port;
        info.commonName = commonName;
        info.subject = subject;
        info.issuer = issuer;
        info.fingerprint = fingerprint;
        std::array<char, 512> message {};
        const DWORD rc = api.ohosCertificateVerify(
            ToOhosCertificatePolicy(policy), &info, message.data(), message.size());
        if (message[0] != '\0') {
            EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, false, rc, port));
        }
        return rc;
    }

    if (policy == CertificatePolicy::Ignore) {
        EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, false, 2, port));
        return 2;
    }
    if (policy == CertificatePolicy::Tofu) {
        EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, false, 1, port));
        return 1;
    }

    EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, false, 0, port));
    if (fingerprint != nullptr && fingerprint[0] != '\0') {
        EmitCertificatePolicyLog("Rejected certificate fingerprint/pem is available in native callback");
    }
    return 0;
}

DWORD HarmonyVerifyChangedCertificateEx(freerdp* instance, const char* host, UINT16 port,
    const char* commonName, const char* subject, const char* issuer, const char* fingerprint,
    const char* oldSubject, const char* oldIssuer, const char* oldFingerprint, DWORD)
{
    CertificatePolicy policy = LookupCertificatePolicy(instance);
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    if (api.ohosCertificateVerify != nullptr) {
        FREERDP_OHOS_CERTIFICATE_VERIFY_INFO info = {};
        info.host = host;
        info.port = port;
        info.commonName = commonName;
        info.subject = subject;
        info.issuer = issuer;
        info.fingerprint = fingerprint;
        info.oldSubject = oldSubject;
        info.oldIssuer = oldIssuer;
        info.oldFingerprint = oldFingerprint;
        info.changed = TRUE;
        std::array<char, 512> message {};
        const DWORD rc = api.ohosCertificateVerify(
            ToOhosCertificatePolicy(policy), &info, message.data(), message.size());
        if (message[0] != '\0') {
            EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, true, rc, port));
        }
        return rc;
    }

    if (policy == CertificatePolicy::Ignore) {
        EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, true, 2, port));
        return 2;
    }

    EmitCertificatePolicyLog(CertificatePolicyDecisionLog(policy, true, 0, port));
    if ((subject != nullptr && subject[0] != '\0') || (oldSubject != nullptr && oldSubject[0] != '\0')) {
        EmitCertificatePolicyLog("Certificate subject changed; values are redacted");
    }
    if ((issuer != nullptr && issuer[0] != '\0') || (oldIssuer != nullptr && oldIssuer[0] != '\0')) {
        EmitCertificatePolicyLog("Certificate issuer changed; values are redacted");
    }
    if ((fingerprint != nullptr && fingerprint[0] != '\0') ||
        (oldFingerprint != nullptr && oldFingerprint[0] != '\0')) {
        EmitCertificatePolicyLog("Changed certificate fingerprint/pem is available in native callback");
    }
    return 0;
}
#endif

} // namespace rdp_bridge
