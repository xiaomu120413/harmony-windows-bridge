#pragma once

#include <functional>
#include <string>

#include "freerdp/freerdp_runtime.h"
#include "freerdp/graphics_config.h"

#if defined(HARMONY_HAS_FREERDP_HEADERS)
#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#endif

namespace rdp_bridge {

#if defined(HARMONY_HAS_FREERDP_HEADERS)
bool EnableFreerdpClientChannels(FreerdpRuntimeApi& api, freerdp* instance,
    const std::function<void(const std::string&)>& log, std::string& error);
bool ConfigureEnhancedRdpSettings(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig,
    const std::function<void(const std::string&)>& log, std::string& error);
bool ConfigureOhosStandardChannels(FreerdpRuntimeApi& api, rdpSettings* settings,
    const GraphicsPipelineConfig& graphicsConfig,
    const std::function<void(const std::string&)>& log, std::string& error);
#endif

} // namespace rdp_bridge
