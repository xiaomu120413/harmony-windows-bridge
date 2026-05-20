#include "freerdp/certificate_policy.h"

#include "common/bridge_log.h"
#include "common/string_utils.h"

#include <array>
#include <cstdlib>
#include <mutex>
#include <unordered_map>
#include <utility>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/settings_keys.h>
#endif

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

bool ConfigureFreerdpStoragePaths(FreerdpRuntimeApi& api, rdpSettings* settings,
    const ConnectParams& params, const CertificatePolicyLogFn& log, std::string& error)
{
    std::string filesDir = TrimTrailingSlashes(TrimAscii(params.appFilesDir));
    if (filesDir.empty()) {
        error = "appFilesDir is required for FreeRDP certificate storage";
        return false;
    }

    const std::string configPath = JoinPath(filesDir, "freerdp");
    if (!EnsureDirectory(configPath, error) ||
        !EnsureDirectory(JoinPath(configPath, "certs"), error) ||
        !EnsureDirectory(JoinPath(configPath, "server"), error)) {
        return false;
    }

    setenv("HOME", filesDir.c_str(), 1);
    setenv("XDG_CONFIG_HOME", filesDir.c_str(), 1);
    if (!SetFreerdpString(api, settings, FreeRDP_HomePath, filesDir, "HomePath", error) ||
        !SetFreerdpString(api, settings, FreeRDP_ConfigPath, configPath, "ConfigPath", error)) {
        return false;
    }
    log("FreeRDP storage path configured: " + configPath);
    return true;
}

CertificatePolicy ParseCertificatePolicy(const std::string& value)
{
    FreerdpRuntimeApi& api = SharedFreerdpRuntimeApi();
    std::string loadError;
    if (EnsureFreerdpRuntimeLoaded(api, loadError) &&
        api.ohosCertificatePolicyFromString != nullptr) {
        return FromOhosCertificatePolicy(api.ohosCertificatePolicyFromString(value.c_str()));
    }

    const std::string normalized = ToLowerAscii(TrimAscii(value));
    if (normalized == "strict" || normalized == "verify" || normalized == "valid-ca") {
        return CertificatePolicy::Strict;
    }
    if (normalized == "ignore" || normalized == "accept" || normalized == "insecure") {
        return CertificatePolicy::Ignore;
    }
    if (normalized == "deny" || normalized == "reject") {
        return CertificatePolicy::Strict;
    }
    return CertificatePolicy::Tofu;
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
            EmitCertificatePolicyLog(message.data());
        }
        return rc;
    }

    const std::string target = SafeCString(host) + ":" + std::to_string(port);
    if (policy == CertificatePolicy::Ignore) {
        EmitCertificatePolicyLog("Certificate accepted for current session by ignore policy: " + target);
        return 2;
    }
    if (policy == CertificatePolicy::Tofu) {
        EmitCertificatePolicyLog("Certificate accepted by TOFU policy and requested for FreeRDP store: " + target +
            " cn=" + SafeCString(commonName));
        return 1;
    }

    EmitCertificatePolicyLog("Certificate rejected by strict policy: " + target +
        " cn=" + SafeCString(commonName) + " issuer=" + SafeCString(issuer));
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
            EmitCertificatePolicyLog(message.data());
        }
        return rc;
    }

    const std::string target = SafeCString(host) + ":" + std::to_string(port);
    if (policy == CertificatePolicy::Ignore) {
        EmitCertificatePolicyLog("Changed certificate accepted for current session by ignore policy: " + target);
        return 2;
    }

    EmitCertificatePolicyLog("Changed certificate rejected by " + std::string(CertificatePolicyName(policy)) +
        " policy: " + target + " cn=" + SafeCString(commonName));
    if ((subject != nullptr && subject[0] != '\0') || (oldSubject != nullptr && oldSubject[0] != '\0')) {
        EmitCertificatePolicyLog("Certificate subject changed from [" + SafeCString(oldSubject) + "] to [" +
            SafeCString(subject) + "]");
    }
    if ((issuer != nullptr && issuer[0] != '\0') || (oldIssuer != nullptr && oldIssuer[0] != '\0')) {
        EmitCertificatePolicyLog("Certificate issuer changed from [" + SafeCString(oldIssuer) + "] to [" +
            SafeCString(issuer) + "]");
    }
    if ((fingerprint != nullptr && fingerprint[0] != '\0') ||
        (oldFingerprint != nullptr && oldFingerprint[0] != '\0')) {
        EmitCertificatePolicyLog("Changed certificate fingerprint/pem is available in native callback");
    }
    return 0;
}
#endif

} // namespace rdp_bridge
