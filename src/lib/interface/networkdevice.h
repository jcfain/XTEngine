#ifndef NETWORKDEVICE_H
#define NETWORKDEVICE_H
#include <QObject>
#include "lib/handler/outputdevicehandler.h"
#include "lib/struct/NetworkAddress.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT NetworkDevice : public OutputDeviceHandler
{

public:
    explicit NetworkDevice(QObject *parent = nullptr) : OutputDeviceHandler(parent) {

    };
    ~NetworkDevice() {};
    virtual void init(NetworkAddress _address, int waitTimeout = 5000) = 0;
};

#endif // NETWORKDEVICE_H
