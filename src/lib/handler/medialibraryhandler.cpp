#include "medialibraryhandler.h"

MediaLibraryHandler::MediaLibraryHandler(QObject* parent)
    : QObject(parent)
{
    _thumbTimeoutTimer.setSingleShot(true);
    connect(this, &MediaLibraryHandler::prepareLibraryLoad, this, &MediaLibraryHandler::onPrepareLibraryLoad);
    connect(this, &MediaLibraryHandler::libraryLoaded, this, &MediaLibraryHandler::onLibraryLoaded);
}

MediaLibraryHandler::~MediaLibraryHandler()
{
    stopLibraryLoading();
}
bool MediaLibraryHandler::isLibraryLoading()
{
    return _loadingLibraryFuture.isRunning();
}

bool MediaLibraryHandler::isLibraryItemVideo(LibraryListItem27 item) {
    return item.type == LibraryListItemType::Video || item.type == LibraryListItemType::VR;
}

void MediaLibraryHandler::stopLibraryLoading()
{
    if(_loadingLibraryFuture.isRunning())
    {
        _loadingLibraryStop = true;
        _loadingLibraryFuture.cancel();
        _loadingLibraryFuture.waitForFinished();
        _loadingLibraryStop = false;
        emit libraryLoadingStatus("Loading medai stopped");
    }
}

void MediaLibraryHandler::onPrepareLibraryLoad()
{
    stopThumbProcess();
    const QMutexLocker locker(&_mutex);
    _cachedLibraryItems.clear();
    _libraryItemIDTracker = 1;
}

void MediaLibraryHandler::loadLibraryAsync()
{
    LogHandler::Debug("loadLibraryAsync");
    onPrepareLibraryLoad();
    stopLibraryLoading();
    LogHandler::Debug("loadLibraryAsync after stop");
    QStringList library = SettingsHandler::getSelectedLibrary();
    QStringList vrLibrary = SettingsHandler::getVRLibrary();
    if(library.isEmpty() && vrLibrary.isEmpty())
    {
        emit libraryLoadingStatus("No media folder specified");
        LogHandler::Debug("libraryStopped No media folder specified");
        emit libraryStopped();
        return;
    }
    if(!isLibraryLoading())
    {
        _loadingLibraryFuture = QtConcurrent::run([this, library, vrLibrary]() {
            on_load_library(library.isEmpty() ? vrLibrary : library, library.isEmpty());
        });
    }
}

