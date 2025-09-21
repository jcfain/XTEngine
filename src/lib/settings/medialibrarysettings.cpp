#include "medialibrarysettings.h"

#include <QCoreApplication>
#include "../tool/file-util.h"

MediaLibrarySettings::MediaLibrarySettings(QObject *parent)
    : SettingsBase(parent)
{}

void MediaLibrarySettings::Load(QSettings *settings)
{
    m_libraryMain = settings->value("selectedLibrary").toStringList();
    m_libraryFunscript = settings->value("selectedFunscriptLibrary").toStringList();
    m_libraryVR = settings->value("vrLibrary").toStringList();
    m_libraryExclusions = settings->value("libraryExclusions").toStringList();
}

void MediaLibrarySettings::Save(QSettings *settings)
{
    settings->setValue("selectedLibrary", m_libraryMain);
    settings->setValue("selectedFunscriptLibrary", m_libraryFunscript);
    settings->setValue("vrLibrary", m_libraryVR);
    settings->setValue("libraryExclusions", m_libraryExclusions);
    // settingsToSaveTo->setValue("libraryExclusions", QVariant::fromValue(_libraryExclusions));// TODO does this mess up existing data?
}

void MediaLibrarySettings::set(const LibraryType& type, const QStringList &value)
{
    auto library = getLibraryPaths(type);
    library = value;
    emit settingsChangedEvent(true);
}

QStringList MediaLibrarySettings::get(const LibraryType& type)
{
    QMutexLocker locker(getLibraryMutex(type));
    return getLibraryPaths(type);
}

QString  MediaLibrarySettings::getLast(const LibraryType& type)
{
    QMutexLocker locker(getLibraryMutex(type));
    auto library = getLibraryPaths(type);
    library.removeAll("");
    if(!library.isEmpty())
        return library.last();
    return QCoreApplication::applicationDirPath();
}

bool MediaLibrarySettings::add(const LibraryType& type, const QString& value, QStringList &errorMessages)
{
    QMutexLocker locker(getLibraryMutex(type));
    auto library = getLibraryPaths(type);
    if(contains(value, errorMessages))
        return false;
    if(!value.isEmpty() && !library.contains(value))
        library << value;
    library.removeDuplicates();
    emit settingsChangedEvent(true);
    return true;
}

void MediaLibrarySettings::add(const LibraryType &type, const QString &value)
{
    QStringList messages;
    add(type, value, messages);
}

void MediaLibrarySettings::remove(const LibraryType& type, const QString& value)
{
    QMutexLocker locker(getLibraryMutex(type));
    getLibraryPaths(type).removeOne(value);
    emit settingsChangedEvent(true);
}

void MediaLibrarySettings::remove(const LibraryType& type, const QList<int>& indexes)
{
    foreach(auto index, indexes)
        getLibraryPaths(type).removeAt(index);
    emit settingsChangedEvent(true);
}

void MediaLibrarySettings::clear(const LibraryType& type)
{
    QMutexLocker locker(getLibraryMutex(type));
    getLibraryPaths(type).clear();
    emit settingsChangedEvent(true);
}


// private
QMutex* MediaLibrarySettings::getLibraryMutex(const LibraryType &type)
{
    switch(type)
    {
    case LibraryType::MAIN:
        return &m_mainLibraryMutex;
    case LibraryType::FUNSCRIPT:
        return &m_funscriptLibraryMutex;
    case LibraryType::VR:
        return &m_vrLibraryMutex;
    default:
        return &m_exclusionMutex;
    }
}

QStringList &MediaLibrarySettings::getLibraryPaths(const LibraryType &type)
{
    switch(type)
    {
    case LibraryType::MAIN:
        return m_libraryMain;
    case LibraryType::FUNSCRIPT:
        return m_libraryFunscript;
    case LibraryType::VR:
        return m_libraryVR;
    default:
        return m_libraryExclusions;
    }
}

bool MediaLibrarySettings::isLibraryParentChildOrEqual(const LibraryType& type, const QString& value, const QString& name, QStringList& errorMessages)
{
    QMutexLocker locker(getLibraryMutex(type));
    return XFileUtil::isDirParentChildOrEqual(getLibraryPaths(type), value, name, errorMessages);
}

