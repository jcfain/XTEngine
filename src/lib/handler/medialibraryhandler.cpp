#include "medialibraryhandler.h"

#include "settingshandler.h"
#include "../tool/imagefactory.h"
#include "../lookup/xtags.h"

MediaLibraryHandler::MediaLibraryHandler(QObject* parent)
    : QObject(parent),
    xVideoPreview(this)
{
    _thumbTimeoutTimer.setSingleShot(true);
    connect(this, &MediaLibraryHandler::prepareLibraryLoad, this, &MediaLibraryHandler::onPrepareLibraryLoad);
    connect(this, &MediaLibraryHandler::libraryLoaded, this, &MediaLibraryHandler::onLibraryLoaded);
    connect(this, &MediaLibraryHandler::thumbProcessEnd, this, [this]() {
        MediaLibraryHandler::startMetadataProcess(SettingsHandler::getForceMetaDataFullProcess());
    });
}

MediaLibraryHandler::~MediaLibraryHandler()
{
    stopLibraryLoading();
    stopThumbProcess();
    stopMetadataProcess();
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
        emit libraryLoadingStatus("Loading media stopped");
    }
}

void MediaLibraryHandler::onPrepareLibraryLoad()
{
    stopThumbProcess();
    stopMetadataProcess();
    _mutex.lock();
    _cachedLibraryItems.clear();
    _mutex.unlock();
    emit libraryLoaded();
    _libraryItemIDTracker = 1;
}

