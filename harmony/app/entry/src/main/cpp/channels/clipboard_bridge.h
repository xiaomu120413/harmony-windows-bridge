#pragma once

#include "freerdp/freerdp_runtime.h"

#include <functional>
#include <memory>
#include <string>

#include <freerdp/freerdp.h>

namespace rdp_bridge {

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

} // namespace rdp_bridge
