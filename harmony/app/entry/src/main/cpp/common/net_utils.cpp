#include "common/net_utils.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace rdp_bridge {

std::string SystemErrorMessage(int errorCode)
{
    if (errorCode == 0) {
        return "ok";
    }
    return std::strerror(errorCode);
}

namespace {

void CloseSocket(int fd)
{
    if (fd >= 0) {
        ::close(fd);
    }
}

TcpConnectResult TryConnectAddress(const addrinfo* address, int timeoutMs)
{
    int fd = ::socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (fd < 0) {
        return {false, "socket failed: " + SystemErrorMessage(errno)};
    }

    int flags = ::fcntl(fd, F_GETFL, 0);
    if (flags < 0 || ::fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "fcntl nonblock failed: " + SystemErrorMessage(error)};
    }

    int rc = ::connect(fd, address->ai_addr, address->ai_addrlen);
    if (rc == 0) {
        CloseSocket(fd);
        return {true, "tcp socket connected"};
    }

    if (errno != EINPROGRESS) {
        int error = errno;
        CloseSocket(fd);
        return {false, "connect failed: " + SystemErrorMessage(error)};
    }

    pollfd pollTarget = {};
    pollTarget.fd = fd;
    pollTarget.events = POLLOUT;
    rc = ::poll(&pollTarget, 1, timeoutMs);
    if (rc == 0) {
        CloseSocket(fd);
        return {false, "connect timed out"};
    }
    if (rc < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "poll failed: " + SystemErrorMessage(error)};
    }

    int socketError = 0;
    socklen_t socketErrorLength = sizeof(socketError);
    if (::getsockopt(fd, SOL_SOCKET, SO_ERROR, &socketError, &socketErrorLength) < 0) {
        int error = errno;
        CloseSocket(fd);
        return {false, "getsockopt failed: " + SystemErrorMessage(error)};
    }

    CloseSocket(fd);
    if (socketError == 0) {
        return {true, "tcp socket connected"};
    }
    return {false, "connect failed: " + SystemErrorMessage(socketError)};
}

} // namespace

TcpConnectResult TestTcpConnect(const std::string& host, const std::string& port, int timeoutMs)
{
    addrinfo hints = {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* addresses = nullptr;
    int resolveStatus = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses);
    if (resolveStatus != 0) {
        return {false, "resolve failed: " + std::string(::gai_strerror(resolveStatus))};
    }

    std::string lastMessage = "no address candidates";
    for (const addrinfo* address = addresses; address != nullptr; address = address->ai_next) {
        TcpConnectResult result = TryConnectAddress(address, timeoutMs);
        if (result.ok) {
            ::freeaddrinfo(addresses);
            return result;
        }
        lastMessage = result.message;
    }

    ::freeaddrinfo(addresses);
    return {false, lastMessage};
}

} // namespace rdp_bridge
