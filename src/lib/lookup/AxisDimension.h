#ifndef AXISDIMENSION_H
#define AXISDIMENSION_H
#include <QString>
#include <QMap>

enum class AxisDimension
{
    None, // 0
    Heave, // 1
    Surge, // 2
    Sway, // 3
    Pitch, // 4
    Roll, // 5
    Yaw // 6
};

const QMap<QString, AxisDimension> AxisDimensions =
{
    {"None", AxisDimension::None},
    {"Heave", AxisDimension::Heave},
    {"Surge", AxisDimension::Surge},
    {"Sway", AxisDimension::Sway},
    {"Pitch", AxisDimension::Pitch},
    {"Roll", AxisDimension::Roll},
    {"Yaw", AxisDimension::Yaw}
};
#endif // AXISDIMENSION_H
