#include "synchandler.h"

SyncHandler::SyncHandler(QObject* parent):
    QObject(parent)
{
    _tcodeHandler = new TCodeHandler(parent);
    //_funscriptHandler = new FunscriptHandler(TCodeChannelLookup::Stroke());
}

SyncHandler::~SyncHandler()
{
    //delete _funscriptHandler;
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
    return _isPaused || m_isSwapping;
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

FunscriptHandler* SyncHandler::getFunscriptHandler() {
    if(_funscriptHandlers.isEmpty())
        return 0;
    auto it = std::find_if(_funscriptHandlers.begin(), _funscriptHandlers.end(), [](const FunscriptHandler* handler) {
        return handler->channel() == TCodeChannelLookup::Stroke();
    });
    return it != _funscriptHandlers.end() ? it.i->t() : _funscriptHandlers.first();
}

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem, bool reset)
{
    if(reset)
        this->reset();
    else {
        if(_funscriptHandlers.length() > 0)
        {
            qDeleteAll(_funscriptHandlers);
            _funscriptHandlers.clear();
        }
    }
    // QString funscript = libraryItem.script;
    // if(!funscript.isEmpty() || libraryItem.isMFS)
    // {
        // QFileInfo scriptInfo(funscript);
        // if(scriptInfo.exists())
        // {
        //     // QString scriptTemp = funscript;
        //     // QString scriptFileNoExtension = scriptTemp.remove(scriptTemp.lastIndexOf('.'), scriptTemp.length() -  1);
        //     // QString fileName = scriptInfo.fileName();
        //     // QString scriptNameNoExtension = fileName.remove(fileName.lastIndexOf('.'), scriptTemp.length() -  1);
        //     // if(funscript.endsWith(".zip"))
        //     // {
        //     //     QZipReader zipFile(funscript, QIODevice::ReadOnly);
        //     //     if(zipFile.isReadable())
        //     //     {
        //     //         QByteArray data = zipFile.fileData(scriptNameNoExtension + ".funscript");
        //     //         if (!data.isEmpty())
        //     //         {
        //     //             if(!load(data))
        //     //             {
        //     //                 _invalidScripts.append("Zip file: " + scriptNameNoExtension + ".funscript");
        //     //             }
        //     //         }
        //     //         else
        //     //         {
        //     //             LogHandler::Debug("Main funscript: '"+scriptNameNoExtension + ".funscript' not found in zip");
        //     //         }
        //     //     }
        //     // }
        //     // else if(!_funscriptHandler->load(funscript))
        //     // {
        //     //     _invalidScripts.append(funscript);
        //     // }
        //     loadMFS(funscript);
        //     emit funscriptLoaded(funscript);
        // }
        // else
        // {
    SyncLoadState loadState;
    QString path = libraryItem.script.isEmpty() ? libraryItem.path : libraryItem.script;
    loadMFS(path, loadState);
        // }
    // }
    return loadState;
}

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem)
{
    LogHandler::Debug("Enter syncHandler load");
    return load(libraryItem, true);
}

SyncLoadState SyncHandler::load(QString funscript) {
    LibraryListItem27 item;
    item.script = funscript;
    return load(item);
}

SyncLoadState SyncHandler::swap(const LibraryListItem27 &libraryItem)
{
    LogHandler::Debug("Enter syncHandler swap");
    m_isSwapping = true;
    auto loadState = load(libraryItem, false);
    m_isSwapping = false;
    return loadState;
}

