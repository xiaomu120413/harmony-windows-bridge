#include "surface/avc444_gpu_compositor_internal_types.h"
#include "surface/avc_gpu_common.h"
#include "common/string_utils.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>

#include <GLES3/gl3.h>

namespace rdp_bridge {

std::string Avc444GpuRenderer::PixelText(const PixelSample& sample)
{
    return std::string(sample.name) + "=" +
        std::to_string(static_cast<uint32_t>(sample.rgba[0])) + "/" +
        std::to_string(static_cast<uint32_t>(sample.rgba[1])) + "/" +
        std::to_string(static_cast<uint32_t>(sample.rgba[2])) + "/" +
        std::to_string(static_cast<uint32_t>(sample.rgba[3])) + "@" +
        std::to_string(sample.x) + "," + std::to_string(sample.y);
}

std::string Avc444GpuRenderer::SampleFramebuffer(const RenderViewport& viewport) const
{
    if (viewport.width == 0 || viewport.height == 0) {
        return "readback=invalid";
    }

    const uint32_t viewportY = targetHeight_ - viewport.y - viewport.height;
    const uint32_t minX = viewport.x;
    const uint32_t maxX = viewport.x + viewport.width - 1U;
    const uint32_t minY = viewportY;
    const uint32_t maxY = viewportY + viewport.height - 1U;
    auto pointX = [&](uint32_t numerator, uint32_t denominator) {
        return std::min(maxX, minX + (viewport.width * numerator) / denominator);
    };
    auto pointY = [&](uint32_t numerator, uint32_t denominator) {
        return std::min(maxY, minY + (viewport.height * numerator) / denominator);
    };

    std::array<PixelSample, 5> samples {{
        {"c", pointX(1, 2), pointY(1, 2), {}},
        {"lt", pointX(1, 4), pointY(3, 4), {}},
        {"rt", pointX(3, 4), pointY(3, 4), {}},
        {"lb", pointX(1, 4), pointY(1, 4), {}},
        {"rb", pointX(3, 4), pointY(1, 4), {}},
    }};

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    for (PixelSample& sample : samples) {
        glReadPixels(static_cast<GLint>(sample.x), static_cast<GLint>(sample.y), 1, 1,
            GL_RGBA, GL_UNSIGNED_BYTE, sample.rgba.data());
    }
    const GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        return "readback=failed:" + Hex32(static_cast<uint32_t>(error));
    }

    uint32_t nonBlack = 0;
    uint32_t maxRgb = 0;
    uint32_t sumR = 0;
    uint32_t sumG = 0;
    uint32_t sumB = 0;
    for (const PixelSample& sample : samples) {
        const uint32_t r = sample.rgba[0];
        const uint32_t g = sample.rgba[1];
        const uint32_t b = sample.rgba[2];
        sumR += r;
        sumG += g;
        sumB += b;
        maxRgb = std::max(maxRgb, std::max(r, std::max(g, b)));
        if (r + g + b > 24U) {
            ++nonBlack;
        }
    }

    std::ostringstream out;
    out << "readback=ok"
        << ",nonBlack:" << nonBlack << "/" << samples.size()
        << ",maxRgb:" << maxRgb
        << ",avg:" << (sumR / samples.size()) << "/"
        << (sumG / samples.size()) << "/"
        << (sumB / samples.size());
    for (const PixelSample& sample : samples) {
        out << "," << PixelText(sample);
    }
    return out.str();
}

} // namespace rdp_bridge
