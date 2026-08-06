#include "napi/clipboard_permission_bridge.h"

#include "napi/permission_request_bridge.h"

namespace rdp_bridge {
namespace {

PermissionRequestBridge& ClipboardPermissionBridge()
{
    static PermissionRequestBridge bridge("clipboard");
    return bridge;
}

} // namespace

EventSink& ClipboardPermissionRequestSink()
{
    return ClipboardPermissionBridge().RequestSink();
}

bool CompleteClipboardPermissionRequestFromUi(uint32_t requestId, bool granted)
{
    return ClipboardPermissionBridge().CompleteFromUi(requestId, granted);
}

BOOL RequestClipboardPermissionForPasteboard(void* userData, UINT32 timeoutMs)
{
    return ClipboardPermissionBridge().Request(userData, timeoutMs);
}

} // namespace rdp_bridge
