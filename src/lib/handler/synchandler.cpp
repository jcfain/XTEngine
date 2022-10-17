#include "synchandler.h"

SyncHandler::SyncHandler(QObject* parent):
    QObject(parent)
{
    _tcodeHandler = new TCodeHandler(parent);
    _funscriptHandler = new FunscriptHandler(TCodeChannelLookup::Stroke());
}

SyncHandler::~SyncHandler()
{
    delete _funscriptHandler;
    qDeleteAll(_funscriptHandlers);
}

void SyncHandler::togglePause()
{
    if((isLoaded() && _isMediaFunscriptPlaying) || (isLoaded() && _isStandAloneFunscriptPlaying))
    {
        QMutexLocker locker(&_mutex);
        _isPaused = !_isPaused;
        emit togglePaused(isPaused());
    }
}
void SyncHandler::setPause(bool paused)
{
    QMutexLocker locker(&_mutex);
    _isPaused = paused;
    emit togglePaused(isPaused());
}

void SyncHandler::setStandAloneLoop(bool enabled)
{
    QMutexLocker locker(&_mutex);
    _standAloneLoop = enabled;
}

bool SyncHandler::isPaused()
{
    return _isPaused;
}
bool SyncHandler::isPlaying()
{
    return _isVRFunscriptPlaying || _isMediaFunscriptPlaying || _isStandAloneFunscriptPlaying;
}
bool SyncHandler::isPlayingStandAlone()
{
    return _isStandAloneFunscriptPlaying;
}
bool SyncHandler::isPlayingVR()
{
    return _isVRFunscriptPlaying;
}

QList<QString> SyncHandler::load(QString scriptFile)
{
    LogHandler::Debug("Enter syncHandler load");
    reset();
    if(!scriptFile.isEmpty())
    {
        QFileInfo scriptInfo(scriptFile);
        if(scriptInfo.exists())
        {
            QString scriptTemp = scriptFile;
            QString scriptFileNoExtension = scriptTemp.remove(scriptTemp.lastIndexOf('.'), scriptTemp.length() -  1);
            QString fileName = scriptInfo.fileName();
            QString scriptNameNoExtension = fileName.remove(fileName.lastIndexOf('.'), scriptTemp.length() -  1);
            if(scriptFile.endsWith(".zip"))
            {
               QZipReader zipFile(scriptFile, QIODevice::ReadOnly);
               if(zipFile.isReadable())
               {
                   QByteArray data = zipFile.fileData(scriptNameNoExtension + ".funscript");
                   if (!data.isEmpty())
                   {
                       if(!load(data))
                       {
                           _invalidScripts.append("Zip file: " + scriptNameNoExtension + ".funscript");
                       }
                   }
                   else
                   {
                       LogHandler::Debug("Main funscript: '"+scriptNameNoExtension + ".funscript' not found in zip");
                   }
               }
            }
            else if(!_funscriptHandler->load(scriptFile))
            {
                _invalidScripts.append(scriptFile);
            }
            loadMFS(scriptFile);
        }
        else
        {
            _invalidScripts.append("File not found: " + scriptFile);
        }
    }
    return _invalidScripts;
}

bool SyncHandler::isLoaded()
{
    return _funscriptHandler->isLoaded() || _funscriptHandlers.length() > 0;
}

void SyncHandler::stopAll()
{
    stopStandAloneFunscript();
    stopInputDeviceFunscript();
    stopOtherMediaFunscript();
}

void SyncHandler::stopStandAloneFunscript()
{
    LogHandler::Debug("Stop standalone sync");
    QMutexLocker locker(&_mutex);
    _currentTime = 0;
    _isStandAloneFunscriptPlaying = false;
    locker.unlock();
    if(_funscriptStandAloneFuture.isRunning())
    {
        _funscriptStandAloneFuture.cancel();
        _funscriptStandAloneFuture.waitForFinished();
        emit funscriptStopped();
    }
}

void SyncHandler::stopOtherMediaFunscript()
{
    LogHandler::Debug("Stop media sync");
    QMutexLocker locker(&_mutex);
    _isMediaFunscriptPlaying = false;
    locker.unlock();
    if(_funscriptMediaFuture.isRunning())
    {
        _funscriptMediaFuture.cancel();
        _funscriptMediaFuture.waitForFinished();
    }
}

