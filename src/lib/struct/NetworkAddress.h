#ifndef NETWORKADDRESS_H
#define NETWORKADDRESS_H
#include <QString>
#include <QJsonObject>
#include <QVariant>

struct NetworkAddress
{
    QString address;
    int port;

    static QJsonObject toJson(const NetworkAddress item)
    {
        QJsonObject obj;
        obj["address"] = item.address;
        obj["port"] = item.port;
        return obj;
    }
    static NetworkAddress fromJson(const QJsonObject obj) {
        NetworkAddress newItem;
        newItem.address = obj["address"].toString();
        newItem.port = obj["port"].toInt();
        return newItem;
    }
    static QVariant toVariant(const NetworkAddress item)
    {
        return QVariant::fromValue(toJson(item));
    }

};

#endif // NETWORKADDRESS_H
