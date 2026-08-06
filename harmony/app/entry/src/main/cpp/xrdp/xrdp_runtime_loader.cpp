#include "xrdp/xrdp_server_internal.h"

#include <cerrno>
#include <cstring>
#include <sys/stat.h>

namespace rdp_bridge::xrdp_bridge_internal {
namespace {

bool EnsureDirectory(const std::string& path, std::vector<std::string>& logs)
{
    if (path.empty() || IsDirectory(path)) {
        return true;
    }
    std::string current = path.front() == '/' ? "/" : "";
    size_t index = path.front() == '/' ? 1 : 0;
    while (index < path.size()) {
        const size_t slash = path.find('/', index);
        const std::string part = path.substr(index,
            slash == std::string::npos ? std::string::npos : slash - index);
        if (!part.empty()) {
            current = current == "/" ? current + part : JoinPath(current, part);
            if (!IsDirectory(current) && mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) {
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

} // namespace

std::string JoinPath(const std::string& left, const std::string& right)
{
    if (left.empty()) {
        return right;
    }
    if (right.empty() || right.front() == '/') {
        return right.empty() ? left : right;
    }
    return left.back() == '/' ? left + right : left + "/" + right;
}

bool PathExists(const std::string& path)
{
    struct stat value {};
    return !path.empty() && stat(path.c_str(), &value) == 0;
}

bool IsDirectory(const std::string& path)
{
    struct stat value {};
    return !path.empty() && stat(path.c_str(), &value) == 0 && S_ISDIR(value.st_mode);
}

XrdpResolvedPaths ResolvePaths(const XrdpServerParams& params)
{
    XrdpResolvedPaths paths;
    const std::string filesRoot = params.appFilesDir.empty() ?
        "/data/storage/el2/base/files" : params.appFilesDir;
    paths.runtimeRoot = JoinPath(filesRoot, "xrdp");
    paths.executablePath = JoinPath(kDefaultHnpRoot, "bin/xrdp");
    paths.modulePath = JoinPath(kDefaultHnpRoot, "lib");
    paths.packagedConfigPath = JoinPath(kDefaultHnpRoot, "config/xrdp.ini");
    paths.configPath = JoinPath(paths.runtimeRoot, "config/xrdp.ini");
    paths.tlsCertificatePath = JoinPath(paths.runtimeRoot, "config/cert.pem");
    paths.tlsKeyPath = JoinPath(paths.runtimeRoot, "config/key.pem");
    paths.sharePath = JoinPath(kDefaultHnpRoot, "share");
    paths.pidPath = JoinPath(paths.runtimeRoot, "run");
    paths.logPath = JoinPath(paths.runtimeRoot, "log");
    return paths;
}

bool PrepareRuntime(const XrdpResolvedPaths& paths, std::vector<std::string>& logs)
{
    bool ok = EnsureDirectory(paths.runtimeRoot, logs);
    ok = EnsureDirectory(JoinPath(paths.runtimeRoot, "config"), logs) && ok;
    ok = EnsureDirectory(paths.pidPath, logs) && ok;
    ok = EnsureDirectory(paths.logPath, logs) && ok;
    if (!PathExists(paths.executablePath)) {
        logs.push_back("HNP executable missing: " + paths.executablePath);
        ok = false;
    }
    if (!IsDirectory(paths.modulePath) || !IsDirectory(paths.sharePath)) {
        logs.push_back("HNP runtime directories are incomplete");
        ok = false;
    }
    return ok;
}

} // namespace rdp_bridge::xrdp_bridge_internal
