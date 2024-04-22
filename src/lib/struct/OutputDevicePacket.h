#ifndef OUTPUTDEVICEPACKET_H
#define OUTPUTDEVICEPACKET_H
#include <QString>
#include <QMetaType>

enum class OutputDeviceCommandType {
    BUTTON
};

struct OutputDevicePacket
{
    OutputDeviceCommandType type;
    QString command;
    double value;
    QString original;
};

Q_DECLARE_METATYPE(OutputDevicePacket);
#endif // OUTPUTDEVICEPACKET_H
