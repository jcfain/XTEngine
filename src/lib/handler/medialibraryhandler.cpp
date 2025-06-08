#include "medialibraryhandler.h"

#include "settingshandler.h"
#include "../tool/imagefactory.h"
#include "../lookup/xtags.h"
#include "../tool/file-util.h"
#include "../struct/ScriptInfo.h"

MediaLibraryHandler::MediaLibraryHandler(QObject* parent)
    : QObject(parent)
    , xVideoPreview(this)
{
    _thumbTimeoutTimer.setSingleShot(true);
    connect(this, &MediaLibraryHandler::prepareLibraryLoad, this, &MediaLibraryHandler::onPrepareLibraryLoad);
    connect(this, &MediaLibraryHandler::libraryLoaded, this, [this]() {
        MediaLibraryHandler::startMetadataProcess(SettingsHandler::getForceMetaDataFullProcess() || SettingsHandler::processMetadataOnStart());
    });
    connect(this, &MediaLibraryHandler::metadataProcessEnd, this, [this]() {
        MediaLibraryHandler::startThumbProcess();
    });
}

MediaLibraryHandler::~MediaLibraryHandler()
{
    stopAllSubProcesses();
}

bool MediaLibraryHandler::isLibraryProcessing()
{
    return isLoadingMediaPaths() ||
           isThumbProcessRunning() ||
           isMetadataProcessing() ||
           isThumbCleanupRunning();
}

bool MediaLibraryHandler::isLoadingMediaPaths()
{
    return _loadingLibraryFuture.isRunning();
}

bool MediaLibraryHandler::isThumbProcessRunning()
{
    return _thumbProcessIsRunning;
}

bool MediaLibraryHandler::isMetadataProcessing()
{
    return _metadataFuture.isRunning();
}

bool MediaLibraryHandler::isThumbCleanupRunning()
{
    return m_thumbCleanupFuture.isRunning();
}

bool MediaLibraryHandler::isLibraryItemVideo(LibraryListItem27 item) {
    return item.type == LibraryListItemType::Video || item.type == LibraryListItemType::VR;
}

void MediaLibraryHandler::stopLibraryLoading()
{
    if(_loadingLibraryFuture.isRunning())
    {
        _loadingLibraryFuture.cancel();
        _loadingLibraryFuture.waitForFinished();
        emit libraryLoadingStatus("Loading media stopped");
    }
}

void MediaLibraryHandler::onPrepareLibraryLoad()
{
    LogHandler::Debug("onPrepareLibraryLoad");
    stopAllSubProcesses();
    _mutex.lock();
    _cachedLibraryItems.clear();
    _mutex.unlock();
    //emit libraryLoaded();
    _libraryItemIDTracker = 1;
}

