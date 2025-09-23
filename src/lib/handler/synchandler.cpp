#include "synchandler.h"

#include "../tool/file-util.h"
#include "xmediastatehandler.h"

SyncHandler::SyncHandler(QObject* parent):
    QObject(parent)
{
    _tcodeHandler = new TCodeHandler(parent);
    connect(&m_funscriptSearch, &FunscriptSearch::searchFinish, this, &SyncHandler::funscriptSearchFinish);
}

SyncHandler::~SyncHandler()
{
}

void SyncHandler::togglePause()
{
    setPause(!_isPaused);
}
void SyncHandler::setPause(bool paused)
{
    _isPaused = paused;
    if(paused) {
        emit sendTCode("DSTOP");
    }
    emit togglePaused(_isPaused);
}
bool SyncHandler::isPaused()
{
    return _isPaused;
}

void SyncHandler::setStandAloneLoop(bool enabled)
{
    _standAloneLoop = enabled;
}

bool SyncHandler::isPlaying()
{
    return _funscriptVRFuture.isRunning() || _funscriptMediaFuture.isRunning() || _funscriptStandAloneFuture.isRunning();
}
bool SyncHandler::isPlayingInternal()
{
    return _funscriptMediaFuture.isRunning();
}
bool SyncHandler::isPlayingStandAlone()
{
    return _funscriptStandAloneFuture.isRunning();
}
bool SyncHandler::isPlayingVR()
{
    return _funscriptVRFuture.isRunning();
}

const Funscript *SyncHandler::getFunscript()
{
    return m_funscriptHandler.getFunscript();
}

const Funscript *SyncHandler::getFunscript(Track channel)
{
    if(!m_funscriptHandler.isLoaded(channel))
        return 0;
    return m_funscriptHandler.getFunscript(channel);
}

// FunscriptHandler* SyncHandler::getFunscriptHandler() {
//     if(_funscriptHandlers.isEmpty())
//         return 0;
//     return getFunscriptHandler(TCodeChannelLookup::Stroke()) ?: _funscriptHandlers.first();
// }

// FunscriptHandler *SyncHandler::getFunscriptHandler(QString channel)
// {
//     if(_funscriptHandlers.isEmpty())
//         return 0;
//     QList<FunscriptHandler *>::iterator it = std::find_if(_funscriptHandlers.begin(), _funscriptHandlers.end(), [channel](const FunscriptHandler* handler) {
//         return handler->channel() == channel;
//     });
//     return it != _funscriptHandlers.end() ? (*it) : 0;
// }

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem)
{
    LogHandler::Debug("Enter syncHandler load");
    return load(libraryItem, true);
}

SyncLoadState SyncHandler::load(const LibraryListItem27 &libraryItem, bool reset)
{
    if(reset)
        this->reset();
    else
    {
        auto loaded = m_funscriptHandler.getLoaded();
        foreach (auto name, loaded) {
            emit channelPositionChange(TCodeChannelLookup::ToString(name), 0, 0, ChannelTimeType::None);
        }
        m_funscriptHandler.unload();
    }
    SyncLoadState loadState;
    loadFunscripts(libraryItem, loadState);
    return loadState;
}

SyncLoadState SyncHandler::load(const QString& funscript) {
    LibraryListItem27 item;
    buildScriptItem(item, funscript);
    return load(item);
}

SyncLoadState SyncHandler::swap(const LibraryListItem27 &libraryItem, const ScriptInfo &script)
{
    LogHandler::Debug("Enter syncHandler swap");
    bool paused = isPaused();
    if(!paused)
        setPause(true);
    LibraryListItem27 swappedScriptItem;
    swappedScriptItem.copyProperties(libraryItem);
    buildScriptItem(swappedScriptItem, script.path);
    auto loadState = load(swappedScriptItem, false);
    if(!paused)
        setPause(false);
    return loadState;
}

SyncLoadState SyncHandler::swap(const ScriptInfo &script)
{
    auto playingItem = XMediaStateHandler::getPlaying();
    if(playingItem)
    {
        return swap(*playingItem, script);
    }
    return SyncLoadState();
}