bool SyncHandler::isLoaded()
{
    return _funscriptHandlers.length() > 0;
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
    //_funscriptHandler->setLoaded(false);
    if(_funscriptHandlers.length() > 0)
    {
        qDeleteAll(_funscriptHandlers);
        _funscriptHandlers.clear();
    }
    QMutexLocker locker(&_mutex);
    _currentTime = 0;
    _standAloneLoop = false;
    _isPaused = false;
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

void SyncHandler::swapScript(const LibraryListItem27 &libraryItem) {
    load(libraryItem);
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
    qint64 funscriptMax = getFunscriptMax();
    emit funscriptStandaloneDurationChanged(funscriptMax);
    LogHandler::Debug("playStandAlone start thread");
    _funscriptStandAloneFuture = QtConcurrent::run([this, funscriptMax]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QMap<QString, std::shared_ptr<FunscriptAction>> actions;
        int secCounter1 = 0;
        int secCounter2 = 0;
        QElapsedTimer mSecTimer;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        while (_isStandAloneFunscriptPlaying)
        {
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                if (_inputDeviceHandler && _inputDeviceHandler->isConnected()) {
                    auto currentVRPacket = _inputDeviceHandler->getCurrentPacket();
                    if(currentVRPacket.duration > 0)
                        setPause(!currentVRPacket.playing);
                }
                if(!isPaused() && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected())
                {
                    if(_seekTime > -1)
                    {
                        _currentTime = _seekTime;
                    }
                    else
                    {
                        _currentTime++;
                    }
                    // if(_funscriptHandler->isLoaded())
                    //     actionPosition = _funscriptHandler->getPosition(_currentTime);
                    // if(actionPosition != nullptr)
                    //     emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
                    foreach(auto funscriptHandler, _funscriptHandlers)
                    {
                        auto action = funscriptHandler->getPosition(_currentTime);
                        if(action != nullptr)
                        {
                            actions.insert(funscriptHandler->channel(), action);
                            emit channelPositionChange(funscriptHandler->channel(), action->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    actions.clear();

                    sendPulse(mSecTimer.elapsed(), nextPulseTime);
                } else {
                    QThread::msleep(100);
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
        emit funscriptStandaloneDurationChanged(0);
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
    auto funscriptHandler = getFunscriptHandler();
    if(funscriptHandler && funscriptHandler->getMax() > -1)
    {
        return funscriptHandler->getMin();
    }
    return 0;
}

qint64 SyncHandler::getFunscriptNext()
{
    auto funscriptHandler = getFunscriptHandler();
    if(funscriptHandler && funscriptHandler->getMax() > -1)
    {
        return funscriptHandler->getNext();
    }
    return 0;
}

qint64 SyncHandler::getFunscriptMax()
{
    auto funscriptHandler = getFunscriptHandler();
    if(funscriptHandler && funscriptHandler->getMax() > -1)
    {
        return funscriptHandler->getMax();
    }
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
        QMap<QString, std::shared_ptr<FunscriptAction>> actions;
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
                if(!isPaused() && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected())
                {
                    qint64 currentTime = getMediaPosition();//_videoHandler->position();
                    // actionPosition = _funscriptHandler->getPosition(currentTime);
                    // if(actionPosition != nullptr)
                    //     emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
                    foreach(auto funscriptHandlerOther, _funscriptHandlers)
                    {
                        auto action = funscriptHandlerOther->getPosition(currentTime);
                        if(action != nullptr)
                        {
                            actions.insert(funscriptHandlerOther->channel(), action);
                            emit channelPositionChange(funscriptHandlerOther->channel(), action->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    actions.clear();

                    sendPulse(mSecTimer.elapsed(), nextPulseTime);
                } else {
                    QThread::msleep(100);
                }
            }
            QThread::usleep(1);
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
    LogHandler::Debug("syncInputDeviceFunscript start thread");
    _funscriptVRFuture = QtConcurrent::run([this, funscript]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QMap<QString, std::shared_ptr<FunscriptAction>> actions;
        InputDevicePacket currentVRPacket;
        qint64 timeTracker = 0;
        qint64 lastVRTime = 0;
        //qint64 lastVRSyncResetTime = 0;
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
            //execute once every millisecond
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                currentVRPacket = _inputDeviceHandler->getCurrentPacket();
                if(lastStatePlaying && !currentVRPacket.playing)
                    emit sendTCode("DSTOP");
                lastStatePlaying = currentVRPacket.playing;
                //timer.start();
                if(!isPaused() && !SettingsHandler::getLiveActionPaused() && _outputDeviceHandler && _outputDeviceHandler->isConnected() && isLoaded() && !currentVRPacket.path.isEmpty() && currentVRPacket.duration > 0 && currentVRPacket.playing)
                {
                    if(videoPath.isEmpty())
                        videoPath = currentVRPacket.path;
                    if(!duration)
                        duration = currentVRPacket.duration;
        //                LogHandler::Debug("timer1: "+QString::number(timer1));
        //                LogHandler::Debug("timer2: "+QString::number(timer2));
                        //LogHandler::Debug("timer2 - timer1 "+QString::number(timer2-timer1));
        //                LogHandler::Debug("Out timeTracker: "+QString::number(timeTracker));
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
//                     actionPosition = _funscriptHandler->getPosition(vrTime);
//                     if(actionPosition != nullptr) {
//                         emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
// //                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////pos: "+QString::number(actionPosition->pos));
// //                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////speed: "+QString::number(actionPosition->speed));
// //                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lastPos: "+QString::number(actionPosition->lastPos));
// //                        LogHandler::Debug("actionPosition != nullptr/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////lastSpeed: "+QString::number(actionPosition->lastSpeed));
//                     }
                    foreach(auto funscriptHandler, _funscriptHandlers)
                    {
                        auto action = funscriptHandler->getPosition(vrTime);
                        if(action != nullptr)
                        {
                            actions.insert(funscriptHandler->channel(), action);
                            emit channelPositionChange(funscriptHandler->channel(), action->pos);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(tcode != nullptr)
                        emit sendTCode(tcode);
                    actions.clear();
               //     LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
                    sendPulse(timer1, nextPulseTime);
                //}
                } else {
                    QThread::msleep(100);
                }
            }
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
            //LogHandler::Debug("timer nsecsElapsed: "+QString::number(timer2));
        }

        QMutexLocker locker(&_mutex);
        _isVRFunscriptPlaying = false;
        emit funscriptVREnded(videoPath, funscript, duration);
        LogHandler::Debug("exit syncInputDeviceFunscript");
    });
}

// Private
// bool SyncHandler::load(QByteArray funscript)
// {
//     return _funscriptHandler->load(funscript);
// }

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

bool SyncHandler::loadMFS(QString path, SyncLoadState &loadState)
{
    QString pathTemp = path;
    QString pathNoExtension = pathTemp.remove(pathTemp.lastIndexOf('.'), pathTemp.length() -  1);
    QFileInfo pathInfo(path);
    QZipReader* zipFile = 0;
    if(path.endsWith(".zip"))
        zipFile = new QZipReader(path, QIODevice::ReadOnly);

    auto availibleAxis = TCodeChannelLookup::getChannels();
    bool hasScript = false;
    QString mainScript;
    foreach(auto axisName, availibleAxis)
    {
        auto track = TCodeChannelLookup::getChannel(axisName);
        if(track->Type == AxisType::HalfOscillate)
            continue;

        QFileInfo fileInfo(pathNoExtension + (axisName == TCodeChannelLookup::Stroke() ? "" : "."+ track->TrackName) + ".funscript");
        if(fileInfo.exists())
        {
            LogHandler::Debug("Loading track: "+ fileInfo.absoluteFilePath());
            if(!loadMFS(axisName, fileInfo.absoluteFilePath()))
                loadState.invalidScripts.append("Script: " + fileInfo.absoluteFilePath());
            else {
                hasScript = true;
                if(axisName == TCodeChannelLookup::Stroke() || mainScript.isEmpty())
                    mainScript = fileInfo.absoluteFilePath();
            }
        }
        else if(zipFile && zipFile->isReadable())
        {
           QString fileName = pathInfo.fileName();
           QString scriptFileNameNoExtension = fileName.remove(fileName.lastIndexOf('.'), pathTemp.length() -  1);
           QString trackFileName = scriptFileNameNoExtension + (axisName == TCodeChannelLookup::Stroke() ? "" : "."+ track->TrackName) + ".funscript";
           QByteArray data = zipFile->fileData(trackFileName);
           if (!data.isEmpty())
           {
               LogHandler::Debug("Loading track from zip: "+ trackFileName);
               if(!loadMFS(axisName, data))
                   loadState.invalidScripts.append("Zip script: " + trackFileName);
               else{
                   hasScript = true;
                   if(axisName == TCodeChannelLookup::Stroke() || mainScript.isEmpty())
                       mainScript = trackFileName;
               }
           }
        }
    }
    loadState.hasScript = hasScript;
    if(hasScript) {
        loadState.mainScript = mainScript;
        emit funscriptLoaded(mainScript);
    }
    if(zipFile)
        delete zipFile;
    return hasScript;
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
    if(packet.stopped) {
        reset();
        return;
    }

    if(!(_outputDeviceHandler && _outputDeviceHandler->isConnected()))
        return;

    QString videoPath = packet.path.isEmpty() ? packet.path : QUrl::fromPercentEncoding(packet.path.toUtf8());
    if(!videoPath.isEmpty() && videoPath != _lastSearchedMediaPath)
    {
        LogHandler::Debug("searchForFunscript video changed: "+videoPath);
        LogHandler::Debug("searchForFunscript old path: "+_lastSearchedMediaPath);
        stopAll();
        _lastSearchedMediaPath = videoPath;
        _funscriptSearchNotFound = false;
        if (_funscriptSearchFuture.isRunning()) {
            _funscriptSearchFuture.cancel();
            _funscriptSearchFuture.waitForFinished();
        }
    } else if(videoPath.isEmpty() || packet.duration <= 0 || isPlaying())
        return;

    if (!_funscriptSearchFuture.isRunning() && !_funscriptSearchNotFound)
    {
        _funscriptSearchFuture = QtConcurrent::run([this, packet, videoPath]() {
            QFileInfo videoFile(videoPath);
            QString libraryScriptFile;
            QString libraryScriptZipFile;
            int extensionIndex = videoFile.fileName().lastIndexOf('.');
            if(extensionIndex < 0) {
                libraryScriptFile = videoFile.fileName() + ".funscript";
                libraryScriptZipFile = videoFile.fileName() + ".zip";
            } else {
                libraryScriptFile = videoFile.fileName().remove(videoFile.fileName().lastIndexOf('.'), videoFile.fileName().length() -  1) + ".funscript";
                libraryScriptZipFile = videoFile.fileName().remove(videoFile.fileName().lastIndexOf('.'), videoFile.fileName().length() -  1) + ".zip";
            }
            QStringList libraryPaths = SettingsHandler::getSelectedLibrary();
            QStringList vrLibraryPaths = SettingsHandler::getVRLibrary();
            QString funscriptPath;
            if(videoPath.contains("http") ||
                videoPath.startsWith("/media") ||
                videoPath.startsWith("media") ||
                videoPath.startsWith("/storage/emulated/0/Interactive/"))
            {
                LogHandler::Debug("searchForFunscript Funscript is http: "+ videoPath);
                QUrl funscriptUrl = QUrl(videoPath);
                QString path = funscriptUrl.path();
                QString localpath = path;
                if(path.startsWith("/media"))
                    localpath = path.remove("/media/");
                else if(path.startsWith("media/"))
                    localpath = path.remove("media/");
                else if(path.startsWith("/storage/emulated/0/Interactive/"))
                    localpath = path.remove("/storage/emulated/0/Interactive/");
                QString localFunscriptPath;
                QString localFunscriptZipPath;
                int indexOfSuffix = localpath.lastIndexOf(".");
                if(indexOfSuffix < 0) {
                    localFunscriptPath = localpath + ".funscript";
                    localFunscriptZipPath = localpath + ".zip";
                } else {
                    localFunscriptPath = localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".funscript");
                    localFunscriptZipPath = localpath.replace(indexOfSuffix, localpath.length() - indexOfSuffix, ".zip");
                }
                foreach(QString libraryPath, libraryPaths)
                {
                    if(_funscriptSearchFuture.isCanceled()) {
                        _funscriptSearchNotFound = true;
                        return;
                    }
                    QString separator = libraryPath.contains("/") ? "/" : "\\";
                    QString libraryScriptPath = libraryPath + separator + localFunscriptPath;
                    QString libraryScriptZipPath = libraryPath + separator + localFunscriptZipPath;
                    if(QFile::exists(libraryScriptPath))
                    {
                        LogHandler::Debug("searchForFunscript Script found in url path: "+libraryScriptPath);
                        funscriptPath = libraryScriptPath;
                    }
                    else if(QFile::exists(libraryScriptZipPath))
                    {
                        LogHandler::Debug("searchForFunscript Script zip found in url path: "+libraryScriptPath);
                        funscriptPath = libraryScriptZipPath;
                    }
                    else {
                        LogHandler::Debug("searchForFunscript Script not found in url path");
                    }
                }
            }


            if (funscriptPath.isEmpty())
            {
                //Check the input device media directory for funscript.
                QString tempPath = videoPath;

                QString localFunscriptPath;
                QString localFunscriptZipPath;
                int indexOfSuffix = tempPath.lastIndexOf(".");
                if(indexOfSuffix < 0) {
                    localFunscriptPath = tempPath + ".funscript";
                    localFunscriptZipPath = tempPath + ".zip";
                } else {
                    localFunscriptPath = tempPath.replace(indexOfSuffix, tempPath.length() - indexOfSuffix, ".funscript");
                    localFunscriptZipPath = tempPath.replace(indexOfSuffix, tempPath.length() - indexOfSuffix, ".zip");
                }
                foreach(QString libraryPath, libraryPaths)
                {
                    if(_funscriptSearchFuture.isCanceled()) {
                        _funscriptSearchNotFound = true;
                        return;
                    }
                    QString separator = libraryPath.contains("/") ? "/" : "\\";
                    QString libraryScriptPath = libraryPath + separator + localFunscriptPath;
                    QString libraryScriptZipPath = libraryPath + separator + localFunscriptZipPath;
                    LogHandler::Debug("searchForFunscript Searching local path: "+localFunscriptPath);
                    if(QFile::exists(libraryScriptPath))
                    {
                        LogHandler::Debug("searchForFunscript script found in path of media");
                        funscriptPath = libraryScriptPath;
                    }
                    else if (QFile::exists(libraryScriptZipPath))
                    {
                        LogHandler::Debug("searchForFunscript script zip found in path of media");
                        funscriptPath = libraryScriptZipPath;
                    }
    //                else if(!vrLibraryPaths.isEmpty())
    //                {
    //                    foreach (auto vrLibraryPath, vrLibraryPaths) {
    //                        QString vrLibraryScriptPath = vrLibraryPath + QDir::separator() + localFunscriptPath;
    //                        QString vrLibraryScriptZipPath = vrLibraryPath + QDir::separator() + localFunscriptZipPath;
    //                        LogHandler::Debug("searchForFunscript Searching for local path in VR library root: "+ vrLibraryScriptPath);
    //                        if(QFile::exists(vrLibraryScriptPath))
    //                        {
    //                            LogHandler::Debug("searchForFunscript script found in path of VR media");
    //                            funscriptPath = vrLibraryScriptPath;
    //                        }
    //                        else if (QFile::exists(vrLibraryScriptZipPath))
    //                        {
    //                            LogHandler::Debug("searchForFunscript script zip found in path of VR media");
    //                            funscriptPath = vrLibraryScriptZipPath;
    //                        }
    //                    }
    //                }
                    if(!funscriptPath.isEmpty())
                        break;
                }
            }
            if(_funscriptSearchFuture.isCanceled()) {
                _funscriptSearchNotFound = true;
                return;
            }

            if (funscriptPath.isEmpty())
            {
                foreach(QString libraryPath, libraryPaths)
                {
                    if(_funscriptSearchFuture.isCanceled()) {
                        _funscriptSearchNotFound = true;
                        return;
                    }
                    LogHandler::Debug("searchForFunscript: Search ALL sub directories of library: "+libraryPath);
                    if(!libraryPath.isEmpty() && QFile::exists(libraryPath)) {
                        QDirIterator directory(libraryPath,QDirIterator::Subdirectories);
                        while (directory.hasNext()) {
                            if(_funscriptSearchFuture.isCanceled()) {
                                _funscriptSearchNotFound = true;
                                return;
                            }
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
                    if(!funscriptPath.isEmpty())
                        break;
                }
            }

            if (funscriptPath.isEmpty() && !vrLibraryPaths.isEmpty())
            {
                foreach (auto vrLibraryPath, vrLibraryPaths) {
                    if(_funscriptSearchFuture.isCanceled()) {
                        _funscriptSearchNotFound = true;
                        return;
                    }
                    if(QFile::exists(vrLibraryPath)) {
                        LogHandler::Debug("searchForFunscript: Search ALL sub directories of VR Library: "+vrLibraryPath);
                        QDirIterator directory(vrLibraryPath,QDirIterator::Subdirectories);
                        while (directory.hasNext()) {
                            if(_funscriptSearchFuture.isCanceled()) {
                                _funscriptSearchNotFound = true;
                                return;
                            }
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

            if(funscriptPath.isEmpty())
            {
                funscriptPath = SettingsHandler::getDeoDnlaFunscript(videoPath);
                if(!funscriptPath.isEmpty())
                {
                    if(!QFile::exists(funscriptPath))
                    {
                        SettingsHandler::removeLinkedVRFunscript(videoPath);
                        funscriptPath = nullptr;
                    }
                }
            }

            if(funscriptPath.isEmpty())
                _funscriptSearchNotFound = true;

            emit funscriptSearchResult(videoPath, funscriptPath, packet.duration);

        });
    }
}
