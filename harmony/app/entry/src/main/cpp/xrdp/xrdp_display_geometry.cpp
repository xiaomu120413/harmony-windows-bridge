#include "xrdp/xrdp_display_geometry.h"

#include <iomanip>
#include <sstream>

#include "ohos/xrdp_ohos.h"

namespace rdp_bridge {

XrdpDisplayGeometry QueryXrdpDisplayGeometry()
{
    XrdpDisplayGeometry geometry;
    xrdp_ohos_display_geometry nativeGeometry {};
    nativeGeometry.size = sizeof(nativeGeometry);
    const int rc = xrdp_ohos_query_display_geometry(&nativeGeometry);
    if (rc != XRDP_OHOS_BACKEND_STATUS_OK || nativeGeometry.valid == 0) {
        return geometry;
    }

    geometry.valid = true;
    geometry.displayId = nativeGeometry.display_id;
    geometry.width = nativeGeometry.width;
    geometry.height = nativeGeometry.height;
    geometry.originX = nativeGeometry.origin_x;
    geometry.originY = nativeGeometry.origin_y;
    geometry.virtualPixelRatioValid = nativeGeometry.virtual_pixel_ratio_valid != 0;
    geometry.virtualPixelRatio = nativeGeometry.virtual_pixel_ratio;
    geometry.refreshRateValid = nativeGeometry.refresh_rate_valid != 0;
    geometry.refreshRate = nativeGeometry.refresh_rate;
    geometry.sourceModeValid = nativeGeometry.source_mode_valid != 0;
    geometry.sourceMode = nativeGeometry.source_mode;

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
