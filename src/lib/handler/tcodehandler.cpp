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

QString TCodeHandler::funscriptToTCode(QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime)
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
    tcode += handleMotionModifier(mainAction, actions, referenceTime);
    // LogHandler::Debug("funscriptToTCode: "+tcode);
    return tcode;
}

QString TCodeHandler::handleMotionModifier(std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime)
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
                QString tcodeTemp = getMotionModifierTCode(channel, mainAction, actions, referenceTime);
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

QString TCodeHandler::getMotionModifierTCode(ChannelModel33* channel, std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime)
{
    int value = -1;
    int interval = -1;
    qint64 gcode = INT64_MAX;
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
                        value = XMath::reverseNumber(value, 0, 49);
                    }
                }
                gcode = INT64_MAX;
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
    }
    else// Choose random values
    {
        modifierSymbol = "S";
        if(mainAction)// This should always be true as there should at least be one action in the list.
        {
            FunscriptActionSequence* mainActionSequence = channel->Offset < 0 ? &mainAction->nextSequence : &mainAction->currentSequence;
            interval = channel->Offset < 0 ? mainActionSequence->currentDestinationInterval : mainActionSequence->currentDestinationInterval;
        }
        int min = 0;
        int max = 100;
        int lastPos = channelValueTracker.contains(channel->Channel) ? channelValueTracker[channel->Channel] : -1;

        bool opposite = XMath::random(0, 100) > 50;// DONT ALWAYS osscillate.
        if(lastPos > -1)
        {
            min = lastPos < 50 && opposite ? 50 : 0;
            max = lastPos > 50 && opposite ? 50 : 100;
            if(channel->Gradient)
                gcode = opposite ? 0 : INT64_MAX;// TODO, track sequence for calculating gradient if possible
        }
        value = XMath::random(min, max);
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
    if (channel->FunscriptInverted && channel->LinkToRelatedMFS && modifier.isEmpty())// Modifier needs to be +/- mid point. Handled above
    {
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
    }
    // tcodeTemp channelDistancePercentage = channelDistance/100.0f;
    if (channel->SpeedEnabled && channel->SpeedValue > 0.0)
    {
        float speedModifierValue = channel->SpeedRandom ? XMath::random(0.1f, channel->SpeedValue) : channel->SpeedValue;
        // interval = qRound(channel->LinkToRelatedMFS ? interval/speedModifierValue : interval * speedModifierValue);
        interval = qRound(channel->LinkToRelatedMFS ? interval/speedModifierValue : interval * speedModifierValue);
    }
    tcodeTemp += modifierSymbol;
    tcodeTemp += QString::number(interval);

    if(channel->Gradient && gcode < INT64_MAX)
    {
        tcodeTemp += "G";
        tcodeTemp += QString::number(gcode);
    }

    if(channel->Offset > 0) // Higher values = delay current action
    {
        // Delay the CURRENT destination
        int delayMS = channel->Offset * interval;
        emit delayTCode(tcodeTemp, delayMS);
        // qint64 currentAt = linkedAction ? linkedAction->at : mainAction ? mainAction->at : -1;
        // if(currentAt > -1)
        //     m_delayedFunscriptActions.append({(currentAt - interval) + delayMS, tcodeTemp});
        // LogHandler::Debug("channel->Offset > 0 index:------------------------------------------------------------------ "+QString::number(m_delayedFunscriptActions.length() - 1));
        // LogHandler::Debug("currentAt: "+QString::number(currentAt));
        // LogHandler::Debug("reference: "+QString::number(referenceTime));
        // LogHandler::Debug("channel->Offset > 0: tcodeTemp: "+tcodeTemp);
        // LogHandler::Debug("channel->Offset > 0: currentAt - interval: "+QString::number(currentAt)+" + "+QString::number(interval) + " = "+QString::number(currentAt - interval));
        // LogHandler::Debug("channel->Offset > 0: '' + delayMS: "+QString::number((currentAt - interval) + delayMS));
        // LogHandler::Debug("channel->Offset > 0 return:------------------------------------------------------------------");
        return QString();
    }
    else if (channel->Offset < 0) // Lower values = execute next action early
    {
        // Delay the NEXT destination (chosen in linked action section above) based of the current destination interval.
        int currentInterval = linkedAction ? linkedAction->interval : mainAction ? mainAction->interval : -1;
        if(currentInterval < 0)
            return QString();// This should never happen because the first action is always chosen.
        int percentageMS = abs(channel->Offset * currentInterval);
        int delayMS = abs(percentageMS - currentInterval);
        // qint64 currentAt = linkedAction ? linkedAction->at : mainAction ? mainAction->at : -1;
        // if(currentAt > -1)
        //     m_delayedFunscriptActions.append({(currentAt - interval) + delayMS, tcodeTemp});
        // LogHandler::Debug("channel->Offset < 0: tcodeTemp: "+tcodeTemp);
        // LogHandler::Debug("channel->Offset < 0: currentAt + delayMS "+QString::number(currentAt)+" + "+QString::number(delayMS) + " = "+QString::number(currentAt + delayMS));
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

///
/// \brief TCodeHandler::getDelayedActions Commented out be cause async emmiter still works, Just cant stop it easily.
/// tbf, this cant be stopped very easy either
/// \param at
/// \return
///
QString TCodeHandler::getDelayedActions(qint64 referenceTime)
{
    // if(m_delayedFunscriptActions.empty())
        return "";
    // if(abs(referenceTime - m_lastTimeTracker) >= 1000)
    //     m_delayedFunscriptActions.clear();
    // m_lastTimeTracker = referenceTime;
    // QString tcode = "";
    // QList<int> toDelete;
    // // LogHandler::Debug("at: "+QString::number(at));
    // for(int i=0; i < m_delayedFunscriptActions.length(); i++)
    // {
    //     // LogHandler::Debug("m_delayedFunscriptActions[i].at: "+QString::number(m_delayedFunscriptActions[i].at));
    //     if(referenceTime >= m_delayedFunscriptActions[i].at)
    //     {
    //         tcode += m_delayedFunscriptActions[i].tcode;
    //         if(i < m_delayedFunscriptActions.length() - 1)
    //             tcode += " ";
    //         toDelete.append(i);
    //         LogHandler::Debug("getDelayedActions: index:------------------------------------------------------------------ "+QString::number(i));
    //         LogHandler::Debug("getDelayedActions: tcode: "+tcode);
    //         LogHandler::Debug("getDelayedActions: referenceTime >= at: "+QString::number(referenceTime)+" >= "+QString::number(m_delayedFunscriptActions[i].at));
    //         LogHandler::Debug("getDelayedActions: return:------------------------------------------------------------------");
    //     }
    // }
    // for(int i=0; i < toDelete.length(); i++)
    // {
    //     m_delayedFunscriptActions.removeAt(toDelete[i]);
    // }
    // // LogHandler::Debug("tcode: "+tcode);
    // return tcode;
}

void TCodeHandler::clearDelayedActions()
{
    // m_delayedFunscriptActions.clear();
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