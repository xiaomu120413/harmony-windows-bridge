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

struct DisplayResizeRequest {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t physicalWidth = 0;
    uint32_t physicalHeight = 0;
    uint32_t orientation = 0;
    uint32_t desktopScaleFactor = 100;
    uint32_t deviceScaleFactor = 100;
    std::string reason;
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
