#ifndef DEVICE_H
#define DEVICE_H

#include <functional>

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include "connection.h"
#include "../lookup/Track.h"

class Device
{
public:
    QString name;
    Connection connection;
    virtual QJsonObject toJson()
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["connection"] = connection.toJson();
        return obj;
    }
    virtual void fromJson(const QJsonObject obj)
    {
        name = obj["name"].toString();
        connection.fromJson(obj["connection"].toObject());
    }
    virtual QVariant toVariant()
    {
        return QVariant::fromValue(toJson());
    }
};

class OutputDevice: public Device
{
    QList<Track> tracks;
    QJsonObject toJson() override
    {
        QJsonObject obj = Device::toJson();
        QJsonArray tracksArray;
        for(auto track : std::as_const(tracks))
            tracksArray.append((int)track);
        obj["tracks"] = tracksArray;
        return obj;
    }
    void fromJson(const QJsonObject obj) override
    {
        Device::fromJson(obj);
        QJsonArray tracksArray = obj["tracks"].toArray();
        tracks.clear();
        for(auto track : std::as_const(tracksArray))
            tracks.append((Track)track.toInt());
    }
};

class InputDevice: public Device
{
    std::function<bool()> isConnected;
    std::function<int64_t()> getTime;
    QJsonObject toJson() override
    {
        QJsonObject obj = Device::toJson();
        return obj;
    }
    void fromJson(const QJsonObject obj) override
    {
        Device::fromJson(obj);
    }
};

#endif // DEVICE_H
