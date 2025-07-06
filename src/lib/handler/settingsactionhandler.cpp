#include "settingsactionhandler.h"

#include "xmediastatehandler.h"
#include "settingshandler.h"
#include "funscripthandler.h"

SettingsActionHandler::SettingsActionHandler(SyncHandler* syncHandler, QObject *parent)
    : QObject{parent}
{
    m_syncHandler = syncHandler;
}

void SettingsActionHandler::media_action(QString action)
{
    if(action == actions.TCodeSpeedUp)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() + SettingsHandler::getGamepadSpeedIncrement();
        SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
        emit actionExecuted(action, "Raise speed "+ QString::number(newGamepadSpeed), newGamepadSpeed);
    }
    else if(action == actions.TCodeSpeedDown)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() - SettingsHandler::getGamepadSpeedIncrement();
        if (newGamepadSpeed > 0)
        {
            SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
            emit actionExecuted(action, "Lower speed "+ QString::number(newGamepadSpeed), newGamepadSpeed);
        } else
            emit actionExecuted(action, "Lower speed at minimum", SettingsHandler::getLiveGamepadSpeed());
    }
    else if(action == actions.TCodeHomeAll)
    {
        emit tcode_action("DHOME");// TODO: figure out a better way to do this.
        emit actionExecuted(action, "Device home", QVariant());
    }
    else if(action == actions.IncreaseXLowerRange)
    {
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = TCodeChannelLookup::getLiveXRangeMin() + SettingsHandler::getXRangeStep();
        int xRangeMax = TCodeChannelLookup::getLiveXRangeMax();
        if(newLiveRange < xRangeMax - xRangeStep)
        {
            TCodeChannelLookup::setLiveXRangeMin(newLiveRange);
            emit actionExecuted(action, "Raise X min to "+ QString::number(newLiveRange), newLiveRange);
        }
        else
        {
            TCodeChannelLookup::setLiveXRangeMin(xRangeMax - xRangeStep);
            emit actionExecuted(action, "X Min limit reached", xRangeMax - xRangeStep);
        }
    }
    else if(action == actions.DecreaseXLowerRange)
    {
        int newLiveRange = TCodeChannelLookup::getLiveXRangeMin() - SettingsHandler::getXRangeStep();
        int axisMin = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->Min;
        if(newLiveRange > axisMin)
        {
            TCodeChannelLookup::setLiveXRangeMin(newLiveRange);
            emit actionExecuted(action, "Lower X min to "+ QString::number(newLiveRange), newLiveRange);
        }
        else
        {
            TCodeChannelLookup::setLiveXRangeMin(axisMin);
            emit actionExecuted(action, "Low X min limit reached", axisMin);
        }
    }
    else if(action == actions.IncreaseXUpperRange)
    {
        int newLiveRange = TCodeChannelLookup::getLiveXRangeMax() + SettingsHandler::getXRangeStep();
        int axisMax = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->Max;
        if(newLiveRange < axisMax)
        {
            TCodeChannelLookup::setLiveXRangeMax(newLiveRange);
            emit actionExecuted(action, "Raise X max to "+ QString::number(newLiveRange), newLiveRange);
        }
        else
        {
            TCodeChannelLookup::setLiveXRangeMax(axisMax);
            emit actionExecuted(action, "High X max limit reached", axisMax);
        }
    }
    else if(action == actions.DecreaseXUpperRange)
    {
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = TCodeChannelLookup::getLiveXRangeMax() - xRangeStep;
        int xRangeMin = TCodeChannelLookup::getLiveXRangeMin();
        if(newLiveRange > xRangeMin + xRangeStep)
        {
            TCodeChannelLookup::setLiveXRangeMax(newLiveRange);
            emit actionExecuted(action, "Lower X max to "+ QString::number(newLiveRange), newLiveRange);
        }
        else
        {
            TCodeChannelLookup::setLiveXRangeMax(xRangeMin + xRangeStep);
            emit actionExecuted(action, "Low X max limit reached", xRangeMin + xRangeStep);
        }
    }
    else if (action == actions.IncreaseXRange)
    {
        int xRangeMax = TCodeChannelLookup::getLiveXRangeMax();
        int xRangeMin = TCodeChannelLookup::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax + xRangeStep;
        int axisMax = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->Max;
        bool atMax = false;
        if(newLiveMaxRange > axisMax)
        {
            atMax = true;
            newLiveMaxRange = axisMax;
        }
        TCodeChannelLookup::setLiveXRangeMax(newLiveMaxRange);

        int newLiveMinRange = xRangeMin - xRangeStep;
        int axisMin = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->Min;
        bool atMin = false;
        if(newLiveMinRange < axisMin)
        {
            atMin = true;
            newLiveMinRange = axisMin;
        }
        TCodeChannelLookup::setLiveXRangeMin(newLiveMinRange);

        if (atMin && atMax)
            emit actionExecuted(action, "Increase X at limit", QVariant());
        else if (atMax)
            emit actionExecuted(action, "Increase X, max at limit, min"+ QString::number(newLiveMinRange), QVariant());
        else if (atMin)
            emit actionExecuted(action, "Increase X, max "+ QString::number(newLiveMaxRange) + ", min at limit", QVariant());
        else
            emit actionExecuted(action, "Increase X, max "+ QString::number(newLiveMaxRange) + ", min "+ QString::number(newLiveMinRange), QVariant());

    }
    else if (action == actions.DecreaseXRange)
    {
        int xRangeMax = TCodeChannelLookup::getLiveXRangeMax();
        int xRangeMin = TCodeChannelLookup::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax - xRangeStep;
        bool maxLessThanMin = false;
        if(newLiveMaxRange < xRangeMin)
        {
            maxLessThanMin = true;
            newLiveMaxRange = xRangeMin + 1;
        }
        TCodeChannelLookup::setLiveXRangeMax(newLiveMaxRange);

        int newLiveMinRange = xRangeMin + xRangeStep;
        bool minGreaterThanMax = false;
        if(newLiveMinRange > xRangeMax)
        {
            minGreaterThanMax = true;
            newLiveMinRange = xRangeMax - 1;
        }
        TCodeChannelLookup::setLiveXRangeMin(newLiveMinRange);
        if (maxLessThanMin && minGreaterThanMax)
            emit actionExecuted(action, "Decrease X at limit", QVariant());
        else if (maxLessThanMin)
            emit actionExecuted(action, "Decrease X, max at limit, min "+ QString::number(newLiveMinRange), QVariant());
        else if (minGreaterThanMax)
            emit actionExecuted(action, "Decrease X, max "+ QString::number(newLiveMaxRange) + ", min at limit", QVariant());
        else
            emit actionExecuted(action, "Decrease X, max "+ QString::number(newLiveMaxRange) + ", min "+ QString::number(newLiveMinRange), QVariant());

    }
    else if (action == actions.ResetLiveXRange)
    {
        emit actionExecuted(action, "Resetting X range", QVariant());
        TCodeChannelLookup::resetLiveXRange();
    }
    else if (action == actions.ToggleAxisMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierEnabled();
        emit actionExecuted(action, multiplier ? "Disable random motion" : "Enable random motion", multiplier);
        SettingsHandler::setLiveMultiplierEnabled(!multiplier);
    }
    else if (action == actions.ToggleChannelRollMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Roll());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Roll(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable roll motion" : "Enable roll motion", multiplier);
    }
    else if (action == actions.ToggleChannelPitchMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Pitch());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Pitch(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable pitch motion" : "Enable pitch motion", multiplier);
    }
    else if (action == actions.ToggleChannelSurgeMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Surge());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Surge(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable surge motion" : "Enable surge motion", multiplier);
    }
    else if (action == actions.ToggleChannelSwayMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Sway());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Sway(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable sway motion" : "Enable sway motion", multiplier);
    }
    else if (action == actions.ToggleChannelTwistMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Twist());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Twist(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable twist motion" : "Enable twist motion", multiplier);
    }
    else if (action == actions.ToggleFunscriptInvert)
    {
        bool inverted = FunscriptHandler::getInverted(Track::Stroke);
        emit actionExecuted(action, inverted ? "Funscript normal" : "Funscript inverted",inverted);
        FunscriptHandler::setInverted(Track::Stroke, !inverted);
    }
    else if(action == actions.TogglePauseAllDeviceActions)
    {
        bool paused = m_syncHandler->isPaused();
        emit actionExecuted(action, paused ? "Resume action" : "Pause action", !paused);
        m_syncHandler->setPause(!paused);
    }
    else if(action == actions.ToggleSkipToMoneyShotPlaysFunscript) {
        auto enabled = !SettingsHandler::getSkipToMoneyShotPlaysFunscript();
        QString verb = enabled ? "Enable" : "Disable";
        SettingsHandler::setSkipToMoneyShotPlaysFunscript(enabled);
        emit actionExecuted(action, verb + " plays funscript.", enabled);
    }
    else if (action == actions.IncreaseFunscriptModifier || action == actions.DecreaseFunscriptModifier || action == actions.ResetFunscriptModifier)
    {
        if(action == actions.ResetFunscriptModifier) {
            FunscriptHandler::resetModifier(Track::Stroke);
            emit actionExecuted(action, "Reset funscript modifier", QVariant());
        } else {
            bool increase = action == actions.IncreaseFunscriptModifier;
            QString verb = increase ? "Increase" : "Decrease";
            double modifier = FunscriptHandler::getModifier(Track::Stroke);
            double modedModifier = increase ? modifier + SettingsHandler::getFunscriptModifierStep() : modifier - SettingsHandler::getFunscriptModifierStep();

            if(modedModifier > 0)
            {
                FunscriptHandler::setModifier(Track::Stroke, modedModifier);
                emit actionExecuted(action, verb + " funscript modifier to "+ QString::number(modedModifier) + "percent", modedModifier);
            }
            else
                emit actionExecuted(action, "Funscript modifier at minimum "+ QString::number(modedModifier) + "percent", modedModifier);

        }
        if (XMediaStateHandler::isPlaying())
        {
            LibraryListItem27* item = XMediaStateHandler::getPlaying();
            if(item)
            {
                item->metadata.funscriptModifier = FunscriptHandler::getModifier(Track::Stroke);
                SettingsHandler::updateLibraryListItemMetaData(*item);
            }
        }
    } else if(MediaActions::HasOtherAction(action, ActionType::CHANNEL_PROFILE)) {
        TCodeChannelLookup::setSelectedChannelProfile(action);
        emit actionExecuted(action, "Set "+ action, action);
    }  else if(MediaActions::HasOtherAction(action, ActionType::TCODE)) {
        emit tcode_action(action);
        emit actionExecuted(action, "Execute TCode "+ action, action);
    } else if(action == actions.ChannelProfileNext || action == actions.ChannelProfilePrevious) {
        auto profiles = TCodeChannelLookup::getChannelProfiles();
        auto currentProfile = TCodeChannelLookup::getSelectedChannelProfile();
        auto indexOfProfile = profiles.indexOf(currentProfile);
        action == actions.ChannelProfileNext ? indexOfProfile++ : indexOfProfile--;
        if(indexOfProfile == profiles.length() && action == actions.ChannelProfileNext) {
            indexOfProfile = 0;
        } else if(indexOfProfile == -1 && action == actions.ChannelProfilePrevious) {
            indexOfProfile = profiles.length() - 1;
        }
        TCodeChannelLookup::setSelectedChannelProfile(profiles[indexOfProfile]);
        emit actionExecuted(profiles[indexOfProfile], "Set "+ profiles[indexOfProfile], profiles[indexOfProfile]);
    }
    else if (action == actions.IncreaseOffset || action == actions.DecreaseOffset || action == actions.ResetOffset)
    {
        bool increase = action == actions.IncreaseOffset;
        QString verb = increase ? "Increase" : "Decrease";
        bool reset = false;
        if(action == actions.ResetOffset) {
            reset = true;
            verb = "reset";
        }
        int newOffset = 0;
        if(!reset)
        {
            if (XMediaStateHandler::isPlaying())
            {
               LibraryListItem27* item = XMediaStateHandler::getPlaying();
               if(item)
               {
                    newOffset = increase ? item->metadata.offset + SettingsHandler::getFunscriptOffsetStep() : item->metadata.offset - SettingsHandler::getFunscriptOffsetStep();
                    item->metadata.offset = newOffset;
                    SettingsHandler::setLiveOffset(newOffset);
                    SettingsHandler::updateLibraryListItemMetaData(*item);
                    emit actionExecuted(action, verb + " offset to " + QString::number(newOffset), newOffset);
               }
            }
            else
            {
                newOffset = increase ? SettingsHandler::getLiveOffSet() + SettingsHandler::getFunscriptOffsetStep() : SettingsHandler::getLiveOffSet() - SettingsHandler::getFunscriptOffsetStep();
            }
        }
        else
        {
            newOffset = 0;
            emit actionExecuted(action, verb + " offset to " + QString::number(newOffset), newOffset);
        }
        SettingsHandler::setLiveOffset(newOffset);
    }
    else if (action == actions.SkipToMoneyShot)
    {
        emit actionExecuted(action, nullptr, QVariant());
    }
    else if (action == actions.SkipToAction)
    {
        emit actionExecuted(action, nullptr, QVariant());
    }
    else if (action == actions.IncreasePlaybackRate || action == actions.DecreasePlaybackRate || action == actions.ResetPlaybackRate)
    {
        bool increase = action == actions.IncreasePlaybackRate;
        QString verb = increase ? "Increase" : "Decrease";
        bool reset = false;
        if(action == actions.ResetPlaybackRate) {
            reset = true;
            verb = "reset";
        }
        double newRate = 0;
        if(!reset)
        {
            if (XMediaStateHandler::isPlaying())
            {
                double currentRate = XMediaStateHandler::getPlaybackSpeed();
                newRate = increase ? currentRate + SettingsHandler::getPlaybackRateStep() : currentRate - SettingsHandler::getPlaybackRateStep();

                emit actionExecuted(action, verb + " playback rate to " + QString::number(newRate), newRate);
            }
            else
            {
                XMediaStateHandler::setPlaybackSpeed(1);
            }
        }
        else
        {
            newRate = 1;
            XMediaStateHandler::setPlaybackSpeed(newRate);
            emit actionExecuted(action, verb + " playback rate to " + QString::number(newRate), newRate);
        }
    }
    else
    {
        emit actionExecuted(action, nullptr, QVariant());
    }
}
