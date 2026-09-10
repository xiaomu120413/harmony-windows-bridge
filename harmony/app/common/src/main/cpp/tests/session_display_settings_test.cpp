#include "session/session_display_settings.h"
#include <cassert>
#include <iostream>
using namespace rdp_bridge;

int main()
{
    SessionDisplaySettings settings;
    assert(!settings.Select(true, 1920, 1080));
    settings.Reset(true);
    assert(!settings.Select(true, 0, 1080));
    assert(settings.Select(true, 1920, 1080));
    assert(settings.IsFixed());
    const auto first = settings.Begin(1920, 1080);
    DisplayResizeResult sent;
    sent.status = DisplayResizeStatus::Sent;
    settings.Result(first, sent);
    assert(settings.BlocksPointer());
    settings.Presented(first, 1280, 720);
    assert(settings.Snapshot().pending);
    settings.Presented(first, 1920, 1080);
    assert(settings.Snapshot().status == "Applied");
    assert(!settings.BlocksPointer());

    const auto second = settings.Begin(2560, 1440);
    settings.Result(first, sent);
    settings.Presented(first, 2560, 1440);
    assert(settings.Snapshot().pending);
    sent.status = DisplayResizeStatus::Deferred;
    settings.Result(second, sent);
    assert(settings.Snapshot().status == "Deferred");
    settings.Presented(second, 2560, 1440);
    assert(settings.Snapshot().status == "Applied");

    auto generation = settings.Begin(2560, 1440);
    sent.status = DisplayResizeStatus::Unchanged;
    settings.Result(generation, sent);
    assert(settings.Snapshot().status == "Applied");
    generation = settings.Begin(1920, 1080);
    settings.Result(generation, sent);
    assert(settings.Snapshot().pending);
    for (auto status : {DisplayResizeStatus::Failed, DisplayResizeStatus::Unsupported}) {
        generation = settings.Begin(1920, 1080);
        sent.status = status;
        settings.Result(generation, sent);
        settings.Presented(generation, 1920, 1080);
        assert(!settings.BlocksPointer());
        assert(settings.Snapshot().status == DisplayResizeStatusName(status));
    }
    settings.InvalidatePresentation();
    settings.Presented(generation, 1920, 1080);
    assert(settings.Snapshot().actualWidth == 0);
    assert(settings.IsFixed());
    settings.SetMultimon(true);
    assert(!settings.IsFixed());
    assert(!settings.Select(true, 1920, 1080));
    settings.SetMultimon(false);
    assert(settings.Select(true, 1920, 1080));
    settings.Reset(true);
    assert(!settings.IsFixed());

    SessionDisplaySettings timeout(std::chrono::milliseconds(0));
    timeout.Begin(1920, 1080);
    assert(timeout.Snapshot().status == "Timeout");
    assert(!timeout.BlocksPointer());
    std::cout << "Session display settings tests passed\n";
}
