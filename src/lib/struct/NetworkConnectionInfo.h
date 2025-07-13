#ifndef NETWORKCONNECTIONINFO_H
#define NETWORKCONNECTIONINFO_H

#include "NetworkAddress.h"
#include "../lookup/enum.h"

struct NetworkConnectionInfo
{
    NetworkAddress address;
    NetworkProtocol protocol;
};
#endif // NETWORKCONNECTIONINFO_H
