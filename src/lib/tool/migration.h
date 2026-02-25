#ifndef MIGRATION_H
#define MIGRATION_H
#include <QSettings>
#include "../struct/LibraryListItemMetaData258.h"

class Migration
{
public:
    static void MigrateTo42(QSettings* settingsToLoadFrom);
    static void MigrateTo46(QSettings* settingsToLoadFrom, QHash<QString, LibraryListItemMetaData258> &libraryListItemMetaDatas);
    static void MigrateTo52(QSettings* settingsToLoadFrom);
    static void MigrateTo592(QSettings* settingsToLoadFrom);
    static void RenameChannelDamperToSpeed(QSettings* settingsToLoadFrom);
};

#endif // MIGRATION_H
