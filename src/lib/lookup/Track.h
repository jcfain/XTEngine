#ifndef TRACK_H
#define TRACK_H
#include <QString>
#include <QMap>

enum class Track
{
    None,
    Stroke,
    StrokeUp,
    StrokeDown,
    Sway,
    SwayLeft,
    SwayRight,
    Surge,
    SurgeBack,
    SurgeForward,
    Twist,
    TwistClockwise,
    TwistCounterClockwise,
    Roll,
    RollLeft,
    RollRight,
    Pitch,
    PitchForward,
    PitchBack,
    Vib,
    Lube,
    Suck,
    SuckMore,
    SuckLess,
    SuckPosition,
    SuckMorePosition,
    SuckLessPosition,
    TRACKS_LENGTH
};
#endif // TRACK_H