void MediaLibraryHandler::on_load_library(QStringList paths, bool vrMode)
{
    emit libraryLoading();
    if (paths.isEmpty())
    {
        emit libraryStopped();
        return;
    }
    else
    {
        bool anyExist = false;
        foreach (QString path, paths) {
            if(QFileInfo::exists(path))
            {
                anyExist = true;
                break;
            }
        }
        if(!anyExist) {
            if(vrMode) {
                emit libraryChange();
                emit libraryLoaded();
                return;
            }
            emit libraryNotFound();
            emit libraryStopped();
            return;
        }
    }

//    QStringList videoTypes = QStringList()
//            << "*.mp4"
//            << "*.avi"
//            << "*.mpg"
//            << "*.wmv"
//            << "*.mkv"
//            << "*.webm"
//            << "*.mp2"
//            << "*.mpeg"
//            << "*.mpv"
//            << "*.ogg"
//            << "*.m4p"
//            << "*.m4v"
//            << "*.mov"
//            << "*.qt"
//            << "*.flv"
//            << "*.swf"
//            << "*.avchd";

//    QStringList audioTypes = QStringList()
//            << "*.m4a"
//            << "*.mp3"
//            << "*.aac"
//            << "*.flac"
//            << "*.wav"
//            << "*.wma";
    QStringList playlistTypes = QStringList()
            << "*.m3u";

    QStringList mediaTypes;
    QStringList videoTypes;
    QStringList audioTypes;
    foreach(auto ext, SettingsHandler::getVideoExtensions())
        videoTypes.append("*."+ext);
    foreach(auto ext, SettingsHandler::getAudioExtensions())
        audioTypes.append("*."+ext);
    mediaTypes.append(videoTypes);
    mediaTypes.append(audioTypes);

    QStringList vrLibrary = SettingsHandler::getVRLibrary();
    QStringList excludedLibraryPaths = SettingsHandler::getLibraryExclusions();
    bool hasVRLibrary = false;

    if(!vrMode)
    {

        auto playlists = SettingsHandler::getPlaylists();
        //For some reason sorting random without any playlists crashes. Add dummy and hide it in proxy model.
        if(playlists.empty())
            setupPlaylistItem("DummyPlaylistThatNoOneShouldEverSeeOrNameTheSame");
        else
            foreach(auto playlist, playlists.keys())
            {
                setupPlaylistItem(playlist);
            }

        foreach(auto path, vrLibrary)
        {
            if(!path.isEmpty()) {
                excludedLibraryPaths.append(vrLibrary);
                hasVRLibrary = true;
            }
        }
    }
    auto availibleChannels = TCodeChannelLookup::getChannels();

    foreach (QString path, paths) {
        auto itr = std::find_if(paths.begin(), paths.end(), [path](const QString& item) {
            return path.startsWith(item) && path.length() > item.length();
        });
        if(itr != paths.end())
            continue;

        emit libraryLoadingStatus((vrMode ? "Loading VR media" : "Loading media") + (paths.length() > 1 ? " " + QString::number(paths.indexOf(path) + 1)+"..." : "..."));

        QDirIterator library(path, mediaTypes, QDir::Files, QDirIterator::Subdirectories);

        QStringList funscriptsWithMedia;

        while (library.hasNext())
        {
            if(_loadingLibraryStop)
            {
                LogHandler::Debug("libraryStopped 1");
                emit libraryStopped();
                return;
            }
            QFileInfo fileinfo(library.next());
            QString fileDir = fileinfo.dir().path();
            bool isExcluded = false;
            foreach(QString dir, excludedLibraryPaths)
            {
                if(dir != path && (fileDir.startsWith(dir, Qt::CaseInsensitive)))
                    isExcluded = true;
            }
            if (isExcluded)
                continue;
            QString videoPath = fileinfo.filePath();
            QString videoPathTemp = fileinfo.filePath();
            QString fileName = fileinfo.fileName();
            QString fileNameTemp = fileinfo.fileName();
            QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
            QString scriptFile = fileNameNoExtension + ".funscript";
            QString scriptPath;
            QString scriptNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() - 1);
            fileNameTemp = fileinfo.fileName();
            QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));

            if (SettingsHandler::getSelectedFunscriptLibrary() == Q_NULLPTR)
            {
                scriptPath = scriptNoExtension + ".funscript";
            }
            else
            {
                scriptNoExtension = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + fileNameNoExtension;
                scriptPath = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + scriptFile;
            }
            if (!QFileInfo::exists(scriptPath))
            {
                scriptPath = nullptr;
            }
            LibraryListItemType libratyItemType = vrMode || isStereo(fileName) ? LibraryListItemType::VR : LibraryListItemType::Video;
            QString zipFile = scriptNoExtension + ".zip";
            if(!QFileInfo::exists(zipFile)) {
                zipFile = nullptr;
            }
            if(audioTypes.contains(mediaExtension))
            {
                libratyItemType = LibraryListItemType::Audio;
            }
            LibraryListItem27 item;
            item.type = libratyItemType;
            item.path = videoPath; // path
            item.name = fileName; // name
            item.nameNoExtension = fileNameNoExtension; //nameNoExtension
            item.script = scriptPath; // script
            item.scriptNoExtension = scriptNoExtension;
            item.hasScript = !scriptPath.isEmpty() || !zipFile.isEmpty();
            item.mediaExtension = mediaExtension;
            item.thumbFile = nullptr;
            item.zipFile = zipFile;
            item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
            item.duration = 0;
            item.thumbState = ThumbState::Waiting;
            item.isMFS = false;
            item.libraryPath = path;
            setLiveProperties(item);

            if(!vrMode && !scriptPath.isEmpty())
                funscriptsWithMedia.append(scriptPath);
            if(!vrMode && !zipFile.isEmpty())
                funscriptsWithMedia.append(zipFile);

            onLibraryItemFound(item);
            //emit libraryItemFound(item);
        }

        if(!vrMode)
        {
            emit libraryLoadingStatus("Searching for lone funscripts...");
            QStringList funscriptTypes = QStringList()
                    << "*.funscript"
                    << "*.zip";
            QDirIterator funscripts(path, funscriptTypes, QDir::Files, QDirIterator::Subdirectories);
            while (funscripts.hasNext())
            {
                if(_loadingLibraryStop)
                {
                    LogHandler::Debug("libraryStopped 2");
                    emit libraryStopped();
                    return;
                }
                QFileInfo fileinfo(funscripts.next());
                QString fileName = fileinfo.fileName();
                QString fileNameTemp = fileinfo.fileName();
                QString scriptPath = fileinfo.filePath();
                QString scriptPathTemp = fileinfo.filePath();
                QString scriptNoExtension = scriptPathTemp.remove(scriptPathTemp.lastIndexOf('.'), scriptPathTemp.length() - 1);
                QString scriptNoExtensionTemp = QString(scriptNoExtension);
                if(funscriptsWithMedia.contains(scriptPath, Qt::CaseSensitivity::CaseInsensitive))
                    continue;

                QString scriptMFSExt = scriptNoExtensionTemp.remove(0, scriptNoExtensionTemp.length() - (scriptNoExtensionTemp.length() - scriptNoExtensionTemp.lastIndexOf('.')));
                bool isMfs = false;
                foreach(auto axisName, availibleChannels)
                {
                    auto track = TCodeChannelLookup::getChannel(axisName);
                    if("."+track->TrackName == scriptMFSExt)
                    {
                        isMfs = true;
                        break;
                    }
                }
                if(isMfs)
                    continue;

                QString fileDir = fileinfo.dir().path();
                bool isExcluded = false;
                foreach(QString dir, excludedLibraryPaths)
                {
                    if(dir != path && (fileDir.startsWith(dir, Qt::CaseInsensitive)))
                        isExcluded = true;
                }
                if (isExcluded)
                    continue;
                QString zipFile = nullptr;
                if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
                {
                    zipFile = scriptPath;
                }
                fileNameTemp = fileinfo.fileName();
                QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
                fileNameTemp = fileinfo.fileName();
                QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));

                LibraryListItem27 item;
                item.type = LibraryListItemType::FunscriptType;
                item.path = scriptPath; // path
                item.name = fileName; // name
                item.nameNoExtension = fileNameNoExtension; //nameNoExtension
                item.script = scriptPath; // script
                item.scriptNoExtension = scriptNoExtension;
                item.hasScript = true;
                item.mediaExtension = mediaExtension;
                item.zipFile = zipFile;
                item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
                item.duration = 0;
                item.thumbState = ThumbState::Waiting;
                item.libraryPath = path;
                item.isMFS = false;
                setLiveProperties(item);
                onLibraryItemFound(item);
                //emit libraryItemFound(item);
            }
        }
    }
    if(vrMode) {
        emit libraryChange();
        emit libraryLoaded();
    }
    else
    {
        if(!hasVRLibrary)
        {
            emit libraryChange();
            emit libraryLoaded();
            return;
        }
        on_load_library(vrLibrary, true);
    }

}