void SyncHandler::updateMetadata(LibraryListItemMetaData258 value)
{
    auto playingMediaID = XMediaStateHandler::getPlayingID();
    if(!playingMediaID.isEmpty() && playingMediaID == value.ID)
        m_funscriptHandler.updateMetadata(value);
}

bool SyncHandler::isLoaded()
{
    return m_funscriptHandler.getLoaded().length() > 0;
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
    auto loaded = m_funscriptHandler.getLoaded();
    foreach (auto name, loaded) {
        emit channelPositionChange(TCodeChannelLookup::ToString(name), 0, 0, ChannelTimeType::None);
    }
    m_funscriptHandler.unload();
    QMutexLocker locker(&_mutex);
    _standAloneFunscriptCurrentTime = 0;
    setStandAloneLoop(false);
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
    stopAll();
    QMutexLocker locker(&_mutex);
    _standAloneFunscriptCurrentTime = 0;
    setStandAloneLoop(false);
    locker.unlock();
    emit funscriptStarted();
    qint64 funscriptMax = getFunscriptMax();
    emit funscriptStandaloneDurationChanged(funscriptMax);
    LogHandler::Debug("playStandAlone start thread");
    _funscriptStandAloneFuture = QtConcurrent::run([this, funscriptMax]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        int secCounter1 = 0;
        int secCounter2 = 0;
        QElapsedTimer mSecTimer;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        qint64 executionTimeNS = 1000000;
        mSecTimer.start();
        emit syncStart();
        while (!_funscriptStandAloneFuture.isCanceled())
        {
            double elapsedNS = timer2 - timer1;
            if (elapsedNS >= executionTimeNS)
            {
                timer1 = timer2;
                // if (_inputDeviceHandler && _inputDeviceHandler->isConnected()) {
                //     auto currentVRPacket = _inputDeviceHandler->getCurrentPacket();
                //     if(currentVRPacket.duration > 0)
                //         setPause(!currentVRPacket.playing);
                // }
                if(!isPaused())
                {
                    if(_seekTime > -1)
                    {
                        _standAloneFunscriptCurrentTime = _seekTime;
                    }
                    else
                    {
                        _standAloneFunscriptCurrentTime += ((elapsedNS/1000000) * XMediaStateHandler::getPlaybackSpeed());
                    }
                    QString tcode = buildChannelActions(_standAloneFunscriptCurrentTime);
                    if(_funscriptStandAloneFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                        sendPulse(mSecTimer.elapsed(), nextPulseTime);
                    }
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
            timer2 = mSecTimer.nsecsElapsed();
            if(!_standAloneLoop && _standAloneFunscriptCurrentTime >= funscriptMax)
            {
                _funscriptStandAloneFuture.cancel();
                if(!_isOtherMediaPlaying)
                    emit funscriptStatusChanged(XMediaStatus::EndOfMedia);
            }
            else if(_standAloneLoop && _standAloneFunscriptCurrentTime >= funscriptMax)
            {
                _standAloneFunscriptCurrentTime = 0;
            }
        }
        QMutexLocker locker(&_mutex);
        // _isMediaFunscriptPlaying = false;
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
    auto funscript = getFunscript();
    if(funscript && funscript->settings.max > -1)
    {
        return funscript->settings.min;
    }
    return 0;
}

qint64 SyncHandler::getFunscriptNext()
{
    auto funscript = getFunscript();
    if(funscript && funscript->settings.max > -1)
    {
        return m_funscriptHandler.getNext(funscript->settings.channel);
    }
    return 0;
}

qint64 SyncHandler::getFunscriptMax()
{
    auto funscript = getFunscript();
    if(funscript && funscript->settings.max > -1)
    {
        return funscript->settings.max;
    }
    qint64 otherMax = -1;
    auto loaded = m_funscriptHandler.getLoaded();
    foreach(auto name, loaded)
    {
        auto max = m_funscriptHandler.getMax(name);
        if(max > -1 && max > otherMax)
            otherMax = max;
    }
    return otherMax;
}

void SyncHandler::syncOtherMediaFunscript(std::function<qint64()> getMediaPosition)
{
    stopAll();
    QMutexLocker locker(&_mutex);
    LogHandler::Debug("syncFunscript start thread");
    _funscriptMediaFuture = QtConcurrent::run([this, getMediaPosition]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        QElapsedTimer mSecTimer;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        emit syncStart();
        while (!_funscriptMediaFuture.isCanceled() && _isOtherMediaPlaying)
        {
            if (timer2 - timer1 >= 1)
            {
                timer1 = timer2;
                if(!isPaused())
                {
                    qint64 currentTime = getMediaPosition();//_videoHandler->position();
                    // foreach(auto funscriptHandlerOther, _funscriptHandlers)
                    // {
                    //     auto action = funscriptHandlerOther->getPosition(currentTime);
                    //     if(action != nullptr)
                    //     {
                    //         actions.insert(funscriptHandlerOther->channel(), action);
                    //         emit channelPositionChange(funscriptHandlerOther->channel(), action->pos, action->speed, ChannelTimeType::Interval);
                    //     }
                    // }
                    QString tcode = buildChannelActions(currentTime);
                    if(_funscriptMediaFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                        sendPulse(mSecTimer.elapsed(), nextPulseTime);
                    }

                } else {
                    QThread::msleep(100);
                }
            }
            QThread::usleep(1);
            timer2 = (round(mSecTimer.nsecsElapsed() / 1000000));
        }
        _currentLocalVideoTime = 0;
        emit sendTCode("DSTOP");
        LogHandler::Debug("exit syncFunscript");
        emit syncEnd();
    });
}

void SyncHandler::syncInputDeviceFunscript(const LibraryListItem27 &libraryItem)
{
    load(libraryItem);
    QMutexLocker locker(&_mutex);
    LogHandler::Debug("syncInputDeviceFunscript start thread");
    _funscriptVRFuture = QtConcurrent::run([this]()
    {
        std::shared_ptr<FunscriptAction> actionPosition;
        InputConnectionPacket currentVRPacket;
        double timeTracker = 0;
        qint64 lastVRTime = 0;
        //qint64 lastVRSyncResetTime = 0;
        QElapsedTimer mSecTimer;
        qint64 timer1 = 0;
        qint64 timer2 = 0;
        mSecTimer.start();
        QString videoPath;
        qint64 duration = 0;
        qint64 nextPulseTime = SettingsHandler::getLubePulseFrequency();
        bool lastStatePlaying = false;
        double lastPlaybackSpeed = 1.0;
        qint64 executionTimeNS = 1000000;
        emit syncStart();
        while (!_funscriptVRFuture.isCanceled() && _inputDeviceHandler && _inputDeviceHandler->isConnected() && !_isOtherMediaPlaying)
        {
            //execute once every millisecond
            double elapsedNS = timer2 - timer1;
            if (elapsedNS >= executionTimeNS)
            {
                timer1 = timer2;
                currentVRPacket = _inputDeviceHandler->getCurrentPacket();
                if(lastStatePlaying && !currentVRPacket.playing) {
                    emit sendTCode("DSTOP");
                }
                lastStatePlaying = currentVRPacket.playing;
                if(currentVRPacket.playbackSpeed > 0 && lastPlaybackSpeed != currentVRPacket.playbackSpeed) {
                    XMediaStateHandler::setPlaybackSpeed(currentVRPacket.playbackSpeed);
                    lastPlaybackSpeed = currentVRPacket.playbackSpeed;
                    LogHandler::Debug("syncInputDeviceFunscript: change playback rate: " + QString::number(currentVRPacket.playbackSpeed));
                }
                //timer.start();
                if(currentVRPacket.playing && !isPaused() && isLoaded() && !currentVRPacket.path.isEmpty() && currentVRPacket.duration > 0)
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
                       //  LogHandler::Debug("elapsedNS: " + QString::number(elapsedNS));
                       //  LogHandler::Debug("elapsedNS/1000000: " + QString::number(elapsedNS/1000000));
                        timeTracker += ((elapsedNS/1000000) * lastPlaybackSpeed);
                       // LogHandler::Debug("timeTracker: " + QString::number(timeTracker));
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
                    // foreach(auto funscriptHandler, _funscriptHandlers)
                    // {
                    //     auto action = funscriptHandler->getPosition(vrTime);
                    //     if(action != nullptr)
                    //     {
                    //         actions.insert(funscriptHandler->channel(), action);
                    //         emit channelPositionChange(funscriptHandler->channel(), action->pos, action->speed, ChannelTimeType::Interval);
                    //     }
                    // }
                    QString tcode = buildChannelActions(vrTime);
                    if(_funscriptVRFuture.isCanceled())
                        break;
                    if(!tcode.isEmpty() && !isPaused())
                    {
                        emit sendTCode(tcode);
                    //     LogHandler::Debug("timer "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
                        sendPulse(timer1/1000000, nextPulseTime);
                    }
                //}
                } else {
                    QThread::msleep(100);
                }
            }
            timer2 = mSecTimer.nsecsElapsed();
            // LogHandler::Debug("timer nsecsElapsed: "+QString::number(timer2));
        }

        QMutexLocker locker(&_mutex);
        emit sendTCode("DSTOP");
        //emit funscriptVREnded(videoPath, funscript, duration);
        LogHandler::Debug("exit syncInputDeviceFunscript");
        XMediaStateHandler::setPlaybackSpeed(1.0);
        emit syncEnd();
    });
}

QString SyncHandler::buildChannelActions(qint64 time)
{
    QMap<QString, std::shared_ptr<FunscriptAction>> actions;
    auto loaded = m_funscriptHandler.getLoaded();
    foreach(auto track, loaded)
    {
        auto action = m_funscriptHandler.getPosition(track, time);
        if(action != nullptr)
        {
            auto channel = TCodeChannelLookup::ToString(track);
            actions.insert(channel, action);
            emit channelPositionChange(channel, action->pos, action->speed, ChannelTimeType::Interval);
        }
    }
    return _tcodeHandler->funscriptToTCode(actions);
}

// Private

// FunscriptHandler* SyncHandler::createFunscriptHandler(QString channel, QString funscript)
// {
//     FunscriptHandler* funscriptHandler = new FunscriptHandler(channel);
//     if(!funscriptHandler->load(funscript)) {
//         delete funscriptHandler;
//         return 0;
//     } else
//         _funscriptHandlers.append(funscriptHandler);
//     return funscriptHandler;
// }

// FunscriptHandler* SyncHandler::createFunscriptHandler(QString channel, QByteArray funscript)
// {
//     FunscriptHandler* funscriptHandler = new FunscriptHandler(channel);
//     if(!funscriptHandler->load(funscript)) {
//         delete funscriptHandler;
//         return 0;
//     } else
//         _funscriptHandlers.append(funscriptHandler);
//     return funscriptHandler;
// }

bool SyncHandler::loadFunscripts(const LibraryListItem27 &libraryItem, SyncLoadState &loadState)
{
    QString path = libraryItem.script.isEmpty() ? libraryItem.path : libraryItem.script;
    QString pathNoExtension = XFileUtil::getPathNoExtension(path);
#if BUILD_QT5
    QZipReader* zipFile = 0;
    if(!libraryItem.zipFile.isEmpty())
        zipFile = new QZipReader(libraryItem.zipFile, QIODevice::ReadOnly);
#endif
    QString mainScript = pathNoExtension + ".funscript";
    if(QFileInfo::exists(mainScript))
    {
        // Attempt to load SFMA format first. This will also load the main actions array if there is one.
        if(!m_funscriptHandler.load(mainScript))
        {
            LogHandler::Error("Found main funscript but there was an error loading it");
            loadState.invalidScripts.append("Script: " + mainScript);
        }
    }
    else
    {
        mainScript.clear();
    }
    // Start searching for stand alone scripts and override any found in the SFMA.
    auto channelNames = TCodeChannelLookup::getChannels();
    foreach(QString channelName, channelNames)
    {
        auto track = TCodeChannelLookup::getChannel(channelName);
        if(track->Type == ChannelType::HalfOscillate || (track->track == Track::Stroke && m_funscriptHandler.isLoaded(Track::Stroke)))
            continue;
        // IF we are the stroke channel and media doesnt exist in the XTP library,
        // then loop below will incorrectly think this is the L0 funscript as we
        // dont have the video path here.
        // Check every channel as a work around for now.
        if(libraryItem.type == LibraryListItemType::External && track->track == Track::Stroke)
        {
            foreach(auto channelName2, channelNames)
            {
                auto track2 = TCodeChannelLookup::getChannel(channelName2);
                if(pathNoExtension.endsWith("."+ track2->trackName))
                    continue;// Looking at this now I have no clue why this is here... need to test external media again.
            }
        }
        QString filePath = pathNoExtension + (track->track == Track::Stroke ? "" : "."+ track->trackName) + ".funscript";
        if(QFileInfo::exists(filePath))
        {
            QFileInfo fileInfo(filePath);
            LogHandler::Debug("Loading track: "+ fileInfo.absoluteFilePath());
            if(!m_funscriptHandler.load(track->track, fileInfo.absoluteFilePath()))
            {
                loadState.invalidScripts.append("Script: " + fileInfo.absoluteFilePath());
            }
            else
            {
                if(track->track == Track::Stroke || mainScript.isEmpty())
                {
                    mainScript = fileInfo.absoluteFilePath();
                }
            }
        }
#if BUILD_QT5
        else if(zipFile && zipFile->isReadable())
        {
            QString scriptFileNameNoExtension = XFileUtil::getNameNoExtension(pathNoExtension);
            //fileName.remove(fileName.lastIndexOf('.'), pathTemp.length() -  1);
            QString trackFileName = scriptFileNameNoExtension + (channelName == TCodeChannelLookup::Stroke() ? "" : "."+ track->TrackName) + ".funscript";
            QByteArray data = zipFile->fileData(trackFileName);
            if (!data.isEmpty())
            {
               LogHandler::Debug("Loading track from zip: "+ trackFileName);
                   funscriptHandler = createFunscriptHandler(channelName, data);
               if(!funscriptHandler)
                   loadState.invalidScripts.append("Zip script: " + trackFileName);
               else{
                   if(channelName == TCodeChannelLookup::Stroke() || mainScript.isEmpty())
                   {
                       mainScript = trackFileName;
                   }
               }
           }
        }
#endif
    }
    FunscriptHandler::updateMetadata(libraryItem.metadata);
    loadState.hasScript = m_funscriptHandler.isLoaded();
    if(loadState.hasScript) {
        loadState.mainScript = mainScript;
        emit funscriptLoaded(mainScript);
        if(libraryItem.type == LibraryListItemType::FunscriptType)
        {
            _playingStandAloneFunscript = loadState.mainScript;
        }
    }
#if BUILD_QT5
    if(zipFile)
        delete zipFile;
#endif
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

void SyncHandler::on_input_device_change(InputConnectionHandler* inputDeviceHandler) {
    _inputDeviceHandler = inputDeviceHandler;
}

void SyncHandler::on_output_device_change(OutputConnectionHandler* outputDeviceHandler) {
   // _outputDeviceHandler = outputDeviceHandler;
}

void SyncHandler::on_other_media_state_change(XMediaState state) {
    _isOtherMediaPlaying = state == XMediaState::Playing || state ==  XMediaState::Paused;
}

void SyncHandler::searchForFunscript(InputConnectionPacket packet)
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
        _funscriptSearchNotFound = false;
        stopAll();
        _lastSearchedMediaPath = videoPath;
    } else if(videoPath.isEmpty() || packet.duration <= 0 || isPlaying())
        return;

    if(!_funscriptSearchNotFound)
        m_funscriptSearch.searchForFunscript(videoPath, packet.duration);
}

void SyncHandler::funscriptSearchFinish(QString mediaPath, QString funscriptPath, qint64 mediaDuration)
{
    if(funscriptPath.isEmpty())
    {
        _funscriptSearchNotFound = true;
    }
    emit funscriptSearchResult(mediaPath, funscriptPath, mediaDuration);
}

void SyncHandler::buildScriptItem(LibraryListItem27 &item, QString altScript)
{
    item.script = altScript;
    item.nameNoExtension = XFileUtil::getNameNoExtension(altScript);
    item.pathNoExtension = XFileUtil::getPathNoExtension(altScript);
}