void MediaLibraryHandler::loadLibraryAsync()
{
    if(isLibraryLoading())
        return;
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
        QStringList nonExistingLibraries;
        foreach (QString path, paths) {
            if(QFileInfo::exists(path))
            {
                anyExist = true;
            } else {
                nonExistingLibraries << path;
            }
        }
        if(!anyExist) {
            if(vrMode) {
                emit libraryLoaded();
                return;
            }
            emit noLibraryFound();
            emit libraryStopped();
            return;
        } else if(!nonExistingLibraries.isEmpty()) {
            emit libraryNotFound(nonExistingLibraries);
        }
    }

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
            QString pathNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() - 1);
            fileNameTemp = fileinfo.fileName();
            QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));

            // if (SettingsHandler::getSelectedFunscriptLibrary() == Q_NULLPTR)
            // {
                scriptPath = pathNoExtension + ".funscript";// Not used
            // }
            // else //Not used
            // {
            //     pathNoExtension = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + fileNameNoExtension;
            //     scriptPath = SettingsHandler::getSelectedFunscriptLibrary() + QDir::separator() + scriptFile;
            // }
            if (!QFileInfo::exists(scriptPath))
            {
                scriptPath = nullptr;
            }
            LibraryListItemType libratyItemType = vrMode || isStereo(fileName) ? LibraryListItemType::VR : LibraryListItemType::Video;
            QString zipFile = pathNoExtension + ".zip";
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
            item.pathNoExtension = pathNoExtension;
            item.hasScript = !scriptPath.isEmpty() || !zipFile.isEmpty();
            item.mediaExtension = mediaExtension;
            item.thumbFile = nullptr;
            item.zipFile = zipFile;
            item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
            item.duration = 0;
            item.thumbState = ThumbState::Waiting;
            item.metadata.isMFS = false;
            item.libraryPath = path;
            setLiveProperties(item);

            SettingsHandler::getLibraryListItemMetaData(item);
            item.metadata.isMFS = item.metadata.tags.contains(SettingsHandler::getXTags().MFS);

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
                foreach(auto axisName, TCodeChannelLookup::getChannels())
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
                item.pathNoExtension = scriptNoExtension;
                item.hasScript = true;
                item.mediaExtension = mediaExtension;
                item.zipFile = zipFile;
                item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
                item.duration = 0;
                item.thumbState = ThumbState::Waiting;
                item.libraryPath = path;
                item.metadata.isMFS = false;
                setLiveProperties(item);
                onLibraryItemFound(item);
                //emit libraryItemFound(item);
            }
        }
    }
    if(vrMode) {
        emit libraryLoaded();
    }
    else
    {
        if(!hasVRLibrary)
        {
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
    item.pathNoExtension = fileNameNoExtension;
    item.mediaExtension = mediaExtension;
    item.zipFile = zipFile;
    item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime().date() : fileinfo.created().date();
    item.duration = 0;
    item.thumbState = ThumbState::Waiting;
    setLiveProperties(item);

    addItemBack(item);
    return item;
}


void MediaLibraryHandler::stopMetadataProcess()
{
    if(_metadataFuture.isRunning()) {
        _metadataFuture.cancel();
        _metadataFuture.waitForFinished();
    }
}

void MediaLibraryHandler::startMetadataProcess(bool fullProcess)
{
    if(!_cachedLibraryItems.count())
        return;
    LogHandler::Debug("Start metadata process");
    emit metadataProcessBegin();
    emit backgroundProcessStateChange("Processing metadata...", -1);
    _metadataFuture = QtConcurrent::run([this, fullProcess](){
        XTags tags = SettingsHandler::getXTags();
        bool saveSettings = false;
        auto cachedLibraryItems = _cachedLibraryItems;
        auto allMetadata = SettingsHandler::getLibraryListItemMetaData();
        emit backgroundProcessStateChange("Cleaning metadata...", -1);
        auto allMetadatakeys = allMetadata.keys();
        auto allTags = SettingsHandler::getTags();
        bool metadataChanged = false;
        foreach (auto key, allMetadatakeys) {
            if(_metadataFuture.isCanceled()) {
                LogHandler::Debug("Cancel metadata process");
                emit metadataProcessEnd();
                emit backgroundProcessStateChange(nullptr, -1);
                return;
            }
            if(!QFileInfo::exists(key)) {
                SettingsHandler::removeLibraryListItemMetaData(key);
                saveSettings = true;
            }
            foreach (auto tag, allMetadata[key].tags) {
                if(!allTags.contains(tag)) {
                    allMetadata[key].tags.removeAll(tag);
                    metadataChanged = true;
                }
            }
            float percentage = round(((float)allMetadatakeys.indexOf(key)/allMetadatakeys.length()) * 100);
            emit backgroundProcessStateChange("Cleaning metadata", percentage);
        }

        foreach (LibraryListItem27 item, cachedLibraryItems) {
            if(_metadataFuture.isCanceled()) {
                LogHandler::Debug("Cancel metadata process");
                emit metadataProcessEnd();
                emit backgroundProcessStateChange(nullptr, -1);
                return;
            }

            QVector<int> rolesChanged;
            processMetadata(item, metadataChanged, rolesChanged, fullProcess);
            if(rolesChanged.count()) {
                updateItem(item, rolesChanged);
            }

            if(metadataChanged) {
                saveSettings = true;
                SettingsHandler::updateLibraryListItemMetaData(item, false);
            }

            float percentage = round(((float)cachedLibraryItems.indexOf(item)/cachedLibraryItems.length()) * 100);
            emit backgroundProcessStateChange("Processing metadata", percentage);
        }
        if(saveSettings)
            SettingsHandler::Save();
        LogHandler::Debug("End metadata process");
        emit metadataProcessEnd();
        emit backgroundProcessStateChange(nullptr, -1);
        SettingsHandler::setForceMetaDataFullProcessComplete();
    });
}

void MediaLibraryHandler::processMetadata(LibraryListItem27 &item, bool &metadataChanged, QVector<int> &rolesChanged, bool fullProcess)
{
    bool hasExistingMetadata = SettingsHandler::hasLibraryListItemMetaData(item);
    if(!hasExistingMetadata || fullProcess)// path is ID for metadata
    {
        if(updateToolTip(item)) {
            metadataChanged = true;
            if(!rolesChanged.contains(Qt::ToolTipRole))
                rolesChanged.append(Qt::ToolTipRole);
        }

        if(item.type != LibraryListItemType::PlaylistInternal)
        {
            if(!item.metadata.isMFS && discoverMFS2(item)) {
                metadataChanged = true;
                if(!rolesChanged.contains(Qt::DisplayRole))
                    rolesChanged.append(Qt::DisplayRole);
                if(!rolesChanged.contains(Qt::ToolTipRole))
                    rolesChanged.append(Qt::ToolTipRole);
                if(!rolesChanged.contains(Qt::ForegroundRole))
                    rolesChanged.append(Qt::ForegroundRole);
                if(!rolesChanged.contains(Qt::FontRole))
                    rolesChanged.append(Qt::FontRole);
            }

            foreach(QString type, SettingsHandler::getSubtitleExtensions())
            {
                QString subtitilePath = item.pathNoExtension + "."+ type;
                if(QFileInfo::exists(subtitilePath))
                {
                    metadataChanged = true;
                    item.metadata.subtitle = subtitilePath;
                    if(!rolesChanged.contains(Qt::DecorationRole))
                        rolesChanged.append(Qt::DecorationRole);
                    break;
                }
            }
            if(item.metadata.isMFS && !item.metadata.tags.contains(XTags::MFS)) {
                item.metadata.tags.append(XTags::MFS);
                metadataChanged = true;
            }
            else if(!item.metadata.isMFS && item.metadata.tags.contains(XTags::MFS)) {
                item.metadata.tags.removeAll(XTags::MFS);
                metadataChanged = true;
            }

            auto userTags = SettingsHandler::getTags();

            if(item.type == LibraryListItemType::Video &&
                userTags.contains(XTags::VIDEO_2D) &&
                !item.metadata.tags.contains(XTags::VIDEO_2D)) {
                item.metadata.tags.append(XTags::VIDEO_2D);
                metadataChanged = true;
            } else if(item.type == LibraryListItemType::VR &&
                       userTags.contains(XTags::VR) &&
                       !item.metadata.tags.contains(XTags::VR)) {
                item.metadata.tags.append(XTags::VR);
                metadataChanged = true;
            } else if(item.type == LibraryListItemType::Audio &&
                       userTags.contains(XTags::AUDIO) &&
                       !item.metadata.tags.contains(XTags::AUDIO)) {
                item.metadata.tags.append(XTags::AUDIO);
                metadataChanged = true;
            } else if(item.type == LibraryListItemType::FunscriptType &&
                       userTags.contains(XTags::FUNSCRIPT) &&
                       !item.metadata.tags.contains(XTags::FUNSCRIPT)) {
                item.metadata.tags.append(XTags::FUNSCRIPT);
                metadataChanged = true;
            }
            // Playlists do not have a way to link to metadata atm.
            // else if(item.type == LibraryListItemType::PlaylistInternal && !metadata.tags.contains(tags.PLAYLIST_INTERNAL)) {
            //     metadata.tags.append(tags.PLAYLIST_INTERNAL);
            //     metadataChanged = true;
            // }


            if(userTags.contains(XTags::SUBTITLE))
            {
                if(!item.metadata.subtitle.isEmpty() &&
                    !item.metadata.tags.contains(XTags::SUBTITLE)) {
                    item.metadata.tags.append(XTags::SUBTITLE);
                    metadataChanged = true;
                } else if(item.metadata.subtitle.isEmpty() && item.metadata.tags.contains(XTags::SUBTITLE)) {
                    item.metadata.tags.removeAll(XTags::SUBTITLE);
                    metadataChanged = true;
                }
            }
            if(userTags.contains(XTags::MISSING_SCRIPT))
            {
                if(!item.hasScript && !item.metadata.tags.contains(XTags::MISSING_SCRIPT)) {
                    item.metadata.tags.append(XTags::MISSING_SCRIPT);
                    metadataChanged = true;
                } else if(item.hasScript && item.metadata.tags.contains(XTags::MISSING_SCRIPT)) {
                    item.metadata.tags.removeAll(XTags::MISSING_SCRIPT);
                    metadataChanged = true;
                }
            }
            if(userTags.contains(XTags::HAS_SCRIPT))
            {
                if(item.hasScript && !item.metadata.tags.contains(XTags::HAS_SCRIPT)) {
                    item.metadata.tags.append(XTags::HAS_SCRIPT);
                    metadataChanged = true;
                } else if(!item.hasScript && item.metadata.tags.contains(XTags::HAS_SCRIPT)) {
                    item.metadata.tags.removeAll(XTags::HAS_SCRIPT);
                    metadataChanged = true;
                }
            }

            if(!item.metadata.tags.contains(XTags::VIEWED) && !item.metadata.tags.contains(XTags::UNVIEWED)) {
                item.metadata.tags.append(XTags::UNVIEWED);
                metadataChanged = true;
            } else if(item.metadata.tags.contains(XTags::VIEWED) && item.metadata.tags.contains(XTags::UNVIEWED)) {
                item.metadata.tags.removeAll(XTags::UNVIEWED);
                metadataChanged = true;
            }

            foreach (QString tag, SettingsHandler::getUserSmartTags()) {
                bool isMatch = false;
                foreach (auto script, item.metadata.MFSScripts) {
                    if(script.contains(tag, Qt::CaseInsensitive)) {
                        isMatch = true;
                        break;
                    }
                }
                if(!item.metadata.tags.contains(tag) && (isMatch || item.path.contains(tag, Qt::CaseInsensitive) || item.script.contains(tag, Qt::CaseInsensitive))) {
                    item.metadata.tags.append(tag);
                    metadataChanged = true;
                }
            }
            // metadata.tags.removeAll(tags.VIEWED);
            // metadata.tags.removeAll(tags.UNVIEWED);
            // metadata.tags.append(tags.UNVIEWED);
            // metadataChanged = true;

            // Waaaay too slow.
            // if(item.md5.isEmpty()) {
            //     item.md5 = XMath::calculateMD5(item.path);
            //     itemChanged = true;
            // }

        }
    } else {
        // if(!rolesChanged.contains(Qt::ToolTipRole))
        //     rolesChanged.append(Qt::ToolTipRole);
        // if(!rolesChanged.contains(Qt::ForegroundRole))
        //     rolesChanged.append(Qt::ForegroundRole);
        bool scriptExists = QFileInfo::exists(item.script);
        bool zipScripExists = QFileInfo::exists(item.zipFile);
        item.hasScript = scriptExists || zipScripExists;
    }
}

void MediaLibraryHandler::onLibraryLoaded()
{
    startThumbProcess();

}

void MediaLibraryHandler::startThumbProcess(bool vrMode)
{
    LogHandler::Debug("Start thumb process, vrMode: " + QString::number(vrMode));
    if(!vrMode)
    {
        emit thumbProcessBegin();
        emit backgroundProcessStateChange("Processing thumbs...", -1);
    }
    QString thumbPath = SettingsHandler::getSelectedThumbsDir();
    QDir thumbDir(thumbPath);
    if (!thumbDir.exists())
    {
        thumbDir.mkdir(thumbPath);
    }
    stopThumbProcess();
    _thumbProcessIsRunning = true;
    saveNewThumbs(vrMode);
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
//        if(_extractor) {
//            disconnect(_extractor, nullptr,  nullptr, nullptr);
//            delete _extractor;
//            _extractor = 0;
//        }
//        delete _thumbNailPlayer;
    }
}