void MediaLibraryHandler::onLibraryItemFound(LibraryListItem27 item)
{
    addItemBack(item);
}
LibraryListItem27 MediaLibraryHandler::createLibraryListItemFromFunscript(QString funscript)
{

    QFileInfo fileinfo(funscript);
    QString fileName = fileinfo.fileName();
    QString fileNameTemp = fileinfo.fileName();
    QString scriptPath = fileinfo.filePath();
    QString scriptPathTemp = fileinfo.filePath();
    QString scriptNoExtension = scriptPathTemp.remove(scriptPathTemp.lastIndexOf('.'), scriptPathTemp.length() - 1);
    QString scriptNoExtensionTemp = QString(scriptNoExtension);
    QString fileDir = fileinfo.dir().path();
    QString zipFile = nullptr;
    if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
    {
        zipFile = scriptPath;
        scriptPath = nullptr;
    }
    fileNameTemp = fileinfo.fileName();
    QString fileNameNoExtension = fileNameTemp.remove(fileNameTemp.lastIndexOf('.'), fileNameTemp.length() -  1);
    fileNameTemp = fileinfo.fileName();
    QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
    LibraryListItem27 item;
    item.type = LibraryListItemType::FunscriptType;
    item.path = scriptPath; // path
    item.name = fileName; // name
    item.nameNoExtension = fileNameNoExtension; //nameNoExtension
    item.script = scriptPath; // script
    item.scriptNoExtension = fileNameNoExtension;
    item.mediaExtension = mediaExtension;
    item.zipFile = zipFile;
    item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
    item.duration = 0;
    item.thumbState = ThumbState::Waiting;
    setLiveProperties(item);

    addItemBack(item);
    return item;
}

void MediaLibraryHandler::onLibraryLoaded()
{
    startThumbProcess();

}

void MediaLibraryHandler::startThumbProcess(bool vrMode)
{
    QString thumbPath = SettingsHandler::getSelectedThumbsDir();
    QDir thumbDir(thumbPath);
    if (!thumbDir.exists())
    {
        thumbDir.mkdir(thumbPath);
    }
    stopThumbProcess();
    LogHandler::Debug("Start thumb process, vrMode: " + QString::number(vrMode));
    _thumbProcessIsRunning = true;
    saveNewThumbs(vrMode);
    emit thumbProcessBegin();
}

void MediaLibraryHandler::stopThumbProcess()
{
    if(_thumbProcessIsRunning)
    {
        LogHandler::Debug("Stop thumb process");
        _thumbNailSearchIterator = 0;
        _thumbProcessIsRunning = false;
        if(_thumbTimeoutTimer.isActive())
        {
            _thumbTimeoutTimer.stop();
            disconnect(&_thumbTimeoutTimer, &QTimer::timeout, nullptr, nullptr);
        }
        if(_extractor) {
            disconnect(_extractor, nullptr,  nullptr, nullptr);
//            delete _extractor;
//            _extractor = 0;
        }
//        delete _thumbNailPlayer;
    }
}

void MediaLibraryHandler::saveSingleThumb(QString id, qint64 position)
{
    auto item = findItemByID(id);
    if(item && !_thumbProcessIsRunning && !item->thumbFile.contains(".lock."))
    {
        saveThumb(*item, position);
    }
}

