#ifndef OUTPUTNETWORKCONNECTIONHANDLER_H
#define OUTPUTNETWORKCONNECTIONHANDLER_H
#include <QObject>
#include "lib/handler/outputconnectionhandler.h"
#include "lib/struct/NetworkAddress.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputNetworkConnectionHandler : public OutputConnectionHandler
{

public:
    explicit OutputNetworkConnectionHandler(QObject *parent = nullptr) : OutputConnectionHandler(ConnectionInterface::Network, parent) {};
    ~OutputNetworkConnectionHandler() {};
    virtual void init(NetworkAddress _address, int waitTimeout = 5000) = 0;
};

#endif // OUTPUTNETWORKCONNECTIONHANDLER_H
