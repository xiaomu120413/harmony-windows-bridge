#pragma once

#include <cstdint>
#include <functional>
#include <string>

struct rdp_context;
typedef struct rdp_context rdpContext;

namespace rdp_bridge {

class FreerdpRuntimeApi;

using RemotePointerImeVisibilityRequest = std::function<bool(bool)>;
using RemotePointerTextLog = std::function<void(const std::string&)>;

void ConfigureRemotePointerTextDetector(RemotePointerImeVisibilityRequest requestImeVisibility,
    RemotePointerTextLog log);
bool RegisterRemotePointerTextDetector(FreerdpRuntimeApi& api, rdpContext* context,
    std::string& message);
void NotifyRemotePointerDirectTouch(uint64_t nowMs);
void ResetRemotePointerTextDetector();

} // namespace rdp_bridge
