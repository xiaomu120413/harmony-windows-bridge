#include "xrdp/xrdp_display_geometry.h"

#include <iomanip>
#include <sstream>

#include <window_manager/oh_display_manager.h>

namespace rdp_bridge {

XrdpDisplayGeometry QueryXrdpDisplayGeometry()
{
    XrdpDisplayGeometry geometry;
    uint64_t displayId = 0;
    int32_t width = 0;
    int32_t height = 0;
    const NativeDisplayManager_ErrorCode idRc = OH_NativeDisplayManager_GetDefaultDisplayId(&displayId);
    const NativeDisplayManager_ErrorCode widthRc = OH_NativeDisplayManager_GetDefaultDisplayWidth(&width);
    const NativeDisplayManager_ErrorCode heightRc = OH_NativeDisplayManager_GetDefaultDisplayHeight(&height);
    if (idRc != DISPLAY_MANAGER_OK || widthRc != DISPLAY_MANAGER_OK ||
        heightRc != DISPLAY_MANAGER_OK || width <= 0 || height <= 0) {
        return geometry;
    }

    geometry.valid = true;
    geometry.displayId = displayId;
    geometry.width = width;
    geometry.height = height;

    int32_t originX = 0;
    int32_t originY = 0;
    if (OH_NativeDisplayManager_GetDisplayPosition(displayId, &originX, &originY) == DISPLAY_MANAGER_OK) {
        geometry.originX = originX;
        geometry.originY = originY;
    }

    float virtualPixelRatio = 0.0F;
    if (OH_NativeDisplayManager_GetDefaultDisplayVirtualPixelRatio(&virtualPixelRatio) == DISPLAY_MANAGER_OK) {
        geometry.virtualPixelRatioValid = true;
        geometry.virtualPixelRatio = virtualPixelRatio;
    }

    uint32_t refreshRate = 0;
    if (OH_NativeDisplayManager_GetDefaultDisplayRefreshRate(&refreshRate) == DISPLAY_MANAGER_OK) {
        geometry.refreshRateValid = true;
        geometry.refreshRate = refreshRate;
    }

    NativeDisplayManager_SourceMode sourceMode = DISPLAY_SOURCE_MODE_NONE;
    if (OH_NativeDisplayManager_GetDisplaySourceMode(displayId, &sourceMode) == DISPLAY_MANAGER_OK) {
        geometry.sourceModeValid = true;
        geometry.sourceMode = static_cast<int32_t>(sourceMode);
    }

    return geometry;
}

std::string FormatXrdpDisplayGeometry(const XrdpDisplayGeometry& geometry)
{
    if (!geometry.valid) {
        return "display=invalid";
    }

    std::ostringstream stream;
    stream << "display id=" << geometry.displayId <<
        " size=" << geometry.width << "x" << geometry.height <<
        " origin=(" << geometry.originX << "," << geometry.originY << ")";
    if (geometry.virtualPixelRatioValid) {
        stream << " vpr=" << std::fixed << std::setprecision(3) << geometry.virtualPixelRatio;
    }
    if (geometry.refreshRateValid) {
        stream << " refresh=" << geometry.refreshRate;
    }
    if (geometry.sourceModeValid) {
        stream << " sourceMode=" << geometry.sourceMode;
    }
    return stream.str();
}

} // namespace rdp_bridge
