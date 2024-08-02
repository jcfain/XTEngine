#include "synchandler.h"

#include "../tool/file-util.h"

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
bool SyncHandler::isPlayingInternal()
{
    return _isMediaFunscriptPlaying;
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
    return getFunscriptHandler(TCodeChannelLookup::Stroke()) ?: _funscriptHandlers.first();
}

FunscriptHandler *SyncHandler::getFunscriptHandler(QString channel)
{
    if(_funscriptHandlers.isEmpty())
        return 0;
    auto it = std::find_if(_funscriptHandlers.begin(), _funscriptHandlers.end(), [channel](const FunscriptHandler* handler) {
        return handler->channel() == channel;
    });
    return it != _funscriptHandlers.end() ? it.i->t() : 0;
}

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem, bool reset)
{
    if(reset)
        this->reset();
    else {
        if(_funscriptHandlers.length() > 0)
        {
            foreach (auto handler, _funscriptHandlers) {
                emit channelPositionChange(handler->channel(), 0, 0, ChannelTimeType::None);
            }
            qDeleteAll(_funscriptHandlers);
            _funscriptHandlers.clear();
        }
    }
    SyncLoadState loadState;
    loadFunscripts(libraryItem, loadState);
    return loadState;
}

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem)
{
    LogHandler::Debug("Enter syncHandler load");
    return load(libraryItem, true);
}

SyncLoadState SyncHandler::load(QString funscript) {
    LibraryListItem27 item;
    buildScriptItem(item, funscript);
    return load(item);
}

SyncLoadState SyncHandler::swap(const LibraryListItem27 &libraryItem, const ScriptInfo &script)
{
    LogHandler::Debug("Enter syncHandler swap");
    m_isSwapping = true;
    LibraryListItem27 swappedScriptItem;
    swappedScriptItem.copyProperties(libraryItem);
    buildScriptItem(swappedScriptItem, script.path);
    auto loadState = load(swappedScriptItem, false);
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
    _standAloneFunscriptCurrentTime = 0;
    _isStandAloneFunscriptPlaying = false;
    locker.unlock();
    if(_funscriptStandAloneFuture.isRunning())
    {
        emit syncStopping();
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
        emit syncStopping();
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
        emit syncStopping();
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
        foreach (auto handler, _funscriptHandlers) {
            emit channelPositionChange(handler->channel(), 0, 0, ChannelTimeType::None);
        }
        qDeleteAll(_funscriptHandlers);
        _funscriptHandlers.clear();
    }
    QMutexLocker locker(&_mutex);
    _standAloneFunscriptCurrentTime = 0;
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

void SyncHandler::playStandAlone() {
    LogHandler::Debug("play Funscript stand alone start thread");
    // if(!funscript.isEmpty()) //Override standalone funscript. aka: skip to moneyshot
    //     load(funscript);
    QMutexLocker locker(&_mutex);
    _standAloneFunscriptCurrentTime = 0;
    _isPaused = false;
    _standAloneLoop = false;
    _isMediaFunscriptPlaying = true;
    _isStandAloneFunscriptPlaying = true;
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
        emit syncStart();
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
                if(!isPaused())
                {
                    if(_seekTime > -1)
                    {
                        _standAloneFunscriptCurrentTime = _seekTime;
                    }
                    else
                    {
                        _standAloneFunscriptCurrentTime++;
                    }
                    // if(_funscriptHandler->isLoaded())
                    //     actionPosition = _funscriptHandler->getPosition(_currentTime);
                    // if(actionPosition != nullptr)
                    //     emit channelPositionChange(TCodeChannelLookup::Stroke(), actionPosition->pos);
                    foreach(auto funscriptHandler, _funscriptHandlers)
                    {
                        auto action = funscriptHandler->getPosition(_standAloneFunscriptCurrentTime);
                        if(action != nullptr)
                        {
                            actions.insert(funscriptHandler->channel(), action);
                            emit channelPositionChange(funscriptHandler->channel(), action->pos, action->speed, ChannelTimeType::Interval);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(_funscriptStandAloneFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                        sendPulse(mSecTimer.elapsed(), nextPulseTime);
                    }
                    actions.clear();
                } else {
                    QThread::msleep(100);
                }
                secCounter2 = round(mSecTimer.elapsed() / 1000);
                if(secCounter2 - secCounter1 >= 1)
                {
                    if(_seekTime == -1 && !_isOtherMediaPlaying)
                        emit funscriptPositionChanged(_standAloneFunscriptCurrentTime);
                    secCounter1 = secCounter2;
                }
                if(_seekTime > -1)
                    _seekTime = -1;
            }
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
            if(!_standAloneLoop && _standAloneFunscriptCurrentTime >= funscriptMax)
            {
                _isStandAloneFunscriptPlaying = false;
                if(!_isOtherMediaPlaying)
                    emit funscriptStatusChanged(XMediaStatus::EndOfMedia);
            }
            else if(_standAloneLoop && _standAloneFunscriptCurrentTime >= funscriptMax)
            {
                _standAloneFunscriptCurrentTime = 0;
            }
        }
        QMutexLocker locker(&_mutex);
        _isMediaFunscriptPlaying = false;
        _playingStandAloneFunscript = nullptr;
        _standAloneFunscriptCurrentTime = 0;
        emit funscriptEnded();
        emit funscriptStandaloneDurationChanged(0);
        emit sendTCode("DSTOP");
        LogHandler::Debug("exit play Funscript stand alone thread");
        emit syncEnd();
    });
}

