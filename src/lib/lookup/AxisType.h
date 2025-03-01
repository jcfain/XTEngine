#ifndef AXISTYPE_H
#define AXISTYPE_H
#include <QString>
#include <QMap>
#include <QVariant>

enum class ChannelType
{
    None, // 0
    Oscillate, // 1
    Ramp, // 2
    HalfOscillate // 3
};

const QMap<QString, ChannelType> ChannelTypes =
{
    {"None", ChannelType::None},
    {"Oscillate", ChannelType::Oscillate},
    {"Ramp", ChannelType::Ramp},
    {"Half oscillate", ChannelType::HalfOscillate}
};

enum class ChannelTimeType {
    None,
    Interval,
    Speed
};

Q_DECLARE_METATYPE(ChannelType);
#endif // AXISTYPE_H
