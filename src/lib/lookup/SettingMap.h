#ifndef SETTINGMAP_H
#define SETTINGMAP_H

#include <QString>
#include <QVariant>
#include <QTime>
#include <QJsonObject>

/**
 * @brief The SettingProfile enum
 * IMPORTANT! Any added profiles need to be added to the web javascript settings.js as well.
 */
enum class SettingProfile {
    System
};
enum class FormControlTypes {
    Double,
    Int,
    Text,
    Radio,
    Combo,
    Checkbox,
    DateTime,
    Date,
    Time,
    Long
};

struct SettingMap {
    SettingProfile profile;
    QString group;
    QString key;
    FormControlTypes type;
    QVariant defaultValue;
    QString label;
    QString description;
    bool internal;
    bool requiresRestart;
    QJsonObject tojson() {
        QJsonObject obj;
        obj["profile"] = (int)profile;
        obj["group"] = group;
        obj["key"] = key;
        obj["type"] = (int)type;
        obj["defaultValue"] = defaultValue.toJsonValue();
        obj["label"] = label;
        obj["description"] = description;
        obj["internal"] = internal;
        obj["requiresRestart"] = requiresRestart;
        return obj;
    }
};

/**
 * @brief The SettingGroups class
 */
struct SettingGroups {
    static inline const QString schedule = "schedule";
    static inline const QString metadata = "metadata";
    static inline const QString tcode = "tcode";
    static inline const QString serial = "serial";
    static inline const QString web = "web";
    static inline const QString media = "media";

};
struct SettingKeys {
    static inline const QString scheduleLibraryLoadEnabled = "scheduleLibraryLoadEnabled";
    static inline const QString scheduleLibraryLoadTime = "scheduleLibraryLoadTime";
    static inline const QString scheduleLibraryLoadFullProcess = "scheduleLibraryLoadFullProcess";
    static inline const QString processMetadataOnStart = "processMetadataOnStart";
    static inline const QString scheduleSettingsSync = "scheduleSettingsSync";
    static inline const QString forceMetaDataFullProcess = "forceMetaDataFullProcess";
    static inline const QString disableUDPHeartBeat = "disableUDPHeartBeat";
    static inline const QString disableTCodeValidation = "disableTCodeValidation";
    static inline const QString useDTRAndRTS = "useDTRAndRTS";
    static inline const QString httpChunkSizeMB = "httpChunkSizeMB";
    static inline const QString playbackRateStep = "playbackRateStep";
    static inline const QString disableAutoThumbGeneration = "disableAutoThumbGeneration";
    static inline const QString enableMediaManagement = "enableMediaManagement";
    static inline const QString useSystemMediaBackend = "useSystemMediaBackend";
    static inline const QString globalOffset = "globalOffset";
    static inline const QString globalOffsetWeb = "globalOffsetWeb";
    static inline const QString viewedThreshold = "viewedThreshold";
    // static inline const QString selectedTCodeVersion = "selectedTCodeVersion";
    // static inline const QString selectedChannelProfile = "selectedChannelProfile";
    // static inline const QString selectedThumbsDir = "selectedThumbsDir";
    // static inline const QString useMediaDirForThumbs = "useMediaDirForThumbs";
    // static inline const QString selectedDevice = "selectedDevice";
    // static inline const QString selectedNetworkDeviceType = "selectedNetworkDeviceType";
    // static inline const QString serialPort = "serialPort";
    // static inline const QString serverAddress = "serverAddress";
    // static inline const QString serverPort = "serverPort";
    // static inline const QString deoAddress = "deoAddress";
    // static inline const QString deoPort = "deoPort";
    // static inline const QString deoEnabled = "deoEnabled";
    // static inline const QString whirligigAddress = "whirligigAddress";
    // static inline const QString whirligigPort = "whirligigPort";
    // static inline const QString whirligigEnabled = "whirligigEnabled";
    // static inline const QString xtpWebSyncEnabled = "xtpWebSyncEnabled";
    // static inline const QString libraryView = "libraryView";
    // static inline const QString selectedLibrarySortMode = "selectedLibrarySortMode";
    // static inline const QString thumbSize = "thumbSize";
    // static inline const QString thumbSizeList = "thumbSizeList";
    // static inline const QString videoIncrement = "videoIncrement";
    // static inline const QString deoDnlaFunscriptLookup = "deoDnlaFunscriptLookup";
    // static inline const QString gamePadEnabled = "gamePadEnabled";
    // static inline const QString multiplierEnabled = "multiplierEnabled";
    // static inline const QString decoderPriority = "decoderPriority";// Array QtAV only?
    // static inline const QString selectedVideoRenderer = "selectedVideoRenderer";// QtAV only?
    // static inline const QString gamepadSpeed = "gamepadSpeed";
    // static inline const QString gamepadSpeedStep = "gamepadSpeedStep";
    // static inline const QString strokeRangeStep = "strokeRangeStep"; // rename xRangeStep
    // static inline const QString disableSpeechToText = "disableSpeechToText";
    // static inline const QString disableVRScriptSelect = "disableVRScriptSelect";
    // static inline const QString disableNoScriptFound = "disableNoScriptFound";
    // static inline const QString userData = "userData";
    // static inline const QString userWebData = "userWebData";
    // static inline const QString skipToMoneyShotPlaysFunscript = "skipToMoneyShotPlaysFunscript";
    // static inline const QString skipToMoneyShotFunscript = "skipToMoneyShotFunscript";
    // static inline const QString skipToMoneyShotSkipsVideo = "skipToMoneyShotSkipsVideo";
    // static inline const QString skipToMoneyShotStandAloneLoop = "skipToMoneyShotStandAloneLoop";
    // static inline const QString hideStandAloneFunscriptsInLibrary = "hideStandAloneFunscriptsInLibrary";
    // static inline const QString showVRInLibraryView = "showVRInLibraryView";
    // static inline const QString skipPlayingSTandAloneFunscriptsInLibrary = "skipPlayingSTandAloneFunscriptsInLibrary";
    // static inline const QString enableHttpServer = "enableHttpServer";
    // static inline const QString httpServerRoot = "httpServerRoot";
    // static inline const QString httpPort = "httpPort";
    // static inline const QString webSocketPort = "webSocketPort";
    // static inline const QString httpThumbQuality = "httpThumbQuality";
    // static inline const QString funscriptModifierStep = "funscriptModifierStep";
    // static inline const QString funscriptOffsetStep = "funscriptOffsetStep";
    // static inline const QString channelPulseAmount = "channelPulseAmount";
    // static inline const QString channelPulseEnabled = "channelPulseEnabled";
    // static inline const QString channelPulseFrequency = "channelPulseFrequency";
    // static inline const QString customTCodeCommands = "customTCodeCommands"; // Array
    // static inline const QString tags = "tags"; // Array
    // static inline const QString smartTags = "smartTags"; // Array
    // static inline const QString playlists = "playlists"; // Not trivial
    // static inline const QString tcodeCommandMap = "tcodeCommandMap"; // Not trivial
    // static inline const QString gamepadButtonMap = "gamepadButtonMap";// Not trivial

