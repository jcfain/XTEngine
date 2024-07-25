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

QString TCodeHandler::funscriptToTCode(QMap<QString, std::shared_ptr<FunscriptAction>> actions)
{
    if(actions.isEmpty())
        return nullptr;
    QMutexLocker locker(&mutex);
    auto axisKeys = TCodeChannelLookup::getChannels();
    QString tcode = nullptr;
    std::shared_ptr<FunscriptAction> mainAction = 0;
    QList<QString> actionKeys = actions.keys();
    //int mainDistance = 0;
    //int mainSpeed = 0;
    // if(strokeAction != nullptr)
    // {
    //     strokeDistance = getDistance(strokeAction->pos, strokeAction->lastPos);
    //     strokeSpeed = strokeAction->speed;
    //     if(strokeDistance > 0)
    //     {
    //         int position = strokeAction->pos;
    //         LogHandler::Debug("Stroke pos: " + QString::number(position) + ", at: " + QString::number(strokeAction->at));
    //         if (FunscriptHandler::getInverted() || SettingsHandler::getChannelFunscriptInverseChecked(TCodeChannelLookup::Stroke()))
    //         {
    //             position = XMath::reverseNumber(position, 0, 100);
    //         }
    //         tcode += TCodeChannelLookup::Stroke();
    //         tcode += QString::number(calculateRange(TCodeChannelLookup::Stroke().toUtf8(), position)).rightJustified(SettingsHandler::getTCodePadding(), '0');
    //         // LogHandler::Debug("Stroke tcode: "+ tcode);
    //         if (strokeSpeed > 0)
    //         {
    //           tcode += "I";
    //           tcode += QString::number(strokeSpeed);
    //         }
    //     }
    // }
    foreach(auto axis, actionKeys)
    {
        if(!TCodeChannelLookup::ChannelExists(axis))
            continue;
        auto axisModel = TCodeChannelLookup::getChannel(axis);
        if (axisModel->AxisName != TCodeChannelLookup::Stroke() && axisModel->TrackName.isEmpty())
            continue;
        if((axisModel->AxisName == TCodeChannelLookup::Suck() || axisModel->AxisName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
            continue;
        std::shared_ptr<FunscriptAction> axisAction = actions.value(axis);
        if (axisModel->Channel == TCodeChannelLookup::Stroke())
        {
            mainAction = axisAction;
            //mainDistance = getDistance(mainAction->pos, mainAction->lastPos);
            //mainSpeed = mainAction->speed;
        }
        int position = axisAction->pos;
        LogHandler::Debug("Channel: "+ axisModel->FriendlyName + " pos: " + QString::number(position) + ", at: " + QString::number(axisAction->at));
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
    if(!mainAction && !actions.empty())
    {
        mainAction = actions.first();
    }

    if(SettingsHandler::getMultiplierEnabled())
    {
        foreach(auto axis, axisKeys)
        {
            if(axis.isEmpty())
                continue;
            if(!TCodeChannelLookup::ChannelExists(axis))
                continue;
            ChannelModel33* channel = TCodeChannelLookup::getChannel(axis);
            if (channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None)
                continue;
            if (SettingsHandler::getFunscriptLoaded(axis))
                continue;
            if((channel->AxisName == TCodeChannelLookup::Suck() || channel->AxisName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
                continue;
            if (channel->MultiplierEnabled)
            {
                // // Establish link to related channel to axis that are NOT stroke.
                // if ((channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && actions.contains(channel->RelatedChannel)))
                //     mainAction = actions.value(channel->RelatedChannel);
                // else if(channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && !actions.contains(channel->RelatedChannel) && channel->RelatedChannel != TCodeChannelLookup::Stroke())
                //     continue;
                // if(mainAction == nullptr)
                //     continue;
                int value = -1;
                // int channelDistance = 100;
                if ((channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel)) && (actions.contains(channel->RelatedChannel)))
                {
                    value = actions.value(channel->RelatedChannel)->pos;
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
                    int lastPos = channelValueTracker.contains(axis) ? channelValueTracker[axis] : -1;
                    //int userMid = TCodeChannelLookup::getChannel(axis)->UserMid;

                    bool opposite = XMath::random(0, 100) > 50;
                    if(lastPos > -1)
                    {
                        min = lastPos < 50 && opposite ? 50 : 0;
                        max = lastPos > 50 && opposite ? 50 : 100;
                    }

                    // if((channelValueTracker.contains(axis) && channelValueTracker[axis] > 50)) {
                    //     max = 50;// - (qRound(strokeDistance / 2.0f) + 1);
                    // } else {
                    //     min = 50;// + (qRound(strokeDistance / 2.0f) - 1);
                    // }
                    value = XMath::random(min, max);
                    // LogHandler::Debug("Channel: "+ axis);
                    // LogHandler::Debug("Value: "+ QString::number(value));
                    // if(lastPos > -1) {
                    //     channelDistance = getDistance(value, lastPos);
                    //     LogHandler::Debug("Last value: "+ QString::number(channelValueTracker[axis]));
                    // }
                    channelValueTracker[axis] = value;
                }
                //lowMin + (highMin-lowMin)*level,lowMax + (highMax-lowMax)*level
                //LogHandler::Debug("randSine: "+ QString::number(value));
                if (value < 0)
                {
                    LogHandler::Warn("Value was less than zero: "+ QString::number(value));
                    value = 0;
                }
                if (value > 100)
                {
                    LogHandler::Warn("Value was greater than 100: "+ QString::number(value));
                    value = 100;
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
                int range = calculateRange(axis.toUtf8(), value);
                if(range < 0) {
                    LogHandler::Warn("Value cant be less than zero: "+ QString::number(range) +" originalValue: " + QString::number(value));
                    range = 0;
                }
                // if(range > 9999) {
                //     LogHandler::Warn("Value cant be greater than 9999: "+ QString::number(range) +" originalValue: " + QString::number(value));
                // }
                tcode += QString::number(range).rightJustified(SettingsHandler::getTCodePadding(), '0');
                tcode += "S";
                // float channelDistancePercentage = channelDistance/100.0f;

                auto speed = mainAction && mainAction->speed > 0 ? mainAction->speed : XMath::random(250, 1500);
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
    return tcode;
}

int TCodeHandler::calculateRange(const char* channel, int rawValue)
{
    int xMax = TCodeChannelLookup::getChannel(channel)->UserMax;
    int xMin = TCodeChannelLookup::getChannel(channel)->UserMin;
    //int xMid = TCodeChannelLookup::getChannel(channel)->UserMid;
    // Update for live x range switch
    if(QString(channel) == TCodeChannelLookup::Stroke())
    {
        xMax = TCodeChannelLookup::getLiveXRangeMax();
        xMin = TCodeChannelLookup::getLiveXRangeMin();
        //xMid = TCodeChannelLookup::getLiveXRangeMid();
    }
    return XMath::mapRange(rawValue, 0, 100, xMin, xMax);
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
