#pragma once

#include <string>
#include <vector>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <winpr/wtypes.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
std::string Utf16LeClipboardToUtf8(const BYTE* data, UINT32 size);
std::vector<BYTE> Utf8ToUtf16LeClipboard(const std::string& text);
#endif

} // namespace rdp_bridge
