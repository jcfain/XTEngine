#include "settingsactionhandler.h"

SettingsActionHandler::SettingsActionHandler(QObject *parent)
    : QObject{parent}
{

}

void SettingsActionHandler::media_action(QString action)
{
    if(action == actions.TCodeSpeedUp)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() + SettingsHandler::getGamepadSpeedIncrement();
        SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
        emit actionExecuted(action, "Raise speed "+ QString::number(newGamepadSpeed));
    }
    else if(action == actions.TCodeSpeedDown)
    {
        int newGamepadSpeed = SettingsHandler::getLiveGamepadSpeed() - SettingsHandler::getGamepadSpeedIncrement();
        if (newGamepadSpeed > 0)
        {
            SettingsHandler::setLiveGamepadSpeed(newGamepadSpeed);
            emit actionExecuted(action, "Lower speed "+ QString::number(newGamepadSpeed));
        } else
            emit actionExecuted(action, "Lower speed at minimum");
    }
    if(action == actions.TCodeHomeAll)
    {
        emit tcode_action("DHOME");// TODO: figure out a better way to do this.
        emit actionExecuted(action, "Device home");
    }
    else if(action == actions.IncreaseXLowerRange)
    {
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = SettingsHandler::getLiveXRangeMin() + SettingsHandler::getXRangeStep();
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        if(newLiveRange < xRangeMax - xRangeStep)
        {
            SettingsHandler::setLiveXRangeMin(newLiveRange);
            emit actionExecuted(action, "Raise X min to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMin(xRangeMax - xRangeStep);
            emit actionExecuted(action, "X Min limit reached");
        }
    }
    else if(action == actions.DecreaseXLowerRange)
    {
        int newLiveRange = SettingsHandler::getLiveXRangeMin() - SettingsHandler::getXRangeStep();
        int axisMin = SettingsHandler::getAxis(TCodeChannelLookup::Stroke()).Min;
        if(newLiveRange > axisMin)
        {
            SettingsHandler::setLiveXRangeMin(newLiveRange);
            emit actionExecuted(action, "Lower X min to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMin(axisMin);
            emit actionExecuted(action, "Low X min limit reached");
        }
    }
    else if(action == actions.IncreaseXUpperRange)
    {
        int newLiveRange = SettingsHandler::getLiveXRangeMax() + SettingsHandler::getXRangeStep();
        int axisMax = SettingsHandler::getAxis(TCodeChannelLookup::Stroke()).Max;
        if(newLiveRange < axisMax)
        {
            SettingsHandler::setLiveXRangeMax(newLiveRange);
            emit actionExecuted(action, "Raise X max to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMax(axisMax);
            emit actionExecuted(action, "High X max limit reached");
        }
    }
    else if(action == actions.DecreaseXUpperRange)
    {
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveRange = SettingsHandler::getLiveXRangeMax() - xRangeStep;
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        if(newLiveRange > xRangeMin + xRangeStep)
        {
            SettingsHandler::setLiveXRangeMax(newLiveRange);
            emit actionExecuted(action, "Lower X max to "+ QString::number(newLiveRange));
        }
        else
        {
            SettingsHandler::setLiveXRangeMax(xRangeMin + xRangeStep);
            emit actionExecuted(action, "Low X max limit reached");
        }
    }
    else if (action == actions.IncreaseXRange)
    {
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax + xRangeStep;
        int axisMax = SettingsHandler::getAxis(TCodeChannelLookup::Stroke()).Max;
        bool atMax = false;
        if(newLiveMaxRange > axisMax)
        {
            atMax = true;
            newLiveMaxRange = axisMax;
        }
        SettingsHandler::setLiveXRangeMax(newLiveMaxRange);

        int newLiveMinRange = xRangeMin - xRangeStep;
        int axisMin = SettingsHandler::getAxis(TCodeChannelLookup::Stroke()).Min;
        bool atMin = false;
        if(newLiveMinRange < axisMin)
        {
            atMin = true;
            newLiveMinRange = axisMin;
        }
        SettingsHandler::setLiveXRangeMin(newLiveMinRange);

        if (atMin && atMax)
            emit actionExecuted(action, "Increase X at limit");
        else if (atMax)
            emit actionExecuted(action, "Increase X, max at limit, min"+ QString::number(newLiveMinRange));
        else if (atMin)
            emit actionExecuted(action, "Increase X, max "+ QString::number(newLiveMaxRange) + ", min at limit");
        else
            emit actionExecuted(action, "Increase X, max "+ QString::number(newLiveMaxRange) + ", min "+ QString::number(newLiveMinRange));

    }
    else if (action == actions.DecreaseXRange)
    {
        int xRangeMax = SettingsHandler::getLiveXRangeMax();
        int xRangeMin = SettingsHandler::getLiveXRangeMin();
        int xRangeStep = SettingsHandler::getXRangeStep();
        int newLiveMaxRange = xRangeMax - xRangeStep;
        bool maxLessThanMin = false;
        if(newLiveMaxRange < xRangeMin)
        {
            maxLessThanMin = true;
            newLiveMaxRange = xRangeMin + 1;
        }
        SettingsHandler::setLiveXRangeMax(newLiveMaxRange);

        int newLiveMinRange = xRangeMin + xRangeStep;
        bool minGreaterThanMax = false;
        if(newLiveMinRange > xRangeMax)
        {
            minGreaterThanMax = true;
            newLiveMinRange = xRangeMax - 1;
        }
        SettingsHandler::setLiveXRangeMin(newLiveMinRange);
        if (maxLessThanMin && minGreaterThanMax)
            emit actionExecuted(action, "Decrease X at limit");
        else if (maxLessThanMin)
            emit actionExecuted(action, "Decrease X, max at limit, min "+ QString::number(newLiveMinRange));
        else if (minGreaterThanMax)
            emit actionExecuted(action, "Decrease X, max "+ QString::number(newLiveMaxRange) + ", min at limit");
        else
            emit actionExecuted(action, "Decrease X, max "+ QString::number(newLiveMaxRange) + ", min "+ QString::number(newLiveMinRange));

    }
    else if (action == actions.ResetLiveXRange)
    {
        emit actionExecuted(action, "Resetting X range");
        SettingsHandler::resetLiveXRange();
    }
    else if (action == actions.ToggleAxisMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierEnabled();
        emit actionExecuted(action, multiplier ? "Disable random motion" : "Enable random motion");
        SettingsHandler::setLiveMultiplierEnabled(!multiplier);
    }
    else if (action == actions.ToggleChannelRollMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Roll());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Roll(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable roll motion" : "Enable roll motion");
    }
    else if (action == actions.ToggleChannelPitchMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Pitch());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Pitch(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable pitch motion" : "Enable pitch motion");
    }
    else if (action == actions.ToggleChannelSurgeMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Surge());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Surge(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable surge motion" : "Enable surge motion");
    }
    else if (action == actions.ToggleChannelSwayMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Sway());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Sway(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable sway motion" : "Enable sway motion");
    }
    else if (action == actions.ToggleChannelTwistMultiplier)
    {
        bool multiplier = SettingsHandler::getMultiplierChecked(TCodeChannelLookup::Twist());
        SettingsHandler::setMultiplierChecked(TCodeChannelLookup::Twist(), !multiplier);
        emit actionExecuted(action, multiplier ? "Disable twist motion" : "Enable twist motion");
    }
    else if (action == actions.ToggleFunscriptInvert)
    {
        bool inverted = FunscriptHandler::getInverted();
        emit actionExecuted(action, inverted ? "Funscript normal" : "Funscript inverted");
        FunscriptHandler::setInverted(!inverted);
    }
    else if(action == actions.TogglePauseAllDeviceActions)
    {
        bool paused = SettingsHandler::getLiveActionPaused();
        emit actionExecuted(action, paused ? "Resume action" : "Pause action");
        SettingsHandler::setLiveActionPaused(!paused);
        emit tcode_action("DSTOP");// TODO: figure out a better way to do this.
    }
    else if(action == actions.ToggleSkipToMoneyShotPlaysFunscript) {
        auto enabled = !SettingsHandler::getSkipToMoneyShotPlaysFunscript();
        QString verb = enabled ? "Enable" : "Disable";
        SettingsHandler::setSkipToMoneyShotPlaysFunscript(enabled);
        emit actionExecuted(action, verb + " plays funscript.");
    }
    else if (action == actions.IncreaseFunscriptModifier || action == actions.DecreaseFunscriptModifier || action == actions.ResetFunscriptModifier)
    {
        if(action == actions.ResetFunscriptModifier) {
            FunscriptHandler::resetModifier();
            emit actionExecuted(action, "Reset funscript modifier");
        } else {
            bool increase = action == actions.IncreaseFunscriptModifier;
            QString verb = increase ? "Increase" : "Decrease";
            int modifier = FunscriptHandler::getModifier();
            int modedModifier = increase ? modifier + SettingsHandler::getFunscriptModifierStep() : modifier - SettingsHandler::getFunscriptModifierStep();

            if(modedModifier > 0)
            {
                FunscriptHandler::setModifier(modedModifier);
                emit actionExecuted(action, verb + " funscript modifier to "+ QString::number(modedModifier) + "percent");
            }
            else
                emit actionExecuted(action, "Funscript modifier at minimum "+ QString::number(modedModifier) + "percent");
        }
    }

    //TODO: nneds to be moved from main window some how
//    else if (action == actions.IncreaseOffset || action == actions.DecreaseOffset)
//    {
//        bool increase = action == actions.IncreaseOffset;
//        QString verb = increase ? "Increase" : "Decrease";
//        int currentLiveOffSet = SettingsHandler::getLiveOffSet();
//        int newOffset = increase ? currentLiveOffSet + SettingsHandler::getFunscriptOffsetStep() : currentLiveOffSet - SettingsHandler::getFunscriptOffsetStep();
//        SettingsHandler::setLiveOffset(newOffset);
//        emit actionExecuted(action, verb + " live offset to " + QString::number(newOffset));
//    }
//    else if (action == actions.IncreaseOffset || action == actions.DecreaseOffset)
//    {
//        bool increase = action == actions.IncreaseOffset;
//        QString verb = increase ? "Increase" : "Decrease";
//        if (xtEngine.syncHandler()->isPlaying())
//        {
//           QString path = playingLibraryListItem.path;
//           if(!path.isEmpty())
//           {
//               auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(path);
//               int newOffset = increase ? libraryListItemMetaData.offset + SettingsHandler::getFunscriptOffsetStep() : libraryListItemMetaData.offset - SettingsHandler::getFunscriptOffsetStep();
//               libraryListItemMetaData.offset = newOffset;
//               SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
//               emit actionExecuted(action, verb + " live offset to " + QString::number(newOffset));
//           }
//        }
//        else
//            emit actionExecuted(action, "No script playing to " + verb + " offset.");
//    }
    else
    {
        emit actionExecuted(action, nullptr);
    }
}
