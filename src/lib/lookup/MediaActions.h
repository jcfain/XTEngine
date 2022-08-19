#ifndef MEDIAACTIONS_H
#define MEDIAACTIONS_H

#include <QString>
#include <QMap>
struct MediaActions
{
    const QString TogglePause = "TogglePause";
    const QString Next = "Next";
    const QString Back = "Back";
    const QString FullScreen = "FullScreen";
    const QString VolumeUp = "VolumeUp";
    const QString VolumeDown = "VolumeDown";
    const QString Mute = "Mute";
    const QString Stop = "Stop";
    const QString Loop = "Loop";
    const QString Rewind = "Rewind";
    const QString FastForward = "Fast forward";
    const QString TCodeSpeedUp = "TCodeSpeedUp";
    const QString TCodeSpeedDown = "TCodeSpeedDown";
    const QString TCodeHomeAll = "TCodeHomeAll";
    const QString IncreaseXRange = "IncreaseXRange";
    const QString DecreaseXRange = "DecreaseXRange";
    const QString IncreaseXUpperRange = "IncreaseXUpperRange";
    const QString DecreaseXUpperRange = "DecreaseXUpperRange";
    const QString IncreaseXLowerRange = "IncreaseXLowerRange";
    const QString DecreaseXLowerRange = "DecreaseXLowerRange";
    const QString ResetLiveXRange = "ResetLiveXRange";
    const QString ToggleAxisMultiplier = "ToggleAxisMiltiplier";
    const QString ToggleChannelRollMultiplier = "ToggleChannelRollMultiplier";
    const QString ToggleChannelPitchMultiplier = "ToggleChannelPitchMultiplier";
    const QString ToggleChannelSurgeMultiplier = "ToggleChannelSurgeMultiplier";
    const QString ToggleChannelSwayMultiplier = "ToggleChannelSwayMultiplier";
    const QString ToggleChannelTwistMultiplier = "ToggleChannelTwistMultiplier";
    const QString ToggleFunscriptInvert = "ToggleFunscriptInvert";
    const QString TogglePauseAllDeviceActions = "TogglePauseAllDeviceActions";
    const QString SkipToMoneyShot = "SkipToMoneyShot";
    const QString ToggleSkipToMoneyShotPlaysFunscript = "ToggleSkipToMoneyShotPlaysFunscript";
    const QString SkipToAction = "SkipToAction";
    const QString IncreaseFunscriptModifier = "IncreaseFunscriptModifier";
    const QString DecreaseFunscriptModifier = "DecreaseFunscriptModifier";
    const QString IncreaseOffset = "IncreaseDelay";
    const QString DecreaseOffset = "DecreaseDelay";
    const QMap<QString, QString> Values {
        {TogglePause, "Media: Pause (on/off)"},
        {Next, "Media: Next video"},
        {Back, "Media: Previous video"},
        {FullScreen, "Media: Fullscreen (full/normal)"},
        {VolumeUp, "Media: Volume up"},
        {VolumeDown, "Media: Volume down"},
        {Mute, "Media: Mute (on/off)"},
        {Stop, "Media: Stop video"},
        {Loop, "Media: Toggle loop (A/B/off)"},
        {Rewind, "Media: Rewind"},
        {FastForward, "Media: Fast forward"},
        {TCodeSpeedUp, "Gamepad: speed up"},
        {TCodeSpeedDown, "Gamepad: speed down"},
        {TCodeHomeAll, "TCode: Device home"},
        {IncreaseXRange, "TCode: Stroke range limit (increase)"},
        {DecreaseXRange, "TCode: Stroke range limit (decrease)"},
        {ResetLiveXRange, "TCode: Stroke range limit reset"},
        {IncreaseXUpperRange, "TCode: Stroke upper range limit (increase)"},
        {DecreaseXUpperRange, "TCode: Stroke upper range limit (decrease)"},
        {IncreaseXLowerRange, "TCode: Stroke lower range limit (increase)"},
        {DecreaseXLowerRange, "TCode: Stroke lower range limit (decrease)"},
        {ToggleAxisMultiplier, "TCode: Random motion all (on/off)"},
        {ToggleChannelRollMultiplier, "TCode: Random motion roll (on/off)"},
        {ToggleChannelPitchMultiplier, "TCode: Random motion pitch (on/off)"},
        {ToggleChannelSurgeMultiplier, "TCode: Random motion surge (on/off)"},
        {ToggleChannelSwayMultiplier, "TCode: Random motion sway (on/off)"},
        {ToggleChannelTwistMultiplier, "TCode: Random motion twist (on/off)"},
        {TogglePauseAllDeviceActions, "TCode: Device actions (on/off)"},
        {SkipToMoneyShot, "Skip to moneyshot"},
        {SkipToAction, "Skip to next action"},
        {ToggleSkipToMoneyShotPlaysFunscript, "ToggleSkipToMoneyShotPlaysFunscript"},
        {ToggleFunscriptInvert, "Funscript: inversion (on/off)"},
        {IncreaseFunscriptModifier, "Funscript: Range modifier (increase)"},
        {DecreaseFunscriptModifier, "Funscript: Range modifier (decrease)"},
        {IncreaseOffset, "Funscript: delay (increase)"},
        {DecreaseOffset, "Funscript: delay (decrease)"},
        {DecreaseOffset, "Funscript: delay (decrease)"},
    };
};
#endif // MEDIAACTIONS_H
