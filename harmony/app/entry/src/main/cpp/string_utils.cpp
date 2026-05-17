#include "string_utils.h"

#include "net_utils.h"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <limits>
#include <sstream>
#include <sys/stat.h>

namespace rdp_bridge {

bool ParseUInt32(const std::string& value, uint32_t& result)
{
    if (value.empty()) {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    unsigned long parsed = std::strtoul(value.c_str(), &end, 10);
    if (errno != 0 || end == value.c_str() || *end != '\0' || parsed > UINT32_MAX) {
        return false;
    }

    result = static_cast<uint32_t>(parsed);
    return true;
}

bool ParsePort(const std::string& value, uint32_t& port)
{
    uint32_t parsed = 0;
    if (!ParseUInt32(value, parsed) || parsed == 0 || parsed > 65535) {
        return false;
    }
    port = parsed;
    return true;
}

bool IsAutoResolution(const std::string& value)
{
    const std::string normalized = ToLowerAscii(TrimAscii(value));
    return normalized.empty() || normalized == "auto" || normalized == "window";
}

bool ParseResolution(const std::string& value, uint32_t& width, uint32_t& height)
{
    size_t separator = value.find('x');
    if (separator == std::string::npos) {
        separator = value.find('X');
    }
    if (separator == std::string::npos) {
        return false;
    }

    uint32_t parsedWidth = 0;
    uint32_t parsedHeight = 0;
    if (!ParseUInt32(value.substr(0, separator), parsedWidth) ||
        !ParseUInt32(value.substr(separator + 1), parsedHeight) ||
        parsedWidth < 320 || parsedHeight < 240) {
        return false;
    }

    width = parsedWidth;
    height = parsedHeight;
    return true;
}

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

std::string TrimAscii(const std::string& value)
{
    size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }

    size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

std::string TrimTrailingSlashes(std::string value)
{
    while (value.size() > 1 && value.back() == '/') {
        value.pop_back();
    }
    return value;
}

std::string JoinPath(const std::string& base, const std::string& child)
{
    if (base.empty()) {
        return child;
    }
    if (child.empty()) {
        return base;
    }
    if (base.back() == '/') {
        return base + child;
    }
    return base + "/" + child;
}

bool EnsureDirectory(const std::string& path, std::string& error)
{
    if (path.empty()) {
        error = "empty directory path";
        return false;
    }
    std::string current;
    size_t index = 0;
    if (path[0] == '/') {
        current = "/";
        index = 1;
    }
    while (index <= path.size()) {
        const size_t next = path.find('/', index);
        const std::string part = path.substr(index, next == std::string::npos ? std::string::npos : next - index);
        if (!part.empty()) {
            if (!current.empty() && current.back() != '/') {
                current += "/";
            }
            current += part;
            if (mkdir(current.c_str(), 0700) != 0 && errno != EEXIST) {
                error = "mkdir " + current + " failed: " + SystemErrorMessage(errno);
                return false;
            }
        }
        if (next == std::string::npos) {
            break;
        }
        index = next + 1;
    }
    return true;
}

std::string Hex32(uint32_t value)
{
    std::ostringstream stream;
    stream << "0x" << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << value;
    return stream.str();
}

std::string SafeCString(const char* value)
{
    return value == nullptr ? "" : value;
}

} // namespace rdp_bridge
