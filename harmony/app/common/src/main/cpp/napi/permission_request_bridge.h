#pragma once

#include "freerdp/freerdp_runtime.h"
#include "napi/napi_event_sink.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>

namespace rdp_bridge {

class PermissionRequestBridge {
public:
    explicit PermissionRequestBridge(std::string label, uint32_t defaultTimeoutMs = 60000);

    EventSink& RequestSink();
    bool CompleteFromUi(uint32_t requestId, bool granted);
    BOOL Request(void* userData, UINT32 timeoutMs);

private:
    uint32_t NextRequestId();
    uint32_t EffectiveTimeoutMs(uint32_t timeoutMs) const;

    std::string label_;
    uint32_t defaultTimeoutMs_;
    EventSink requests_;
    std::mutex mutex_;
    std::condition_variable cv_;
    uint32_t nextRequestId_ = 1;
    uint32_t pendingRequestId_ = 0;
    bool pending_ = false;
    bool completed_ = false;
    bool granted_ = false;
};

} // namespace rdp_bridge