void MediaLibraryHandler::loadLibraryAsync()
{
    if(isLibraryProcessing()) {
        LogHandler::Warn("loadLibraryAsync called when process already running");
        return;
    }
    LogHandler::Debug("loadLibraryAsync");
    onPrepareLibraryLoad();
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
    _loadingLibraryFuture = QtConcurrent::run([this, library, vrLibrary]() {
        on_load_library(library.isEmpty() ? vrLibrary : library, library.isEmpty());
    });
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

    QStringList funscriptsWithMedia;
    foreach (QString path, paths) {
        auto itr = std::find_if(paths.begin(), paths.end(), [path](const QString& item) {
            return path.startsWith(item) && path.length() > item.length();
        });
        if(itr != paths.end())
            continue;

        emit libraryLoadingStatus((vrMode ? "Loading VR media" : "Loading media") + (paths.length() > 1 ? " " + QString::number(paths.indexOf(path) + 1)+"..." : "..."));

        QDirIterator library(path, mediaTypes, QDir::Files, QDirIterator::Subdirectories);


        while (library.hasNext())
        {
            if(_loadingLibraryFuture.isCanceled())
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
            int fileNameExtIndex = fileNameTemp.lastIndexOf('.');
            QString fileNameNoExtension = fileNameExtIndex > -1 ? fileNameTemp.remove(fileNameExtIndex, fileNameTemp.length() -  1) : fileNameTemp;
            QString scriptFile = fileNameNoExtension + ".funscript";
            QString scriptPath;
            int videoPathExtIndex = videoPathTemp.lastIndexOf('.');
            QString pathNoExtension = videoPathExtIndex > -1 ? videoPathTemp.remove(videoPathExtIndex, videoPathTemp.length() - 1) : videoPathTemp;
            fileNameTemp = fileinfo.fileName();
            QString mediaExtension = "*" + (fileNameExtIndex > -1 ? fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameExtIndex)) : "");

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

#if BUILD_QT5
            QString zipFile = pathNoExtension + ".zip";
            if(!QFileInfo::exists(zipFile)) {
                zipFile = nullptr;
            }
#endif
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
            item.hasScript = !scriptPath.isEmpty()
#if BUILD_QT5
                             || !zipFile.isEmpty()
#endif
                ;
            item.mediaExtension = mediaExtension;
            item.thumbFile = nullptr;
#if BUILD_QT5
            item.zipFile = zipFile;
#endif
#if BUILD_QT5
            item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime() : fileinfo.created();
#else
            item.modifiedDate = fileinfo.birthTime();// TODO: test linux because fileinfo.created() doesnt exist anymore.
#endif
            item.duration = 0;
            item.thumbState = ThumbState::Waiting;
            item.metadata.isMFS = false;
            item.libraryPath = path;
            setLiveProperties(item);


            if(!vrMode && !scriptPath.isEmpty())
                funscriptsWithMedia.append(scriptPath);
#if BUILD_QT5
            if(!vrMode && !zipFile.isEmpty())
                funscriptsWithMedia.append(zipFile);
#endif

            onLibraryItemFound(item);
            //emit libraryItemFound(item);
        }
    }

    if(!vrMode)
    {
        emit libraryLoadingStatus("Searching for lone funscripts...");
        foreach (QString path, paths)
        {
            QStringList funscriptTypes = QStringList()
                                         << "*.funscript"
#if BUILD_QT5
                                         << "*.zip"
#endif
                ;
            QDirIterator funscripts(path, funscriptTypes, QDir::Files, QDirIterator::Subdirectories);
            while (funscripts.hasNext())
            {
                if(_loadingLibraryFuture.isCanceled())
                {
                    LogHandler::Debug("libraryStopped 2");
                    emit libraryStopped();
                    return;
                }
                QFileInfo fileinfo(funscripts.next());
                QString fileName = fileinfo.fileName();
                QString fileNameTemp = fileinfo.fileName();
                int fileNameExtIndex = fileName.lastIndexOf('.');
                QString fileNameMfsExtension = fileinfo.completeBaseName();
                QString fileNameNoExtension = XFileUtil::getNameNoExtension(fileName);// fileNameExtIndex > -1 ? fileNameTemp.remove(fileNameExtIndex, fileNameTemp.length() -  1) : fileNameTemp;
                QString mediaExtension = "*." + (fileNameExtIndex > -1 ? fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameExtIndex)) : "");

                QString scriptPath = fileinfo.filePath();
                if(fileName == "I'M DISGUSTING! - belle delphine (ft. my dad) hard.funscript") {
                    LogHandler::Debug("");
                }
                //QString scriptPathTemp = fileinfo.filePath();
                //int scriptPathExtIndex = scriptPathTemp.lastIndexOf('.');
                QString pathNoExtension = fileinfo.absolutePath() + XFileUtil::getSeperator(scriptPath) + fileNameNoExtension;//scriptPathExtIndex > -1 ? scriptPathTemp.remove(scriptPathExtIndex, scriptPathTemp.length() - 1) : scriptPathTemp;
                //QString filenameNoMfsExtension = QString(fileNameNoExtension);
                if(funscriptsWithMedia.contains(scriptPath, Qt::CaseSensitivity::CaseInsensitive))
                    continue;
                int scriptNoExtensionIndex = fileNameMfsExtension.lastIndexOf('.');
                bool isMfs = false;
                if(scriptNoExtensionIndex > -1)
                {
                    //pathNoExtension.remove(scriptNoExtensionIndex, pathNoExtension.length() - 1);
                    //QString scriptMFSExt = pathNoExtension.remove(0, pathNoExtension.length() - (pathNoExtension.length() - scriptNoExtensionIndex));
                    foreach(auto axisName, TCodeChannelLookup::getChannels())
                    {
                        auto track = TCodeChannelLookup::getChannel(axisName);
                        if(track->Dimension != ChannelDimension::Heave &&
                            track->Type != ChannelType::HalfOscillate &&
                            track->Type != ChannelType::None &&
                            fileNameNoExtension.endsWith(track->TrackName, Qt::CaseInsensitive))
                        {
                            isMfs = true;
                            break;
                        }
                    }
                    if(isMfs)
                        fileNameNoExtension = XFileUtil::getNameNoExtension(fileNameNoExtension);
                }
                LibraryListItem27* otherMedia = findItemByNameNoExtension(fileNameNoExtension);
                if(otherMedia)
                {
                    LogHandler::Debug("Script belongs to other media item: "+fileNameNoExtension);
                    continue;
                }
                otherMedia = findItemByAltScript(fileNameNoExtension);
                if(otherMedia)
                {
                    LogHandler::Debug("Script belongs to other media item alt script: "+fileName);
                    continue;
                }

                QString fileDir = fileinfo.dir().path();
                bool isExcluded = false;
                foreach(QString dir, excludedLibraryPaths)
                {
                    if(dir != path && (fileDir.startsWith(dir, Qt::CaseInsensitive)))
                        isExcluded = true;
                }
                if (isExcluded)
                    continue;
#if BUILD_QT5
                QString zipFile = nullptr;
                if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
                {
                    zipFile = scriptPath;
                }
#endif


                LibraryListItem27 item;
                item.type = LibraryListItemType::FunscriptType;
                item.path = scriptPath; // path
                item.name = fileName; // name
                item.nameNoExtension = fileNameNoExtension; //nameNoExtension
                item.script = scriptPath; // script
                item.pathNoExtension = pathNoExtension;
                item.hasScript = true;
                item.mediaExtension = mediaExtension;
#if BUILD_QT5
                item.zipFile = zipFile;
#endif
#if BUILD_QT5
                item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime() : fileinfo.cr_Qt5eated();
#else
                item.modifiedDate = fileinfo.birthTime();// TODO: test linux because fileinfo.created() doesnt exist anymore.
#endif
                item.duration = 0;
                item.thumbState = ThumbState::Waiting;
                item.libraryPath = path;
                item.metadata.isMFS = isMfs;
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
    int scriptPathExtIndex = scriptPathTemp.lastIndexOf('.');
    QString scriptNoExtension = scriptPathExtIndex > -1 ? scriptPathTemp.remove(scriptPathExtIndex, scriptPathTemp.length() - 1) : scriptPathTemp;
    QString scriptNoExtensionTemp = QString(scriptNoExtension);
    QString fileDir = fileinfo.dir().path();
#if BUILD_QT5
    QString zipFile = nullptr;
    if(scriptPath.endsWith(".zip", Qt::CaseInsensitive))
    {
        zipFile = scriptPath;
        scriptPath = nullptr;
    }
#endif
    fileNameTemp = fileinfo.fileName();
    int fileNameExtIndex = fileNameTemp.lastIndexOf('.');
    QString fileNameNoExtension = fileNameExtIndex > -1 ? fileNameTemp.remove(fileNameExtIndex, fileNameTemp.length() -  1) : fileNameTemp;
    fileNameTemp = fileinfo.fileName();
    QString mediaExtension = "*" + (fileNameExtIndex > -1 ? fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameExtIndex)) : "");
    LibraryListItem27 item;
    item.type = LibraryListItemType::FunscriptType;
    item.path = scriptPath; // path
    item.name = fileName; // name
    item.nameNoExtension = fileNameNoExtension; //nameNoExtension
    item.script = scriptPath; // script
    item.pathNoExtension = fileNameNoExtension;
    item.mediaExtension = mediaExtension;