void MediaLibraryHandler::saveSingleThumb(QString id, qint64 position)
{
    auto item = findItemByID(id);
    if(item && !_thumbProcessIsRunning && item->thumbFile.endsWith(SettingsHandler::getThumbFormatExtension()) && !item->thumbFile.contains(".lock."))
    {
        saveThumb(*item, position);
    }
}

bool MediaLibraryHandler::thumbProcessRunning()
{
    return _thumbProcessIsRunning;
}

void MediaLibraryHandler::saveNewThumbs(bool vrMode)
{
    if (_thumbProcessIsRunning && _thumbNailSearchIterator < _cachedLibraryItems.count())
    {
        LibraryListItem27 &item = _cachedLibraryItems[_thumbNailSearchIterator];
        emit backgroundProcessStateChange("Processing thumbs", round((_thumbNailSearchIterator/(float)_cachedLibraryItems.length())*100));
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

            emit backgroundProcessStateChange(nullptr, -1);
            emit thumbProcessEnd();// Must be callsed after backgroundProcessStateChange for queue order
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
        QString itemID = item.ID;
        QString itemPath = item.path;
        QDir dir; // Make thumb path if doesnt exist
        dir.mkpath(SettingsHandler::getSelectedThumbsDir());


        connect(&_thumbTimeoutTimer, &QTimer::timeout, &_thumbTimeoutTimer, [this, itemID, vrMode]() {
            disconnect(&xVideoPreview, nullptr,  nullptr, nullptr);
            onSaveThumb(itemID, vrMode, "Thumb loading timed out.");
        });
        _thumbTimeoutTimer.start(30000);
        //        if(!vrMode)
        // Webhandler
        emit saveNewThumbLoading(item);
        // Get the duration and randomize the position with in the video.
        QString videoFile = item.path;
        LogHandler::Debug("Getting thumb: " + item.thumbFile);
        connect(&xVideoPreview, &XVideoPreview::durationChanged, this,
                [this, videoFile, position](qint64 duration)
                {
                    disconnect(&xVideoPreview, &XVideoPreview::durationChanged,  nullptr, nullptr);
                    LogHandler::Debug("Loaded video for thumb. Duration: " + QString::number(duration));
                    qint64 randomPosition = position > 0 ? position : XMath::random((qint64)1, duration);
                    LogHandler::Debug("Extracting at: " + QString::number(randomPosition));
                    xVideoPreview.extract(videoFile, randomPosition);
                });


        //        connect(_thumbNailPlayer, &AVPlayer::error, _thumbNailPlayer,
        //           [this, cachedListItem, vrMode](QtAV::AVError er)
        //            {
        //                QString error = "Video load error from: " + cachedListItem.path + " Error: " + er.ffmpegErrorString();
        //                onSaveThumb(cachedListItem, vrMode, error);
        //            });


        connect(&xVideoPreview, &XVideoPreview::frameExtracted, this,
                [this, itemID, vrMode](QImage frame)
                {
                    if(!frame.isNull())
                    {
                        LogHandler::Debug("Enter frameExtracted");
                        auto item = findItemByID(itemID);
                        if(!item) {
                            onSaveThumb(itemID, vrMode, "Media item no longer exists: "+itemID);
                            return;
                        }
                        bool hasError = frame.isNull() || !frame.save(item->thumbFile, nullptr, 15);
                        if (hasError)
                        {
                            onSaveThumb(itemID, vrMode, "Error saving thumbnail");
                            return;
                        }

                    }
                    frame = QImage();
                    disconnect(&xVideoPreview, nullptr,  nullptr, nullptr);
                    onSaveThumb(itemID, vrMode);
                });

        connect(&xVideoPreview, &XVideoPreview::frameExtractionError, this,
                [this, itemID, itemPath, vrMode](const QString &errorMessage)
                {
                    disconnect(&xVideoPreview, nullptr,  nullptr, nullptr);
                    QString error = "Error extracting image from: " + itemPath + " Error: " + errorMessage;
                    onSaveThumb(itemID, vrMode, error);
                });

        xVideoPreview.load(videoFile);
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

void MediaLibraryHandler::onSaveThumb(QString itemID, bool vrMode, QString errorMessage)
{
    LibraryListItem27* item = findItemByID(itemID);
    if(!item || item->ID.isEmpty()) {
        LogHandler::Error("onSaveThumb item ID empty.");
    } else {
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
            setThumbState(ThumbState::Error, *item);
            emit saveThumbError(cachedItem, vrMode, errorMessage);
        }
        else
        {
            LogHandler::Debug("Thumb saved: " + item->thumbFile);
            ImageFactory::removeCache(item->thumbFile);
            setThumbPath(cachedItem);
            setThumbState(ThumbState::Ready, cachedItem);
            emit saveNewThumb(item, vrMode, item->thumbFile);
        }
    }
    _thumbTimeoutTimer.stop();
    disconnect(&_thumbTimeoutTimer, &QTimer::timeout, nullptr, nullptr);

    if(_thumbProcessIsRunning)
        saveNewThumbs(vrMode);
    else {
        //disconnect(xVideoPreview.data(), nullptr,  nullptr, nullptr);
//        delete _extractor;
//        _extractor = 0;
    }
}

QList<LibraryListItem27> MediaLibraryHandler::getLibraryCache()
{
    const QMutexLocker locker(&_mutex);
    return _cachedLibraryItems;
}
QList<LibraryListItem27> MediaLibraryHandler::getPlaylist(QString name) {
    auto playlists = SettingsHandler::getPlaylists();
    auto playlist = playlists.value(name);
    foreach (auto item, playlist) {
        int index = playlist.indexOf(item);
        float percentage = round(((float)playlist.indexOf(item)/playlist.length()) * 100);
        emit backgroundProcessStateChange("Loading playlist", percentage);
        LibraryListItem27& itemRef = playlist[index];
        setLiveProperties(itemRef);
        bool metaDataChanged = false;
        QVector<int> rolesChanged;
        processMetadata(itemRef, metaDataChanged, rolesChanged);
        setThumbState(itemRef.thumbFileExists ? ThumbState::Ready : ThumbState::Error, itemRef);
    }

    emit backgroundProcessStateChange(nullptr, -1);
    return playlist;
}
LibraryListItem27 MediaLibraryHandler::setupPlaylistItem(QString playlistName)
{
    LibraryListItem27 item;
    item.type = LibraryListItemType::PlaylistInternal;
    item.nameNoExtension = playlistName; //nameNoExtension
    item.modifiedDate = QDateTime::currentDateTime().date();
    item.duration = 0;
    item.metadata.isMFS = false;
    setThumbState(ThumbState::Ready, item);
    setLiveProperties(item);
    addItemFront(item);
    //emit playListItem(item);
    return item;
}
void MediaLibraryHandler::updateItem(LibraryListItem27 item, QVector<int> roles, bool notify) {
    auto index = findItemIndexByID(item.ID);
    if(index > -1) {
        _mutex.lock();
        LibraryListItem27::copyProperties(item, _cachedLibraryItems[index]);
        _mutex.unlock();
        if(notify) {
            emit itemUpdated(index, {roles});
        }
    }
}

void MediaLibraryHandler::updateItem(int index, QVector<int> roles)
{
    if(index > -1) {
        emit itemUpdated(index, roles);
    }
}

void MediaLibraryHandler::removeFromCache(LibraryListItem27 itemToRemove) {
    auto index = findItemIndexByID(itemToRemove.ID);
    _mutex.lock();
    _cachedLibraryItems.removeOne(itemToRemove);
    _mutex.unlock();
    //if(!isLibraryLoading()) {
        emit itemRemoved(index, _cachedLibraryItems.count());
        //emit libraryChange();
    //}
}
void MediaLibraryHandler::addItemFront(LibraryListItem27 item) {
    _mutex.lock();
    _cachedLibraryItems.push_front(item);
    _mutex.unlock();
    auto index = 0;
    if(!isLibraryLoading()) {
        emit itemAdded(index, _cachedLibraryItems.count());
        //emit libraryChange();
    }
}
void MediaLibraryHandler::addItemBack(LibraryListItem27 item) {
    _mutex.lock();
    _cachedLibraryItems.push_back(item);
    auto index = _cachedLibraryItems.count() - 1;
    _mutex.unlock();
    if(!isLibraryLoading()) {
        emit itemAdded(index, _cachedLibraryItems.count());
        //emit libraryChange();
    }
}
void MediaLibraryHandler::setLiveProperties(LibraryListItem27 &libraryListItem)
{
    assignID(libraryListItem);
    setThumbPath(libraryListItem);
    SettingsHandler::getLibraryListItemMetaData(libraryListItem);
}

void MediaLibraryHandler::lockThumb(LibraryListItem27 &item)
{
    if(!QFile::exists(item.thumbFile))
    {
        LogHandler::Error("File did not exist when locking: "+ item.thumbFile);
        return;
    }
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
                emit itemUpdated(index, {Qt::DecorationRole});
            }
        } else {
            LogHandler::Error("File rename failed: "+ newName);
        }
    }
}
void MediaLibraryHandler::unlockThumb(LibraryListItem27 &item)
{
    if(!QFile::exists(item.thumbFile))
    {
        LogHandler::Error("File did not exist when unlocking: "+ item.thumbFile);
        return;
    }
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
                emit itemUpdated(index, {Qt::DecorationRole});
            }
        } else {
            LogHandler::Error("File NOT renamed: "+ newName);
        }
    }
}

