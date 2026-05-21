#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

enum class XrdpVideoPixelFormat {
    Rgba,
    Bgra,
};

struct XrdpVideoFrame {
    const uint8_t* data = nullptr;
    uint32_t width = 0;
    uint32_t height = 0;
    int32_t strideBytes = 0;
    std::string label;
    uint64_t sourceSequence = 0;
    XrdpVideoPixelFormat pixelFormat = XrdpVideoPixelFormat::Rgba;
};

} // namespace rdp_bridge