#if BUILD_QT5
    item.zipFile = zipFile;
#endif
#if BUILD_QT5
    item.modifiedDate = fileinfo.birthTime().isValid() ? fileinfo.birthTime() : fileinfo.created();
#else
    item.modifiedDate = fileinfo.birthTime();// TODO: test linux because fileinfo.created() doesnt exist anymore.
#endif
    item.duration = 0;
    item.thumbState = ThumbState::Waiting;
    setLiveProperties(item);

    addItemBack(item);
    return item;
}

void MediaLibraryHandler::stopThumbCleanupProcess()
{
    if(m_thumbCleanupFuture.isRunning())
    {
        m_thumbCleanupFuture.cancel();
        m_thumbCleanupFuture.waitForFinished();
    }
}


void MediaLibraryHandler::stopMetadataProcess()
{
    if(_metadataFuture.isRunning())
    {
        _metadataFuture.cancel();
        _metadataFuture.waitForFinished();
    }
}

void MediaLibraryHandler::startMetadataProcess(bool fullProcess)
{
    if(!_cachedLibraryItems.count() || _metadataFuture.isRunning())
        return;
    _metadataFuture = QtConcurrent::run([this, fullProcess](){
        LogHandler::Debug("Start metadata process");
        emit metadataProcessBegin();
        emit backgroundProcessStateChange("Processing metadata...", -1);
        //XTags tags = SettingsHandler::getXTags();
        bool saveSettings = false;
        auto cachedLibraryItems = _cachedLibraryItems;

        foreach (LibraryListItem27 item, cachedLibraryItems) {
            if(_metadataFuture.isCanceled()) {
                LogHandler::Debug("Cancel metadata process");
                emit metadataProcessEnd();
                emit backgroundProcessStateChange(nullptr, -1);
                return;
            }
            bool metadataChanged = false;

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
        if(SettingsHandler::getForceMetaDataFullProcess() && cachedLibraryItems.length()) {
            SettingsHandler::setForceMetaDataFullProcessComplete();
            saveSettings = true;
        }
        if(saveSettings)
            SettingsHandler::Save();
        LogHandler::Debug("End metadata process");
        emit metadataProcessEnd();
        emit backgroundProcessStateChange(nullptr, -1);
    });
}

void MediaLibraryHandler::startMetadataCleanProcess()
{
    if(_metadataFuture.isRunning())
        return;
    _metadataFuture = QtConcurrent::run([this](){
        bool saveSettings = false;
        auto allMetadata = SettingsHandler::getLibraryListItemMetaData();
        emit backgroundProcessStateChange("Cleaning metadata...", -1);
        auto allMetadatakeys = allMetadata.keys();
        auto allTags = SettingsHandler::getTags();
        foreach (auto key, allMetadatakeys) {
            if(_metadataFuture.isCanceled()) {
                LogHandler::Debug("Cancel metadata clean process");
                emit metadataProcessEnd();
                emit backgroundProcessStateChange(nullptr, -1);
                return;
            }
            auto item = findItemByNameNoExtension(key);
            if(!item) {
                SettingsHandler::removeLibraryListItemMetaData(key);
                saveSettings = true;
            } else {
                bool tagUpdated = false;
                foreach (auto tag, item->metadata.tags) {
                    if(!allTags.contains(tag)) {
                        item->metadata.tags.removeAll(tag);
                        tagUpdated = true;
                        saveSettings = true;
                    }
                }
                if(tagUpdated)
                    SettingsHandler::updateLibraryListItemMetaData(*item, false);
            }
            float percentage = round(((float)allMetadatakeys.indexOf(key)/allMetadatakeys.length()) * 100);
            emit backgroundProcessStateChange("Cleaning metadata", percentage);
        }
        if(saveSettings)
            SettingsHandler::Save();
        emit metadataProcessEnd();
        emit backgroundProcessStateChange(nullptr, -1);

        LogHandler::Debug("End metadata clean process");
    });
}

void MediaLibraryHandler::startMetadata1024Cleanup()
{
    if(_metadataFuture.isRunning())
        return;
    _metadataFuture = QtConcurrent::run([this](){
    emit backgroundProcessStateChange("Cleaning metadata 1024...", -1);
        auto cachedLibraryItems = _cachedLibraryItems;
        bool saveSettings = false;
        foreach (LibraryListItem27 item, cachedLibraryItems) {
            if(_metadataFuture.isCanceled()) {
                LogHandler::Debug("Cancel metadata clean 1024 process");
                emit metadataProcessEnd();
                emit backgroundProcessStateChange(nullptr, -1);
                return;
            }
            if(item.metadata.offset == 1024) {
                saveSettings = true;
                item.metadata.offset = 0;
                SettingsHandler::updateLibraryListItemMetaData(item, false);
                LogHandler::Info("Change offset 1024 for media: "+item.name);
            }

            float percentage = round(((float)cachedLibraryItems.indexOf(item)/cachedLibraryItems.length()) * 100);
            emit backgroundProcessStateChange("Cleaning metadata 1024", percentage);
        }
        if(saveSettings)
            SettingsHandler::Save();
        emit metadataProcessEnd();
        emit backgroundProcessStateChange(nullptr, -1);
        LogHandler::Debug("End metadata clean 1024 process");
    });
}

void MediaLibraryHandler::processMetadata(LibraryListItem27 &item, bool &metadataChanged, QVector<int> &rolesChanged, bool fullProcess)
{
    bool hasExistingMetadata = SettingsHandler::hasLibraryListItemMetaData(item);
    if(!hasExistingMetadata || fullProcess || item.forceProcessMetadata)// path is ID for metadata
    {
        LogHandler::Debug("Process metadata for: "+item.path + " Full process: "+ QString::number(fullProcess));
        if(!fullProcess)
        {
            LogHandler::Debug("Force item: "+QString::number(item.forceProcessMetadata));
            LogHandler::Debug("New item: "+QString::number(!hasExistingMetadata));
        }
        // Did this cause an issue? Need to test data integrety after full process?
        // item.metadata.libraryItemPath = item.path;
        // item.metadata.key = item.nameNoExtension;
        if(!hasExistingMetadata) {
            item.metadata.defaultValues(item.nameNoExtension, item.path);
            metadataChanged = true;
        }
        if(item.path != item.metadata.libraryItemPath) {
            item.metadata.libraryItemPath = item.path;
            metadataChanged = true;
        }
        if(item.nameNoExtension != item.metadata.key) {
            item.metadata.key = item.nameNoExtension;
            metadataChanged = true;
        }
        if(updateToolTip(item)) {
            metadataChanged = true;
            if(!rolesChanged.contains(Qt::ToolTipRole))
                rolesChanged.append(Qt::ToolTipRole);
        }
        if(!item.metadata.dateAdded.isValid()) {
            item.metadata.dateAdded = item.modifiedDate;
        }

        if(item.type != LibraryListItemType::PlaylistInternal)
        {
            if(!item.metadata.isMFS && discoverMFS(item))
            {
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

            if(item.metadata.isMFS && !item.metadata.tags.contains(XTags::MFS)) {
                item.metadata.tags.append(XTags::MFS);
                metadataChanged = true;
            }
            else if(!item.metadata.isMFS && item.metadata.tags.contains(XTags::MFS)) {
                item.metadata.tags.removeAll(XTags::MFS);
                metadataChanged = true;
            }

            LogHandler::Debug("Find alternate scripts");
            auto ogScripts = item.metadata.scripts;
            findAlternateFunscripts(item);
            if(item.metadata.scripts != ogScripts)
                metadataChanged = true;

            LogHandler::Debug("Find subtitles");
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

            LogHandler::Debug("Update tags");
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
                if(!item.hasScript && !item.metadata.isMFS && !item.metadata.tags.contains(XTags::MISSING_SCRIPT)) {
                    item.metadata.tags.append(XTags::MISSING_SCRIPT);
                    metadataChanged = true;
                } else if((item.hasScript  || item.metadata.isMFS) && item.metadata.tags.contains(XTags::MISSING_SCRIPT)) {
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
            if(userTags.contains(XTags::ALTSCRIPT))
            {
                if(item.metadata.hasAlternate && !item.metadata.tags.contains(XTags::ALTSCRIPT)) {
                    item.metadata.tags.append(XTags::ALTSCRIPT);
                    metadataChanged = true;
                } else if(!item.metadata.hasAlternate && item.metadata.tags.contains(XTags::ALTSCRIPT)) {
                    item.metadata.tags.removeAll(XTags::ALTSCRIPT);
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

            item.forceProcessMetadata = false;
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
#if BUILD_QT5
        bool zipScripExists = QFileInfo::exists(item.zipFile);
#else
        bool zipScripExists = false;
#endif
        item.hasScript = scriptExists || zipScripExists;
    }
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
    LogHandler::Debug("[saveSingleThumb]");
    auto item = findItemByID(id);
    if(item && !_thumbProcessIsRunning && item->thumbFile.endsWith(SettingsHandler::getThumbFormatExtension()) && !item->thumbFile.contains(".lock."))
    {
        item->metadata.thumbExtractError = nullptr;
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
        if (isLibraryItemVideo(item) && !item.thumbFileExists && !SettingsHandler::getDisableAutoThumbGeneration())
        {
            saveThumb(item, -1, vrMode);
        }
        else
        {
            if(!item.thumbFileExists && SettingsHandler::getDisableAutoThumbGeneration())
            {
                setThumbState(ThumbState::Unknown, item);
            }
            else
            {
                setThumbState(ThumbState::Ready, item);
            }
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
    LogHandler::Debug("[saveThumb]");
    if(_thumbProcessIsRunning && (item.type == LibraryListItemType::Audio || item.type == LibraryListItemType::FunscriptType || item.thumbFile.contains(".lock.")))
    {
        LogHandler::Debug("[saveThumb] audio or locked");
        setThumbState(ThumbState::Ready, item);
        saveNewThumbs(vrMode);
    }
    else
    {
        LogHandler::Debug("[saveThumb] start");
        setThumbState(ThumbState::Loading, item);
        QString itemID = item.ID;
        QString itemPath = item.path;
        QDir dir; // Make thumb path if doesnt exist
        dir.mkpath(SettingsHandler::getSelectedThumbsDir());
        if(!item.thumbFileExists && !item.metadata.thumbExtractError.isEmpty())
        {
            LogHandler::Debug("[saveThumb] Thumb historically marked as errored. Skipping. Manually extract the thumb if available.");
            onSaveThumb(itemID, vrMode, item.metadata.thumbExtractError);
            return;
        }

        connect(&_thumbTimeoutTimer, &QTimer::timeout, &_thumbTimeoutTimer, [this, itemID, vrMode]() {
            disconnect(&xVideoPreview, nullptr,  nullptr, nullptr);
            onSaveThumb(itemID, vrMode, "Thumb loading timed out.");
        });
        _thumbTimeoutTimer.start(10000);
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
        int cachedIndex = _cachedLibraryItems.indexOf(*item);
        if(cachedIndex == -1) {
            LibraryListItem27 emptyItem;
            emit saveThumbError(emptyItem, vrMode, "Missing media");
            return;
        }
        LibraryListItem27 &cachedItem = _cachedLibraryItems[cachedIndex];
        if(!errorMessage.isEmpty())
        {
            LogHandler::Error("Save thumb error: " + errorMessage);
            item->metadata.thumbExtractError = errorMessage;
            SettingsHandler::updateLibraryListItemMetaData(*item);
            setThumbState(ThumbState::Error, *item);
            emit saveThumbError(cachedItem, vrMode, errorMessage);
        }
        else
        {
            LogHandler::Debug("Thumb saved: " + item->thumbFile);
            ImageFactory::removeCache(item->thumbFile);
            item->metadata.thumbExtractError = nullptr;
            SettingsHandler::updateLibraryListItemMetaData(*item, false);
            setThumbPath(cachedItem);
            setThumbState(ThumbState::Ready, cachedItem);
            emit saveNewThumb(*item, vrMode, item->thumbFile);
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
        LibraryListItem27* itemRef = findItemByReference(&item);
        if(itemRef)
        {
            playlist[index] = *itemRef;
        }
        else
        {
            if(!QFile::exists(item.path)) {
                playlist[index].error = true;
                playlist[index].metadata.toolTip += "\nMedia path not found!";
                continue;
            }
            itemRef = &playlist[index];
            setLiveProperties(*itemRef);
            bool metaDataChanged = false;
            QVector<int> rolesChanged;
            processMetadata(*itemRef, metaDataChanged, rolesChanged);
            setThumbState(itemRef->thumbFileExists ? ThumbState::Ready : ThumbState::Error, *itemRef);
        }
    }

    emit backgroundProcessStateChange(nullptr, -1);
    return playlist;
}

void MediaLibraryHandler::fixPlaylist(QString name)
{
    // auto playlists = SettingsHandler::getPlaylists();
    // auto playlist = playlists.value(name);
    // foreach (auto item, playlist) {
    //     int index = playlist.indexOf(item);
    //     float percentage = round(((float)playlist.indexOf(item)/playlist.length()) * 100);
    //     emit backgroundProcessStateChange("Fixing playlist", percentage);
    //     LibraryListItem27& itemRef = playlist[index];
    //     setLiveProperties(itemRef);
    //     bool metaDataChanged = false;
    //     QVector<int> rolesChanged;
    //     processMetadata(itemRef, metaDataChanged, rolesChanged);
    //     setThumbState(itemRef.thumbFileExists ? ThumbState::Ready : ThumbState::Error, itemRef);
    // }
}

LibraryListItem27 MediaLibraryHandler::setupPlaylistItem(QString playlistName)
{
    LibraryListItem27 item;
    item.type = LibraryListItemType::PlaylistInternal;
    item.nameNoExtension = playlistName; //nameNoExtension
    item.modifiedDate = QDateTime::currentDateTime();
    item.duration = 0;
    item.metadata.isMFS = false;
    setThumbState(ThumbState::Ready, item);
    setLiveProperties(item);
    addItemFront(item);
    //emit playListItem(item);
    return item;
}

LibraryListItem27 MediaLibraryHandler::setupTempExternalItem(QString mediapath, QString scriptPath, quint64 duration)
{
    LibraryListItem27 item;
    QFileInfo fileInfo(mediapath);
    item.type = LibraryListItemType::External;
    item.name = fileInfo.fileName();
    item.path = mediapath;
    item.script = scriptPath;
    item.hasScript = !scriptPath.isEmpty();
    item.nameNoExtension = XFileUtil::getNameNoExtension(fileInfo.fileName()); //nameNoExtension
    item.modifiedDate = QDateTime::currentDateTime();
    item.duration = duration;
    setLiveProperties(item);
    processMetadata(item);
    addItemBack(item);
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
    if(!isLibraryProcessing()) {
        emit itemAdded(index, _cachedLibraryItems.count());
        //emit libraryChange();
    }
}
void MediaLibraryHandler::addItemBack(LibraryListItem27 item) {
    _mutex.lock();
    _cachedLibraryItems.push_back(item);
    auto index = _cachedLibraryItems.count() - 1;
    _mutex.unlock();
    if(!isLibraryProcessing()) {
        emit itemAdded(index, _cachedLibraryItems.count());
        //emit libraryChange();
    }
}
void MediaLibraryHandler::setLiveProperties(LibraryListItem27 &libraryListItem)
{
    assignID(libraryListItem);
    setThumbPath(libraryListItem);
    if(SettingsHandler::hasLibraryListItemMetaData(libraryListItem)) {
        SettingsHandler::getLibraryListItemMetaData(libraryListItem);
        libraryListItem.metadata.isMFS = libraryListItem.metadata.tags.contains(SettingsHandler::getXTags().MFS);
    } else {
        LogHandler::Debug("New item found: "+libraryListItem.nameNoExtension);
        libraryListItem.metadata.dateAdded = QDateTime::currentDateTime();
        libraryListItem.forceProcessMetadata = true;
    }
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
        int thumbExtIndex = thumbTemp.lastIndexOf('.');
        QString extension = thumbExtIndex > -1 ? thumbTemp.remove(0, thumbTemp.length() - (thumbTemp.length() - thumbExtIndex)) : "";
        QString newName = indexOfSuffix > -1 ? localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".lock"+extension) : localpath + ".lock";
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
#if BUILD_QT5
    bool zipScripExists = QFileInfo::exists(localData.zipFile);
#else
    bool zipScripExists = false;
#endif
    localData.hasScript = scriptExists || zipScripExists;
    if (localData.type != LibraryListItemType::PlaylistInternal && !scriptExists && !zipScripExists)
    {
        localData.metadata.toolTip = localData.nameNoExtension + "\nMedia:";
        localData.metadata.toolTip = localData.path + "\nNo script file of the same name found.";
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

// bool MediaLibraryHandler::discoverMFS1(LibraryListItem27 &item) {
//     auto channels = TCodeChannelLookup::getChannels();
//     QString script;
//     script.reserve(item.pathNoExtension.length() + 1 + 5 + 10);
//     foreach(auto axisName, channels)
//     {
//         auto track = TCodeChannelLookup::getChannel(axisName);
//         if(axisName == TCodeChannelLookup::Stroke() || track->Type == AxisType::HalfOscillate || track->TrackName.isEmpty())
//             continue;

//         script = item.pathNoExtension + "." + track->TrackName + ".funscript";
//         if (QFileInfo::exists(script))
//         {
//             item.hasScript = true;
//             item.metadata.isMFS = true;
//             item.metadata.toolTip += "\n";
//             item.metadata.toolTip += script;
//             item.metadata.MFSScripts << script;
//         }
//     }
//     return item.metadata.isMFS;
// }
bool MediaLibraryHandler::discoverMFS(LibraryListItem27 &item) {
    LogHandler::Debug("Discover MFS: "+item.ID);
    QStringList funscripts = TCodeChannelLookup::getValidMFSExtensions();
    item.metadata.isMFS = false;
    item.metadata.toolTip.clear();
    item.metadata.MFSScripts.clear();
    item.metadata.MFSTracks.clear();
    foreach(auto scriptExtension, funscripts)
    {
        if (QFileInfo::exists(item.pathNoExtension + scriptExtension))
        {
            item.metadata.isMFS = true;
            item.metadata.toolTip += "\n";
            item.metadata.toolTip += item.pathNoExtension + scriptExtension;
            item.metadata.MFSScripts << item.pathNoExtension + scriptExtension;
            item.metadata.MFSTracks << scriptExtension.remove("funscript").remove(".");
        }
    }
    return item.metadata.isMFS;
}

void MediaLibraryHandler::cleanGlobalThumbDirectory() {
    if(isThumbCleanupRunning())
        return;
    m_thumbCleanupFuture = QtConcurrent::run([this]() {
        if(isLoadingMediaPaths() ||
            isThumbProcessRunning() ||
            isMetadataProcessing())
        {
            emit cleanUpThumbsFailed();
            return;
        }
        auto cachedLibraryItems = _cachedLibraryItems;
        emit backgroundProcessStateChange("Cleaning thumbs...", -1);
        foreach(auto libraryListItem, cachedLibraryItems) {
            if(m_thumbCleanupFuture.isCanceled()) {
                LogHandler::Debug("thumb cleanup stopped 1");
                emit backgroundProcessStateChange(nullptr, -1);
                emit cleanUpThumbsFailed();
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
                if(m_thumbCleanupFuture.isCanceled())
                {
                    LogHandler::Debug("thumb cleanup stopped 2");
                    emit backgroundProcessStateChange(nullptr, -1);
                    emit cleanUpThumbsFailed();
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
            if(m_thumbCleanupFuture.isCanceled())
            {
                LogHandler::Debug("thumb cleanup stopped 2");
                emit backgroundProcessStateChange(nullptr, -1);
                emit cleanUpThumbsFailed();
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
        emit cleanUpThumbsFinished();
    });
}

void MediaLibraryHandler::findAlternateFunscripts(LibraryListItem27& item)
{
    if(item.type == LibraryListItemType::FunscriptType || item.type == LibraryListItemType::PlaylistInternal)
        return;
    QList<ScriptInfo> funscriptsWithMedia;
    QString path = item.path;
    if(!QFileInfo::exists(path)) {
        LogHandler::Error("No file found when searching for alternate scripts: "+ item.path);
        item.metadata.scripts = {};
        return;
    }
    QFileInfo fileInfo(path);
    QString location = fileInfo.path();
    QDirIterator scripts(location, QStringList() << "*.funscript"
#if BUILD_QT5
                                                 << "*.zip"
#endif
                         , QDir::Files);

    QString fileName = XFileUtil::getNameNoExtension(path);
    auto channels = TCodeChannelLookup::getChannels();
    ScriptContainerType containerType = ScriptContainerType::BASE;
    LogHandler::Debug("Searching for alternative scripts for: "+ fileName);
    while (scripts.hasNext())
    {
        containerType = ScriptContainerType::BASE;
        QString filepath = scripts.next();
        // QFileInfo scriptInfo(filepath);
        QString trackname = "";
        auto baseName = XFileUtil::getNameNoExtension(filepath);
#if BUILD_QT5
        if(filepath.endsWith(".zip", Qt::CaseInsensitive)) {
            containerType = ScriptContainerType::ZIP;
        }
#endif
        if(filepath == item.script
#if BUILD_QT5
            || filepath == item.zipFile
#endif
            )
        {
            LogHandler::Debug("Is default script");
            funscriptsWithMedia.push_front({"Default", baseName, filepath, trackname, ScriptType::MAIN, containerType, "" });
            continue;
        }
        //Is alternate script?
        if(baseName.startsWith(fileName, Qt::CaseInsensitive)) {
            LogHandler::Debug("Found possible alt script: " + baseName);
            // Ignore mfs files
            LibraryListItem27* otherMedia = findItemByNameNoExtension(baseName);
            if(otherMedia && otherMedia->type != LibraryListItemType::FunscriptType)
            {
                LogHandler::Debug("Belongs to ther media item");
                continue;
            }
            LogHandler::Debug("Is MFS?");
            foreach (auto channel, channels) {
                auto track = TCodeChannelLookup::getChannel(channel);
                if(channel == TCodeChannelLookup::Stroke() || track->Type == ChannelType::HalfOscillate || track->TrackName.isEmpty())
                    continue;
                if(filepath.endsWith("."+track->TrackName+".funscript") && !path.endsWith("."+track->TrackName+".funscript")) {
                    LogHandler::Debug("Is MFS track: " + track->TrackName);
                    trackname = track->TrackName;
                    containerType = ScriptContainerType::MFS;
                    break;
                }
            }
            if(containerType == ScriptContainerType::MFS)
                baseName = XFileUtil::getNameNoExtension(baseName);
            if(containerType == ScriptContainerType::MFS && !baseName.compare(fileName, Qt::CaseInsensitive)) {
                // bool hasTrack = std::find_if(funscriptsWithMedia.begin(), funscriptsWithMedia.end(), [trackname](const ScriptInfo& value) {
                //     return value.track == trackname;
                // }) != funscriptsWithMedia.end();
                LogHandler::Debug("MFS script found: " +trackname);
                funscriptsWithMedia.append({fileName, baseName, filepath, trackname, ScriptType::MAIN, containerType, "" });
            } else if(baseName.compare(fileName, Qt::CaseInsensitive)) {
                auto name = QString(baseName).replace(fileName, "").trimmed();
                LogHandler::Debug("Alt script found: " +name);
                item.metadata.hasAlternate = true;
                funscriptsWithMedia.append({name, baseName, filepath, trackname, ScriptType::ALTERNATE, containerType, "" });
            }
            // else {
            //     LogHandler::Debug("Is default script");
            //     funscriptsWithMedia.push_front({"Default", baseName, filepath, trackname, ScriptType::MAIN, containerType });
            // }
        }
    }
    item.metadata.scripts = funscriptsWithMedia;
}

QList<ScriptInfo> MediaLibraryHandler::filterAlternateFunscriptsForMediaItem(QList<ScriptInfo> scriptInfos)
{
    QList<ScriptInfo> scriptInfosRet;
    foreach(auto scriptInfo, scriptInfos)
    {
        if(!QFileInfo::exists(scriptInfo.path)) {
            LogHandler::Error("Missing script file when grabbing alternate scripts, Try updating the metadata for the media item with path: "+ scriptInfo.path);
            continue;
        }
        if((scriptInfo.containerType == ScriptContainerType::MFS || scriptInfo.containerType == ScriptContainerType::ZIP) && scriptInfo.type == ScriptType::MAIN)
        {
            continue;
        }
        scriptInfosRet.append(scriptInfo);
    }
    return scriptInfosRet;
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

void MediaLibraryHandler::stopAllSubProcesses()
{
    stopLibraryLoading();
    stopThumbProcess();
    stopMetadataProcess();
    stopThumbCleanupProcess();
}

QString MediaLibraryHandler::getStereoMode(QString mediaPath)
{
    if(mediaPath.contains("360", Qt::CaseSensitivity::CaseInsensitive))
        return "360";
    if(mediaPath.contains("180", Qt::CaseSensitivity::CaseInsensitive))
        return "180";
    if(mediaPath.contains("fisheye190", Qt::CaseSensitivity::CaseInsensitive))
        return "fisheye190";
    if(mediaPath.contains("fisheye", Qt::CaseSensitivity::CaseInsensitive))
        return "fisheye";
    if(mediaPath.contains("mkx200", Qt::CaseSensitivity::CaseInsensitive))
        return "mkx200";
    if(mediaPath.contains("vrca220", Qt::CaseSensitivity::CaseInsensitive))
        return "vrca220";
    return "flat";
}

QString MediaLibraryHandler::getScreenType(QString mediaPath)
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
    return getScreenType(mediaPath) != "off" || getStereoMode(mediaPath) != "flat";
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
        return !item.nameNoExtension.compare(nameNoExtension, Qt::CaseInsensitive);
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
        return !item.name.compare(name, Qt::CaseInsensitive);
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
        return !item.path.compare(mediaPath, Qt::CaseInsensitive);
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
        return item.path.startsWith(partialMediaPath, Qt::CaseInsensitive) || item.path.endsWith(partialMediaPath, Qt::CaseInsensitive);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryHandler::findItemByThumbPath(QString thumbPath) {

    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [thumbPath](const LibraryListItem27& item) {
        return !item.thumbFile.compare(thumbPath, Qt::CaseInsensitive);
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
        return item.thumbFile.startsWith(partialThumbPath, Qt::CaseInsensitive) || item.thumbFile.endsWith(partialThumbPath, Qt::CaseInsensitive);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;

}

LibraryListItem27 *MediaLibraryHandler::findItemBySubtitle(QString subtitle)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [subtitle](const LibraryListItem27& item) {
        return !item.metadata.subtitle.compare(subtitle, Qt::CaseInsensitive);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByPartialSubtitle(QString partialSubtitle)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [partialSubtitle](const LibraryListItem27& item) {
        return item.metadata.subtitle.startsWith(partialSubtitle, Qt::CaseInsensitive) || item.metadata.subtitle.endsWith(partialSubtitle, Qt::CaseInsensitive);
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByAltScript(QString value)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [value](const LibraryListItem27& item) {
            auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& script) {
                return script.type == ScriptType::ALTERNATE && !script.filename.compare(value, Qt::CaseInsensitive);
            });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByPartialAltScript(QString value)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [value](const LibraryListItem27& item) {
        auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& item) {
            return item.type == ScriptType::ALTERNATE && (item.filename.startsWith(value, Qt::CaseInsensitive) || item.filename.endsWith(value, Qt::CaseInsensitive));
        });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByMetadataKey(QString value)
{
    const QMutexLocker locker(&_mutex);
    auto itr = std::find_if(_cachedLibraryItems.begin(), _cachedLibraryItems.end(), [value](const LibraryListItem27& item) {
        return item.metadata.key == value;
    });
    if(itr != _cachedLibraryItems.end())
        return &_cachedLibraryItems[itr - _cachedLibraryItems.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryHandler::findItemByReference(const LibraryListItem27 *playListItem)
{
    auto item = findItemByID(playListItem->ID);
    if(!item) {
        item = findItemByNameNoExtension(playListItem->nameNoExtension);
    }
    if(!item) {
        item = findItemByMediaPath(playListItem->path);
    }
    return item;
}