void MediaLibraryHandler::saveNewThumbs(bool vrMode)
{
    if (_thumbProcessIsRunning && _thumbNailSearchIterator < _cachedLibraryItems.count())
    {
        LibraryListItem27 &item = _cachedLibraryItems[_thumbNailSearchIterator];
        _thumbNailSearchIterator++;
        QFileInfo thumbInfo(item.thumbFile);
        if (isLibraryItemVideo(item) && !thumbInfo.exists())
        {
            saveThumb(item, -1, vrMode);
        }
        else
        {
            setThumbState(ThumbState::Ready, item);
            saveNewThumbs(vrMode);
        }
    }
    else
    {
        stopThumbProcess();
        if(!vrMode)
            startThumbProcess(true);
        else
        {
            LogHandler::Debug("Thumb process finished");
            emit thumbProcessEnd();
        }
    }
}

void MediaLibraryHandler::saveThumb(LibraryListItem27 &item, qint64 position, bool vrMode)
{
    if(_thumbProcessIsRunning && (item.type == LibraryListItemType::Audio || item.type == LibraryListItemType::FunscriptType || item.thumbFile.contains(".lock.")))
    {
        setThumbState(ThumbState::Ready, item);
        saveNewThumbs(vrMode);
    }
    else
    {
        setThumbState(ThumbState::Loading, item);
        QDir dir; // Make thumb path if doesnt exist
        dir.mkpath(SettingsHandler::getSelectedThumbsDir());
        _thumbTimeoutTimer.stop();
        disconnect(&_thumbTimeoutTimer, &QTimer::timeout, nullptr, nullptr);
        if(!_extractor)
            _extractor = new XVideoPreview(this);
        connect(&_thumbTimeoutTimer, &QTimer::timeout, &_thumbTimeoutTimer, [this, &item, vrMode]() {
            if(_thumbProcessIsRunning)
            {
                disconnect(_extractor, nullptr,  nullptr, nullptr);
                onSaveThumb(item, vrMode, "Thumb loading timed out.");
            }
        });
        _thumbTimeoutTimer.start(30000);
//        if(!vrMode)
        // Webhandler
        emit saveNewThumbLoading(item);
        // Get the duration and randomize the position with in the video.
        QString videoFile = item.path;
        LogHandler::Debug("Getting thumb: " + item.thumbFile);
        connect(_extractor, &XVideoPreview::durationChanged, this,
           [this, videoFile, position](qint64 duration)
            {
               disconnect(_extractor, &XVideoPreview::durationChanged,  nullptr, nullptr);
               LogHandler::Debug("Loaded video for thumb. Duration: " + QString::number(duration));
               qint64 randomPosition = position > 0 ? position : XMath::rand((qint64)1, duration);
               LogHandler::Debug("Extracting at: " + QString::number(randomPosition));
               _extractor->extract(videoFile, randomPosition);
            });


//        connect(_thumbNailPlayer, &AVPlayer::error, _thumbNailPlayer,
//           [this, cachedListItem, vrMode](QtAV::AVError er)
//            {
//                QString error = "Video load error from: " + cachedListItem.path + " Error: " + er.ffmpegErrorString();
//                onSaveThumb(cachedListItem, vrMode, error);
//            });


        connect(_extractor, &XVideoPreview::frameExtracted, this,
           [this, &item, vrMode](QImage frame)
            {
                disconnect(_extractor, &XVideoPreview::frameExtracted,  nullptr, nullptr);
                disconnect(_extractor, &XVideoPreview::frameExtractionError,  nullptr, nullptr);
                if(!frame.isNull())
                {
                    bool hasError = frame.isNull() || !frame.save(item.thumbFile, nullptr, 15);
                    if (hasError)
                    {
                       onSaveThumb(item, vrMode, "Error saving thumbnail");
                       return;
                    }

                }
                frame = QImage();
                onSaveThumb(item, vrMode);
            });

        connect(_extractor, &XVideoPreview::frameExtractionError, this,
           [this, &item, vrMode](const QString &errorMessage)
            {
                disconnect(_extractor, &XVideoPreview::frameExtracted,  nullptr, nullptr);
                disconnect(_extractor, &XVideoPreview::frameExtractionError,  nullptr, nullptr);
                QString error = "Error extracting image from: " + item.path + " Error: " + errorMessage;
                onSaveThumb(item, vrMode, error);
            });

        _extractor->load(videoFile);
    }
}

