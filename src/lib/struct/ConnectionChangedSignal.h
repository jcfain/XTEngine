#ifndef CONNECTIONCHANGEDSIGNAL_H
#define CONNECTIONCHANGEDSIGNAL_H
#include <QString>
#include <QMetaType>
#include "../lookup/enum.h"


struct ConnectionChangedSignal
{
    DeviceType type;
    DeviceName deviceName;
    ConnectionStatus status;
    QString message;
};

Q_DECLARE_METATYPE(ConnectionChangedSignal);
#endif // CONNECTIONCHANGEDSIGNAL_H
