#ifndef NETWORKCONNECTIONINFO_H
#define NETWORKCONNECTIONINFO_H

#include "NetworkAddress.h"
#include "../lookup/enum.h"

struct NetworkConnectionInfo
{
    NetworkAddress address;
    NetworkProtocol protocol;
    QJsonObject toJson()
    {
        QJsonObject obj;
        obj["address"] = address.toJson();
        obj["protocol"] = static_cast<int>(protocol);
        return obj;
    }
    static NetworkConnectionInfo fromJson(const QJsonObject obj) {
        NetworkConnectionInfo newItem;
        newItem.address = NetworkAddress::fromJson(obj["address"].toObject());
        newItem.protocol = static_cast<NetworkProtocol>(obj["protocol"].toInt());
        return newItem;
    }
    QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }
};
#endif // NETWORKCONNECTIONINFO_H