//void MediaLibraryHandler::saveThumbs(QList<LibraryListItem27> items, qint64 position, bool vrMode)
//{
//    thumbNailPlayer->setAsyncLoad(false);
//    extractor->setAsync(false);
//    //extractor->setAutoExtract(false);
//    QtConcurrent::run([this, items, position, vrMode]() {
//        foreach(LibraryListItem27 item, items)
//        {
//            if(!thumbProcessIsRunning)
//                return;
//            if(thumbProcessIsRunning && (item.type == LibraryListItemType::Audio || item.type == LibraryListItemType::FunscriptType || item.thumbFile.contains(".lock.")))
//                continue;
//            thumbNailPlayer->setFile(item.path);
//            if(!thumbNailPlayer->load()) {
//                QString error = tr("Video load error from: ") + item.path;
//                onSaveThumb(item, vrMode, error);
//                continue;
//            }
//            extractor->setSource(item.path);
//            qint64 randomPosition = position > 0 ? position : XMath::rand((qint64)1, thumbNailPlayer->duration());
//            extractor->setPosition(randomPosition);
//        }
//    });
//}

void MediaLibraryHandler::onSaveThumb(LibraryListItem27 &item, bool vrMode, QString errorMessage)
{
    int cachedIndex = _cachedLibraryItems.indexOf(item);
    if(cachedIndex == -1) {
        LibraryListItem27 emptyItem;
        emit saveThumbError(emptyItem, vrMode, "Missing media");
        return;
    }
    LibraryListItem27 &cachedItem = _cachedLibraryItems[cachedIndex];
    if(!errorMessage.isEmpty())
    {
        LogHandler::Error("Save thumb error: " + errorMessage);
        setThumbState(ThumbState::Error, item);
        emit saveThumbError(cachedItem, vrMode, errorMessage);
    }
    else
    {
        LogHandler::Debug("Thumb saved: " + item.thumbFile);
        ImageFactory::removeCache(item.thumbFile);
        setThumbPath(cachedItem);
        setThumbState(ThumbState::Ready, cachedItem);
        emit saveNewThumb(item, vrMode, item.thumbFile);
    }

    if(_thumbProcessIsRunning)
        saveNewThumbs(vrMode);
    else {
        disconnect(_extractor, nullptr,  nullptr, nullptr);
//        delete _extractor;
//        _extractor = 0;
    }
}

QList<LibraryListItem27> MediaLibraryHandler::getLibraryCache()
{
    return _cachedLibraryItems;
}
QList<LibraryListItem27> MediaLibraryHandler::getPlaylist(QString name) {
    auto playlists = SettingsHandler::getPlaylists();
    auto playlist = playlists.value(name);
    foreach (auto item, playlist) {
        LibraryListItem27& itemRef = playlist[playlist.indexOf(item)];
        setLiveProperties(itemRef);
        setThumbState(itemRef.thumbFileExists ? ThumbState::Ready : ThumbState::Error, itemRef);
    }
    return playlist;
}
LibraryListItem27 MediaLibraryHandler::setupPlaylistItem(QString playlistName)
{
    LibraryListItem27 item;
    item.type = LibraryListItemType::PlaylistInternal;
    item.nameNoExtension = playlistName; //nameNoExtension
    item.modifiedDate = QDateTime::currentDateTime().date();
    item.duration = 0;
    item.isMFS = false;
    setThumbState(ThumbState::Ready, item);
    setLiveProperties(item);
    addItemFront(item);
    //emit playListItem(item);
    return item;
}
void MediaLibraryHandler::updateItem(LibraryListItem27 item) {
    const QMutexLocker locker(&_mutex);
    auto index = findItemIndexByID(item.ID);
    if(index > -1)
        _cachedLibraryItems[index] = item;
    emit itemUpdated(item);
}
void MediaLibraryHandler::removeFromCache(LibraryListItem27 itemToRemove) {
    const QMutexLocker locker(&_mutex);
    _cachedLibraryItems.removeOne(itemToRemove);
    if(!isLibraryLoading()) {
        emit itemRemoved(itemToRemove);
        //emit libraryChange();
    }
}
void MediaLibraryHandler::addItemFront(LibraryListItem27 item) {
    const QMutexLocker locker(&_mutex);
    _cachedLibraryItems.push_front(item);
    if(!isLibraryLoading()) {
        emit itemAdded(item);
        //emit libraryChange();
    }
}
void MediaLibraryHandler::addItemBack(LibraryListItem27 item) {
    const QMutexLocker locker(&_mutex);
    _cachedLibraryItems.push_back(item);
    if(!isLibraryLoading()) {
        emit itemAdded(item);
        //emit libraryChange();
    }
}
void MediaLibraryHandler::setLiveProperties(LibraryListItem27 &libraryListItem)
{
    assignID(libraryListItem);
    setThumbPath(libraryListItem);
    updateToolTip(libraryListItem);
}

