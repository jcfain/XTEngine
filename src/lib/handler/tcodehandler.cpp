#include "tcodehandler.h"

#include "../lookup/tcodechannellookup.h"
#include "../tool/xmath.h"
#include "settingshandler.h"
#include "funscripthandler.h"
#include "loghandler.h"

TCodeHandler::TCodeHandler(QObject* parent) : QObject(parent)
{
}


TCodeHandler::~TCodeHandler() {

}

QString TCodeHandler::funscriptToTCode(std::shared_ptr<FunscriptAction> strokeAction, QMap<QString, std::shared_ptr<FunscriptAction>> otherActions)
{
    QMutexLocker locker(&mutex);
    auto axisKeys = TCodeChannelLookup::getChannels();
    QString tcode = "";
    int strokeDistance = 0;
    int strokeSpeed = 0;
    if(strokeAction != nullptr)
    {
        strokeDistance = getDistance(strokeAction->pos, strokeAction->lastPos);
        strokeSpeed = strokeAction->speed;
        if(strokeDistance > 0)
        {
            int position = strokeAction->pos;
            LogHandler::Debug("Stroke pos: " + QString::number(position) + ", at: " + QString::number(strokeAction->at));
            if (FunscriptHandler::getInverted() || SettingsHandler::getChannelFunscriptInverseChecked(TCodeChannelLookup::Stroke()))
            {
                position = XMath::reverseNumber(position, 0, 100);
            }
            tcode += TCodeChannelLookup::Stroke();
            tcode += QString::number(calculateRange(TCodeChannelLookup::Stroke().toUtf8(), position)).rightJustified(SettingsHandler::getTCodePadding(), '0');
            // LogHandler::Debug("Stroke tcode: "+ tcode);
            if (strokeSpeed > 0)
            {
              tcode += "I";
              tcode += QString::number(strokeSpeed);
            }
        }
    }
    if(!otherActions.empty())
    {
        foreach(auto axis, axisKeys)
        {
            if(!TCodeChannelLookup::ChannelExists(axis))
                continue;
            auto axisModel = TCodeChannelLookup::getChannel(axis);
            if (axisModel->Channel == TCodeChannelLookup::Stroke() || axisModel->TrackName.isEmpty())
                continue;
            if((axisModel->AxisName == TCodeChannelLookup::Suck() || axisModel->AxisName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
                continue;
            if (otherActions.contains(axis))
            {
                std::shared_ptr<FunscriptAction> axisAction = otherActions.value(axis);
                int position = axisAction->pos;
                LogHandler::Debug("MFS: "+ axisModel->FriendlyName + " pos: " + QString::number(position) + ", at: " + QString::number(axisAction->at));
                if (axisModel->FunscriptInverted)
                {
                    position = XMath::reverseNumber(position, 0, 100);
                }
                if(!tcode.isEmpty())
                    tcode += " ";
                tcode += axis;
                tcode += QString::number(calculateRange(axis.toUtf8(), position)).rightJustified(SettingsHandler::getTCodePadding(), '0');
                if (axisAction->speed > 0)
                {
                  tcode += "I";
                  tcode += QString::number(axisAction->speed);
                }
            }
        }
    }

    if(strokeAction != nullptr && SettingsHandler::getMultiplierEnabled())
    {
        foreach(auto axis, axisKeys)
        {
            if(axis.isEmpty())
                continue;
            if(!TCodeChannelLookup::ChannelExists(axis))
                continue;
            ChannelModel33* channel = TCodeChannelLookup::getChannel(axis);
            if (channel->Dimension == AxisDimension::Heave || channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None)
                continue;
            if (SettingsHandler::getFunscriptLoaded(axis))
                continue;
            if((channel->AxisName == TCodeChannelLookup::Suck() || channel->AxisName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
                continue;
            if (channel->MultiplierEnabled)
            {
                // Establish link to related channel to axis that are NOT stroke.
                if ((channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && otherActions.contains(channel->RelatedChannel)))
                    strokeAction = otherActions.value(channel->RelatedChannel);
                else if(channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && !otherActions.contains(channel->RelatedChannel) && channel->RelatedChannel != TCodeChannelLookup::Stroke())
                    continue;
                if(strokeAction == nullptr)
                    continue;
                int value = -1;
                int channelDistance = 100;
                if ((channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel)) && (otherActions.contains(channel->RelatedChannel) || channel->RelatedChannel == TCodeChannelLookup::Stroke()))
                {
                    value = channel->RelatedChannel == TCodeChannelLookup::Stroke() ? strokeAction->pos : otherActions.value(channel->RelatedChannel)->pos;
//                        LogHandler::Debug("Channel: "+ axis);
//                        LogHandler::Debug("FriendlyName: "+ channel->FriendlyName);
//                        LogHandler::Debug("RelatedChannel: "+ channel->RelatedChannel);
//                        LogHandler::Debug("RelatedChannel FriendlyName: "+ SettingsHandler::getAxis(channel->RelatedChannel).FriendlyName);
//                        LogHandler::Debug("LinkToRelatedMFS value: "+ QString::number(value));
//                        LogHandler::Debug("currentAction->pos: "+ QString::number(currentAction->pos));
//                        LogHandler::Debug("action->pos: "+ QString::number(action->pos));
                }
                else
                {
                    int min = 0;
                    int max = 100;
                    // if((channelValueTracker.contains(axis) && channelValueTracker[axis] > 50)) {
                    //     max = 50;// - (qRound(strokeDistance / 2.0f) + 1);
                    // } else {
                    //     min = 50;// + (qRound(strokeDistance / 2.0f) - 1);
                    // }
                    value = XMath::random(min, max);
                    LogHandler::Debug("Channel: "+ axis);
                    LogHandler::Debug("Value: "+ QString::number(value));
                    if(channelValueTracker.contains(axis)) {
                        channelDistance = getDistance(value, channelValueTracker[axis]);
                        LogHandler::Debug("Last value: "+ QString::number(channelValueTracker[axis]));
                    }
                    channelValueTracker[axis] = value;
                }
                //lowMin + (highMin-lowMin)*level,lowMax + (highMax-lowMax)*level
                //LogHandler::Debug("randSine: "+ QString::number(value));
                if (value < 0)
                {
                    LogHandler::Warn("Value was less than zero: "+ QString::number(value));
                    continue;
                }
                //LogHandler::Debug("Multiplier: "+ channel->FriendlyName + " pos: " + QString::number(value) + ", at: " + QString::number(currentAction->at));
                if ((channel->FunscriptInverted && channel->LinkToRelatedMFS) ||
                        (SettingsHandler::getChannelFunscriptInverseChecked(TCodeChannelLookup::Stroke()) && !channel->FunscriptInverted && !channel->LinkToRelatedMFS) )
                {
                    //LogHandler::Debug("inverted: "+ QString::number(value));
                    value = XMath::reverseNumber(value, 0, 100);
                }
                tcode += " ";
                tcode += axis;
                tcode += QString::number(calculateRange(axis.toUtf8(), value)).rightJustified(SettingsHandler::getTCodePadding(), '0');
                tcode += "S";
                float channelDistancePercentage = channelDistance/100.0f;
                auto speed = strokeAction->speed > 0 ? qRound(strokeAction->speed * channelDistancePercentage) : 500;
                if (channel->DamperEnabled && channel->DamperValue > 0.0)
                {
                    float speedModifierValue = channel->DamperRandom ? XMath::random(0.1f, channel->DamperValue) : channel->DamperValue;
                    speed = qRound(speed * speedModifierValue);
                    tcode += QString::number(speed);
                }
                else
                {
                    tcode += QString::number(speed);
                }
            }
//            else
//            {
//                getChannelHome(channel, tcode);
//            }
        }
    }
    return tcode.isEmpty() ? nullptr : tcode;
}

int TCodeHandler::calculateRange(const char* channel, int rawValue)
{
    int xMax = TCodeChannelLookup::getChannel(channel)->UserMax;
    //int xMin = TCodeChannelLookup::getChannel(channel)->UserMin;
    int xMid = TCodeChannelLookup::getChannel(channel)->UserMid;
    // Update for live x range switch
    if(QString(channel) == TCodeChannelLookup::Stroke())
    {
        xMax = TCodeChannelLookup::getLiveXRangeMax();
        //xMin = TCodeChannelLookup::getLiveXRangeMin();
        xMid = TCodeChannelLookup::getLiveXRangeMid();
    }
    return XMath::mapRange(rawValue, 50, 100, xMid, xMax);
}

QString TCodeHandler::getRunningHome()
{
    QString tcode;
    auto axisKeys = TCodeChannelLookup::getChannels();
    foreach(auto axis, axisKeys)
    {
        auto channel = TCodeChannelLookup::getChannel(axis);
        if(channel->Dimension == AxisDimension::Heave || channel->Type != AxisType::Oscillate)
            continue;
        getChannelHome(channel, tcode);
    }
    return tcode;
}

QString TCodeHandler::getAllHome()
{
    QString tcode;
    auto axisKeys = TCodeChannelLookup::getChannels();
    foreach(auto axis, axisKeys)
    {
        auto channel = TCodeChannelLookup::getChannel(axis);
        if(channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None )
            continue;
        getChannelHome(channel, tcode);
    }
    return tcode;
}

QString TCodeHandler::getSwitchedHome()
{
    QString tcode;
    auto axisKeys = TCodeChannelLookup::getChannels();
    foreach(auto axis, axisKeys)
    {
        auto channel = TCodeChannelLookup::getChannel(axis);
        if(channel->Type != AxisType::Ramp )
            continue;
        getChannelHome(channel, tcode);
    }
    return tcode;
}

QString TCodeHandler::getChannelHome(QString channel)
{
    QString tcode = "";
    auto channelModel = TCodeChannelLookup::getChannel(channel);
    getChannelHome(channelModel, tcode);
    return tcode;
}

void TCodeHandler::getChannelHome(ChannelModel33* channel, QString &tcode)
{
    if(channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None || channel->Channel == TCodeChannelLookup::Suck() || channel->Channel == TCodeChannelLookup::SuckPosition()) {
        return;
    }
    if(!tcode.isEmpty())
        tcode += " ";
    tcode += channel->Channel;
    int homeValue = channel->Type == AxisType::Ramp ? channel->Min : channel->Mid;
    tcode += QString::number(homeValue).rightJustified(SettingsHandler::getTCodePadding(), '0');
    tcode += "S1000";
}

int TCodeHandler::getDistance(int current, int last)
{
    return current >= last ? current - last : last - current;
}