void SyncHandler::setFunscriptTime(qint64 msecs)
{
    QMutexLocker locker(&_mutex);
    _standAloneFunscriptCurrentTime = msecs;
}

qint64 SyncHandler::getFunscriptTime()
{
    return _standAloneFunscriptCurrentTime;
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
        emit syncStart();
        while (_isMediaFunscriptPlaying && _isOtherMediaPlaying)
        {
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                if(!isPaused())
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
                            emit channelPositionChange(funscriptHandlerOther->channel(), action->pos, action->speed, ChannelTimeType::Interval);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(_funscriptMediaFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                        sendPulse(mSecTimer.elapsed(), nextPulseTime);
                    }
                    actions.clear();

                } else {
                    QThread::msleep(100);
                }
            }
            QThread::usleep(1);
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
        }
        _currentLocalVideoTime = 0;
        _isMediaFunscriptPlaying = false;
        emit sendTCode("DSTOP");
        LogHandler::Debug("exit syncFunscript");
        emit syncEnd();
    });
}

void SyncHandler::syncInputDeviceFunscript(const LibraryListItem27 &libraryItem)
{
    load(libraryItem);
    QMutexLocker locker(&_mutex);
    _isVRFunscriptPlaying = true;
    LogHandler::Debug("syncInputDeviceFunscript start thread");
    _funscriptVRFuture = QtConcurrent::run([this]()
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
        emit syncStart();
        while (_isVRFunscriptPlaying && _inputDeviceHandler && _inputDeviceHandler->isConnected() && !_isOtherMediaPlaying)
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
                if(!isPaused() && isLoaded() && !currentVRPacket.path.isEmpty() && currentVRPacket.duration > 0 && currentVRPacket.playing)
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
                            emit channelPositionChange(funscriptHandler->channel(), action->pos, action->speed, ChannelTimeType::Interval);
                        }
                    }
                    QString tcode = _tcodeHandler->funscriptToTCode(actions);
                    if(_funscriptVRFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                    //     LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
                        sendPulse(timer1, nextPulseTime);
                    }
                    actions.clear();
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
        emit sendTCode("DSTOP");
        //emit funscriptVREnded(videoPath, funscript, duration);
        LogHandler::Debug("exit syncInputDeviceFunscript");
        emit syncEnd();
    });
}

// Private
// bool SyncHandler::load(QByteArray funscript)
// {
//     return _funscriptHandler->load(funscript);
// }

FunscriptHandler* SyncHandler::createFunscriptHandler(QString channel, QString funscript)
{
    FunscriptHandler* funscriptHandler = new FunscriptHandler(channel);
    if(!funscriptHandler->load(funscript)) {
        delete funscriptHandler;
        return 0;
    } else
        _funscriptHandlers.append(funscriptHandler);
    return funscriptHandler;
}

FunscriptHandler* SyncHandler::createFunscriptHandler(QString channel, QByteArray funscript)
{
    FunscriptHandler* funscriptHandler = new FunscriptHandler(channel);
    if(!funscriptHandler->load(funscript)) {
        delete funscriptHandler;
        return 0;
    } else
        _funscriptHandlers.append(funscriptHandler);
    return funscriptHandler;
}

