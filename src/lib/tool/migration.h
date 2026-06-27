#ifndef MIGRATION_H
#define MIGRATION_H
#include <QSettings>
#include "../struct/LibraryListItemMetaData258.h"
#include "../struct/TCodeCommand.h"
#include "../lookup/xtags.h"

class Migration
{
public:
    static void MigrateTo42(QSettings* settingsToLoadFrom);
    static void MigrateTo46(QSettings* settingsToLoadFrom, QHash<QString, LibraryListItemMetaData258> &libraryListItemMetaDatas);
    static void MigrateTo52(QSettings* settingsToLoadFrom);
    static void MigrateTo592(QSettings* settingsToLoadFrom);
    static void MigrateTo595(QSettings* settingsToLoadFrom, QList<TCodeCommand>& list);
    static void MigrateTo61(QSettings* settingsToLoadFromm, int& major, int& minor, int& rev, QString& phase);
    static void MigrateTo62(QSettings* settingsToLoadFromm);
    static void MigrateTo63(QSettings* settingsToLoadFromm, XTags& xtags);

    static void RenameChannelDamperToSpeed(QSettings* settingsToLoadFrom);
};

#endif // MIGRATION_H
