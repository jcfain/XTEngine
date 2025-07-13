#ifndef OUTPUTCONNECTIONPACKET_H
#define OUTPUTCONNECTIONPACKET_H
#include <QString>
#include <QMetaType>

#include "XTEngine_global.h"

enum class OutputDeviceCommandType {
    BUTTON
};

struct XTENGINE_EXPORT OutputConnectionPacket
{
    OutputDeviceCommandType type;
    QString command;
    double value;
    QString original;
};

Q_DECLARE_METATYPE(OutputConnectionPacket);
#endif // OUTPUTCONNECTIONPACKET_H