bool SyncHandler::loadFunscripts(const LibraryListItem27 &libraryItem, SyncLoadState &loadState)
{
    QString path = libraryItem.script.isEmpty() ? libraryItem.path : libraryItem.script;
    QString pathNoExtension = libraryItem.type == LibraryListItemType::External ?
        XFileUtil::getPathNoExtension(path) :
        //path.remove(path.lastIndexOf('.'), path.length() -  1) :
        libraryItem.pathNoExtension;
    //QFileInfo pathInfo(path);
    QZipReader* zipFile = 0;
    if(!libraryItem.zipFile.isEmpty())
        zipFile = new QZipReader(libraryItem.zipFile, QIODevice::ReadOnly);

    auto availibleAxis = TCodeChannelLookup::getChannels();
    QString mainScript;
    foreach(auto axisName, availibleAxis)
    {
        auto track = TCodeChannelLookup::getChannel(axisName);
        if(track->Type == ChannelType::HalfOscillate)
            continue;
        FunscriptHandler* funscriptHandler = 0;
        // IF we are the stroke channel and media doesnt exist in the XTP library,
        // then loop below will incorrectly think this is the L0 funscript as we
        // dont have the video path here.
        // Check every channel as a work around for now.
        if(libraryItem.type == LibraryListItemType::External && axisName == TCodeChannelLookup::Stroke())
        {
            foreach(auto axisName2, availibleAxis)
            {
                auto track2 = TCodeChannelLookup::getChannel(axisName2);
                if(pathNoExtension.endsWith("."+ track2->TrackName))
                    continue;
            }
        }
        QFileInfo fileInfo(pathNoExtension + (axisName == TCodeChannelLookup::Stroke() ? "" : "."+ track->TrackName) + ".funscript");
        if(fileInfo.exists())
        {
            LogHandler::Debug("Loading track: "+ fileInfo.absoluteFilePath());
            funscriptHandler = createFunscriptHandler(axisName, fileInfo.absoluteFilePath());
            if(!funscriptHandler)
                loadState.invalidScripts.append("Script: " + fileInfo.absoluteFilePath());
            else {
                if(axisName == TCodeChannelLookup::Stroke() || mainScript.isEmpty())
                {
                    mainScript = fileInfo.absoluteFilePath();
                }
            }
        }
        else if(zipFile && zipFile->isReadable())
        {
            QString scriptFileNameNoExtension = XFileUtil::getNameNoExtension(pathNoExtension);
            //fileName.remove(fileName.lastIndexOf('.'), pathTemp.length() -  1);
            QString trackFileName = scriptFileNameNoExtension + (axisName == TCodeChannelLookup::Stroke() ? "" : "."+ track->TrackName) + ".funscript";
            QByteArray data = zipFile->fileData(trackFileName);
            if (!data.isEmpty())
            {
               LogHandler::Debug("Loading track from zip: "+ trackFileName);
                   funscriptHandler = createFunscriptHandler(axisName, data);
               if(!funscriptHandler)
                   loadState.invalidScripts.append("Zip script: " + trackFileName);
               else{
                   if(axisName == TCodeChannelLookup::Stroke() || mainScript.isEmpty())
                   {
                       mainScript = trackFileName;
                   }
               }
           }
        }
    }
    FunscriptHandler::setModifier(libraryItem.metadata.funscriptModifier);
    loadState.hasScript = !_funscriptHandlers.isEmpty();
    if(loadState.hasScript) {
        loadState.mainScript = mainScript;
        emit funscriptLoaded(mainScript);
        if(libraryItem.type == LibraryListItemType::FunscriptType)
        {
            _playingStandAloneFunscript = loadState.mainScript;
        }
    }
    if(zipFile)
        delete zipFile;
    return loadState.hasScript;
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
   // _outputDeviceHandler = outputDeviceHandler;
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

    // if(!(_outputDeviceHandler && _outputDeviceHandler->isConnected()))
    //     return;

    QString videoPath = packet.path.isEmpty() ? packet.path : QUrl::fromPercentEncoding(packet.path.toUtf8());
    if(!videoPath.isEmpty() && videoPath != _lastSearchedMediaPath)
    {
        LogHandler::Debug("searchForFunscript video changed: "+videoPath);
        LogHandler::Debug("searchForFunscript old path: "+_lastSearchedMediaPath);
        stopAll();
        _lastSearchedMediaPath = videoPath;
        _funscriptSearchNotFound = false;
        if (_funscriptSearchFuture.isRunning()) {
            m_stopFunscript = true;
            _funscriptSearchFuture.cancel();
            _funscriptSearchFuture.waitForFinished();
            m_stopFunscript = false;
        }
    } else if(videoPath.isEmpty() || packet.duration <= 0 || isPlaying())
        return;

    if (!_funscriptSearchFuture.isRunning() && !_funscriptSearchNotFound)
    {
        _funscriptSearchFuture = QtConcurrent::run([this, packet, videoPath]() {
            LogHandler::Debug("searchForFunscript thread start for media: "+videoPath);
            QStringList extensions = QStringList() << ".funscript" << ".zip";
            QStringList libraryPaths = SettingsHandler::getVRLibrary();
            QStringList mainLibraryPaths = SettingsHandler::getSelectedLibrary();
            foreach(QString libraryPath, mainLibraryPaths)
            {
                libraryPaths << libraryPath;
            }
            QString funscriptPath = searchForFunscript(videoPath, extensions, libraryPaths);

            if(_funscriptSearchFuture.isCanceled()) {
                LogHandler::Debug("searchForFunscript canceled: "+videoPath);
                _funscriptSearchNotFound = true;
                return;
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
                funscriptPath = searchForFunscriptMFS(videoPath, libraryPaths);

            // Deep search last
            if(funscriptPath.isEmpty())
                funscriptPath = searchForFunscriptDeep(videoPath, extensions, libraryPaths);
            if(funscriptPath.isEmpty())
                funscriptPath = searchForFunscriptMFSDeep(videoPath, libraryPaths);

            if(funscriptPath.isEmpty())
                _funscriptSearchNotFound = true;

            emit funscriptSearchResult(videoPath, funscriptPath, packet.duration);

        });
    }
}

QString SyncHandler::searchForFunscript(QString videoPath, QStringList extensions, QStringList pathsToSearch)
{
    QString funscriptPath;
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript paths canceled: "+videoPath);
            _funscriptSearchNotFound = true;
            return funscriptPath;
        }
        LogHandler::Debug("searchForFunscript in library: "+libraryPath);
        funscriptPath = searchForFunscriptHttp(videoPath, extensions, libraryPath);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
        funscriptPath = searchForFunscript(videoPath, extensions, libraryPath);
        if(!funscriptPath.isEmpty())
            return funscriptPath;

    }
    return funscriptPath;
}

