#ifndef MEDIALIBRARYSETTINGS_H
#define MEDIALIBRARYSETTINGS_H

#include <QMutex>
#include "settingsbase.h"
#include "../lookup/enum.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT MediaLibrarySettings : public SettingsBase
{
    Q_OBJECT
public:
    MediaLibrarySettings(QObject *parent = nullptr);
    void Load(QSettings* settingsToLoadFrom) override;
    void Save(QSettings* settingsToSaveTo) override;

    void set(const LibraryType& type, const QStringList& value);
    QStringList get(const LibraryType& type);
    QString getLast(const LibraryType& type);
    bool add(const LibraryType& type, const QString& value, QStringList &messages);
    void add(const LibraryType& type, const QString& value);
    void remove(const LibraryType& type, const QString& value);
    void remove(const LibraryType& type, const QList<int>& indexes);
    void clear(const LibraryType& type);

signals:

private:
    QMutex m_mainLibraryMutex;
    QMutex m_vrLibraryMutex;
    QMutex m_funscriptLibraryMutex;
    QMutex m_exclusionMutex;
    QStringList m_libraryMain;
    QStringList m_libraryVR;
    QStringList m_libraryFunscript;
    QStringList m_libraryExclusions;

    QStringList* getLibraryPaths(const LibraryType& type);
    QMutex* getLibraryMutex(const LibraryType& type);
    bool isLibraryParentChildOrEqual(const LibraryType& type, const QString& value, const QString& name, QStringList& errorMessages);
    bool isLibraryChildOrEqual(const LibraryType& type, const QString& value, const QString& name, QStringList& errorMessages);
    bool mediaContains(const QString& value, QStringList& errorMessages);
    bool exclusionContains(const QString& value, QStringList& errorMessages);
    // bool contains(const LibraryType& type, const QString& value, QStringList& errorMessages);
};

#endif // MEDIALIBRARYSETTINGS_H
