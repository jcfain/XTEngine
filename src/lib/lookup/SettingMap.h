#ifndef SETTINGMAP_H
#define SETTINGMAP_H

#include <QString>
#include <QVariant>
#include <QTime>

enum class SettingProfile {
    System
};

struct SettingMap {
    SettingProfile profile;
    QString group;
    QString key;
    QVariant defaultValue;
};

struct SettingGroups {
    static inline const QString schedule = "schedule";
    static inline const QString metadata = "metadata";
    static inline const QString tcode = "tcode";
    static inline const QString serial = "serial";
    static inline const QString web = "web";
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

};

class XSettingsMap {
public:
    const static inline QHash<QString, SettingMap> SettingsMap =
    {
        {
            { SettingKeys::scheduleLibraryLoadEnabled, {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadEnabled, false} },
            { SettingKeys::scheduleLibraryLoadTime, {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadTime, QTime(2,0)} },
            { SettingKeys::scheduleLibraryLoadFullProcess, {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleLibraryLoadFullProcess, false} },
            { SettingKeys::scheduleSettingsSync, {SettingProfile::System, SettingGroups::schedule, SettingKeys::scheduleSettingsSync, true} },
            { SettingKeys::processMetadataOnStart, {SettingProfile::System, SettingGroups::metadata, SettingKeys::processMetadataOnStart, false} },
            { SettingKeys::forceMetaDataFullProcess, {SettingProfile::System, SettingGroups::metadata, SettingKeys::forceMetaDataFullProcess, false} },
            { SettingKeys::disableUDPHeartBeat, {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableUDPHeartBeat, false} },
            { SettingKeys::disableTCodeValidation, {SettingProfile::System, SettingGroups::tcode, SettingKeys::disableTCodeValidation, false} },
            { SettingKeys::useDTRAndRTS, {SettingProfile::System, SettingGroups::serial, SettingKeys::useDTRAndRTS, false} },
            { SettingKeys::httpChunkSizeMB, {SettingProfile::System, SettingGroups::web, SettingKeys::httpChunkSizeMB, 26.214400} }
        },
    };
    const static inline QMap<SettingProfile, QMap<QString, QMap<QString, SettingMap>>> SettingsGroupMap =
    {
        {
            SettingProfile::System,
            {
                {
                    SettingGroups::schedule,
                    {
                        { SettingKeys::scheduleLibraryLoadEnabled, SettingsMap[SettingKeys::scheduleLibraryLoadEnabled] },
                        { SettingKeys::scheduleLibraryLoadTime, SettingsMap[SettingKeys::scheduleLibraryLoadTime] },
                        { SettingKeys::scheduleLibraryLoadFullProcess, SettingsMap[SettingKeys::scheduleLibraryLoadFullProcess] },
                        { SettingKeys::scheduleSettingsSync, SettingsMap[SettingKeys::scheduleSettingsSync] }
                    }
                },
                {
                    SettingGroups::metadata,
                    {
                        { SettingKeys::processMetadataOnStart, SettingsMap[SettingKeys::processMetadataOnStart] },
                        { SettingKeys::forceMetaDataFullProcess, SettingsMap[SettingKeys::forceMetaDataFullProcess] }
                    }
                },
                {
                    SettingGroups::tcode,
                    {
                        { SettingKeys::disableUDPHeartBeat, SettingsMap[SettingKeys::disableUDPHeartBeat] },
                        { SettingKeys::disableTCodeValidation, SettingsMap[SettingKeys::disableTCodeValidation] }
                    }
                },
                {
                    SettingGroups::serial,
                    {
                      { SettingKeys::useDTRAndRTS, SettingsMap[SettingKeys::useDTRAndRTS] }
                    }
                },
                {
                    SettingGroups::web,
                    {
                      { SettingKeys::httpChunkSizeMB, SettingsMap[SettingKeys::httpChunkSizeMB] }
                    }
                }
            }
        }
    };
};

#endif // SETTINGMAP_H