void MediaLibraryHandler::lockThumb(LibraryListItem27 &item)
{
    QFile file(item.thumbFile);
    if(!item.thumbFile.contains(".lock."))
    {
        LogHandler::Debug("Rename: "+ item.thumbFile);
        QString path = item.thumbFile;
        QString localpath = path;
        QString thumbTemp = localpath;
        int indexOfSuffix = localpath.lastIndexOf(".");
        QString extension = thumbTemp.remove(0, thumbTemp.length() - (thumbTemp.length() - thumbTemp.lastIndexOf('.')));
        QString newName = localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".lock"+extension);
        bool success = file.rename(newName);
        if(success) {
            LogHandler::Debug("File renamed: "+ newName);
            item.thumbFile = newName;
            int index = findItemIndexByID(item.ID);
            if(index > -1) {
                _cachedLibraryItems[index].thumbFile = newName;
                emit itemUpdated(item);
            }
        } else {
            LogHandler::Error("File rename failed: "+ newName);
        }
    }
}
void MediaLibraryHandler::unlockThumb(LibraryListItem27 &item)
{
    QFile file(item.thumbFile);
    if(item.thumbFile.contains(".lock."))
    {
        QString path = item.thumbFile;
        QString newName = path.remove(".lock");
        bool success = file.rename(newName);
        if(success) {
            LogHandler::Debug("File renamed: "+ newName);
            item.thumbFile = newName;
            int index = findItemIndexByID(item.ID);
            if(index > -1) {
                _cachedLibraryItems[index].thumbFile = newName;
                emit itemUpdated(item);
            }
        } else {
            LogHandler::Error("File NOT renamed: "+ newName);
        }
    }
}

void MediaLibraryHandler::setThumbState(ThumbState state, LibraryListItem27 &item) {
    item.thumbState = state;
    emit itemUpdated(item);
}

void MediaLibraryHandler::setThumbPath(LibraryListItem27 &libraryListItem)
{
    if(libraryListItem.type == LibraryListItemType::Audio)
    {
        libraryListItem.thumbFile = "://images/icons/audio.png";
        libraryListItem.thumbFileExists = true;
        return;
    }
    else if(libraryListItem.type == LibraryListItemType::PlaylistInternal)
    {
        libraryListItem.thumbFile = "://images/icons/playlist.png";
        libraryListItem.thumbFileExists = true;
        return;
    }
    else if(libraryListItem.type == LibraryListItemType::FunscriptType)
    {
        libraryListItem.thumbFile = "://images/icons/funscript.png";
        libraryListItem.thumbFileExists = true;
        return;
    }
    QStringList imageExtensions;
    imageExtensions << ".jpg" << ".jpeg" << ".png" << ".jfif" << ".webp";
    QFileInfo mediaInfo(libraryListItem.path);
    QString globalPath = SettingsHandler::getSelectedThumbsDir() + libraryListItem.name;
    QString absolutePath = mediaInfo.absolutePath() + QDir::separator() + libraryListItem.nameNoExtension;
    foreach(QString ext, imageExtensions) {
        QString filepathGlobal = globalPath + ext;
        if(QFileInfo::exists(filepathGlobal))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.thumbFile = filepathGlobal;
            return;
        }
        QString filepathGlobalLocked = globalPath + ".lock" + ext;
        if(QFileInfo::exists(filepathGlobalLocked))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.thumbFile = filepathGlobalLocked;
            return;
        }
        QString filepathLocked = absolutePath + ".lock" + ext;
        if(QFileInfo::exists(filepathLocked))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.thumbFile = filepathLocked;
            return;
        }
        QString filepath = absolutePath + ext;
        if(QFileInfo::exists(filepath))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.thumbFile = filepath;
            return;
        }
    }
    if(SettingsHandler::getUseMediaDirForThumbs())
    {
        libraryListItem.thumbFile = mediaInfo.absolutePath() + libraryListItem.nameNoExtension + ".jpg";
        libraryListItem.thumbFileExists = QFileInfo::exists(libraryListItem.thumbFile);
        return;
    }

    libraryListItem.thumbFile = SettingsHandler::getSelectedThumbsDir() + libraryListItem.name + ".jpg";
    libraryListItem.thumbFileExists = QFileInfo::exists(libraryListItem.thumbFile);

//    QStringList thumbDirectoryImaged;
//    thumbDirectoryImaged << libraryListItem.name +".jpg" <<
//                       libraryListItem.name +".jpeg" <<
//                       libraryListItem.name +".png" <<
//                       libraryListItem.name +".jfif" <<
//                       libraryListItem.name +".webp" <<
//                       libraryListItem.name +".lock.jpg" <<
//                       libraryListItem.name +".lock.jpeg" <<
//                       libraryListItem.name +".lock.png" <<
//                       libraryListItem.name +".lock.jfif" <<
//                       libraryListItem.name +".lock.webp";

//    QDir thumbDirectory(SettingsHandler::getSelectedThumbsDir());
//    QFileInfoList thumbFiles = thumbDirectory.entryInfoList(thumbDirectoryImaged, QDir::Files);
//    LogHandler::Debug(QString::number(thumbFiles.count()));
//    if(thumbFiles.count() > 0) {
//        libraryListItem.thumbFileExists = true;
//        libraryListItem.thumbFile = thumbFiles.first().filePath();
//        return;
//    }

