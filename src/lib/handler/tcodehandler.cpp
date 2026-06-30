#include "tcodehandler.h"

#include <QTimer>

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
    QString tcode = nullptr;
    std::shared_ptr<FunscriptAction> mainAction = 0;
    int mainActionGradient = 0;
    QList<QString> tcodeChannelNames = actions.keys();

    foreach(auto tcodeChannelName, tcodeChannelNames)
    {
        if(!TCodeChannelLookup::ChannelExists(tcodeChannelName))
            continue;
        auto channelModel = TCodeChannelLookup::getChannel(tcodeChannelName);
        if (channelModel->ChannelName != TCodeChannelLookup::Stroke() && channelModel->trackName.isEmpty())
            continue;
        if((channelModel->ChannelName == TCodeChannelLookup::Suck() || channelModel->ChannelName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
            continue;
        std::shared_ptr<FunscriptAction> axisAction = actions.value(tcodeChannelName);
        int position = axisAction->pos;
        //LogHandler::Debug("Channel: "+ axisModel->FriendlyName + " pos: " + QString::number(position) + ", at: " + QString::number(axisAction->at));
        if (channelModel->FunscriptInverted)
        {
            position = XMath::reverseNumber(position, 0, 100);
        }
        if(!tcode.isEmpty())
            tcode += " ";
        tcode += tcodeChannelName;
        QByteArray channelNameUtf8 = tcodeChannelName.toUtf8();
        int currentDestinationTCode = calculateRange(channelNameUtf8, position);
        tcode += QString::number(currentDestinationTCode).rightJustified(SettingsHandler::getTCodePadding(), '0');
        if (axisAction->interval > 0)
        {
          tcode += "I";
          tcode += QString::number(axisAction->interval);
        }
        int gradient = 0;
        if(channelModel->Gradient)
        {
            tcode += "G";
            int previousDestinationTCode = calculateRange(channelNameUtf8, axisAction->currentSequence.lastDestinationPos);
            int nextDestinationTCode = calculateRange(channelNameUtf8, axisAction->currentSequence.nextDestinationPos);
            gradient = calculateGradient(previousDestinationTCode, currentDestinationTCode, nextDestinationTCode, axisAction->currentSequence.lastDestinationAt, axisAction->currentSequence.nextDestinationAt);
            tcode += QString::number(gradient);
        }
        if (channelModel->Channel == TCodeChannelLookup::Stroke())
        {
            mainAction = axisAction;
            mainActionGradient = gradient;
        }
    }
    if(!mainAction && !actions.empty())
    {
        mainAction = actions.first();
    }
    if(!tcode.isEmpty())
        tcode += " ";
    tcode += handleMotionModifier(mainAction, mainActionGradient, actions);
    // LogHandler::Debug("funscriptToTCode: "+tcode);
    return tcode;
}

QString TCodeHandler::handleMotionModifier(std::shared_ptr<FunscriptAction> mainAction, int mainActionGradient, QMap<QString, std::shared_ptr<FunscriptAction>> actions)
{
    QString tcode;
    if(SettingsHandler::getMultiplierEnabled())
    {
        QList<QString> tcodeChannelNames = TCodeChannelLookup::getChannels();
        foreach(auto tcodeChannelName, tcodeChannelNames)
        {
            if(tcodeChannelName.isEmpty())
                continue;
            if(!TCodeChannelLookup::ChannelExists(tcodeChannelName))
                continue;
            ChannelModel33* channel = TCodeChannelLookup::getChannel(tcodeChannelName);
            if (channel->Type == ChannelType::HalfOscillate || channel->Type == ChannelType::None)
                continue;
            if (SettingsHandler::getFunscriptLoaded(tcodeChannelName))
                continue;
            if((channel->ChannelName == TCodeChannelLookup::Suck() || channel->ChannelName == TCodeChannelLookup::SuckPosition()) && (tcode.contains(TCodeChannelLookup::Suck()) || tcode.contains(TCodeChannelLookup::SuckPosition())))
                continue;
            if (channel->MultiplierEnabled)
            {
                multiplierEnabledTracker[channel->track] = true;
                QString tcodeTemp = getMotionModifierTCode(channel, mainAction, mainActionGradient, actions);
                if(!tcodeTemp.isEmpty())
                {
                    if(!tcode.isEmpty())
                        tcode += " ";
                    tcode += tcodeTemp;
                }
            }
            else if(multiplierEnabledTracker.value(channel->track, false))
            {
                multiplierEnabledTracker[channel->track] = false;
                getChannelHome(channel, tcode);
            }
        }
    }
    return tcode;
}

QString TCodeHandler::getMotionModifierTCode(ChannelModel33* channel, std::shared_ptr<FunscriptAction> mainAction, int mainActionGradient, QMap<QString, std::shared_ptr<FunscriptAction>> actions)
{
    // // Establish link to related channel to axis that are NOT stroke.
    // if ((channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && actions.contains(channel->RelatedChannel)))
    //     mainAction = actions.value(channel->RelatedChannel);
    // else if(channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(channel->RelatedChannel) && !actions.contains(channel->RelatedChannel) && channel->RelatedChannel != TCodeChannelLookup::Stroke())
    //     continue;
    // if(mainAction == nullptr)
    //     continue;
    int value = -1;
    int interval = mainAction ? mainAction->interval : 0;
    int gcode = mainAction ? mainActionGradient : 0;
    QString modifierSymbol = "I";
    // int channelDistance = 100;
    auto relatedChannel = channel->RelatedChannel;
    auto modifier = TCodeChannelLookup::removeModifier(relatedChannel);
    std::shared_ptr<FunscriptAction> linkedAction = 0;
    QByteArray channelNameUtf8 = channel->Channel.toUtf8();
    if (channel->LinkToRelatedMFS && SettingsHandler::getFunscriptLoaded(relatedChannel))
    {
        if(actions.contains(relatedChannel))
        {
            linkedAction = actions.value(relatedChannel);
            FunscriptActionSequence* linkedActionSequence = channel->Offset < 0 ? &linkedAction->nextSequence : &linkedAction->currentSequence;
            value = linkedActionSequence->currentDestinationPos;
            interval = linkedActionSequence->currentDestinationInterval;
            if(value < 0)
                return QString();// -1 = No next pos from funscriptHandler
            if(!modifier.isEmpty())
            {
                if(modifier == TCodeChannelLookup::PositiveModifier)
                {
                    if(value >= 50)
                        value = XMath::mapRange(value, 0, 100, 50, 100);
                    else
                        value = 50;
                    if (channel->FunscriptInverted)
                    {
                        //LogHandler::Debug("inverted: "+ QString::number(value));
                        value = XMath::reverseNumber(value, 50, 100);
                    }
                }
                else
                {
                    if(value < 50)
                        value = XMath::mapRange(value, 0, 100, 0, 49);
                    else
                        value = 49;
                    if (channel->FunscriptInverted)
                    {
                        //LogHandler::Debug("inverted: "+ QString::number(value));
                        value = XMath::reverseNumber(value, 0, 49);
                    }
                }
            }
            else// Im not sure how to handle modifier links in sequence. Would need to setup a seq. tracker for linked/random motion I think.
            {
                int seqA = calculateRange(channelNameUtf8, linkedActionSequence->lastDestinationPos);
                int seqB = calculateRange(channelNameUtf8, linkedActionSequence->currentDestinationPos);
                int seqC = calculateRange(channelNameUtf8, linkedActionSequence->nextDestinationPos);
                gcode = calculateGradient(seqA, seqB, seqC, linkedActionSequence->lastDestinationAt, linkedActionSequence->nextDestinationAt);
            }
        } else
            return QString();
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
        int lastPos = channelValueTracker.contains(channel->Channel) ? channelValueTracker[channel->Channel] : -1;
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
        channelValueTracker[channel->Channel] = value;
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
    if (channel->FunscriptInverted && channel->LinkToRelatedMFS && modifier.isEmpty())
    {
        //LogHandler::Debug("inverted: "+ QString::number(value));
        value = XMath::reverseNumber(value, 0, 100);
    }
    QString tcodeTemp = "";
    tcodeTemp += channel->Channel;
    int range = calculateRange(channelNameUtf8, value);
    if(range < 0)
    {
        LogHandler::Warn("Value cant be less than zero: "+ QString::number(range) +" originalValue: " + QString::number(value));
        range = 0;
    }
    // if(range > 9999) {
    //     LogHandler::Warn("Value cant be greater than 9999: "+ QString::number(range) +" originalValue: " + QString::number(value));
    // }
    tcodeTemp += QString::number(range).rightJustified(SettingsHandler::getTCodePadding(), '0');
    if(interval <= 0)
    {
        interval = XMath::random(250, 1500);
        modifierSymbol = "S";
    }
    tcodeTemp += modifierSymbol;
    // tcodeTemp channelDistancePercentage = channelDistance/100.0f;
    if (channel->SpeedEnabled && channel->SpeedValue > 0.0)
    {
        float speedModifierValue = channel->SpeedRandom ? XMath::random(0.1f, channel->SpeedValue) : channel->SpeedValue;
        interval = qRound(interval * speedModifierValue);
    }
    tcodeTemp += QString::number(interval);

    if(channel->Gradient)
    {
        tcodeTemp += "G";
        tcodeTemp += QString::number(gcode);
    }

    if(channel->Offset > 0)
    {
        // Delay the CURRENT destination
        int delayMS = channel->Offset * interval;
        emit delayTCode(tcodeTemp, delayMS);
        return QString();
    }
    else if (channel->Offset < 0)
    {
        // Delay the NEXT destination (chosen in linked action section above) based of the current destination interval.
        int currentInterval = linkedAction ? linkedAction->interval : mainAction ? mainAction->interval : -1;
        if(currentInterval < 0)
            return QString();// This should never happen because the first action is always chosen.
        int percentageMS = abs(channel->Offset * currentInterval);
        int delayMS = abs(percentageMS - currentInterval);
        emit delayTCode(tcodeTemp, delayMS);
        return QString();
    }
    return tcodeTemp;
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
        if(channel->Dimension == ChannelDimension::Heave || channel->Type != ChannelType::Oscillate)
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
        if(channel->Type == ChannelType::HalfOscillate || channel->Type == ChannelType::None )
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
        if(channel->Type != ChannelType::Ramp )
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
    if(channel->Type == ChannelType::HalfOscillate || channel->Type == ChannelType::None || channel->Channel == TCodeChannelLookup::Suck() || channel->Channel == TCodeChannelLookup::SuckPosition()) {
        return;
    }
    if(!tcode.isEmpty())
        tcode += " ";
    tcode += channel->Channel;
    int homeValue = channel->Type == ChannelType::Ramp ? channel->Min : channel->Mid;
    tcode += QString::number(homeValue).rightJustified(SettingsHandler::getTCodePadding(), '0');
    tcode += "S1000";
}

int TCodeHandler::getDistance(int current, int last)
{
    return current >= last ? current - last : last - current;
}

int TCodeHandler::calculateGradient(int lastPos, int currentPos, int nextPos, qint64 lastAtMilli, qint64 nextAtMilli)
{
    if((currentPos-lastPos)*(nextPos-currentPos) < 0)
        return 0;
    // Guard aginst dividing by 0
    double differencePos = (double)nextPos - (double)lastPos;
    if(!differencePos)
        return 0;
    if(nextAtMilli == -1 || lastAtMilli == -1)// -1 means we are at the end or begining
        return 0;
    double differenceAtSec = ((double)nextAtMilli - (double)lastAtMilli)/1000;
    if(!differenceAtSec)
        return 0;
    // LogHandler::Debug("TCodeHandler::calculateGradient: lastPos: "+ QString::number(lastPos) +" currentPos: "+ QString::number(currentPos) +" nextPos: "+ QString::number(nextPos) +" lastAtMilli: "+ QString::number(lastAtMilli) +" nextAtMilli: "+ QString::number(nextAtMilli)+" differenceAtSec: "+ QString::number(differenceAtSec)+" differencePos/differenceAtSec: "+ QString::number((differencePos/10)/differenceAtSec));
    return (differencePos/10)/differenceAtSec;
}