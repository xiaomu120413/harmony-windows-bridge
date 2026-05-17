#include "napi_event_sink.h"

#include "bridge_log.h"

#include <memory>

namespace rdp_bridge {
namespace {

struct CallbackData {
    std::string value;
};

void CallStringCallback(napi_env env, napi_value jsCallback, void*, void* data)
{
    std::unique_ptr<CallbackData> callbackData(static_cast<CallbackData*>(data));
    if (env == nullptr || jsCallback == nullptr || callbackData == nullptr) {
        return;
    }

    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);
    napi_value value = nullptr;
    napi_create_string_utf8(env, callbackData->value.c_str(), callbackData->value.size(), &value);
    napi_value argv[1] = {value};
    napi_call_function(env, undefined, jsCallback, 1, argv, nullptr);
}

} // namespace

EventSink::~EventSink()
{
    Reset();
}

bool EventSink::Set(napi_env env, napi_value callback, const char* name, bool mirrorToHilog)
{
    napi_valuetype type = napi_undefined;
    napi_typeof(env, callback, &type);
    if (type != napi_function) {
        return false;
    }

    napi_value resourceName = nullptr;
    napi_create_string_utf8(env, name, NAPI_AUTO_LENGTH, &resourceName);

    napi_threadsafe_function next = nullptr;
    napi_status status = napi_create_threadsafe_function(
        env,
        callback,
        nullptr,
        resourceName,
        0,
        1,
        nullptr,
        nullptr,
        nullptr,
        CallStringCallback,
        &next);
    if (status != napi_ok || next == nullptr) {
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    if (function_ != nullptr) {
        napi_release_threadsafe_function(function_, napi_tsfn_abort);
    }
    function_ = next;
    mirrorToHilog_.store(mirrorToHilog);
    return true;
}

void EventSink::Emit(const std::string& value)
{
    if (mirrorToHilog_.load()) {
        EmitHilogInfo(value);
    }

    napi_threadsafe_function current = nullptr;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        current = function_;
        if (current == nullptr) {
            return;
        }
        napi_acquire_threadsafe_function(current);
    }

    auto data = new CallbackData{value};
    napi_status status = napi_call_threadsafe_function(current, data, napi_tsfn_nonblocking);
    if (status != napi_ok) {
        delete data;
    }
    napi_release_threadsafe_function(current, napi_tsfn_release);
}

void EventSink::Reset()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (function_ != nullptr) {
        napi_release_threadsafe_function(function_, napi_tsfn_abort);
        function_ = nullptr;
    }
    mirrorToHilog_.store(false);
}

} // namespace rdp_bridge
