#include "xrdp/xrdp_server_internal.h"

#include <cerrno>
#include <ctime>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#if defined(HARMONY_HAS_OPENSSL_CRYPTO)
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#endif

namespace rdp_bridge {
namespace xrdp_bridge_internal {
namespace {

std::string ParentDir(const std::string& path)
{
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos) {
        return "";
    }
    if (slash == 0) {
        return "/";
    }
    return path.substr(0, slash);
}

bool IsRegularFile(const std::string& path)
{
    struct stat st {};
    return !path.empty() && stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

bool EnsureDirectoryLocal(const std::string& path, std::vector<std::string>& logs)
{
    if (path.empty() || IsDirectory(path)) {
        return true;
    }

    std::string current;
    size_t index = 0;
    if (path[0] == '/') {
        current = "/";
        index = 1;
    }

    while (index < path.size()) {
        const size_t slash = path.find('/', index);
        const std::string part = path.substr(index, slash == std::string::npos ?
            std::string::npos : slash - index);
        if (!part.empty()) {
            current = current == "/" ? current + part : JoinPath(current, part);
            if (!IsDirectory(current) &&
                mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) {
                logs.push_back("mkdir failed: " + current + " error=" + std::strerror(errno));
                return false;
            }
        }
        if (slash == std::string::npos) {
            break;
        }
        index = slash + 1;
    }
    return true;
}

bool CopyFile(const std::string& source, const std::string& destination,
    mode_t mode, std::vector<std::string>& logs)
{
    if (!IsRegularFile(source)) {
        return false;
    }

    std::ifstream input(source, std::ios::binary);
    if (!input) {
        logs.push_back("open source failed: " + source + " error=" + std::strerror(errno));
        return false;
    }

    const std::string temp = destination + ".tmp";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if (!output) {
        logs.push_back("open destination failed: " + temp + " error=" + std::strerror(errno));
        return false;
    }
    output << input.rdbuf();
    output.close();
    if (!output) {
        logs.push_back("write destination failed: " + temp);
        unlink(temp.c_str());
        return false;
    }
    chmod(temp.c_str(), mode);
    if (rename(temp.c_str(), destination.c_str()) != 0) {
        logs.push_back("rename failed: " + temp + " -> " + destination +
            " error=" + std::strerror(errno));
        unlink(temp.c_str());
        return false;
    }
    return true;
}

void CopyPackagedConfigFiles(const std::string& packagedConfigPath,
    const std::string& runtimeConfigDir, std::vector<std::string>& logs)
{
    const std::string packagedConfigDir = ParentDir(packagedConfigPath);
    if (!IsDirectory(packagedConfigDir)) {
        logs.push_back("packaged xrdp config directory unavailable: " + packagedConfigDir);
        return;
    }

    DIR* dir = opendir(packagedConfigDir.c_str());
    if (dir == nullptr) {
        logs.push_back("opendir failed: " + packagedConfigDir + " error=" + std::strerror(errno));
        return;
    }

    int copied = 0;
    while (dirent* entry = readdir(dir)) {
        const std::string name = entry->d_name;
        if (name == "." || name == ".." || name == "xrdp.ini" ||
            name == "cert.pem" || name == "key.pem") {
            continue;
        }
        const std::string source = JoinPath(packagedConfigDir, name);
        if (CopyFile(source, JoinPath(runtimeConfigDir, name), 0644, logs)) {
            copied++;
        }
    }
    closedir(dir);
    logs.push_back("copied packaged xrdp config files=" + std::to_string(copied));
}

bool WriteTextFile(const std::string& path, const std::string& text,
    mode_t mode, std::vector<std::string>& logs)
{
    const std::string temp = path + ".tmp";
    std::ofstream output(temp, std::ios::binary | std::ios::trunc);
    if (!output) {
        logs.push_back("open text file failed: " + temp + " error=" + std::strerror(errno));
        return false;
    }
    output << text;
    output.close();
    if (!output) {
        logs.push_back("write text file failed: " + temp);
        unlink(temp.c_str());
        return false;
    }
    chmod(temp.c_str(), mode);
    if (rename(temp.c_str(), path.c_str()) != 0) {
        logs.push_back("rename failed: " + temp + " -> " + path +
            " error=" + std::strerror(errno));
        unlink(temp.c_str());
        return false;
    }
    return true;
}

std::string BuildSecureXrdpIni(const XrdpResolvedPaths& paths, uint32_t port)
{
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
        << "name=HarmonyOS dummy\n"
        << "lib=" << kBackendLibraryName << "\n"
        << "username=na\n"
        << "password=na\n"
        << "port=0\n"
        << "code=0\n";
    return ini.str();
}

#if defined(HARMONY_HAS_OPENSSL_CRYPTO)
std::string OpenSslErrors()
{
    std::string errors;
    unsigned long code = 0;
    while ((code = ERR_get_error()) != 0) {
        char buffer[256] {};
        ERR_error_string_n(code, buffer, sizeof(buffer));
        if (!errors.empty()) {
            errors += "; ";
        }
        errors += buffer;
    }
    return errors.empty() ? "unknown OpenSSL error" : errors;
}

bool FormatFingerprint(X509* cert, std::string& fingerprint)
{
    unsigned char digest[EVP_MAX_MD_SIZE] {};
    unsigned int digestLength = 0;
    if (X509_digest(cert, EVP_sha256(), digest, &digestLength) != 1) {
        return false;
    }

    std::ostringstream stream;
    stream << std::uppercase << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digestLength; i++) {
        if (i != 0) {
            stream << ':';
        }
        stream << std::setw(2) << static_cast<unsigned int>(digest[i]);
    }
    fingerprint = stream.str();
    return true;
}

bool AddNameEntry(X509_NAME* name, const char* field, const char* value)
{
    return X509_NAME_add_entry_by_txt(name, field, MBSTRING_ASC,
        reinterpret_cast<const unsigned char*>(value), -1, -1, 0) == 1;
}

bool AddExtension(X509* cert, int nid, const char* value)
{
    X509V3_CTX context;
    X509V3_set_ctx_nodb(&context);
    X509V3_set_ctx(&context, cert, cert, nullptr, nullptr, 0);
    X509_EXTENSION* extension = X509V3_EXT_conf_nid(nullptr, &context, nid, value);
    if (extension == nullptr) {
        return false;
    }
    const bool ok = X509_add_ext(cert, extension, -1) == 1;
    X509_EXTENSION_free(extension);
    return ok;
}

bool LoadExistingTlsMaterial(const std::string& certPath, const std::string& keyPath,
    std::string& fingerprint, std::vector<std::string>& logs)
{
    if (access(certPath.c_str(), R_OK) != 0 || access(keyPath.c_str(), R_OK) != 0) {
        return false;
    }

    FILE* certFile = fopen(certPath.c_str(), "r");
    if (certFile == nullptr) {
        return false;
    }
    X509* cert = PEM_read_X509(certFile, nullptr, nullptr, nullptr);
    fclose(certFile);
    if (cert == nullptr) {
        logs.push_back("existing xrdp TLS certificate is invalid: " + OpenSslErrors());
        return false;
    }

    FILE* keyFile = fopen(keyPath.c_str(), "r");
    if (keyFile == nullptr) {
        X509_free(cert);
        return false;
    }
    EVP_PKEY* key = PEM_read_PrivateKey(keyFile, nullptr, nullptr, nullptr);
    fclose(keyFile);
    if (key == nullptr) {
        logs.push_back("existing xrdp TLS private key is invalid: " + OpenSslErrors());
        X509_free(cert);
        return false;
    }

    const bool usable = X509_cmp_current_time(X509_get0_notAfter(cert)) > 0 &&
        X509_check_private_key(cert, key) == 1 &&
        FormatFingerprint(cert, fingerprint);
    EVP_PKEY_free(key);
    X509_free(cert);
    if (!usable) {
        logs.push_back("existing xrdp TLS material is expired or mismatched; regenerating");
    }
    return usable;
}

bool WritePrivateKey(EVP_PKEY* key, const std::string& path, std::vector<std::string>& logs)
{
    const std::string temp = path + ".tmp";
    FILE* file = fopen(temp.c_str(), "w");
    if (file == nullptr) {
        logs.push_back("open TLS private key failed: " + temp + " error=" + std::strerror(errno));
        return false;
    }
    const int ok = PEM_write_PrivateKey(file, key, nullptr, nullptr, 0, nullptr, nullptr);
    fclose(file);
    if (ok != 1) {
        logs.push_back("write TLS private key failed: " + OpenSslErrors());
        unlink(temp.c_str());
        return false;
    }
    chmod(temp.c_str(), 0600);
    if (rename(temp.c_str(), path.c_str()) != 0) {
        logs.push_back("rename TLS private key failed: " + std::string(std::strerror(errno)));
        unlink(temp.c_str());
        return false;
    }
    return true;
}

bool WriteCertificate(X509* cert, const std::string& path, std::vector<std::string>& logs)
{
    const std::string temp = path + ".tmp";
    FILE* file = fopen(temp.c_str(), "w");
    if (file == nullptr) {
        logs.push_back("open TLS certificate failed: " + temp + " error=" + std::strerror(errno));
        return false;
    }
    const int ok = PEM_write_X509(file, cert);
    fclose(file);
    if (ok != 1) {
        logs.push_back("write TLS certificate failed: " + OpenSslErrors());
        unlink(temp.c_str());
        return false;
    }
    chmod(temp.c_str(), 0644);
    if (rename(temp.c_str(), path.c_str()) != 0) {
        logs.push_back("rename TLS certificate failed: " + std::string(std::strerror(errno)));
        unlink(temp.c_str());
        return false;
    }
    return true;
}

bool GenerateTlsMaterial(const std::string& certPath, const std::string& keyPath,
    std::string& fingerprint, std::vector<std::string>& logs)
{
    EVP_PKEY_CTX* keyContext = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    EVP_PKEY* key = nullptr;
    X509* cert = nullptr;
    bool ok = false;

    if (keyContext == nullptr ||
        EVP_PKEY_keygen_init(keyContext) != 1 ||
        EVP_PKEY_CTX_set_rsa_keygen_bits(keyContext, 2048) != 1 ||
        EVP_PKEY_keygen(keyContext, &key) != 1) {
        logs.push_back("generate xrdp TLS key failed: " + OpenSslErrors());
        goto cleanup;
    }

    cert = X509_new();
    if (cert == nullptr ||
        X509_set_version(cert, 2) != 1 ||
        ASN1_INTEGER_set(X509_get_serialNumber(cert),
            static_cast<long>(std::time(nullptr) ^ getpid())) != 1 ||
        X509_gmtime_adj(X509_getm_notBefore(cert), 0) == nullptr ||
        X509_gmtime_adj(X509_getm_notAfter(cert), 3650L * 24L * 60L * 60L) == nullptr ||
        X509_set_pubkey(cert, key) != 1) {
        logs.push_back("create xrdp TLS certificate failed: " + OpenSslErrors());
        goto cleanup;
    }

    {
        X509_NAME* name = X509_get_subject_name(cert);
        if (name == nullptr ||
            !AddNameEntry(name, "C", "CN") ||
            !AddNameEntry(name, "O", "HarmonyOS xrdp") ||
            !AddNameEntry(name, "CN", "HarmonyOS xrdp local server") ||
            X509_set_issuer_name(cert, name) != 1) {
            logs.push_back("set xrdp TLS certificate subject failed: " + OpenSslErrors());
            goto cleanup;
        }
    }

    if (!AddExtension(cert, NID_basic_constraints, "critical,CA:FALSE") ||
        !AddExtension(cert, NID_key_usage, "critical,digitalSignature,keyEncipherment") ||
        !AddExtension(cert, NID_ext_key_usage, "serverAuth") ||
        !AddExtension(cert, NID_subject_alt_name, "DNS:localhost,IP:127.0.0.1")) {
        logs.push_back("set xrdp TLS certificate extensions failed: " + OpenSslErrors());
        goto cleanup;
    }

    if (X509_sign(cert, key, EVP_sha256()) <= 0 ||
        !FormatFingerprint(cert, fingerprint)) {
        logs.push_back("sign xrdp TLS certificate failed: " + OpenSslErrors());
        goto cleanup;
    }

    ok = WritePrivateKey(key, keyPath, logs) && WriteCertificate(cert, certPath, logs);

cleanup:
    X509_free(cert);
    EVP_PKEY_free(key);
    EVP_PKEY_CTX_free(keyContext);
    return ok;
}
#endif

bool EnsureTlsMaterial(const std::string& certPath, const std::string& keyPath,
    std::string& fingerprint, std::vector<std::string>& logs)
{
#if defined(HARMONY_HAS_OPENSSL_CRYPTO)
    if (LoadExistingTlsMaterial(certPath, keyPath, fingerprint, logs)) {
        chmod(keyPath.c_str(), 0600);
        logs.push_back("reusing xrdp TLS certificate sha256=" + fingerprint);
        return true;
    }

    if (!GenerateTlsMaterial(certPath, keyPath, fingerprint, logs)) {
        return false;
    }
    logs.push_back("generated xrdp TLS certificate sha256=" + fingerprint);
    return true;
#else
    (void)certPath;
    (void)keyPath;
    (void)fingerprint;
    logs.push_back("OpenSSL crypto headers/library unavailable; xrdp TLS material cannot be generated");
    return false;
#endif
}

} // namespace

bool PrepareSecureRuntimeConfig(const XrdpResolvedPaths& paths, uint32_t port,
    std::vector<std::string>& logs)
{
    const std::string runtimeConfigDir = ParentDir(paths.configPath);
    std::string fingerprint;

    if (!EnsureDirectoryLocal(runtimeConfigDir, logs)) {
        return false;
    }

    CopyPackagedConfigFiles(paths.packagedConfigPath, runtimeConfigDir, logs);

    if (!EnsureTlsMaterial(paths.tlsCertificatePath, paths.tlsKeyPath, fingerprint, logs)) {
        logs.push_back("xrdp TLS material preparation failed; refusing to start without TLS");
        return false;
    }

    if (!WriteTextFile(paths.configPath, BuildSecureXrdpIni(paths, port), 0644, logs)) {
        return false;
    }

    if (access(paths.configPath.c_str(), R_OK) != 0 ||
        access(paths.tlsCertificatePath.c_str(), R_OK) != 0 ||
        access(paths.tlsKeyPath.c_str(), R_OK) != 0) {
        logs.push_back("xrdp TLS config is not readable after generation");
        return false;
    }

    logs.push_back("xrdp TLS config ready: " + paths.configPath);
    logs.push_back("xrdp TLS certificate fingerprint sha256=" + fingerprint);
    return true;
}

} // namespace xrdp_bridge_internal
} // namespace rdp_bridge
