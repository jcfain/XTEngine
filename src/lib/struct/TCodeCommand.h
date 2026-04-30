#ifndef TCODECOMMAND_H
#define TCODECOMMAND_H

#include <QString>
#include <QJsonObject>

struct TCodeCommand
{
    QString name;
    QString command;
    bool hidden;

    friend QDataStream & operator<<(QDataStream &dataStream, const TCodeCommand &object )
    {
        dataStream << object.name;
        dataStream << object.command;
        dataStream << object.hidden;
        return dataStream;
    }
    friend QDataStream & operator>>(QDataStream &dataStream, TCodeCommand &object)
    {
        dataStream >> object.name;
        dataStream >> object.command;
        dataStream >> object.hidden;
        return dataStream;
    }
    friend bool operator==(TCodeCommand value1, TCodeCommand value2)
    {
        return value1.name == value2.name && value1.command == value2.command;
    }
    friend bool operator!=(TCodeCommand value1, TCodeCommand value2)
    {
        return value1.name != value2.name && value1.command != value2.command;
    }


    QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }

    QJsonObject toJson()
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["command"] = command;
        obj["hidden"] = hidden;
        return obj;
    }

    static TCodeCommand fromVariant(QVariant item)
    {
        QJsonObject obj = item.toJsonObject();
        return fromJson(obj);
    }

    static TCodeCommand fromJson(QJsonObject obj)
    {
        TCodeCommand item;
        item.name = obj["name"].toString();
        item.command = obj["command"].toString();
        item.hidden = obj["hidden"].toBool();
        return item;
    }
};

#endif // TCODECOMMAND_H