    // XTPlayer only /////////////////////
    // static inline const QString playerVolume = "playerVolume";
    // static inline const QString hideWelcomeScreen = "hideWelcomeScreen";
    // static inline const QString keyboardKeyMap = "keyboardKeyMap";
};

class XSettingsMap {
public:
    static void init() {
        foreach (auto setting, SettingsList) {
            SettingsMap.insert(setting.key, setting);
            // QMap<QString, SettingMap> settingKey;
            // settingKey.insert(setting.key, setting);
            // QMap<QString, QMap<QString, SettingMap>> settingGroup;
            // settingGroup.insert(setting.group, settingKey);
            // SettingsGroupMap.insert(setting.profile, settingGroup);
        }
    }
    const static inline QList<SettingMap> SettingsList =
    {
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadEnabled, FormControlTypes::Checkbox, false, "Enabled", "This will refresh the libraries at the specified time.", false, false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadTime, FormControlTypes::Time, QTime(2,0), "At time", "What time to load the libraries.", false, false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadFullProcess, FormControlTypes::Checkbox, false, "Full metadata process", "Run a full metadata process after the library has been loaded per the schedule", false, false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleSettingsSync, FormControlTypes::Checkbox, true, "Sync settings", "Flush the settings to disk.", false, false},
        {SettingProfile::System, SettingGroups::metadata, SettingKeys::processMetadataOnStart, FormControlTypes::Checkbox, false, "Process metadata on start", "Process the metadata when the application starts up.", false, false},
        {SettingProfile::System, SettingGroups::metadata, SettingKeys::forceMetaDataFullProcess, FormControlTypes::Checkbox, false, "Force metadata process after next restart", "Force the metadata process on next start.", true, false},
        {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableUDPHeartBeat, FormControlTypes::Checkbox, true, "Disable UDP heartbeat", "Disable periodic UDP ping.", false, false},
        {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableTCodeValidation, FormControlTypes::Checkbox, true, "Disable TCode validation", "Disable the D1 validation when connecting to a tcode device.", false, false},
        {SettingProfile::System, SettingGroups::serial, SettingKeys::useDTRAndRTS, FormControlTypes::Checkbox, false, "Disable serial RTS/DTS", "Disable serial RTS/DTS when connecting via serial.", false, false},
        {SettingProfile::System, SettingGroups::web, SettingKeys::httpChunkSizeMB, FormControlTypes::Double, 26.214400, "Media streaming chunk size", "The chunk sise the web browser should ask for when steaming media.", true, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::playbackRateStep, FormControlTypes::Double, 0.01, "Playback rate step", "The amount to change the playback rate by when using gamepad or input scroller.", false, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::disableAutoThumbGeneration, FormControlTypes::Checkbox, false, "Disable automatic thumb generation", "If checked, when a new media item has been found, no thumb will be automattically generated. Manual generation will still be attemped..", false, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::enableMediaManagement, FormControlTypes::Checkbox, false, "Enable media management", "If checked, certain media management options like delete will become available", false, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::useSystemMediaBackend, FormControlTypes::Checkbox, false, "Use the media backend of the OS", "If checked, XTP will use the OS media backend. Requires restart...", false, true},
        {SettingProfile::System, SettingGroups::media, SettingKeys::globalOffset, FormControlTypes::Int, 0, "Global offset", "Offest specified in milliseconds", false, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::globalOffsetWeb, FormControlTypes::Int, 0, "Global offset (web)", "Offset for scripts played from web ONLY. Offest specified in milliseconds", false, false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::viewedThreshold, FormControlTypes::Int, 90, "Auto mark viewed %", "When the current playing media reaches the percentage of time specified here\nThe metadata will automattically have the viewed tag appended and the unviewed tag removed.", false, false},


    };
    static inline QHash<QString, SettingMap> SettingsMap;
    // static inline QMap<SettingProfile, QMap<QString, QMap<QString, SettingMap>>> SettingsGroupMap;
};

#endif // SETTINGMAP_H