void MediaLibraryHandler::setThumbState(ThumbState state, LibraryListItem27 &item) {
    item.thumbState = state;
    int index = findItemIndexByID(item.ID);
    emit itemUpdated(index, {Qt::DecorationRole});
}

void MediaLibraryHandler::setThumbPath(LibraryListItem27 &libraryListItem)
{
    if(libraryListItem.type == LibraryListItemType::Audio)
    {
        libraryListItem.thumbFile = "://images/icons/audio.png";
        libraryListItem.thumbFileExists = true;
        libraryListItem.managedThumb = true;
        return;
    }
    else if(libraryListItem.type == LibraryListItemType::PlaylistInternal)
    {
        libraryListItem.thumbFile = "://images/icons/playlist.png";
        libraryListItem.thumbFileExists = true;
        libraryListItem.managedThumb = true;
        return;
    }
    else if(libraryListItem.type == LibraryListItemType::FunscriptType)
    {
        libraryListItem.thumbFile = "://images/icons/funscript.png";
        libraryListItem.thumbFileExists = true;
        libraryListItem.managedThumb = true;
        return;
    }
    QFileInfo mediaInfo(libraryListItem.path);
    QString globalPath = SettingsHandler::getSelectedThumbsDir() + libraryListItem.name;

    QString filepathGlobal = globalPath + "." + SettingsHandler::getThumbFormatExtension();
    if(QFileInfo::exists(filepathGlobal))
    {
        libraryListItem.thumbFileExists = true;
        libraryListItem.managedThumb = true;
        libraryListItem.thumbFile = filepathGlobal;
        return;
    }
    QString filepathGlobalLocked = globalPath + ".lock." + SettingsHandler::getThumbFormatExtension();
    if(QFileInfo::exists(filepathGlobalLocked))
    {
        libraryListItem.thumbFileExists = true;
        libraryListItem.managedThumb = true;
        libraryListItem.thumbFile = filepathGlobalLocked;
        return;
    }

    QString absolutePath = mediaInfo.absolutePath() + QDir::separator() + libraryListItem.nameNoExtension;
    QStringList imageExtensions = SettingsHandler::getImageExtensions();
    foreach(QString ext, imageExtensions) {
        QString filepathLocked = absolutePath + ".lock." + ext;
        if(QFileInfo::exists(filepathLocked))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.managedThumb = false;
            libraryListItem.thumbFile = filepathLocked;
            return;
        }
        QString filepath = absolutePath + "." + ext;
        if(QFileInfo::exists(filepath))
        {
            libraryListItem.thumbFileExists = true;
            libraryListItem.managedThumb = false;
            libraryListItem.thumbFile = filepath;
            return;
        }
    }

    if(SettingsHandler::getUseMediaDirForThumbs())
    {
        libraryListItem.thumbFile = mediaInfo.absolutePath() + libraryListItem.nameNoExtension + "." + SettingsHandler::getThumbFormatExtension();
        libraryListItem.thumbFileExists = QFileInfo::exists(libraryListItem.thumbFile);
        libraryListItem.managedThumb = true;
        return;
    }

    libraryListItem.managedThumb = true;
    libraryListItem.thumbFile = SettingsHandler::getSelectedThumbsDir() + libraryListItem.name + "." + SettingsHandler::getThumbFormatExtension();
    libraryListItem.thumbFileExists = QFileInfo::exists(libraryListItem.thumbFile);

}

