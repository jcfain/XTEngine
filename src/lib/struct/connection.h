#ifndef CONNECTION_H
#define CONNECTION_H

#include <QObject>
#include "NetworkAddress.h"
#include "../lookup/enum.h"

class Connection
{
public:
    ConnectionDirection direction;
    ConnectionInterface interface;
    ConnectionStatus status;
    virtual QJsonObject toJson()
    {
        QJsonObject obj;
        obj["direction"] = (int)direction;
        obj["interface"] = (int)interface;
        obj["status"] = (int)status;
        return obj;
    }
    virtual void fromJson(const QJsonObject obj)
    {
        direction = (ConnectionDirection)obj["direction"].toInt();
        interface = (ConnectionInterface)obj["interface"].toInt();
        status = (ConnectionStatus)obj["status"].toInt();
    }
    virtual QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }
};

class NetworkConnection: public Connection
{
    NetworkProtocol protocol;
    NetworkAddress addresss;
    QJsonObject toJson() override
    {
        QJsonObject obj = Connection::toJson();
        obj["protocol"] = (int)protocol;
        obj["addresss"] = NetworkAddress::toJson(addresss);
        return obj;
    }
    void fromJson(const QJsonObject obj) override
    {
        Connection::fromJson(obj);
        protocol = (NetworkProtocol)obj["protocol"].toInt();
        addresss = NetworkAddress::fromJson(obj["addresss"].toObject());
    }
};

class SerialConnection: public Connection
{
    QString port;
    QJsonObject toJson() override
    {
        QJsonObject obj = Connection::toJson();
        obj["port"] = port;
        return obj;
    }
    void fromJson(const QJsonObject obj) override
    {
        Connection::fromJson(obj);
        port = obj["port"].toString();
    }
};

#endif // CONNECTION_H
