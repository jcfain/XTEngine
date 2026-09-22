#ifndef MEDIAACTIONS_H
#define MEDIAACTIONS_H
#include <QObject>
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
        {TogglePause, QObject::tr("Media: Pause (on/off)")},
        {Next, QObject::tr("Media: Next video")},
        {Back, QObject::tr("Media: Previous video")},
        {FullScreen, QObject::tr("Media: Fullscreen (full/normal)")},
        {VolumeUp, QObject::tr("Media: Volume up")},
        {VolumeDown, QObject::tr("Media: Volume down")},
        {Mute, QObject::tr("Media: Mute (on/off)")},
        {Stop, QObject::tr("Media: Stop video")},
        {Loop, QObject::tr("Media: Toggle loop (A/B/off)")},
        {Rewind, QObject::tr("Media: Rewind")},
        {FastForward, QObject::tr("Media: Fast forward")},
        {IncreasePlaybackRate, QObject::tr("Media: Playback rate (increase)")},
        {DecreasePlaybackRate, QObject::tr("Media: Playback rate (decrease)")},
        {ResetPlaybackRate, QObject::tr("Media: Playback rate reset to normal (1.0)")},
        {TCodeSpeedUp, QObject::tr("Gamepad: speed up")},
        {TCodeSpeedDown, QObject::tr("Gamepad: speed down")},
        {TCodeHomeAll, QObject::tr("TCode: Device home")},
        {IncreaseXRange, QObject::tr("TCode: Stroke range limit (increase)")},
        {DecreaseXRange, QObject::tr("TCode: Stroke range limit (decrease)")},
        {ResetLiveXRange, QObject::tr("TCode: Stroke range limit reset")},
        {IncreaseXUpperRange, QObject::tr("TCode: Stroke upper range limit (increase)")},
        {DecreaseXUpperRange, QObject::tr("TCode: Stroke upper range limit (decrease)")},
        {IncreaseXLowerRange, QObject::tr("TCode: Stroke lower range limit (increase)")},
        {DecreaseXLowerRange, QObject::tr("TCode: Stroke lower range limit (decrease)")},
        {ToggleAxisMultiplier, QObject::tr("TCode: Random motion all (on/off)")},
        {ToggleChannelRollMultiplier, QObject::tr("TCode: Random motion roll (on/off)")},
        {ToggleChannelPitchMultiplier, QObject::tr("TCode: Random motion pitch (on/off)")},
        {ToggleChannelSurgeMultiplier, QObject::tr("TCode: Random motion surge (on/off)")},
        {ToggleChannelSwayMultiplier, QObject::tr("TCode: Random motion sway (on/off)")},
        {ToggleChannelTwistMultiplier, QObject::tr("TCode: Random motion twist (on/off)")},
        {TogglePauseAllDeviceActions, QObject::tr("TCode: Device actions (on/off)")},
        {SkipToMoneyShot, QObject::tr("Skip to moneyshot")},
        {SkipToAction, QObject::tr("Skip to next action")},
        {ToggleSkipToMoneyShotPlaysFunscript, QObject::tr("ToggleSkipToMoneyShotPlaysFunscript")},
        {ToggleFunscriptInvert, QObject::tr("Funscript: inversion (on/off)")},
        {ResetFunscriptModifier, QObject::tr("Funscript: Range modifier (reset)")},
        {IncreaseFunscriptModifier, QObject::tr("Funscript: Range modifier (increase)")},
        {DecreaseFunscriptModifier, QObject::tr("Funscript: Range modifier (decrease)")},
        {IncreaseOffset, QObject::tr("Funscript: delay (increase)")},
        {DecreaseOffset, QObject::tr("Funscript: delay (decrease)")},
        {ResetOffset, QObject::tr("Funscript: delay reset to 0")},
        {AltFunscriptNext, QObject::tr("Funscript: Next alternative ")},
        {AltFunscriptPrev, QObject::tr("Funscript: Previous alternative")},
        {ChannelProfileNext, QObject::tr("Channel profile next")},
        {ChannelProfilePrevious, QObject::tr("Channel profile previous")}
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
