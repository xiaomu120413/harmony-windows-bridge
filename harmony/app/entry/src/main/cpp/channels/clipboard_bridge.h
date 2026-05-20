#pragma once

#include "freerdp/freerdp_runtime.h"

#include <functional>
#include <memory>
#include <string>

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/freerdp.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
class HarmonyClipboardBridge {
public:
    using LogFn = std::function<void(const std::string&)>;

    HarmonyClipboardBridge();
    ~HarmonyClipboardBridge();

    bool Initialize(rdpContext* context, FreerdpRuntimeApi& api, const LogFn& log, std::string& error);
    void Uninitialize();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
#endif

} // namespace rdp_bridge
