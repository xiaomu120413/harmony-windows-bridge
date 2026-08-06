#include "input/remote_pointer_text_policy.h"

#include <cassert>
#include <cstdint>
#include <vector>

namespace {

void SetPixel(std::vector<uint8_t>& rgba, uint32_t width, uint32_t x, uint32_t y)
{
    rgba[(static_cast<size_t>(y) * width + x) * 4 + 3] = 255;
}

} // namespace

int main()
{
    constexpr uint32_t width = 15;
    constexpr uint32_t height = 25;
    std::vector<uint8_t> ibeam(static_cast<size_t>(width) * height * 4);
    for (uint32_t y = 2; y <= 22; ++y) {
        SetPixel(ibeam, width, 7, y);
    }
    for (uint32_t x = 3; x <= 11; ++x) {
        SetPixel(ibeam, width, x, 2);
        SetPixel(ibeam, width, x, 3);
        SetPixel(ibeam, width, x, 21);
        SetPixel(ibeam, width, x, 22);
    }
    assert(rdp_bridge::IsRemoteTextPointerCandidate(
        ibeam.data(), width, height, width * 4, 7, 12));

    std::vector<uint8_t> arrow(static_cast<size_t>(width) * height * 4);
    for (uint32_t y = 1; y <= 14; ++y) {
        for (uint32_t x = 1; x <= y / 2 + 1; ++x) {
            SetPixel(arrow, width, x, y);
        }
    }
    assert(!rdp_bridge::IsRemoteTextPointerCandidate(
        arrow.data(), width, height, width * 4, 1, 1));

    std::vector<uint8_t> block(static_cast<size_t>(width) * height * 4, 255);
    assert(!rdp_bridge::IsRemoteTextPointerCandidate(
        block.data(), width, height, width * 4, 7, 12));
    assert(!rdp_bridge::IsRemoteTextPointerCandidate(nullptr, width, height, width * 4, 0, 0));
    return 0;
}