bool MediaLibrarySettings::isLibraryChildOrEqual(const LibraryType& type, const QString &value, const QString& name, QStringList& errorMessages)
{
    QMutexLocker locker(getLibraryMutex(type));
    return XFileUtil::isDirChildOrEqual(getLibraryPaths(type), value, name, errorMessages);
}

bool MediaLibrarySettings::contains(const QString &value, QStringList& errorMessages)
{
    isLibraryParentChildOrEqual(LibraryType::MAIN, value, "main media library", errorMessages);
    isLibraryParentChildOrEqual(LibraryType::VR, value, "VR media library", errorMessages);
    isLibraryParentChildOrEqual(LibraryType::FUNSCRIPT, value, "funscript library", errorMessages);
    isLibraryParentChildOrEqual(LibraryType::EXCLUSION, "library exclusions", value, errorMessages);
    return !errorMessages.isEmpty();
}
// QStringList SettingsHandler::getVRLibrary()
// {
//     return _vrLibrary;
// }

// QString SettingsHandler::getLastSelectedVRLibrary() {
//     _vrLibrary.removeAll("");
//     if(!_vrLibrary.isEmpty())
//         return _vrLibrary.last();
//     //return QCoreApplication::applicationDirPath();
//     return "";
//     // Used for selecting new main libraries but not used with
//     // VR libraries as only one is possible at this time.
//     // If this changes need to add a new method or something for default VR library
// }
// bool SettingsHandler::addSelectedVRLibrary(QString value, QStringList &messages)
// {
//     QMutexLocker locker(&mutex);
//     if(hasAnyLibrary(value, messages))
//         return false;
//     _vrLibrary.clear();
//     if(!value.isEmpty())
//         _vrLibrary << value;
//     settingsChangedEvent(true);
//     return true;
// }
// void SettingsHandler::removeSelectedVRLibrary(QString value)
// {
//     QMutexLocker locker(&mutex);
//     _vrLibrary.clear();
//     _vrLibrary.removeOne(value);
//     settingsChangedEvent(true);
// }

// void SettingsHandler::removeAllVRLibraries()
// {
//     QMutexLocker locker(&mutex);
//     _vrLibrary.clear();
//     settingsChangedEvent(true);
// }

// QStringList SettingsHandler::getLibraryExclusions()
// {
//     return _libraryExclusions;
// }

// bool SettingsHandler::addToLibraryExclusions(QString value, QStringList& errors)
// {
//     bool hasDuplicate = false;
//     bool isChild = false;
//     if(XFileUtil::isDirParentChildOrEqual(_libraryExclusions, value, "library exclusions", errors))
//         hasDuplicate = true;
//     foreach (auto originalPath, getSelectedLibrary()) {
//         if(originalPath==value) {
//             errors << "Directory '"+value+"' is already in the main library list!";
//             hasDuplicate = true;
//         } else if(value.startsWith(originalPath)) {
//             isChild = true;
//         }
//     }
//     foreach (auto originalPath, getVRLibrary()) {
//         if(originalPath==value) {
//             errors << "Directory '"+value+"' is in the vr library list!";
//             hasDuplicate = true;
//         } else if(value.startsWith(originalPath)) {
//             isChild = true;
//         }
//     }
//     if(!isChild) {
//         errors << "Directory '"+value+"' is NOT a child of any of the select libraries!";
//         return false;
//     }
//     if(hasDuplicate) {
//         return false;
//     }
//     _libraryExclusions.append(value);
//     settingsChangedEvent(true);
//     return true;
// }
// void SettingsHandler::removeFromLibraryExclusions(QList<int> indexes)
// {
//     foreach(auto index, indexes)
//     _libraryExclusions.removeAt(index);
//     settingsChangedEvent(true);
// }

// QStringList SettingsHandler::getSelectedFunscriptLibrary()
// {
//     QMutexLocker locker(&mutex);
//     return selectedFunscriptLibrary;
// }
