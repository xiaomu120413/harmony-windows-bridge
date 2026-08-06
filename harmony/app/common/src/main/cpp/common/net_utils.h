#pragma once

#include <string>

namespace rdp_bridge {

struct TcpConnectResult {
    bool ok = false;
    std::string message;
};

std::string SystemErrorMessage(int errorCode);
TcpConnectResult TestTcpConnect(const std::string& host, const std::string& port, int timeoutMs);

} // namespace rdp_bridge