QString SyncHandler::searchForFunscriptDeep(QString videoPath, QStringList extensions, QStringList pathsToSearch)
{
    QString funscriptPath;
    LogHandler::Debug("searchForFunscript deep "+ videoPath);
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscriptDeep canceled: "+videoPath);
            _funscriptSearchNotFound = true;
            return funscriptPath;
        }
        funscriptPath = XFileUtil::searchForFileRecursive(videoPath, extensions, libraryPath, m_stopFunscript);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
    }
    return funscriptPath;
}

QString SyncHandler::searchForFunscript(QString videoPath, QStringList extensions, QString pathToSearch)
{
    //Check the input device media directory for funscript.
    LogHandler::Debug("searchForFunscript local: "+ videoPath);
    QString funscriptPath;
    QStringList libraryScriptPaths = XFileUtil::getFunscriptPaths(videoPath, extensions, pathToSearch);
    foreach (QString libraryScriptPath, libraryScriptPaths)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript path canceled: "+videoPath);
            _funscriptSearchNotFound = true;
            return funscriptPath;
        }
        LogHandler::Debug("searchForFunscript Searching path: "+libraryScriptPath);
        if(QFile::exists(libraryScriptPath))
        {
            LogHandler::Debug("searchForFunscript script found in path of media");
            funscriptPath = libraryScriptPath;
        }
    }
    return funscriptPath;
}

QString SyncHandler::searchForFunscriptHttp(QString videoPath, QStringList extensions, QString pathToSearch)
{
    QString funscriptPath;
    if(videoPath.contains("http") ||
        videoPath.startsWith("/media") ||
        videoPath.startsWith("media") ||
        videoPath.startsWith("/storage/emulated/0/Interactive/"))
    {
        LogHandler::Debug("searchForFunscript from http in: "+ pathToSearch);
        QUrl funscriptUrl = QUrl(videoPath);
        QString path = funscriptUrl.path();
        QString localpath = path;
        if(path.startsWith("/media"))
            localpath = path.remove("/media/");
        else if(path.startsWith("media/"))
            localpath = path.remove("media/");
        else if(path.startsWith("/storage/emulated/0/Interactive/"))
            localpath = path.remove("/storage/emulated/0/Interactive/");

        QFileInfo fileInfo(localpath);
        QString localpathTemp = localpath;
        QString mediaPath = pathToSearch + XFileUtil::getSeperator(pathToSearch) + localpathTemp.remove(fileInfo.fileName());
        LogHandler::Debug("searchForFunscript http in library path: "+ mediaPath);
        funscriptPath = searchForFunscript(localpath, extensions, mediaPath);
        if(!funscriptPath.isEmpty()) {
            LogHandler::Debug("searchForFunscript from http found: "+ funscriptPath);
            return funscriptPath;
        } else {
            LogHandler::Debug("searchForFunscript from http not found");
        }
    } else {
        LogHandler::Debug("searchForFunscript not http");
    }
    return funscriptPath;
}


QString SyncHandler::searchForFunscriptMFS(QString mediaPath, QStringList pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscript(mediaPath, extensions, pathsToSearch);
}

QString SyncHandler::searchForFunscriptMFSDeep(QString mediaPath, QStringList pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs deep: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscriptDeep(mediaPath, extensions, pathsToSearch);
}

void SyncHandler::buildScriptItem(LibraryListItem27 &item, QString altScript)
{
    item.script = altScript;
    item.nameNoExtension = XFileUtil::getNameNoExtension(altScript);
    item.pathNoExtension = XFileUtil::getPathNoExtension(altScript);
}
