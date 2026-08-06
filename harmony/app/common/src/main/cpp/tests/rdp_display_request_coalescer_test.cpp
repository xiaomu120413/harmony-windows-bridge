#include "session/rdp_display_request_coalescer.h"

#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

int main()
{
    using namespace rdp_bridge;
    using namespace std::chrono_literals;

    RdpDisplayRequestCoalescer coalescer(30ms);
    std::mutex mutex;
    std::condition_variable condition;
    std::vector<DisplayResizeRequest> delivered;
    coalescer.SetCallback([&](const DisplayResizeRequest& request) {
        {
            std::lock_guard<std::mutex> lock(mutex);
            delivered.push_back(request);
        }
        condition.notify_all();
    });

    coalescer.Schedule({1280, 720, 0, 0, 0, 100, 100, "drag-1"});
    coalescer.Schedule({1600, 900, 0, 0, 0, 100, 100, "drag-2"});
    coalescer.Schedule({2144, 1296, 344, 207, 90, 200, 100, "drag-final"});
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(condition.wait_for(lock, 500ms, [&]() { return delivered.size() == 1; }));
        assert(delivered[0].width == 2144);
        assert(delivered[0].height == 1296);
        assert(delivered[0].physicalWidth == 344);
        assert(delivered[0].physicalHeight == 207);
        assert(delivered[0].orientation == 90);
        assert(delivered[0].desktopScaleFactor == 200);
        assert(delivered[0].deviceScaleFactor == 100);
        assert(delivered[0].reason == "drag-final");
    }

    coalescer.Schedule({1024, 768, 0, 0, 0, 100, 100, "cancelled"});
    coalescer.Cancel();
    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard<std::mutex> lock(mutex);
        assert(delivered.size() == 1);
    }

    coalescer.Schedule({1366, 768, 0, 0, 0, 100, 100, "superseded"});
    coalescer.Flush({1920, 1080, 0, 0, 90, 100, 100, "multimon-to-single"});
    {
        std::lock_guard<std::mutex> lock(mutex);
        assert(delivered.size() == 2);
        assert(delivered[1].width == 1920);
        assert(delivered[1].height == 1080);
        assert(delivered[1].orientation == 90);
    }
    std::this_thread::sleep_for(50ms);
    {
        std::lock_guard<std::mutex> lock(mutex);
        assert(delivered.size() == 2);
    }
    return 0;
}
