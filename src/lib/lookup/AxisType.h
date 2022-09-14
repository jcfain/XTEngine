#ifndef AXISTYPE_H
#define AXISTYPE_H
#include <QString>
#include <QMap>
#include <QVariant>

enum class AxisType
{
    None, // 0
    Range, // 1
    Switch, // 2
    HalfRange // 3
};

const QMap<QString, AxisType> AxisTypes =
{
    {"None", AxisType::None},
    {"Oscillate", AxisType::Range},
    {"Ramp", AxisType::Switch},
    {"Half oscillate", AxisType::HalfRange}
};

Q_DECLARE_METATYPE(AxisType);
#endif // AXISTYPE_H
