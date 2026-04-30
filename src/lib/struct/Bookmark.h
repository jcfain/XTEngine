#ifndef BOOKMARK_H
#define BOOKMARK_H

#include <QString>
#include <QJsonObject>

struct Bookmark
{
    QString Name;
    qint64 Time;
    QString ChannelProfile;

    friend QDataStream & operator<<(QDataStream &dataStream, const Bookmark &object )
    {
        dataStream << object.Name;
        dataStream << object.Time;
        dataStream << object.ChannelProfile;
        return dataStream;
    }
    friend QDataStream & operator>>(QDataStream &dataStream, Bookmark &object)
    {
        dataStream >> object.Name;
        dataStream >> object.Time;
        dataStream >> object.ChannelProfile;
        return dataStream;
    }
    QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }

    QJsonObject toJson()
    {
        QJsonObject obj;
        obj["name"] = Name;
        obj["time"] = QString::number(Time);
        obj["channelProfile"] = ChannelProfile;
        return obj;
    }

    static Bookmark fromJson(QJsonObject obj)
    {
        Bookmark item;
        item.Name = obj["name"].toString();
        item.Time = obj["time"].toString().toLongLong();
        item.ChannelProfile = obj["channelProfile"].toString();
    }
};

#endif // BOOKMARK_H