void SyncHandler::stopInputDeviceFunscript()
{
    LogHandler::Debug("Stop VR sync");
    QMutexLocker locker(&_mutex);
    _isVRFunscriptPlaying = false;
    locker.unlock();
    if(_funscriptVRFuture.isRunning())
    {
        _funscriptVRFuture.cancel();
        _funscriptVRFuture.waitForFinished();
    }
}

void SyncHandler::clear()
{
    LogHandler::Debug("Clear sync");
    _funscriptHandler->setLoaded(false);
    if(_funscriptHandlers.length() > 0)
    {
        qDeleteAll(_funscriptHandlers);
        _funscriptHandlers.clear();
    }
    QMutexLocker locker(&_mutex);
    _currentTime = 0;
    _standAloneLoop = false;
    _isPaused = false;
    _invalidScripts.clear();
}

void SyncHandler::reset()
{
    LogHandler::Debug("Reset sync");
    stopAll();
    clear();
}

QString SyncHandler::getPlayingStandAloneScript()
{
    return _playingStandAloneFunscript;
}

void SyncHandler::skipToMoneyShot()
{
    if (SettingsHandler::getSkipToMoneyShotPlaysFunscript()) {
        QString funscript = SettingsHandler::getSkipToMoneyShotFunscript();
        if(!funscript.isEmpty())
        {
            QFile file(funscript);
            if(file.exists())
            {
                stopAll();
                load(funscript);
                playStandAlone();
                setStandAloneLoop(SettingsHandler::getSkipToMoneyShotStandAloneLoop());
            }
        }
    }
}