bool MediaLibraryHandler::updateToolTip(LibraryListItem27 &localData)
{
    localData.metadata.isMFS = false;
    bool itemChanged = false;
    bool scriptExists = QFileInfo::exists(localData.script);
    bool zipScripExists = QFileInfo::exists(localData.zipFile);
    localData.hasScript = scriptExists || zipScripExists;
    if (localData.type != LibraryListItemType::PlaylistInternal && !scriptExists && !zipScripExists)
    {
        localData.metadata.toolTip = localData.nameNoExtension + "\nMedia:";
        localData.metadata.toolTip = localData.path + "\nNo script file of the same name found.\nRight click and Play with chosen funscript.";
        itemChanged = true;
    }
    else if (localData.type != LibraryListItemType::PlaylistInternal)
    {
        localData.metadata.toolTip = localData.nameNoExtension + "\nMedia:";
        localData.metadata.toolTip += "\n";
        localData.metadata.toolTip += localData.path;
        localData.metadata.toolTip += "\n";
        localData.metadata.toolTip += "Scripts:\n";
        if(zipScripExists)
        {
            localData.metadata.toolTip += localData.zipFile;
            localData.metadata.isMFS = true;
        }
        else
        {
            localData.metadata.toolTip += localData.script;
        }
        itemChanged = true;
//         if(!SettingsHandler::getMFSDiscoveryDisabled() || MFSDiscovery) {
// //            LogHandler::Debug("before discoverMFS1: "+QString::number(mSecTimer.elapsed() - lastTime));
// //            lastTime = mSecTimer.elapsed();
// //            discoverMFS1(localData);
// //            LogHandler::Debug("after discoverMFS1: "+QString::number(mSecTimer.elapsed() - lastTime));
// //            lastTime = mSecTimer.elapsed();

//             discoverMFS2(localData);

//             if(MFSDiscovery)//Update on tooltip demand
//                 updateItem(localData);
//         } else {
//             localData.toolTip += "Unknown";
//         }
    }
    else if (localData.type == LibraryListItemType::PlaylistInternal)
    {
        localData.metadata.toolTip = localData.nameNoExtension + "\nMedia:";
        auto playlists = SettingsHandler::getPlaylists();
        auto playlist = playlists.value(localData.nameNoExtension);
        for(auto i = 0; i < playlist.length(); i++)
        {
            localData.metadata.toolTip += "\n";
            localData.metadata.toolTip += QString::number(i + 1);
            localData.metadata.toolTip += ": ";
            localData.metadata.toolTip += playlist[i].nameNoExtension;
        }
        itemChanged = true;
    }
    return itemChanged;
}

