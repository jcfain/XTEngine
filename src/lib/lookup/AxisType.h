#ifndef AXISTYPE_H
#define AXISTYPE_H
#include <QString>
#include <QMap>
#include <QVariant>

enum class AxisType
{
    None, // 0
    Oscillate, // 1
    Ramp, // 2
    HalfOscillate // 3
};

const QMap<QString, AxisType> AxisTypes =
{
    {"None", AxisType::None},
    {"Oscillate", AxisType::Oscillate},
    {"Ramp", AxisType::Ramp},
    {"Half oscillate", AxisType::HalfOscillate}
};

Q_DECLARE_METATYPE(AxisType);
#endif // AXISTYPE_H
