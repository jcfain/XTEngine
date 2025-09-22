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
    Time
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
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadEnabled, FormControlTypes::Checkbox, false, "Enabled", "This will refresh the libraries at the specified time.", false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadTime, FormControlTypes::Time, QTime(2,0), "At time", "What time to load the libraries.", false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadFullProcess, FormControlTypes::Checkbox, false, "Full metadata process", "Run a full metadata process after the library has been loaded per the schedule", false},
        {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleSettingsSync, FormControlTypes::Checkbox, true, "Sync settings", "Flush the settings to disk.", false},
        {SettingProfile::System, SettingGroups::metadata, SettingKeys::processMetadataOnStart, FormControlTypes::Checkbox, false, "Process metadata on start", "Process the metadata when the application starts up.", false},
        {SettingProfile::System, SettingGroups::metadata, SettingKeys::forceMetaDataFullProcess, FormControlTypes::Checkbox, false, "Force metadata process after next restart", "Force the metadata process on next start.", true},
        {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableUDPHeartBeat, FormControlTypes::Checkbox, true, "Disable UDP heartbeat", "Disable periodic UDP ping.", false},
        {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableTCodeValidation, FormControlTypes::Checkbox, false, "Disable TCode validation", "Disable the D1 validation when connecting to a tcode device.", false},
        {SettingProfile::System, SettingGroups::serial, SettingKeys::useDTRAndRTS, FormControlTypes::Checkbox, false, "Disable serial RTS/DTS", "Disable serial RTS/DTS when connecting via serial.", false},
        {SettingProfile::System, SettingGroups::web, SettingKeys::httpChunkSizeMB, FormControlTypes::Double, 26.214400, "Media streaming chunk size", "The chunk sise the web browser should ask for when steaming media.", true},
        {SettingProfile::System, SettingGroups::media, SettingKeys::playbackRateStep, FormControlTypes::Double, 0.01, "Playback rate step", "The amount to change the playback rate by when using gamepad or input scroller.", false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::disableAutoThumbGeneration, FormControlTypes::Checkbox, false, "Disable automatic thumb generation", "If checked, when a new media item has been found, no thumb will be automattically generated. Manual generation will still be attemped..", false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::enableMediaManagement, FormControlTypes::Checkbox, false, "Enable media management", "If checked, certain media management options like delete will become available", false},
        {SettingProfile::System, SettingGroups::media, SettingKeys::useSystemMediaBackend, FormControlTypes::Checkbox, false, "Use the media backend of the OS", "If checked, XTP will use the OS media backend. Requires restart...", false, true},

    };
    static inline QHash<QString, SettingMap> SettingsMap;
    // static inline QMap<SettingProfile, QMap<QString, QMap<QString, SettingMap>>> SettingsGroupMap;
};

#endif // SETTINGMAP_H
