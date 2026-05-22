#include "xrdp/xrdp_server_internal.h"

#include <cctype>
#include <sstream>

namespace rdp_bridge {
namespace xrdp_bridge_internal {
namespace {

std::string TrimAscii(const std::string& value)
{
    size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0) {
        begin++;
    }
    size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        end--;
    }
    return value.substr(begin, end - begin);
}

bool IsAccessCodeValid(const std::string& code)
{
    if (code.size() < 6 || code.size() > 12) {
        return false;
    }
    for (char c : code) {
        if (std::isdigit(static_cast<unsigned char>(c)) == 0) {
            return false;
        }
    }
    return true;
}

} // namespace

bool BuildSecureXrdpIni(const XrdpResolvedPaths& paths, const XrdpServerParams& params,
    uint32_t port, std::string& iniText, std::vector<std::string>& logs)
{
    const std::string accessCode = TrimAscii(params.accessCode);
    if (!IsAccessCodeValid(accessCode)) {
        logs.push_back("xrdp access code missing or invalid; refusing to start");
        return false;
    }

    std::ostringstream ini;
    ini << "[Globals]\n"
        << "ini_version=1\n"
        << "fork=false\n"
        << "port=" << port << "\n"
        << "tcp_nodelay=true\n"
        << "tcp_keepalive=true\n"
        << "security_layer=tls\n"
        << "crypt_level=high\n"
        << "certificate=" << paths.tlsCertificatePath << "\n"
        << "key_file=" << paths.tlsKeyPath << "\n"
        << "ssl_protocols=TLSv1.2, TLSv1.3\n"
        << "tls_ciphers=HIGH:!aNULL:!eNULL:!EXPORT:!RC4:!DES:!3DES:!MD5:!PSK:!SRP:!DSS\n"
        << "require_credentials=true\n"
        << "autorun=OHOS\n"
        << "allow_channels=true\n"
        << "allow_multimon=true\n"
        << "bitmap_cache=true\n"
        << "bitmap_compression=true\n"
        << "bulk_compression=true\n"
        << "max_bpp=32\n"
        << "new_cursors=true\n"
        << "use_fastpath=both\n\n"
        << "[Logging]\n"
        << "LogFile=xrdp.log\n"
        << "LogLevel=INFO\n"
        << "EnableSyslog=false\n"
        << "EnableConsole=true\n"
        << "ConsoleLevel=INFO\n"
        << "EnableProcessId=true\n\n"
        << "[LoggingPerLogger]\n\n"
        << "[Channels]\n"
        << "rdpdr=false\n"
        << "rdpsnd=true\n"
        << "drdynvc=true\n"
        << "cliprdr=true\n"
        << "rail=false\n"
        << "xrdpvr=false\n\n"
        << "[OHOS]\n"
        << "name=HarmonyOS remote assistance\n"
        << "lib=" << kBackendLibraryName << "\n"
        << "username=ohos\n"
        << "password=ask\n"
        << "access_code=" << accessCode << "\n"
        << "port=0\n"
        << "code=0\n";

    iniText = ini.str();
    logs.push_back("xrdp access code enabled code_length=" + std::to_string(accessCode.size()));
    return true;
}

} // namespace xrdp_bridge_internal
} // namespace rdp_bridge
