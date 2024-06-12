#ifndef OUTPUTDEVICEPACKET_H
#define OUTPUTDEVICEPACKET_H
#include <QString>
#include <QMetaType>

#include "XTEngine_global.h"

enum class XTENGINE_EXPORT OutputDeviceCommandType {
    BUTTON
};

struct XTENGINE_EXPORT OutputDevicePacket
{
    OutputDeviceCommandType type;
    QString command;
    double value;
    QString original;
};

Q_DECLARE_METATYPE(OutputDevicePacket);
#endif // OUTPUTDEVICEPACKET_H
