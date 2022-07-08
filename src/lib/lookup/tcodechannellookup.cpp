#include "tcodechannellookup.h"

void TCodeChannelLookup::setSelectedTCodeVersion(TCodeVersion version)
{
    _selectedTCodeVersionMap = TCodeVersionMap.value(version);
}
QMap<AxisName,  QString> TCodeChannelLookup::GetSelectedVersionMap()
{
    return _selectedTCodeVersionMap;
}

QString TCodeChannelLookup::ToString(AxisName axisName)
{
    return _selectedTCodeVersionMap.value(axisName);
}
void TCodeChannelLookup::AddUserAxis(QString channel)
{
    _selectedTCodeVersionMap.insert((AxisName)_channelCount, channel);
    _channelCount++;
}
bool TCodeChannelLookup::ChannelExists(QString channel)
{
    return _selectedTCodeVersionMap.values().contains(channel);
}
QString TCodeChannelLookup::Stroke()
{
    return _selectedTCodeVersionMap.value(AxisName::Stroke);
}
QString TCodeChannelLookup::StrokeUp()
{
    return _selectedTCodeVersionMap.value(AxisName::StrokeUp);
}
QString TCodeChannelLookup::StrokeDown()
{
    return _selectedTCodeVersionMap.value(AxisName::StrokeDown);
}
QString TCodeChannelLookup::Sway()
{
    return _selectedTCodeVersionMap.value(AxisName::Sway);
}
QString TCodeChannelLookup::SwayRight()
{
    return _selectedTCodeVersionMap.value(AxisName::SwayRight);
}
QString TCodeChannelLookup::SwayLeft()
{
    return _selectedTCodeVersionMap.value(AxisName::SwayLeft);
}
QString TCodeChannelLookup::Surge()
{
    return _selectedTCodeVersionMap.value(AxisName::Surge);
}
QString TCodeChannelLookup::SurgeForward()
{
    return _selectedTCodeVersionMap.value(AxisName::SurgeForward);
}
QString TCodeChannelLookup::SurgeBack()
{
    return _selectedTCodeVersionMap.value(AxisName::SurgeBack);
}
QString TCodeChannelLookup::Twist()
{
    return _selectedTCodeVersionMap.value(AxisName::Twist);
}
QString TCodeChannelLookup::TwistCounterClockwise()
{
    return _selectedTCodeVersionMap.value(AxisName::TwistCounterClockwise);
}
QString TCodeChannelLookup::TwistClockwise()
{
    return _selectedTCodeVersionMap.value(AxisName::TwistClockwise);
}
QString TCodeChannelLookup::Roll()
{
    return _selectedTCodeVersionMap.value(AxisName::Roll);
}
QString TCodeChannelLookup::RollRight()
{
    return _selectedTCodeVersionMap.value(AxisName::RollRight);
}
QString TCodeChannelLookup::RollLeft()
{
    return _selectedTCodeVersionMap.value(AxisName::RollLeft);
}
QString TCodeChannelLookup::Pitch()
{
    return _selectedTCodeVersionMap.value(AxisName::Pitch);
}
QString TCodeChannelLookup::PitchForward()
{
    return _selectedTCodeVersionMap.value(AxisName::PitchForward);
}
QString TCodeChannelLookup::PitchBack()
{
    return _selectedTCodeVersionMap.value(AxisName::PitchBack);
}
QString TCodeChannelLookup::Vib()
{
    return _selectedTCodeVersionMap.value(AxisName::Vib);
}
QString TCodeChannelLookup::Lube()
{
    return _selectedTCodeVersionMap.value(AxisName::Lube);
}
QString TCodeChannelLookup::Suck()
{
    return _selectedTCodeVersionMap.value(AxisName::Suck);
}
QString TCodeChannelLookup::SuckMore()
{
    return _selectedTCodeVersionMap.value(AxisName::SuckMore);
}
QString TCodeChannelLookup::SuckLess()
{
    return _selectedTCodeVersionMap.value(AxisName::SuckLess);
}
QString TCodeChannelLookup::SuckPosition()
{
    return _selectedTCodeVersionMap.value(AxisName::SuckPosition);
}
QString TCodeChannelLookup::SuckMorePosition()
{
    return _selectedTCodeVersionMap.value(AxisName::SuckMorePosition);
}
QString TCodeChannelLookup::SuckLessPosition()
{
    return _selectedTCodeVersionMap.value(AxisName::SuckLessPosition);
}

