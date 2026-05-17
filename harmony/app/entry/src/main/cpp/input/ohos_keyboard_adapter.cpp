#include "input/ohos_keyboard_adapter.h"

#include <sstream>

namespace rdp_bridge {
namespace {

const char* BoolText(bool value)
{
    return value ? "true" : "false";
}

} // namespace

std::string FormatOhosKeyEvent(const OhosKeyEvent& event)
{
    std::ostringstream line;
    line << "ohos.key keyCode=" << event.keyCode
         << " down=" << BoolText(event.down)
         << " repeat=" << BoolText(event.repeat)
         << " ctrl=" << BoolText(event.ctrl)
         << " shift=" << BoolText(event.shift)
         << " alt=" << BoolText(event.alt)
         << " meta=" << BoolText(event.meta);
    return line.str();
}

bool OhosKeyboardAdapter::SendPlatformKey(const OhosKeyEvent& event, std::string& message) const
{
    message = FormatOhosKeyEvent(event) + " adapter=log-only";
    return true;
}

} // namespace rdp_bridge
