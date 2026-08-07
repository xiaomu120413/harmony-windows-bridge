#include "surface/remote_content_geometry.h"

#include <cassert>

using namespace rdp_bridge;

namespace {

void TestLandscapeIntoFourByThree()
{
    const RemoteContentGeometry geometry = FitRemoteContentIntoTarget(1200, 900, 1920, 1080);
    assert(geometry.valid);
    assert(geometry.contentX == 0);
    assert(geometry.contentY == 112);
    assert(geometry.contentWidth == 1200);
    assert(geometry.contentHeight == 675);
    assert(!IsPointInsideRemoteContent(geometry, 600, 111));
    assert(IsPointInsideRemoteContent(geometry, 0, 112));
    assert(IsPointInsideRemoteContent(geometry, 1199, 786));
    assert(!IsPointInsideRemoteContent(geometry, 1199, 787));
}

void TestFourByThreeIntoLandscape()
{
    const RemoteContentGeometry geometry = FitRemoteContentIntoTarget(1600, 900, 1024, 768);
    assert(geometry.valid);
    assert(geometry.contentX == 200);
    assert(geometry.contentY == 0);
    assert(geometry.contentWidth == 1200);
    assert(geometry.contentHeight == 900);
}

void TestOneToOneToleranceAndOddSizes()
{
    RemoteContentGeometry geometry = FitRemoteContentIntoTarget(1928, 1088, 1920, 1080);
    assert(geometry.contentX == 4 && geometry.contentY == 4);
    assert(geometry.contentWidth == 1920 && geometry.contentHeight == 1080);

    geometry = FitRemoteContentIntoTarget(1001, 701, 1920, 1080);
    assert(geometry.contentX == 0);
    assert(geometry.contentWidth == 1001);
    assert(geometry.contentHeight == 563);
    assert(geometry.contentY == 69);
}

void TestPublishedGeometryMustMatchCurrentRemote()
{
    RemoteContentGeometry geometry = ResolveRemoteContentGeometry(
        1200, 900, 1920, 1080, 10, 20, 1000, 700, 1920, 1080);
    assert(geometry.published);
    assert(geometry.contentX == 10 && geometry.contentY == 20);
    assert(geometry.contentWidth == 1000 && geometry.contentHeight == 700);

    geometry = ResolveRemoteContentGeometry(
        1200, 900, 1600, 900, 10, 20, 1000, 700, 1920, 1080);
    assert(!geometry.published);
    assert(geometry.contentX == 0 && geometry.contentY == 112);
    assert(geometry.contentWidth == 1200 && geometry.contentHeight == 675);

    geometry = ResolveRemoteContentGeometry(
        1200, 900, 1920, 1080, 1100, 0, 200, 900, 1920, 1080);
    assert(!geometry.published);
}

void TestInvalidGeometry()
{
    assert(!FitRemoteContentIntoTarget(0, 900, 1920, 1080).valid);
    assert(!FitRemoteContentIntoTarget(1200, 0, 1920, 1080).valid);
    assert(!FitRemoteContentIntoTarget(1200, 900, 0, 1080).valid);
    assert(!FitRemoteContentIntoTarget(1200, 900, 1920, 0).valid);
}

} // namespace

int main()
{
    TestLandscapeIntoFourByThree();
    TestFourByThreeIntoLandscape();
    TestOneToOneToleranceAndOddSizes();
    TestPublishedGeometryMustMatchCurrentRemote();
    TestInvalidGeometry();
    return 0;
}