bool MediaLibraryHandler::discoverMFS1(LibraryListItem27 &item) {
    auto channels = TCodeChannelLookup::getChannels();
    QString script;
    script.reserve(item.pathNoExtension.length() + 1 + 5 + 10);
    foreach(auto axisName, channels)
    {
        auto track = TCodeChannelLookup::getChannel(axisName);
        if(axisName == TCodeChannelLookup::Stroke() || track->Type == AxisType::HalfOscillate || track->TrackName.isEmpty())
            continue;

        script = item.pathNoExtension + "." + track->TrackName + ".funscript";
        if (QFileInfo::exists(script))
        {
            item.hasScript = true;
            item.metadata.isMFS = true;
            item.metadata.toolTip += "\n";
            item.metadata.toolTip += script;
            item.metadata.MFSScripts << script;
        }
    }
    return item.metadata.isMFS;
}
bool MediaLibraryHandler::discoverMFS2(LibraryListItem27 &item) {
    QStringList funscripts = TCodeChannelLookup::getValidMFSExtensions();
    foreach(auto scriptExtension, funscripts)
    {
        if (QFileInfo::exists(item.pathNoExtension + scriptExtension))
        {
            item.hasScript = true;
            item.metadata.isMFS = true;
            item.metadata.toolTip += "\n";
            item.metadata.toolTip += item.pathNoExtension + scriptExtension;
            item.metadata.MFSScripts << item.pathNoExtension + scriptExtension;
        }
    }
    return item.metadata.isMFS;
}