//    QStringList mediaDirectoryImages;
//    mediaDirectoryImages << libraryListItem.nameNoExtension +".jpg" <<
//                       libraryListItem.nameNoExtension +".jpeg" <<
//                       libraryListItem.nameNoExtension +".png" <<
//                       libraryListItem.nameNoExtension +".jfif" <<
//                       libraryListItem.nameNoExtension +".webp" <<
//                       libraryListItem.nameNoExtension +".lock.jpg" <<
//                       libraryListItem.nameNoExtension +".lock.jpeg" <<
//                       libraryListItem.nameNoExtension +".lock.png" <<
//                       libraryListItem.nameNoExtension +".lock.jfif" <<
//                       libraryListItem.nameNoExtension +".lock.webp";

//    QFileInfo mediaInfo(libraryListItem.path);
//    QDir directory(mediaInfo.absolutePath());
//    QFileInfoList txtFilesAndDirectories = directory.entryInfoList(mediaDirectoryImages, QDir::Files);
//    LogHandler::Debug(QString::number(txtFilesAndDirectories.count()));
//    if(txtFilesAndDirectories.count() > 0) {
//        libraryListItem.thumbFileExists = true;
//        libraryListItem.thumbFile = txtFilesAndDirectories.first().filePath();
//        return;
//    }

}

void MediaLibraryHandler::updateToolTip(LibraryListItem27 &localData, bool MFSDiscovery)
{
    localData.isMFS = false;
    bool scriptExists = QFileInfo::exists(localData.script);
    bool zipScripExists = QFileInfo::exists(localData.zipFile);
    localData.hasScript = scriptExists || zipScripExists;
    if (localData.type != LibraryListItemType::PlaylistInternal && !scriptExists && !zipScripExists)
    {
        localData.toolTip = localData.nameNoExtension + "\nMedia:";
        localData.toolTip = localData.path + "\nNo script file of the same name found.\nRight click and Play with chosen funscript.";
    }
    else if (localData.type != LibraryListItemType::PlaylistInternal)
    {
        localData.toolTip = localData.nameNoExtension + "\nMedia:";
        localData.toolTip += "\n";
        localData.toolTip += localData.path;
        localData.toolTip += "\n";
        localData.toolTip += "Scripts:\n";
        if(zipScripExists)
        {
            localData.toolTip += localData.zipFile;
            localData.isMFS = true;
        }
        else
        {
            localData.toolTip += localData.script;
        }
        if(!SettingsHandler::getMFSDiscoveryDisabled() || MFSDiscovery) {
//            LogHandler::Debug("before discoverMFS1: "+QString::number(mSecTimer.elapsed() - lastTime));
//            lastTime = mSecTimer.elapsed();
//            discoverMFS1(localData);
//            LogHandler::Debug("after discoverMFS1: "+QString::number(mSecTimer.elapsed() - lastTime));
//            lastTime = mSecTimer.elapsed();

            discoverMFS2(localData);

            if(MFSDiscovery)//Update on tooltip demand
                updateItem(localData);
        } else {
            localData.toolTip += "Unknown";
        }
    }
    else if (localData.type == LibraryListItemType::PlaylistInternal)
    {
        localData.toolTip = localData.nameNoExtension + "\nMedia:";
        auto playlists = SettingsHandler::getPlaylists();
        auto playlist = playlists.value(localData.nameNoExtension);
        for(auto i = 0; i < playlist.length(); i++)
        {
            localData.toolTip += "\n";
            localData.toolTip += QString::number(i + 1);
            localData.toolTip += ": ";
            localData.toolTip += playlist[i].nameNoExtension;
        }
    }
}

void MediaLibraryHandler::discoverMFS1(LibraryListItem27 &item) {
    auto channels = TCodeChannelLookup::getChannels();
    QString script;
    script.reserve(item.scriptNoExtension.length() + 1 + 5 + 10);
    foreach(auto axisName, channels)
    {
        auto track = TCodeChannelLookup::getChannel(axisName);
        if(axisName == TCodeChannelLookup::Stroke() || track->Type == AxisType::HalfRange || track->TrackName.isEmpty())
            continue;

        script = item.scriptNoExtension + "." + track->TrackName + ".funscript";
        if (QFileInfo::exists(script))
        {
            item.hasScript = true;
            item.isMFS = true;
            item.toolTip += "\n";
            item.toolTip += script;
            item.MFSScripts << script;
        }
    }
}
void MediaLibraryHandler::discoverMFS2(LibraryListItem27 &item) {
    QStringList funscripts = TCodeChannelLookup::getValidMFSExtensions();
    foreach(auto scriptExtension, funscripts)
    {
        if (QFileInfo::exists(item.scriptNoExtension + scriptExtension))
        {
            item.hasScript = true;
            item.isMFS = true;
            item.toolTip += "\n";
            item.toolTip += item.scriptNoExtension + scriptExtension;
            item.MFSScripts << item.scriptNoExtension + scriptExtension;
        }
    }
}

