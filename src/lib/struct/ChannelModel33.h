#ifndef CHANNELMODEL33_H
#define CHANNELMODEL33_H

#include <QString>
#include <QMetaType>
#include <QDataStream>
#include <QVariant>
#include <QJsonObject>
#include "../lookup/AxisType.h"
#include "../lookup/AxisDimension.h"


struct ChannelValueModel
{
    int Value;
    QString Channel;
    friend bool operator==(const ChannelValueModel &p1, const ChannelValueModel &p2)
    {
       if(p1.Channel == p2.Channel)
         return true;
       else
         return false;
    }
};

struct ChannelModel33
{
    QString FriendlyName;
    QString AxisName;
    QString Channel;
    int Min;
    int Mid;
    int Max;
    int UserMin;
    int UserMid;
    int UserMax;
    ChannelDimension Dimension;
    ChannelType Type;
    QString TrackName;
    bool MultiplierEnabled;
    bool DamperEnabled;
    bool DamperRandom;
    float DamperValue;
    bool FunscriptInverted;
    bool GamepadInverted;
    bool LinkToRelatedMFS;
    QString RelatedChannel;

//    friend QDataStream & operator<<( QDataStream &dataStream, const ChannelModel33 &object )
//    {
//        dataStream << object.FriendlyName;
//        dataStream << object.AxisName;
//        dataStream << object.Channel;
//        dataStream << object.Min;
//        dataStream << object.Mid;
//        dataStream << object.Max;
//        dataStream << object.UserMin;
//        dataStream << object.UserMid;
//        dataStream << object.UserMax;
//        dataStream << (int)object.Dimension;
//        dataStream << (int)object.Type;
//        dataStream << object.TrackName;
//        dataStream << object.MultiplierEnabled;
//        dataStream << object.MultiplierValue;
//        dataStream << object.DamperEnabled;
//        dataStream << object.DamperValue;
//        dataStream << object.FunscriptInverted;
//        dataStream << object.GamepadInverted;
//        dataStream << object.LinkToRelatedMFS;
//        dataStream << object.RelatedChannel;
//        return dataStream;
//    }

//    friend QDataStream & operator>>(QDataStream &dataStream, ChannelModel33 &object)
//    {
//        dataStream >> object.FriendlyName;
//        dataStream >> object.AxisName;
//        dataStream >> object.Channel;
//        dataStream >> object.Min;
//        dataStream >> object.Mid;
//        dataStream >> object.Max;
//        dataStream >> object.UserMin;
//        dataStream >> object.UserMid;
//        dataStream >> object.UserMax;
//        dataStream >> object.Dimension;
//        dataStream >> object.Type;
//        dataStream >> object.TrackName;
//        dataStream >> object.MultiplierEnabled;
//        dataStream >> object.MultiplierValue;
//        dataStream >> object.DamperEnabled;
//        dataStream >> object.DamperValue;
//        dataStream >> object.FunscriptInverted;
//        dataStream >> object.GamepadInverted;
//        dataStream >> object.LinkToRelatedMFS;
//        dataStream >> object.RelatedChannel;
//        return dataStream;
//    }

    static ChannelModel33 fromVariant(QVariant item)
    {
        return fromJson(item.toJsonObject());
    }

    static ChannelModel33 fromJson(QJsonObject obj)
    {
        ChannelModel33 newItem;
        newItem.FriendlyName = obj["friendlyName"].toString();
        newItem.AxisName = obj["axisName"].toString();
        newItem.Channel = obj["channel"].toString();
        newItem.Min = obj["min"].toInt();
        newItem.Mid = obj["mid"].toInt();
        newItem.Max = obj["max"].toInt();
        newItem.UserMin = obj["userMin"].toInt();
        newItem.UserMid = obj["userMid"].toInt();
        newItem.UserMax = obj["userMax"].toInt();
        newItem.Dimension = (ChannelDimension)obj["dimension"].toInt();
        newItem.Type = (ChannelType)obj["type"].toInt();
        newItem.TrackName = obj["trackName"].toString();
        newItem.MultiplierEnabled = obj["multiplierEnabled"].toBool();
        newItem.DamperEnabled = obj["damperEnabled"].toBool();
        newItem.DamperRandom = obj["damperRandom"].toBool();
        newItem.DamperValue = obj["damperValue"].toDouble();
        newItem.FunscriptInverted = obj["funscriptInverted"].toBool();
        newItem.GamepadInverted = obj["gamepadInverted"].toBool();
        newItem.LinkToRelatedMFS = obj["linkToRelatedMFS"].toBool();
        newItem.RelatedChannel = obj["relatedChannel"].toString();
        return newItem;
    }

    static QVariant toVariant(ChannelModel33 item)
    {
        return QVariant::fromValue(toJson(item));
     }

    static QJsonObject toJson(ChannelModel33 item)
    {
        QJsonObject obj;
        obj["friendlyName"] = item.FriendlyName;
        obj["axisName"] = item.AxisName;
        obj["channel"] = item.Channel;
        obj["min"] = item.Min;
        obj["mid"] = item.Mid;
        obj["max"] = item.Max;
        obj["userMin"] = item.UserMin;
        obj["userMid"] = item.UserMid;
        obj["userMax"] = item.UserMax;
        obj["dimension"] = (int)item.Dimension;
        obj["type"] = (int)item.Type;
        obj["trackName"] = item.TrackName;
        obj["multiplierEnabled"] = item.MultiplierEnabled;
        obj["damperEnabled"] = item.DamperEnabled;
        obj["speedRandom"] = item.DamperRandom;
        obj["damperValue"] = item.DamperValue;
        obj["funscriptInverted"] = item.FunscriptInverted;
        obj["gamepadInverted"] = item.GamepadInverted;
        obj["linkToRelatedMFS"] = item.LinkToRelatedMFS;
        obj["relatedChannel"] = item.RelatedChannel;
        return obj;
    }
};
Q_DECLARE_METATYPE(ChannelModel33);
#endif // CHANNELMODEL33_H
