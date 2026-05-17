#pragma once

#include "napi/native_api.h"

#include <atomic>
#include <mutex>
#include <string>

namespace rdp_bridge {

class EventSink {
public:
    ~EventSink();

    bool Set(napi_env env, napi_value callback, const char* name, bool mirrorToHilog = false);
    void Emit(const std::string& value);
    void Reset();

private:
    std::mutex mutex_;
    napi_threadsafe_function function_ = nullptr;
    std::atomic_bool mirrorToHilog_{false};
};

struct SessionEventHub {
    EventSink state;
    EventSink log;
    EventSink error;
};

} // namespace rdp_bridge