int TCodeChannelLookup::_channelCount = (int)AxisName::AXIS_NAMES_LENGTH;
QString TCodeChannelLookup::PositiveModifier = "+";
QString TCodeChannelLookup::NegativeModifier = "-";
QString TCodeChannelLookup::L0 = "L0";
QString TCodeChannelLookup::L2 = "L2";
QString TCodeChannelLookup::L1 = "L1";
QString TCodeChannelLookup::R0 = "R0";
QString TCodeChannelLookup::R1 = "R1";
QString TCodeChannelLookup::R2 = "R2";
QString TCodeChannelLookup::V0 = "V0";
QString TCodeChannelLookup::V1 = "V1";
QString TCodeChannelLookup::L3 = "L3";
QString TCodeChannelLookup::A0 = "A0";
QString TCodeChannelLookup::A1 = "A1";
QString TCodeChannelLookup::A2 = "A2";
QMap<AxisName, QString> TCodeChannelLookup::_selectedTCodeVersionMap;
QHash<TCodeVersion, QMap<AxisName, QString>> TCodeChannelLookup::TCodeVersionMap =
{
    {
        TCodeVersion::v2,
        {
            {AxisName::Stroke, L0},
            {AxisName::StrokeUp, L0 + PositiveModifier},
            {AxisName::StrokeDown, L0 + NegativeModifier},
            {AxisName::Roll, R1},
            {AxisName::RollRight, R1 + PositiveModifier},
            {AxisName::RollLeft, R1 + NegativeModifier},
            {AxisName::Pitch, R2},
            {AxisName::PitchForward, R2 + PositiveModifier},
            {AxisName::PitchBack, R2 + NegativeModifier},
            {AxisName::Twist, R0},
            {AxisName::TwistClockwise, R0 + PositiveModifier},
            {AxisName::TwistCounterClockwise, R0 + NegativeModifier},
            {AxisName::Surge, L1},
            {AxisName::SurgeForward, L1 + PositiveModifier},
            {AxisName::SurgeBack, L1 + NegativeModifier},
            {AxisName::Sway, L2},
            {AxisName::SwayLeft, L2 + PositiveModifier},
            {AxisName::SwayRight, L2 + NegativeModifier},
            {AxisName::Vib, V0},
            {AxisName::Lube, V1},
            {AxisName::Suck, L3},
            {AxisName::SuckMore, L3 + NegativeModifier},
            {AxisName::SuckLess, L3 + PositiveModifier}
        }
    },
    {
        TCodeVersion::v3,
        {
            {AxisName::Stroke, L0},
            {AxisName::StrokeUp, L0 + PositiveModifier},
            {AxisName::StrokeDown, L0 + NegativeModifier},
            {AxisName::Roll, R1},
            {AxisName::RollRight, R1 + PositiveModifier},
            {AxisName::RollLeft, R1 + NegativeModifier},
            {AxisName::Pitch, R2},
            {AxisName::PitchForward, R2 + PositiveModifier},
            {AxisName::PitchBack, R2 + NegativeModifier},
            {AxisName::Twist, R0},
            {AxisName::TwistClockwise, R0 + PositiveModifier},
            {AxisName::TwistCounterClockwise, R0 + NegativeModifier},
            {AxisName::Surge, L1},
            {AxisName::SurgeForward, L1 + PositiveModifier},
            {AxisName::SurgeBack, L1 + NegativeModifier},
            {AxisName::Sway, L2},
            {AxisName::SwayLeft, L2 + PositiveModifier},
            {AxisName::SwayRight, L2 + NegativeModifier},
            {AxisName::Vib, V0},
            {AxisName::Suck, A1},
            {AxisName::SuckMore, A1 + NegativeModifier},
            {AxisName::SuckLess, A1 + PositiveModifier},
            {AxisName::SuckPosition, A0},
            {AxisName::SuckMorePosition, A0 + NegativeModifier},
            {AxisName::SuckLessPosition, A0 + PositiveModifier},
            {AxisName::Lube, A2}
        }
    }
};