void MediaLibraryHandler::assignID(LibraryListItem27 &item)
{
    //.replace(/^[^a-z]+|[^\w:.-]+/gi, "")+"item"+i
    // /^[^a-zA-Z]+|[^\\w\\s:.-]+/g
    QString name = item.nameNoExtension;
    item.ID = name.remove(" ") + "Item" + QString::number(_libraryItemIDTracker);
    _libraryItemIDTracker++;
}

QString MediaLibraryHandler::getScreenType(QString mediaPath)
{
    if(mediaPath.contains("360", Qt::CaseSensitivity::CaseInsensitive))
        return "360";
    if(mediaPath.contains("180", Qt::CaseSensitivity::CaseInsensitive))
        return "180";
    if(mediaPath.contains("fisheye", Qt::CaseSensitivity::CaseInsensitive))
        return "fisheye";
    if(mediaPath.contains("mkx200", Qt::CaseSensitivity::CaseInsensitive))
        return "mkx200";
    if(mediaPath.contains("vrca220", Qt::CaseSensitivity::CaseInsensitive))
        return "vrca220";
    return "flat";
}

QString MediaLibraryHandler::getStereoMode(QString mediaPath)
{
    if(mediaPath.contains(" tb", Qt::CaseSensitivity::CaseInsensitive) ||
        mediaPath.contains("_tb", Qt::CaseSensitivity::CaseInsensitive) ||
        mediaPath.contains("tb_", Qt::CaseSensitivity::CaseInsensitive) ||
        mediaPath.contains("tb ", Qt::CaseSensitivity::CaseInsensitive))
        return "TB";
    if(mediaPath.contains(" sbs", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("_sbs", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("sbs_", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("sbs ", Qt::CaseSensitivity::CaseInsensitive))
        return "SBS";
    if(mediaPath.contains(" 3DH", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("_3DH", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("3DH_", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("3DH ", Qt::CaseSensitivity::CaseInsensitive))
        return "3DH";
    if(mediaPath.contains(" lr", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("_lr", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("lr_", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("lr ", Qt::CaseSensitivity::CaseInsensitive))
        return "LR";
    if(mediaPath.contains(" OverUnder", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("_OverUnder", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("OverUnder_", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("OverUnder ", Qt::CaseSensitivity::CaseInsensitive))
        return "OverUnder";
    if(mediaPath.contains(" 3DV", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("_3DV", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("3DV_", Qt::CaseSensitivity::CaseInsensitive) ||
            mediaPath.contains("3DV ", Qt::CaseSensitivity::CaseInsensitive))
        return "3DV";
    return "off";
}

bool MediaLibraryHandler::isStereo(QString mediaPath) {
    return getStereoMode(mediaPath) != "off";
}
/***
 * Searches library for the ID
 * Returns default if not found
 * **/
LibraryListItem27* MediaLibraryHandler::findItemByID(QString id) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.ID == id)
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [id](const LibraryListItem27& item) {
        return item.ID == id;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}
int MediaLibraryHandler::findItemIndexByID(QString id) {
//    int foundItem = -1;
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.ID == id)
//            return _cachedLibraryItems.indexOf(item);
//    }

    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [id](const LibraryListItem27& item) {
        return item.ID == id;
    });
    if(itr != _cachedLibraryItems.end())
        return itr - _cachedLibraryItems.begin();
    return -1;
}
LibraryListItem27* MediaLibraryHandler::findItemByNameNoExtension(QString nameNoExtension) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.nameNoExtension == nameNoExtension)
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [nameNoExtension](const LibraryListItem27& item) {
        return item.nameNoExtension == nameNoExtension;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryHandler::findItemByName(QString name) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.name == name)
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [name](const LibraryListItem27& item) {
        return item.name == name;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryHandler::findItemByMediaPath(QString mediaPath) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.path == mediaPath)
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [mediaPath](const LibraryListItem27& item) {
        return item.path == mediaPath;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryHandler::findItemByPartialMediaPath(QString partialMediaPath) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.path.startsWith(partialMediaPath))
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//        else if(item.path.endsWith(partialMediaPath))
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialMediaPath](const LibraryListItem27& item) {
        return item.path.startsWith(partialMediaPath) || item.path.endsWith(partialMediaPath);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryHandler::findItemByPartialThumbPath(QString partialThumbPath) {
//    foreach (LibraryListItem27 item, _cachedLibraryItems) {
//        if(item.thumbFile.startsWith(partialThumbPath) || item.thumbFile.endsWith(partialThumbPath))
//            return &_cachedLibraryItems[_cachedLibraryItems.indexOf(item)];
//    }
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialThumbPath](const LibraryListItem27& item) {
        return item.thumbFile.startsWith(partialThumbPath) || item.thumbFile.endsWith(partialThumbPath);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;

}
