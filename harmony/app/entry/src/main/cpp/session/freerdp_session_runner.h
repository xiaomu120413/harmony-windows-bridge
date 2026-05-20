#pragma once

#include "common/bridge_types.h"
#include "session/rdp_session_core.h"

#include <atomic>
#include <functional>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include "freerdp/freerdp_runtime.h"

#include <freerdp/freerdp.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
using FreerdpSetActiveFn = std::function<void(FreerdpRuntimeApi*, freerdp*, rdpContext*)>;
using FreerdpClearActiveFn = std::function<void(freerdp*)>;
using FreerdpConnectedFn = std::function<void()>;
using FreerdpInputPumpFn = std::function<void(FreerdpRuntimeApi*, rdpContext*)>;

RdpSessionRunResult RunFreerdpSession(const ConnectParams& params, std::atomic_bool& running,
    const RdpSessionCallbacks& callbacks, const FreerdpSetActiveFn& setActive,
    const FreerdpClearActiveFn& clearActive, const std::function<void(const std::string&)>& log,
    const FreerdpConnectedFn& onConnected, const FreerdpInputPumpFn& pumpInput);
#else
RdpSessionRunResult RunFreerdpSessionUnavailable();
#endif

} // namespace rdp_bridge
