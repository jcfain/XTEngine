#ifndef NETWORKADDRESS_H
#define NETWORKADDRESS_H
#include <QString>
#include <QJsonObject>
#include <QVariant>

struct NetworkAddress
{
    QString address;
    int port;

    QJsonObject toJson()
    {
        QJsonObject obj;
        obj["address"] = address;
        obj["port"] = port;
        return obj;
    }
    static NetworkAddress fromJson(const QJsonObject obj) {
        NetworkAddress newItem;
        newItem.address = obj["address"].toString();
        newItem.port = obj["port"].toInt();
        return newItem;
    }
    QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }

};

#endif // NETWORKADDRESS_H
