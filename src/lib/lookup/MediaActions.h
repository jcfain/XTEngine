#ifndef MEDIAACTIONS_H
#define MEDIAACTIONS_H

#include <QString>
#include <QMap>
#include "XTEngine_global.h"
enum class ActionType {
    NONE,
    CHANNEL_PROFILE,
    TCODE
};

class XTENGINE_EXPORT MediaActions
{
public:
    MediaActions();
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
    const QString ChannelProfileNext = "ChannelProfileNext";
    const QString ChannelProfilePrevious = "ChannelProfilePrevious";
    const QString SkipToMoneyShot = "SkipToMoneyShot";
    const QString ToggleSkipToMoneyShotPlaysFunscript = "ToggleSkipToMoneyShotPlaysFunscript";
    const QString SkipToAction = "SkipToAction";
    const QString IncreaseFunscriptModifier = "IncreaseFunscriptModifier";
    const QString DecreaseFunscriptModifier = "DecreaseFunscriptModifier";
    const QString ResetFunscriptModifier = "ResetFunscriptModifier";
    const QString IncreaseOffset = "IncreaseDelay";
    const QString DecreaseOffset = "DecreaseDelay";
    const QString ResetOffset = "ResetOffset";
    const QString IncreasePlaybackRate = "IncreasePlaybackRate";
    const QString DecreasePlaybackRate = "DecreasePlaybackRate";
    const QString ResetPlaybackRate = "ResetPlaybackRate";
    const QString AltFunscriptNext = "AltFunscriptNext";
    const QString AltFunscriptPrev = "AltFunscriptPrev";
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
        {IncreasePlaybackRate, "Media: Playback rate (increase)"},
        {DecreasePlaybackRate, "Media: Playback rate (decrease)"},
        {ResetPlaybackRate, "Media: Playback rate reset to normal (1.0)"},
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
        {ResetFunscriptModifier, "Funscript: Range modifier (reset)"},
        {IncreaseFunscriptModifier, "Funscript: Range modifier (increase)"},
        {DecreaseFunscriptModifier, "Funscript: Range modifier (decrease)"},
        {IncreaseOffset, "Funscript: delay (increase)"},
        {DecreaseOffset, "Funscript: delay (decrease)"},
        {ResetOffset, "Funscript: delay reset to 0"},
        {AltFunscriptNext, "Funscript: Next alternative "},
        {AltFunscriptPrev, "Funscript: Previous alternative"},
        {ChannelProfileNext, "Channel profile next"},
        {ChannelProfilePrevious, "Channel profile previous"}
    };

    static bool HasOtherAction(QString action, ActionType type = ActionType::NONE);
    static QMap<QString, QString> GetOtherActions(ActionType type = ActionType::NONE);
    static void AddOtherAction(QString action, QString friendlyName, ActionType type);
    static void RemoveOtherAction(QString action, ActionType type);
    static void EditOtherAction(QString action, QString newAction, QString newFriendlyName, ActionType type);
private:
    static QMap<QString, QString> m_tCodeChannelProfileActions;
    static QMap<QString, QString> m_tCodeActions;
    static QMap<QString, QString> m_allOtherActions;
};
#endif // MEDIAACTIONS_H
