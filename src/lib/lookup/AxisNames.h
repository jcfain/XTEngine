#ifndef AXISNAMES_H
#define AXISNAMES_H
#include <QString>
#include <QMap>

enum class ChannelName
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
    AXIS_NAMES_LENGTH
};
#endif // AXISNAMES_H
