#ifndef MEDIALIBRARYHANDLER_H
#define MEDIALIBRARYHANDLER_H

#include <QObject>
#include <QFileInfo>
#include <QtConcurrent/QtConcurrent>

#include "../handler/settingshandler.h"
#include "../handler/xvideopreview.h"
#include "../struct/LibraryListItem.h"
#include "lib/tool/imagefactory.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT MediaLibraryHandler : public QObject
{
    Q_OBJECT
signals:
    void prepareLibraryLoad();
    void libraryNotFound();
    //void libraryItemFound(LibraryListItem27 item);
    void libraryLoadingStatus(QString message);
    void libraryLoading();
    void libraryLoaded();
    void libraryStopped();
    void libraryChange();
    void itemAdded(LibraryListItem27 item);
    void itemRemoved(LibraryListItem27 item);
    void itemUpdated(LibraryListItem27 item);
    //void playListItem(LibraryListItem27 item);

    void thumbProcessBegin();
    void thumbProcessEnd();
    void saveNewThumbLoading(LibraryListItem27 item);
    void saveNewThumb(LibraryListItem27 item, bool vrMode, QString thumbFile);
    void saveThumbError(LibraryListItem27 item, bool vrMode, QString error);
    void frameExtracted(LibraryListItem27 item, bool vrMode, QPixmap frame);
    void frameExtractedError(LibraryListItem27 item, bool vrMode, const QString &errorMessage);
//    void videoLoadError(LibraryListItem27 item, bool vrMode, QtAV::AVError er);
public:
    MediaLibraryHandler(QObject* parent = nullptr);
    ~MediaLibraryHandler();
    void saveSingleThumb(QString id, qint64 position = 0);
    void startThumbProcess(bool vrMode = false);
    void stopThumbProcess();
    void loadLibraryAsync();
    bool isLibraryLoading();
    void stopLibraryLoading();
    LibraryListItem27 setupPlaylistItem(QString name);
    void updateItem(LibraryListItem27 item);
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
    LibraryListItem27* findItemByPartialThumbPath(QString partialThumbPath);
    int findItemIndexByID(QString id);
    bool isLibraryItemVideo(LibraryListItem27 item);

private:
    int _libraryItemIDTracker = 1;
    bool _thumbProcessIsRunning = false;
    bool _loadingLibraryStop = false;
    int _thumbNailSearchIterator = 0;
    QList<LibraryListItem27> _cachedLibraryItems;
    //QList<LibraryListItem27> _cachedVRItems;
    QFuture<void> _loadingLibraryFuture;
    QTimer _thumbTimeoutTimer;
    QMutex _mutex;
    XVideoPreview* _extractor = 0;

    void on_load_library(QStringList paths, bool vrMode);
    void onLibraryLoaded();
    //void saveThumbs(QList<LibraryListItem27> items, qint64 position = 0, bool vrMode = false);
    void onPrepareLibraryLoad();
    void onLibraryItemFound(LibraryListItem27 item);
    void onSaveThumb(LibraryListItem27 &item, bool vrMode, QString errorMessage = nullptr);
    void setThumbPath(LibraryListItem27 &item);
    void setThumbState(ThumbState state, LibraryListItem27 &item);
    void saveNewThumbs(bool vrMode = false);
    void saveThumb(LibraryListItem27 &item, qint64 position = 0, bool vrMode = false);
    void updateToolTip(LibraryListItem27 &item);
    void assignID(LibraryListItem27 &item);

    LibraryListItem27 createLibraryListItemFromFunscript(QString funscript);



};

#endif // MEDIALIBRARYHANDLER_H