void MediaLibraryHandler::cleanGlobalThumbDirectory() {

    if(thumbProcessRunning() || isLibraryLoading())
        return;
    auto cachedLibraryItems = _cachedLibraryItems;
    emit backgroundProcessStateChange("Cleaning thumbs...", -1);
    foreach(auto libraryListItem, cachedLibraryItems) {
        if(_loadingLibraryStop) {
            LogHandler::Debug("thumb cleanup stopped 1");
            emit backgroundProcessStateChange(nullptr, -1);
            emit libraryStopped();
            return;
        }
        emit backgroundProcessStateChange("Cleaning duplicate thumbs:", round((cachedLibraryItems.indexOf(libraryListItem)/(float)cachedLibraryItems.length())*100));
        if(libraryListItem.type != LibraryListItemType::VR && libraryListItem.type != LibraryListItemType::Video)
            continue;
        QString hasGlobal;
        QString hasGlobalLocked;
        QString hasLocal;
        QString hasLocalLocked;
        QFileInfo mediaInfo(libraryListItem.path);
        QString globalPath = SettingsHandler::getSelectedThumbsDir() + libraryListItem.name;

        QString filepathGlobal = globalPath + "." + SettingsHandler::getThumbFormatExtension();
        if(QFileInfo::exists(filepathGlobal))
        {
            hasGlobal = filepathGlobal;
        }
        QString filepathGlobalLocked = globalPath + ".lock." + SettingsHandler::getThumbFormatExtension();
        if(QFileInfo::exists(filepathGlobalLocked))
        {
            hasGlobalLocked = filepathGlobalLocked;
        }

        QString absolutePath = mediaInfo.absolutePath() + QDir::separator() + libraryListItem.nameNoExtension;
        QStringList imageExtensions = SettingsHandler::getImageExtensions();
        foreach(QString ext, imageExtensions) {
            if(_loadingLibraryStop) {
                LogHandler::Debug("thumb cleanup stopped 2");
                emit backgroundProcessStateChange(nullptr, -1);
                emit libraryStopped();
                return;
            }
            QString filepathLocked = absolutePath + ".lock." + ext;
            if(QFileInfo::exists(filepathLocked))
            {
                hasLocalLocked = filepathLocked;
            }
            QString filepath = absolutePath + "." + ext;
            if(QFileInfo::exists(filepath))
            {
                hasLocal = filepath;
            }
        }
        if((!hasLocal.isEmpty() || !hasLocalLocked.isEmpty()) && (!hasGlobal.isEmpty() || !hasGlobalLocked.isEmpty())) {
            if(!hasGlobal.isEmpty())
                QFile::remove(hasGlobal);
            if(!hasGlobalLocked.isEmpty())
                QFile::remove(hasGlobalLocked);
            if(!hasLocal.isEmpty()) {
                libraryListItem.thumbFile = hasLocal;
            } else if(!hasLocalLocked.isEmpty()) {
                libraryListItem.thumbFile = hasLocalLocked;
            }
            updateItem(cachedLibraryItems.indexOf(libraryListItem), {Qt::DecorationRole});
        }
    }

    QDir dir(SettingsHandler::getSelectedThumbsDir(),"*." + SettingsHandler::getThumbFormatExtension(), QDir::NoSort, QDir::Filter::Files);
    int fileCount = dir.count();
    QDirIterator thumbs(SettingsHandler::getSelectedThumbsDir(), QStringList() << "*." + SettingsHandler::getThumbFormatExtension(), QDir::Files, QDirIterator::Subdirectories);

    qint64 currentFileCount = 0;
    while (thumbs.hasNext())
    {
        if(_loadingLibraryStop) {
            LogHandler::Debug("thumb cleanup stopped 3");
            emit backgroundProcessStateChange(nullptr, -1);
            emit libraryStopped();
            return;
        }
        emit backgroundProcessStateChange("Cleaning stale thumbs", round((currentFileCount/(float)fileCount)*100));
        QString filepath = thumbs.next();
//        QFileInfo fileinfo(filepath);
//        QString fileName = fileinfo.fileName();
        if(!findItemByThumbPath(filepath))
            QFile::remove(filepath);
        currentFileCount++;
    }
    emit backgroundProcessStateChange(nullptr, -1);
}

