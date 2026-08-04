#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

enum class DisplayResizeStatus {
    Sent,
    Deferred,
    Unchanged,
    Unsupported,
    Failed,
};

struct DisplayResizeResult {
    DisplayResizeStatus status = DisplayResizeStatus::Failed;
    uint32_t normalizedWidth = 0;
    uint32_t normalizedHeight = 0;
    uint32_t sentWidth = 0;
    uint32_t sentHeight = 0;
    uint32_t orientation = 0;
    std::string message;
};

inline const char* DisplayResizeStatusName(DisplayResizeStatus status)
{
    switch (status) {
        case DisplayResizeStatus::Sent:
            return "Sent";
        case DisplayResizeStatus::Deferred:
            return "Deferred";
        case DisplayResizeStatus::Unchanged:
            return "Unchanged";
        case DisplayResizeStatus::Unsupported:
            return "Unsupported";
        case DisplayResizeStatus::Failed:
        default:
            return "Failed";
    }
}

} // namespace rdp_bridge
