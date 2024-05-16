#pragma once
#include "motionChannel.h"
#include <QJsonArray>
#include <QJsonObject>

#define maxMotionProfileCount 5
#define maxMotionProfileNameLength 31
#define motionDefaultProfileName "Profile"

struct MotionProfile {
    MotionProfile() { }
    MotionProfile(int profileNumber) {
        motionProfileName = "Profile "+ QString::number(profileNumber);
    }
    MotionProfile(QString profileName) {
        motionProfileName = profileName;
    }
    QString motionProfileName = motionDefaultProfileName;
    bool edited = false;
    QList<MotionChannel> channels;

    void addDefaultChannel(const QString name) {
        edited = true;
        auto channel = MotionChannel(name);
        channel.edited = true;
        channels.push_back(channel);
    }

    int getMotionChannelIndex(const QString name) {
        for (int i = 0; i < channels.count(); i++)
        {
            if(channels[i].name == name) {
                return i;
            }
        }
        return -1;
    }

    void toJson(QJsonObject &obj) {
        obj["name"] = motionProfileName;
        obj["edited"] = edited;
        auto array = QJsonArray();
        for(int i = 0; i < channels.count(); i++) {
            QJsonObject channelObj;
            channels[i].toJson(channelObj);
            array.append(channelObj);
        }
        obj["channels"] = array;
    }

    void fromJson(const QJsonObject &obj) {
        motionProfileName = obj["name"].toString("Profile");
        edited = obj["edited"].toBool();
        channels.clear();
        QJsonArray array = obj["channels"].toArray();
        for (int i = 0; i < array.count(); i++)
        {
            auto channel = MotionChannel();
            channel.fromJson(array[i].toObject());
            channels.push_back(channel);
        }
    }
};
