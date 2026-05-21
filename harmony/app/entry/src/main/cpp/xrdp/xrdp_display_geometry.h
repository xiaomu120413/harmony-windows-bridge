#pragma once

#include <cstdint>
#include <string>

namespace rdp_bridge {

struct XrdpDisplayGeometry {
    bool valid = false;
    uint64_t displayId = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t originX = 0;
    int32_t originY = 0;
    bool availableValid = false;
    int32_t availableLeft = 0;
    int32_t availableTop = 0;
    uint32_t availableWidth = 0;
    uint32_t availableHeight = 0;
    bool virtualPixelRatioValid = false;
    float virtualPixelRatio = 0.0F;
    bool refreshRateValid = false;
    uint32_t refreshRate = 0;
    bool sourceModeValid = false;
    int32_t sourceMode = 0;
};

XrdpDisplayGeometry QueryXrdpDisplayGeometry();
std::string FormatXrdpDisplayGeometry(const XrdpDisplayGeometry& geometry);

} // namespace rdp_bridge
