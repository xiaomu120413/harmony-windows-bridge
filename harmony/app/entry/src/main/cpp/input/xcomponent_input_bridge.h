#pragma once

#include "session/rdp_session_core.h"

#include <cstdint>
#include <functional>
#include <string>

#include <ace/xcomponent/native_interface_xcomponent.h>

namespace rdp_bridge {

class RemoteImeClient;

struct XComponentInputRegisterResult {
    int32_t mouseRc = -1;
    int32_t focusRc = -1;
    int32_t blurRc = -1;
    int32_t keyRc = -1;
    int32_t softKeyboardRc = -1;
    int32_t axisRc = -1;
};

void ConfigureXComponentInputBridge(
    RdpSession* session, RemoteImeClient* remoteIme,
    std::function<void(const std::string&)> log);
void ResetXComponentInputBridge();
bool IsXComponentFocused();
void OnXComponentTouchEvent(OH_NativeXComponent* component, void* window);
XComponentInputRegisterResult RegisterXComponentInputCallbacks(OH_NativeXComponent* component);

} // namespace rdp_bridge
