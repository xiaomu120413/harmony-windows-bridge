#include "clipboard_format.h"

#include <cstdint>

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::string Utf16LeClipboardToUtf8(const BYTE* data, UINT32 size)
{
    std::string text;
    if (data == nullptr || size < 2) {
        return text;
    }

    auto appendCodePoint = [&text](uint32_t cp) {
        if (cp <= 0x7F) {
            text.push_back(static_cast<char>(cp));
        } else if (cp <= 0x7FF) {
            text.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else if (cp <= 0xFFFF) {
            text.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            text.push_back(static_cast<char>(0xF0 | (cp >> 18)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            text.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    };

    const UINT32 units = size / 2U;
    for (UINT32 index = 0; index < units; ++index) {
        uint16_t unit = static_cast<uint16_t>(data[index * 2U]) |
            (static_cast<uint16_t>(data[index * 2U + 1U]) << 8U);
        if (unit == 0) {
            break;
        }

        if (unit >= 0xD800 && unit <= 0xDBFF && index + 1U < units) {
            uint16_t next = static_cast<uint16_t>(data[(index + 1U) * 2U]) |
                (static_cast<uint16_t>(data[(index + 1U) * 2U + 1U]) << 8U);
            if (next >= 0xDC00 && next <= 0xDFFF) {
                const uint32_t cp = 0x10000U +
                    (((static_cast<uint32_t>(unit) - 0xD800U) << 10U) |
                        (static_cast<uint32_t>(next) - 0xDC00U));
                appendCodePoint(cp);
                ++index;
                continue;
            }
        }

        appendCodePoint(unit);
    }
    return text;
}

bool ReadUtf8CodePoint(const std::string& text, size_t& offset, uint32_t& cp)
{
    if (offset >= text.size()) {
        return false;
    }

    const uint8_t first = static_cast<uint8_t>(text[offset++]);
    if (first < 0x80) {
        cp = first;
        return true;
    }

    uint32_t value = 0;
    size_t trailing = 0;
    if ((first & 0xE0U) == 0xC0U) {
        value = first & 0x1FU;
        trailing = 1;
    } else if ((first & 0xF0U) == 0xE0U) {
        value = first & 0x0FU;
        trailing = 2;
    } else if ((first & 0xF8U) == 0xF0U) {
        value = first & 0x07U;
        trailing = 3;
    } else {
        cp = 0xFFFD;
        return true;
    }

    if (offset + trailing > text.size()) {
        cp = 0xFFFD;
        offset = text.size();
        return true;
    }

    for (size_t index = 0; index < trailing; ++index) {
        const uint8_t next = static_cast<uint8_t>(text[offset++]);
        if ((next & 0xC0U) != 0x80U) {
            cp = 0xFFFD;
            return true;
        }
        value = (value << 6U) | (next & 0x3FU);
    }

    cp = value;
    return true;
}

std::vector<BYTE> Utf8ToUtf16LeClipboard(const std::string& text)
{
    std::vector<BYTE> output;
    output.reserve(text.size() * 2U + 2U);

    auto appendUnit = [&output](uint16_t unit) {
        output.push_back(static_cast<BYTE>(unit & 0xFFU));
        output.push_back(static_cast<BYTE>((unit >> 8U) & 0xFFU));
    };

    size_t offset = 0;
    uint32_t cp = 0;
    while (ReadUtf8CodePoint(text, offset, cp)) {
        if (cp > 0x10FFFFU) {
            cp = 0xFFFD;
        }

        if (cp <= 0xFFFFU) {
            appendUnit(static_cast<uint16_t>(cp));
        } else {
            cp -= 0x10000U;
            appendUnit(static_cast<uint16_t>(0xD800U | (cp >> 10U)));
            appendUnit(static_cast<uint16_t>(0xDC00U | (cp & 0x3FFU)));
        }
    }

    appendUnit(0);
    return output;
}
#endif

} // namespace rdp_bridge
