#ifndef XMESSAGE_H
#define XMESSAGE_H

#include <QString>
#include "../handler/loghandler.h"

struct XMessage
{
    QString message;
    XLogLevel loglevel;

    QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }

    QJsonObject toJson()
    {
        QJsonObject obj;
        obj["message"] = message;
        obj["loglevel"] = (int)loglevel;
        return obj;
    }

    static XMessage fromVariant(QVariant item)
    {
        return fromJson(item.toJsonObject());
    }

    static XMessage fromJson(QJsonObject obj)
    {
        XMessage newItem;
        newItem.message = obj["message"].toString();
        newItem.loglevel = (XLogLevel)obj["loglevel"].toInt();
        return newItem;
    }
};

#endif // XMESSAGE_H
