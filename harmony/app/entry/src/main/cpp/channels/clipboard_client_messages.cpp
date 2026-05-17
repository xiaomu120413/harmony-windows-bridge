#include "channels/clipboard_client_messages.h"

#include "bridge_log.h"
#include "string_utils.h"

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/client/channels.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
namespace {

void LogClipboardMessage(const ClipboardMessageLogFn& log, const std::string& line)
{
    if (log) {
        log(line);
    } else {
        EmitHilogInfo(line);
    }
}

} // namespace

UINT SendCliprdrClientCapabilities(CliprdrClientContext* cliprdr)
{
    if (cliprdr == nullptr || cliprdr->ClientCapabilities == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }

    CLIPRDR_CAPABILITIES capabilities = {};
    CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet = {};
    capabilities.cCapabilitiesSets = 1;
    capabilities.capabilitySets = reinterpret_cast<CLIPRDR_CAPABILITY_SET*>(&generalCapabilitySet);
    generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
    generalCapabilitySet.capabilitySetLength = 12;
    generalCapabilitySet.version = CB_CAPS_VERSION_2;
    generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;
    return cliprdr->ClientCapabilities(cliprdr, &capabilities);
}

UINT SendCliprdrFormatListResponse(CliprdrClientContext* cliprdr, bool accepted,
    const ClipboardMessageLogFn& log)
{
    if (cliprdr == nullptr || cliprdr->ClientFormatListResponse == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }

    CLIPRDR_FORMAT_LIST_RESPONSE response = {};
    response.common.msgType = CB_FORMAT_LIST_RESPONSE;
    response.common.msgFlags = accepted ? CB_RESPONSE_OK : CB_RESPONSE_FAIL;
    response.common.dataLen = 0;
    const UINT rc = cliprdr->ClientFormatListResponse(cliprdr, &response);
    if (rc == CHANNEL_RC_OK) {
        LogClipboardMessage(log, std::string("cliprdr server format list ") +
            (accepted ? "accepted" : "rejected"));
    }
    return rc;
}

UINT SendCliprdrLocalFormatList(CliprdrClientContext* cliprdr, ClipboardPasteboard& pasteboard,
    const char* reason, const ClipboardMessageLogFn& log)
{
    if (cliprdr == nullptr || cliprdr->ClientFormatList == nullptr) {
        return CHANNEL_RC_OK;
    }

    std::string text;
    std::string error;
    const bool hasText = pasteboard.ReadPlainText(text, error);
    if (!hasText && !error.empty()) {
        LogClipboardMessage(log, "HarmonyOS Pasteboard read warning: " + error);
    }

    CLIPRDR_FORMAT format = {};
    format.formatId = CF_UNICODETEXT;
    CLIPRDR_FORMAT_LIST formatList = {};
    formatList.common.msgType = CB_FORMAT_LIST;
    formatList.common.msgFlags = 0;
    formatList.numFormats = hasText ? 1U : 0U;
    formatList.formats = hasText ? &format : nullptr;

    UINT rc = cliprdr->ClientFormatList(cliprdr, &formatList);
    if (rc == CHANNEL_RC_OK) {
        LogClipboardMessage(log, std::string("cliprdr local format list sent: ") +
            (hasText ? "CF_UNICODETEXT" : "empty") + " reason=" + SafeCString(reason));
    }
    return rc;
}
#endif

} // namespace rdp_bridge
