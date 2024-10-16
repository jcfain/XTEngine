#ifndef MEDIALIBRARYHANDLER_H
#define MEDIALIBRARYHANDLER_H

#include <QObject>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>
#include <algorithm>
#include <QImage>

#include "../struct/LibraryListItem27.h"
#include "../struct/ScriptInfo.h"
#include "../struct/LibraryListItemMetaData258.h"
#include "xvideopreview.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT MediaLibraryHandler : public QObject
{
    Q_OBJECT
signals:
    void prepareLibraryLoad();
    void libraryNotFound(QStringList paths);
    void noLibraryFound();
    //void libraryItemFound(LibraryListItem27 item);
    void libraryLoadingStatus(QString message);
    void libraryLoading();
    void libraryLoaded();
    void libraryStopped();
    void libraryChange();
    void itemAdded(int index, int newSize);
    void itemRemoved(int index, int newSize);
    void itemUpdated(int index, QVector<int> roles);
    //void playListItem(LibraryListItem27 item);

    void backgroundProcessStateChange(QString message, float percentage);
    void thumbProcessBegin();
    void thumbProcessEnd();
    void metadataProcessBegin();
    void metadataProcessEnd();
    void cleanUpThumbsFinished();
    void cleanUpThumbsFailed();
    void saveNewThumbLoading(LibraryListItem27 item);
    void saveNewThumb(LibraryListItem27 item, bool vrMode, QString thumbFile);
    void saveThumbError(LibraryListItem27 item, bool vrMode, QString error);
    void frameExtracted(LibraryListItem27 item, bool vrMode, QImage frame);
    void frameExtractedError(LibraryListItem27 item, bool vrMode, const QString &errorMessage);
//    void videoLoadError(LibraryListItem27 item, bool vrMode, QtAV::AVError er);
public:
    MediaLibraryHandler(QObject* parent = nullptr);
    ~MediaLibraryHandler();
    void saveSingleThumb(QString id, qint64 position = 0);
    bool thumbProcessRunning();
    void startThumbProcess(bool vrMode = false);
    void stopThumbProcess();
    void loadLibraryAsync();
    bool isLibraryProcessing();
    bool isLoadingMediaPaths();
    bool isThumbProcessRunning();
    bool isMetadataProcessing();
    bool isThumbCleanupRunning();
    void stopLibraryLoading();
    LibraryListItem27 setupPlaylistItem(QString name);
    LibraryListItem27 setupTempExternalItem(QString mediapath, QString scriptPath = nullptr, quint64 duration = 0);
    void updateItem(LibraryListItem27 item, QVector<int> roles, bool notify = true);
    void updateItem(int index, QVector<int> roles);
    void removeFromCache(LibraryListItem27 item);
    void addItemFront(LibraryListItem27 item);
    void addItemBack(LibraryListItem27 item);
    QList<LibraryListItem27> getLibraryCache();
    QList<LibraryListItem27> getPlaylist(QString name);
    //QList<LibraryListItem27> getVRLibraryCache();
    void setLiveProperties(LibraryListItem27 &item);
    void lockThumb(LibraryListItem27 &item);
    void unlockThumb(LibraryListItem27 &item);
    QString getScreenType(QString mediaPath);
    QString getStereoMode(QString mediaPath);
    bool isStereo(QString mediaPath);
    LibraryListItem27* findItemByID(QString id);
    LibraryListItem27* findItemByNameNoExtension(QString nameNoExtension);
    LibraryListItem27* findItemByName(QString name);
    LibraryListItem27* findItemByMediaPath(QString mediaPath);
    LibraryListItem27* findItemByPartialMediaPath(QString partialMediaPath);
    LibraryListItem27* findItemByThumbPath(QString thumbPath);
    LibraryListItem27* findItemByPartialThumbPath(QString partialThumbPath);
    LibraryListItem27* findItemBySubtitle(QString subtitle);
    LibraryListItem27* findItemByPartialSubtitle(QString partialSubtitle);
    LibraryListItem27 *findItemByAltScript(QString value);
    LibraryListItem27 *findItemByPartialAltScript(QString value);
    int findItemIndexByID(QString id);
    bool isLibraryItemVideo(LibraryListItem27 item);
    void cleanGlobalThumbDirectory();
    void findAlternateFunscripts(LibraryListItem27& item);
    bool metadataProcessing();
    void processMetadata(LibraryListItem27 &item);
    void startMetadataProcess(bool fullProcess = false);
    void startMetadataCleanProcess();
    void startMetadata1024Cleanup();

private:
    int _libraryItemIDTracker = 1;
    bool _thumbProcessIsRunning = false;
    int _thumbNailSearchIterator = 0;
    QList<LibraryListItem27> _cachedLibraryItems;
    //QList<LibraryListItem27> _cachedVRItems;
    QFuture<void> _metadataFuture;
    QFuture<void> _loadingLibraryFuture;
    QFuture<void> m_thumbCleanupFuture;
    QTimer _thumbTimeoutTimer;
    QMutex _mutex;
    //XVideoPreview* _extractor = 0;

    void on_load_library(QStringList paths, bool vrMode);
    //void saveThumbs(QList<LibraryListItem27> items, qint64 position = 0, bool vrMode = false);
    void onPrepareLibraryLoad();
    void onLibraryItemFound(LibraryListItem27 item);
    void onSaveThumb(QString itemID, bool vrMode, QString errorMessage = nullptr);
    void setThumbPath(LibraryListItem27 &item);
    void setThumbState(ThumbState state, LibraryListItem27 &item);
    void saveNewThumbs(bool vrMode = false);
    void saveThumb(LibraryListItem27 &item, qint64 position = 0, bool vrMode = false);
    void assignID(LibraryListItem27 &item);

    void stopAllSubProcesses();
    LibraryListItem27 createLibraryListItemFromFunscript(QString funscript);
    void stopThumbCleanupProcess();
    void processMetadata(LibraryListItem27 &item, bool &metadataChanged, QVector<int> &rolesChanged, bool fullProcess = false);
    void stopMetadataProcess();
    bool updateToolTip(LibraryListItem27 &item);
    bool discoverMFS(LibraryListItem27 &item);

    XVideoPreview xVideoPreview;

};

#endif // MEDIALIBRARYHANDLER_H
