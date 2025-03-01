#ifndef AXISDIMENSION_H
#define AXISDIMENSION_H
#include <QString>
#include <QMap>
#include <QVariant>

enum class ChannelDimension
{
    None, // 0
    Heave, // 1
    Surge, // 2
    Sway, // 3
    Pitch, // 4
    Roll, // 5
    Yaw // 6
};

const QMap<QString, ChannelDimension> ChannelDimensions =
{
    {"None", ChannelDimension::None},
    {"Heave", ChannelDimension::Heave},
    {"Surge", ChannelDimension::Surge},
    {"Sway", ChannelDimension::Sway},
    {"Pitch", ChannelDimension::Pitch},
    {"Roll", ChannelDimension::Roll},
    {"Yaw", ChannelDimension::Yaw}
};

Q_DECLARE_METATYPE(ChannelDimension);
#endif // AXISDIMENSION_H
