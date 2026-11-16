// Link stubs for symbols referenced by the library sources under test but
// never exercised by the tests. They keep the test app free of the full
// implementation and its dependency tree (e.g. XMediaStateHandler pulls in
// MediaLibraryHandler and the Qt Multimedia module).
#include "lib/handler/xmediastatehandler.h"

qreal XMediaStateHandler::getPlaybackSpeed()
{
    // Normal speed: calculateSpeedModifier() leaves intervals untouched.
    return 1.0;
}