void MediaLibraryHandler::findAlternateFunscripts(QString path)
{
    if(!QFileInfo::exists(path)) {
        LogHandler::Error("No file found when searching for alternate scripts: "+ path);
        return;
    }
    QtConcurrent::run([this, path]() {
        QFileInfo fileInfo(path);
        QString location = fileInfo.path();
        QDirIterator scripts(location, QStringList() << "*.funscript" << "*.zip", QDir::Files);

        QList<ScriptInfo> funscriptsWithMedia;
        QString fileName = fileInfo.baseName();
        auto channels = TCodeChannelLookup::getChannels();
        ScriptContainerType containerType = ScriptContainerType::BASE;
        LogHandler::Debug("Searching for alternative scripts for: "+ fileName);
        while (scripts.hasNext())
        {
            containerType = ScriptContainerType::BASE;
            QString filepath = scripts.next();
            QFileInfo scriptInfo(filepath);
            if(filepath.endsWith(".zip")) {
                containerType = ScriptContainerType::ZIP;
            }
            //Is alternate script?
            auto baseName = scriptInfo.baseName();
            if(baseName.startsWith(fileName)) {
                LogHandler::Debug("Found possible alt script: " + baseName);
                // Ignore mfs files
                LogHandler::Debug("Is MFS?");
                QString trackname;
                foreach (auto channel, channels) {
                    auto track = TCodeChannelLookup::getChannel(channel);
                    if(channel == TCodeChannelLookup::Stroke() || track->Type == AxisType::HalfOscillate || track->TrackName.isEmpty())
                        continue;
                    if(filepath.endsWith("."+track->TrackName+".funscript") && !path.endsWith("."+track->TrackName+".funscript")) {
                        LogHandler::Debug("Is MFS track: " + track->TrackName);
                        trackname = track->TrackName;
                        containerType = ScriptContainerType::MFS;
                        break;
                    }
                }
                if(containerType == ScriptContainerType::MFS) {
                    LogHandler::Debug("MFS script found: " +trackname);
                    funscriptsWithMedia.append({trackname, scriptInfo.baseName(), filepath, ScriptType::MAIN, containerType });
                } else if(scriptInfo.baseName() != fileName) {
                    auto name = scriptInfo.baseName().replace(fileName, "").trimmed();
                    LogHandler::Debug("Alt script found: " +name);
                    funscriptsWithMedia.append({name, scriptInfo.baseName(), filepath, ScriptType::ALTERNATE, containerType });
                } else {
                    LogHandler::Debug("Is default script");
                    funscriptsWithMedia.push_front({"Default", scriptInfo.baseName(), filepath, ScriptType::MAIN, containerType });
                }
            }
        }
        emit alternateFunscriptsFound(funscriptsWithMedia);
    });
}

bool MediaLibraryHandler::metadataProcessing()
{
    return _metadataFuture.isRunning();
}

void MediaLibraryHandler::processMetadata(LibraryListItem27 &item)
{
    bool metadataChanged = false;
    QVector<int> roleChanged;
    processMetadata(item, metadataChanged, roleChanged, true);
    if(roleChanged.count()) {
        updateItem(item, roleChanged);
    }

    if(metadataChanged) {
        SettingsHandler::updateLibraryListItemMetaData(item, true);
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
    const QMutexLocker locker(&_mutex);
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
    const QMutexLocker locker(&_mutex);

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

    const QMutexLocker locker(&_mutex);
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
    const QMutexLocker locker(&_mutex);
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
    const QMutexLocker locker(&_mutex);
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
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialMediaPath](const LibraryListItem27& item) {
        return item.path.startsWith(partialMediaPath) || item.path.endsWith(partialMediaPath);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryHandler::findItemByThumbPath(QString thumbPath) {

    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [thumbPath](const LibraryListItem27& item) {
        return item.thumbFile == thumbPath;
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
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialThumbPath](const LibraryListItem27& item) {
        return item.thumbFile.startsWith(partialThumbPath) || item.thumbFile.endsWith(partialThumbPath);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;

}

LibraryListItem27 *MediaLibraryHandler::findItemBySubtitle(QString subtitle)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [subtitle](const LibraryListItem27& item) {
        return item.metadata.subtitle == subtitle;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByPartialSubtitle(QString partialSubtitle)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialSubtitle](const LibraryListItem27& item) {
        return item.metadata.subtitle.startsWith(partialSubtitle) || item.metadata.subtitle.endsWith(partialSubtitle);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}
