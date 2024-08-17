#ifndef NETWORKDEVICEINFO_H
#define NETWORKDEVICEINFO_H

#include "NetworkAddress.h"
#include "../lookup/enum.h"

struct NetworkDeviceInfo
{
    NetworkAddress address;
    NetworkProtocol protocol;
};
#endif // NETWORKDEVICEINFO_H
