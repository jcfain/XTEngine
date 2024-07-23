#ifndef SETTINGMAP_H
#define SETTINGMAP_H

#include <QString>
#include <QVariant>
#include <QTime>

struct SettingMap {
    QString key;
    QVariant value;
};

struct SettingKeys {
    static inline const QString scheduleLibraryLoadEnabled = "scheduleLibraryLoadEnabled";
    static inline const QString scheduleLibraryLoadTime = "scheduleLibraryLoadTime";
    static inline const QString scheduleLibraryLoadFullProcess = "scheduleLibraryLoadFullProcess";
};

class XSettingsMap {
    const static inline QHash<QString, SettingMap> settingsMap = {
        { SettingKeys::scheduleLibraryLoadEnabled, {SettingKeys::scheduleLibraryLoadEnabled, false} },
        { SettingKeys::scheduleLibraryLoadTime, {SettingKeys::scheduleLibraryLoadTime, QTime(2,0)} },
        { SettingKeys::scheduleLibraryLoadFullProcess, {SettingKeys::scheduleLibraryLoadFullProcess, false} },
    };
};

#endif // SETTINGMAP_H
