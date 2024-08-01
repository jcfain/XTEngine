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
};
struct SettingKeys {
    static inline const QString scheduleLibraryLoadEnabled = "scheduleLibraryLoadEnabled";
    static inline const QString scheduleLibraryLoadTime = "scheduleLibraryLoadTime";
    static inline const QString scheduleLibraryLoadFullProcess = "scheduleLibraryLoadFullProcess";
    static inline const QString processMetadataOnStart = "processMetadataOnStart";
    static inline const QString scheduleSettingsSync = "scheduleSettingsSync";
    static inline const QString forceMetaDataFullProcess = "forceMetaDataFullProcess";

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
            { SettingKeys::forceMetaDataFullProcess, {SettingProfile::System, SettingGroups::metadata, SettingKeys::forceMetaDataFullProcess, false} }
        },
    };
    const static inline QHash<QString, QHash<QString, SettingMap>> SettingsGroupMap =
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
    };
};

#endif // SETTINGMAP_H
