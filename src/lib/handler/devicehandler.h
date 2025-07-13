#ifndef DEVICEHANDLER_H
#define DEVICEHANDLER_H

#include <QObject>

#include "../struct/device.h"

class DeviceHandler : public QObject
{
    Q_OBJECT
public:
    explicit DeviceHandler(QObject *parent = nullptr);

signals:

private:
    QList<Device> m_devices;
};

#endif // DEVICEHANDLER_H
