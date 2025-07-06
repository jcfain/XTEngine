#ifndef CHANNELMODEL_H
#define CHANNELMODEL_H

#include <QString>
#include <QMetaType>
#include <QDataStream>
#include <QVariant>
#include <QJsonObject>
#include "ChannelModel33.h"
#include "../lookup/Track.h"
#include "../lookup/ChannelType.h"
#include "../lookup/ChannelDimension.h"


struct ChannelModel
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
    float MultiplierValue;
    bool DamperEnabled;
    float DamperValue;
    bool Inverted;
    bool LinkToRelatedMFS;
    QString RelatedChannel;

    friend QDataStream & operator<<( QDataStream &dataStream, const ChannelModel &object )
    {
        dataStream << object.FriendlyName;
        dataStream << object.AxisName;
        dataStream << object.Channel;
        dataStream << object.Min;
        dataStream << object.Mid;
        dataStream << object.Max;
        dataStream << object.UserMin;
        dataStream << object.UserMid;
        dataStream << object.UserMax;
        dataStream << (int)object.Dimension;
        dataStream << (int)object.Type;
        dataStream << object.TrackName;
        dataStream << object.MultiplierEnabled;
        dataStream << object.MultiplierValue;
        dataStream << object.DamperEnabled;
        dataStream << object.DamperValue;
        dataStream << object.Inverted;
        dataStream << object.LinkToRelatedMFS;
        dataStream << object.RelatedChannel;
        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, ChannelModel &object)
    {
        dataStream >> object.FriendlyName;
        dataStream >> object.AxisName;
        dataStream >> object.Channel;
        dataStream >> object.Min;
        dataStream >> object.Mid;
        dataStream >> object.Max;
        dataStream >> object.UserMin;
        dataStream >> object.UserMid;
        dataStream >> object.UserMax;
        dataStream >> object.Dimension;
        dataStream >> object.Type;
        dataStream >> object.TrackName;
        dataStream >> object.MultiplierEnabled;
        dataStream >> object.MultiplierValue;
        dataStream >> object.DamperEnabled;
        dataStream >> object.DamperValue;
        dataStream >> object.Inverted;
        dataStream >> object.LinkToRelatedMFS;
        dataStream >> object.RelatedChannel;
        return dataStream;
    }

    ChannelModel33 toChannelModel33() {
        ChannelModel33 item;
        item.FriendlyName = FriendlyName;
        item.ChannelName = AxisName;
        item.Channel = Channel;
        item.Min = Min;
        item.Mid = Mid;
        item.Max = Max;
        item.UserMin = UserMin;
        item.UserMid = UserMid;
        item.UserMax = UserMax;
        item.Dimension = Dimension;
        item.Type = Type;
        item.trackName = TrackName;
        item.MultiplierEnabled = MultiplierEnabled;
        item.DamperEnabled = DamperEnabled;
        item.DamperValue = DamperValue;
        item.FunscriptInverted = Inverted;
        item.GamepadInverted = false;
        item.LinkToRelatedMFS = LinkToRelatedMFS;
        item.RelatedChannel = RelatedChannel;
        return item;
    }
};
Q_DECLARE_METATYPE(ChannelModel);
#endif // CHANNELMODEL_H
