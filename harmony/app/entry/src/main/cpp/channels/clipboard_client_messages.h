#pragma once

#include "channels/clipboard_pasteboard.h"

#include <functional>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client/cliprdr.h>
#include <freerdp/channels/cliprdr.h>
#include <winpr/clipboard.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
using ClipboardMessageLogFn = std::function<void(const std::string&)>;

UINT SendCliprdrClientCapabilities(CliprdrClientContext* cliprdr);
UINT SendCliprdrFormatListResponse(CliprdrClientContext* cliprdr, bool accepted,
    const ClipboardMessageLogFn& log);
UINT SendCliprdrLocalFormatList(CliprdrClientContext* cliprdr, ClipboardPasteboard& pasteboard,
    const char* reason, const ClipboardMessageLogFn& log);
#endif

} // namespace rdp_bridge