void SyncHandler::playStandAlone(QString funscript) {
    LogHandler::Debug("play Funscript stand alone start thread");
    if(!funscript.isEmpty()) //Override standalone funscript. aka: skip to moneyshot
        load(funscript);
    QMutexLocker locker(&_mutex);
    _currentTime = 0;
    _isPaused = false;
    _standAloneLoop = false;
    _isMediaFunscriptPlaying = true;
    _isStandAloneFunscriptPlaying = true;
    _playingStandAloneFunscript = funscript;
    locker.unlock();
    emit funscriptStarted();
    LogHandler::Debug("playStandAlone start thread");
    _funscriptStandAloneFuture = QtConcurrent::run([this]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QMap<QString, std::shared_ptr<FunscriptAction>> otherActions;
        int secCounter1 = 0;
        int secCounter2 = 0;
        QElapsedTimer mSecTimer;
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        qint64 funscriptMax = getFunscriptMax();
        while (_isStandAloneFunscriptPlaying)
        {
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                if (_inputDeviceHandler && _inputDeviceHandler->isConnected()) {
                    auto currentVRPacket = _inputDeviceHandler->getCurrentPacket();
                    if(currentVRPacket.duration > 0)
                        _isPaused = !currentVRPacket.playing;
                }
                if(!_isPaused && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected())
                {
                    if(_seekTime > -1)
                    {
                        _currentTime = _seekTime;
                    }
                    else
                    {
                        _currentTime++;
                    }
                    if(_funscriptHandler->isLoaded())
                        actionPosition = _funscriptHandler->getPosition(_currentTime);
                    if(actionPosition != nullptr)
                        emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
                    foreach(auto funscriptHandlerOther, _funscriptHandlers)
                    {
                        auto otherAction = funscriptHandlerOther->getPosition(_currentTime);
                        if(otherAction != nullptr)
                        {
                            otherActions.insert(funscriptHandlerOther->channel(), otherAction);
                            emit channelPositionChange(funscriptHandlerOther->channel(), otherAction->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actionPosition, otherActions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    otherActions.clear();
                }
                secCounter2 = round(mSecTimer.elapsed() / 1000);
                if(secCounter2 - secCounter1 >= 1)
                {
                    if(_seekTime == -1 && !_isOtherMediaPlaying)
                        emit funscriptPositionChanged(_currentTime);
                    secCounter1 = secCounter2;
                }
                if(_seekTime > -1)
                    _seekTime = -1;
            }
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
            if(!_standAloneLoop && _currentTime >= funscriptMax)
            {
                _isStandAloneFunscriptPlaying = false;
                if(!_isOtherMediaPlaying)
                    emit funscriptStatusChanged(XMediaStatus::EndOfMedia);
            }
            else if(_standAloneLoop && _currentTime >= funscriptMax)
            {
                _currentTime = 0;
            }
        }
        QMutexLocker locker(&_mutex);
        _isMediaFunscriptPlaying = false;
        _playingStandAloneFunscript = nullptr;
        _currentTime = 0;
        emit funscriptEnded();
        LogHandler::Debug("exit play Funscript stand alone thread");
    });
}

void SyncHandler::setFunscriptTime(qint64 msecs)
{
    QMutexLocker locker(&_mutex);
    _currentTime = msecs;
}

qint64 SyncHandler::getFunscriptTime()
{
    return _currentTime;
}

qint64 SyncHandler::getFunscriptMin()
{
    return _funscriptHandler->getMin();
}

qint64 SyncHandler::getFunscriptNext()
{
    return _funscriptHandler->getNext();
}

qint64 SyncHandler::getFunscriptMax()
{
    if(_funscriptHandler->getMax() > -1)
        return _funscriptHandler->getMax();
    qint64 otherMax = -1;
    foreach(auto handler, _funscriptHandlers)
    {
        auto max = handler->getMax();
        if(max > -1 && max > otherMax)
            otherMax = max;
    }
    return otherMax;
}

void SyncHandler::syncOtherMediaFunscript(std::function<qint64()> getMediaPosition)
{
    stopAll();
    QMutexLocker locker(&_mutex);
    _isMediaFunscriptPlaying = true;
    //emit funscriptStatusChanged(QtAV::MediaStatus::LoadedMedia);
    LogHandler::Debug("syncFunscript start thread");
    _funscriptMediaFuture = QtConcurrent::run([this, getMediaPosition]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QMap<QString, std::shared_ptr<FunscriptAction>> otherActions;
        QElapsedTimer mSecTimer;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        while (_isMediaFunscriptPlaying && _isOtherMediaPlaying)
        {
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                if(!_isPaused && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected())
                {
                    qint64 currentTime = getMediaPosition();//_videoHandler->position();
                    actionPosition = _funscriptHandler->getPosition(currentTime);
                    if(actionPosition != nullptr)
                        emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
                    foreach(auto funscriptHandlerOther, _funscriptHandlers)
                    {
                        auto otherAction = funscriptHandlerOther->getPosition(currentTime);
                        if(otherAction != nullptr)
                        {
                            otherActions.insert(funscriptHandlerOther->channel(), otherAction);
                            emit channelPositionChange(funscriptHandlerOther->channel(), otherAction->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actionPosition, otherActions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    otherActions.clear();

                    sendPulse(mSecTimer.elapsed(), nextPulseTime);
                }
            }
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
        }
        _currentLocalVideoTime = 0;
        _isMediaFunscriptPlaying = false;
        emit funscriptEnded();
        LogHandler::Debug("exit syncFunscript");
    });
}

void SyncHandler::syncInputDeviceFunscript(QString funscript)
{
    load(funscript);
    QMutexLocker locker(&_mutex);
    _isVRFunscriptPlaying = true;
    LogHandler::Debug("syncVRFunscript start thread");
    _funscriptVRFuture = QtConcurrent::run([this, funscript]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QMap<QString, std::shared_ptr<FunscriptAction>> otherActions;
        InputDevicePacket currentVRPacket;
        qint64 timeTracker = 0;
        qint64 lastVRTime = 0;
        qint64 lastVRSyncResetTime = 0;
        QElapsedTimer mSecTimer;
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        QString videoPath;
        qint64 duration;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        bool lastStatePlaying = false;
        while (_isVRFunscriptPlaying && _inputDeviceHandler && _inputDeviceHandler->isConnected() && _outputDeviceHandler && _outputDeviceHandler->isConnected() && !_isOtherMediaPlaying)
        {
            currentVRPacket = _inputDeviceHandler->getCurrentPacket();
            if(lastStatePlaying && !currentVRPacket.playing)
                emit sendTCode("DSTOP");
            lastStatePlaying = currentVRPacket.playing;
            //timer.start();
            if(!_isPaused && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected() && isLoaded() && !currentVRPacket.path.isEmpty() && currentVRPacket.duration > 0 && currentVRPacket.playing)
            {
                if(videoPath.isEmpty())
                    videoPath = currentVRPacket.path;
                if(!duration)
                    duration = currentVRPacket.duration;
                //execute once every millisecond
                if (timer2 - timer1 >= 1)
                {
    //                LogHandler::Debug("timer1: "+QString::number(timer1));
    //                LogHandler::Debug("timer2: "+QString::number(timer2));
                    //LogHandler::Debug("timer2 - timer1 "+QString::number(timer2-timer1));
    //                LogHandler::Debug("Out timeTracker: "+QString::number(timeTracker));
                    timer1 = timer2;
                    qint64 vrTime = currentVRPacket.currentTime;
                    if(lastVRTime != vrTime)
                    {
                        //LogHandler::Debug("VR time reset: "+QString::number(currentTime));
                        lastVRTime = vrTime;
                        timeTracker = vrTime;
                    }
                    else
                    {
                        timeTracker++;
//                        LogHandler::Debug("else: " + QString::number(timeTracker));
                        vrTime = timeTracker;
                    }
                    //LogHandler::Debug("funscriptHandler->getPosition: "+QString::number(currentTime));
                    actionPosition = _funscriptHandler->getPosition(vrTime);
                    if(actionPosition != nullptr) {
                        emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
//                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////pos: "+QString::number(actionPosition->pos));
//                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////speed: "+QString::number(actionPosition->speed));
//                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lastPos: "+QString::number(actionPosition->lastPos));
//                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lastSpeed: "+QString::number(actionPosition->lastSpeed));
                    }
                    foreach(auto funscriptHandlerOther, _funscriptHandlers)
                    {
                        auto otherAction = funscriptHandlerOther->getPosition(vrTime);
                        if(otherAction != nullptr)
                        {
                            otherActions.insert(funscriptHandlerOther->channel(), otherAction);
                            emit channelPositionChange(funscriptHandlerOther->channel(), otherAction->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actionPosition, otherActions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    otherActions.clear();
               //     LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
                    sendPulse(timer1, nextPulseTime);
                }
                timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
                //LogHandler::Debug("timer nsecsElapsed: "+QString::number(timer2));
            }

        }

        QMutexLocker locker(&_mutex);
        _isVRFunscriptPlaying = false;
        emit funscriptVREnded(videoPath, funscript, duration);
        LogHandler::Debug("exit syncInputDeviceFunscript");
    });
}

// Private
bool SyncHandler::load(QByteArray funscript)
{
    return _funscriptHandler->load(funscript);
}

bool SyncHandler::loadMFS(QString channel, QString funscript)
{
    FunscriptHandler* otherFunscript = new FunscriptHandler(channel);
    if(!otherFunscript->load(funscript))
        return false;
    else
        _funscriptHandlers.append(otherFunscript);
    return true;
}

bool SyncHandler::loadMFS(QString channel, QByteArray funscript)
{
    FunscriptHandler* otherFunscript = new FunscriptHandler(channel);
    if(!otherFunscript->load(funscript))
        return false;
    else
        _funscriptHandlers.append(otherFunscript);
    return true;
}

void SyncHandler::loadMFS(QString scriptFile)
{
    QString scriptTemp = scriptFile;
    QString scriptFileNoExtension = scriptTemp.remove(scriptTemp.lastIndexOf('.'), scriptTemp.length() -  1);
    QFileInfo scriptFileInfo(scriptFile);
    QZipReader* zipFile = 0;
    if(scriptFile.endsWith(".zip"))
        zipFile = new QZipReader(scriptFile, QIODevice::ReadOnly);

    auto availibleAxis = TCodeChannelLookup::getChannels();
    foreach(auto axisName, availibleAxis)
    {
        auto track = TCodeChannelLookup::getChannel(axisName);
        if(axisName == TCodeChannelLookup::Stroke() || track->Type == AxisType::HalfRange || track->TrackName.isEmpty())
            continue;

        QFileInfo fileInfo(scriptFileNoExtension + "." + track->TrackName + ".funscript");
        if(fileInfo.exists())
        {
            LogHandler::Debug("Loading MFS track: "+ scriptFileNoExtension + "." + track->TrackName + ".funscript");
            if(!loadMFS(axisName, fileInfo.absoluteFilePath()))
                _invalidScripts.append("MFS script: " + fileInfo.absoluteFilePath());
        }
        else if(scriptFile.endsWith(".zip") && zipFile->isReadable())
        {
           QString fileName = scriptFileInfo.fileName();
           QString scriptFileNameNoExtension = fileName.remove(fileName.lastIndexOf('.'), scriptTemp.length() -  1);
           QString trackFileName = scriptFileNameNoExtension + "." + track->TrackName + ".funscript";
           QByteArray data = zipFile->fileData(trackFileName);
           if (!data.isEmpty())
           {
               LogHandler::Debug("Loading MFS track from zip: "+ trackFileName);
               if(!loadMFS(axisName, data))
                   _invalidScripts.append("MFS zip script: " + trackFileName);
           }
        }
    }
    if(zipFile)
        delete zipFile;
}


void SyncHandler::sendPulse(qint64 currentMsecs, qint64 &nextPulseTime)
{
    if(SettingsHandler::getLubePulseEnabled() && currentMsecs >= nextPulseTime)
    {
        QString pulseTCode = TCodeChannelLookup::Lube() + QString::number(SettingsHandler::getLubePulseAmount());
        emit sendTCode(pulseTCode);
        nextPulseTime = currentMsecs + SettingsHandler::getLubePulseFrequency();
    }
}

void SyncHandler::on_input_device_change(InputDeviceHandler* inputDeviceHandler) {
    _inputDeviceHandler = inputDeviceHandler;
}

void SyncHandler::on_output_device_change(OutputDeviceHandler* outputDeviceHandler) {
    _outputDeviceHandler = outputDeviceHandler;
}

void SyncHandler::on_other_media_state_change(XMediaState state) {
    _isOtherMediaPlaying = state == XMediaState::Playing || state ==  XMediaState::Paused;
}

void SyncHandler::searchForFunscript(InputDevicePacket packet)
{
        //LogHandler::Debug("VR path: "+packet.path);
//LogHandler::Debug("VR duration: "+QString::number(packet.duration));
        //LogHandler::Debug("VR currentTime: "+QString::number(packet.currentTime));
//        LogHandler::Debug("VR playbackSpeed: "+QString::number(packet.playbackSpeed));
//        LogHandler::Debug("VR playing: "+QString::number(packet.playing));
    if(!(_outputDeviceHandler && _outputDeviceHandler->isConnected()))
        return;

    QString videoPath = packet.path.isEmpty() ? packet.path : QUrl::fromPercentEncoding(packet.path.toUtf8());
    if(!videoPath.isEmpty() && videoPath != _lastSearchedMediaPath)
    {
        LogHandler::Debug("searchForFunscript video changed: "+videoPath);
        LogHandler::Debug("searchForFunscript old path: "+_lastSearchedMediaPath);
        _funscriptSearchNotFound = false;
        _funscriptSearchRunning = false;
    } else if(videoPath.isEmpty() || packet.duration <= 0 || isPlaying())
        return;

    if (!_funscriptSearchRunning && !_funscriptSearchNotFound)
    {
        _funscriptSearchRunning = true;
        QFileInfo videoFile(videoPath);
        QStringList libraryPaths = SettingsHandler::getSelectedLibrary();
        QStringList vrLibraryPaths = SettingsHandler::getVRLibrary();
        QString funscriptPath;
        foreach(QString libraryPath, libraryPaths)
        {
            if(videoPath.contains("http"))
            {
                LogHandler::Debug("searchForFunscript Funscript is http: "+ videoPath);
                QUrl funscriptUrl = QUrl(videoPath);
                QString path = funscriptUrl.path();
                QString localpath = path;
                if(path.startsWith("/media"))
                    localpath = path.remove("/media/");
                int indexOfSuffix = localpath.lastIndexOf(".");
                QString localFunscriptPath = localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".funscript");
                QString localFunscriptZipPath = localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".zip");
                QString libraryScriptPath = libraryPath + QDir::separator() + localFunscriptPath;
                QString libraryScriptZipPath = libraryPath + QDir::separator() + localFunscriptZipPath;
                QFile libraryFile(libraryScriptPath);
                QFile libraryZipFile(libraryScriptZipPath);
                if(libraryFile.exists())
                {
                    LogHandler::Debug("searchForFunscript Script found in url path: "+libraryScriptPath);
                    funscriptPath = libraryScriptPath;
                }
                else if(libraryZipFile.exists())
                {
                    LogHandler::Debug("searchForFunscript Script zip found in url path: "+libraryScriptPath);
                    funscriptPath = libraryScriptZipPath;
                }
                else {
                    LogHandler::Debug("searchForFunscript Script not found in url path");
                }
            }

            if(funscriptPath.isEmpty())
            {
                funscriptPath = SettingsHandler::getDeoDnlaFunscript(videoPath);
                if(!funscriptPath.isEmpty())
                {
                    QFileInfo funscriptFile(funscriptPath);
                    if(!funscriptFile.exists())
                    {
                        SettingsHandler::removeLinkedVRFunscript(videoPath);
                        funscriptPath = nullptr;
                    }
                }
            }

            if (funscriptPath.isEmpty())
            {
                //Check the input device media directory for funscript.
                QString tempPath = videoPath;
                QString tempZipPath = videoPath;
                int indexOfSuffix = tempPath.lastIndexOf(".");
                QString localFunscriptPath = tempPath.replace(indexOfSuffix, tempPath.length() - indexOfSuffix, ".funscript");
                QString localFunscriptZipPath = tempZipPath.replace(indexOfSuffix, tempZipPath.length() - indexOfSuffix, ".zip");
                QString libraryScriptPath = libraryPath + QDir::separator() + localFunscriptPath;
                QString libraryScriptZipPath = libraryPath + QDir::separator() + localFunscriptZipPath;
                QFile localFile(localFunscriptPath);
                QFile libraryZipFile(libraryScriptZipPath);
                LogHandler::Debug("searchForFunscript Searching local path: "+localFunscriptPath);
                if(localFile.exists())
                {
                    LogHandler::Debug("searchForFunscript script found in path of media");
                    funscriptPath = localFunscriptPath;
                }
                else if (libraryZipFile.exists())
                {
                    LogHandler::Debug("searchForFunscript script zip found in path of media");
                    funscriptPath = libraryScriptZipPath;
                }
                else if(!vrLibraryPaths.isEmpty())
                {
                    foreach (auto vrLibraryPath, vrLibraryPaths) {
                        QString vrLibraryScriptPath = vrLibraryPath + QDir::separator() + localFunscriptPath;
                        QString vrLibraryScriptZipPath = vrLibraryPath + QDir::separator() + localFunscriptZipPath;
                        LogHandler::Debug("searchForFunscript Searching for local path in VR library root: "+ vrLibraryScriptPath);
                        QFile localVRFile(vrLibraryScriptPath);
                        QFile libraryVRZipFile(vrLibraryScriptZipPath);
                        if(localVRFile.exists())
                        {
                            LogHandler::Debug("searchForFunscript script found in path of VR media");
                            funscriptPath = vrLibraryScriptPath;
                        }
                        else if (libraryVRZipFile.exists())
                        {
                            LogHandler::Debug("searchForFunscript script zip found in path of VR media");
                            funscriptPath = vrLibraryScriptZipPath;
                        }
                    }
                }
            }

            if (funscriptPath.isEmpty())
            {
                LogHandler::Debug("searchForFunscript: Search ALL sub directories of both libraries for the funscript");
                QString libraryScriptFile = videoFile.fileName().remove(videoFile.fileName().lastIndexOf('.'), videoFile.fileName().length() -  1) + ".funscript";
                QString libraryScriptZipFile = videoFile.fileName().remove(videoFile.fileName().lastIndexOf('.'), videoFile.fileName().length() -  1) + ".zip";
                if(!libraryPath.isEmpty() && QFileInfo(libraryPath).exists()) {
                    QDirIterator directory(libraryPath,QDirIterator::Subdirectories);
                    while (_funscriptSearchRunning && directory.hasNext()) {
                        directory.next();
                        if (QFileInfo(directory.filePath()).isFile()) {
                            QString fileName = directory.fileName();
                            if (fileName.contains(libraryScriptFile) || fileName.contains(libraryScriptZipFile)) {
                                funscriptPath = directory.filePath();
                                LogHandler::Debug("searchForFunscript Script found in library: "+funscriptPath);
                                break;
                            }
                        }
                    }
                }
                if (funscriptPath.isEmpty() && !vrLibraryPaths.isEmpty())
                {

                    foreach (auto vrLibraryPath, vrLibraryPaths) {
                        if(QFileInfo(vrLibraryPath).exists()) {
                            QDirIterator directory(vrLibraryPath,QDirIterator::Subdirectories);
                            while (_funscriptSearchRunning && directory.hasNext()) {
                                directory.next();
                                if (QFileInfo(directory.filePath()).isFile()) {
                                    QString fileName = directory.fileName();
                                    if (fileName.contains(libraryScriptFile) || fileName.contains(libraryScriptZipFile)){
                                        funscriptPath = directory.filePath();
                                        LogHandler::Debug("searchForFunscript Script found in VR library: "+funscriptPath);
                                        break;
                                    }
                                }
                            }
                        }
                        if(!funscriptPath.isEmpty())
                            break;
                    }
                }
            }
            if(!funscriptPath.isEmpty())
                break;
        }
        if(funscriptPath.isEmpty())
            _funscriptSearchNotFound = true;

        emit funscriptSearchResult(videoPath, funscriptPath, packet.duration);

        _lastSearchedMediaPath = videoPath;
        _funscriptSearchRunning = false;
    }
}
