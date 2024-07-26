#include "xtengine.h"

#include "lib/handler/xmediastatehandler.h"
#include "lib/struct/ScriptInfo.h"
#include "lib/lookup/SettingMap.h"

XTEngine::XTEngine(QString appName, QObject* parent) : QObject(parent)
{
    QCoreApplication::setOrganizationName("cUrbSide prOd");
    QCoreApplication::setApplicationName(appName.isEmpty() ? "XTEngine" : appName);
    QCoreApplication::setApplicationVersion(SettingsHandler::XTEVersion);

    qRegisterMetaType<QItemSelection>();
    qRegisterMetaTypeStreamOperators<QList<QString>>("QList<QString>");
    qRegisterMetaTypeStreamOperators<ChannelModel>("ChannelModel");
    qRegisterMetaType<AxisDimension>("AxisDimension");
    qRegisterMetaType<AxisType>("AxisType");
    qRegisterMetaTypeStreamOperators<AxisName>("AxisName");
    qRegisterMetaTypeStreamOperators<DecoderModel>("DecoderModel");
    qRegisterMetaTypeStreamOperators<XMediaStatus>("XMediaStatus");
    qRegisterMetaType<LibraryListItem>();
    qRegisterMetaTypeStreamOperators<LibraryListItem>("LibraryListItem");
    qRegisterMetaTypeStreamOperators<QMap<QString, QList<LibraryListItem>>>("QMap<QString, QList<LibraryListItem>>");
    qRegisterMetaTypeStreamOperators<QList<LibraryListItem>>("QList<LibraryListItem>");
    qRegisterMetaType<LibraryListItem27>();
    qRegisterMetaTypeStreamOperators<LibraryListItem27>("LibraryListItem27");
    qRegisterMetaTypeStreamOperators<QMap<QString, QList<LibraryListItem27>>>("QMap<QString, QList<LibraryListItem27>>");
    qRegisterMetaTypeStreamOperators<QList<LibraryListItem27>>("QList<LibraryListItem27>");
    qRegisterMetaTypeStreamOperators<TCodeVersion>("TCodeVersion");
    qRegisterMetaTypeStreamOperators<LibraryListItemMetaData>("LibraryListItemMetaData");
    qRegisterMetaTypeStreamOperators<LibraryListItemMetaData258>("LibraryListItemMetaData258");
    qRegisterMetaTypeStreamOperators<Bookmark>("Bookmark");
    qRegisterMetaType<QVector<int> >("QVector<int>");

    qRegisterMetaType<ScriptInfo>("ScriptInfo");
    qRegisterMetaType<QList<ScriptInfo>>("QList<ScriptInfo>");


    SettingsHandler::Load();
    _tcodeFactory = new TCodeFactory(0.0, 1.0, this);
    _tcodeHandler = new TCodeHandler(this);
    _settingsActionHandler = new SettingsActionHandler(this);
    connect(_settingsActionHandler, &SettingsActionHandler::actionExecuted, this, [this](QString action, QString actionExecuted) {
        if (action == actions.SkipToMoneyShot)
        {
            skipToMoneyShot();
        }
        else if (action == actions.SkipToAction)
        {
            skipToNextAction();
        }
    });
    _mediaLibraryHandler = new MediaLibraryHandler(this);
    XMediaStateHandler::setMediaLibraryHandler(_mediaLibraryHandler);

    m_scheduler = new Scheduler(_mediaLibraryHandler, this);
    if(SettingsHandler::scheduleLibraryLoadEnabled())
        m_scheduler->startLibraryLoadSchedule();
    // connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoading, this, [this](){
    //     emit stopAllMedia();
    // });
    _connectionHandler = new ConnectionHandler(this);
    _syncHandler = new SyncHandler(this);
    m_heatmap = new HeatMap(this);
    connect(m_heatmap, &HeatMap::maxHeat, this, [](qint64 maxHeatAt) {
        if(maxHeatAt > 0) {
            auto item = XMediaStateHandler::getPlaying();
            if(item)
                SettingsHandler::instance()->setMoneyShot(item, maxHeatAt, false);
        }
    });

    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, [this]() {
        LogHandler::Info("XTEngine about to quit");
        SettingsHandler::Save();
        _syncHandler->stopAll();
    });
    connect(this, &XTEngine::destroyed, this, []() {
        LogHandler::Debug("XTEngine destroyed");
    });
}

XTEngine::~XTEngine() {
}

void XTEngine::init()
{
    if(SettingsHandler::getEnableHttpServer())
    {
        _httpHandler = new HttpHandler(_mediaLibraryHandler, this);
        connect(_httpHandler, &HttpHandler::tcode, _connectionHandler, &ConnectionHandler::sendTCode);
        connect(_httpHandler, &HttpHandler::xtpWebPacketRecieve, _connectionHandler, &ConnectionHandler::inputMessageSend);
        connect(_httpHandler, &HttpHandler::restartService, SettingsHandler::instance(), &SettingsHandler::Restart);
        connect(_httpHandler, &HttpHandler::skipToMoneyShot, this, &XTEngine::skipToMoneyShot);
        connect(_httpHandler, &HttpHandler::skipToNextAction, this, &XTEngine::skipToNextAction);
        connect(_httpHandler, &HttpHandler::cleanupThumbs, _mediaLibraryHandler, &MediaLibraryHandler::cleanGlobalThumbDirectory);
        connect(_httpHandler, &HttpHandler::connectInputDevice, _connectionHandler, &ConnectionHandler::initInputDevice);
        connect(_httpHandler, &HttpHandler::connectOutputDevice, _connectionHandler, &ConnectionHandler::initOutputDevice);
        connect(this, &XTEngine::stopAllMedia, _httpHandler, &HttpHandler::stopAllMedia);
        connect(_connectionHandler, &ConnectionHandler::connectionChange, _httpHandler, &HttpHandler::on_DeviceConnection_StateChange);
        connect(_httpHandler, &HttpHandler::settingChange, qOverload<QString, QVariant>(SettingsHandler::changeSetting));

        _httpHandler->listen();
    }

    connect(SettingsHandler::instance(), &SettingsHandler::settingChange, this, [this](QString key, QVariant value) {
        if(key == SettingKeys::scheduleLibraryLoadTime)
        {
            QTime newTime = QTime::fromString(value.toString());
            if(newTime.isValid())
                m_scheduler->runTimeChange(newTime);
        }
        else if(key == SettingKeys::scheduleLibraryLoadEnabled)
        {
            scheduleLibraryLoadEnableChange(value.toBool());
        }
    });
    
    connect(_connectionHandler, &ConnectionHandler::inputMessageRecieved, _syncHandler, [this](InputDevicePacket packet) {
        if(!_mediaLibraryHandler->isLoadingMediaPaths())
            _syncHandler->searchForFunscript(packet);
        else
            LogHandler::Warn("Waiting for media paths to load to search for funscripts....");
    });
    connect(_connectionHandler, &ConnectionHandler::inputMessageRecieved, this, [](InputDevicePacket packet) {
        XMediaStateHandler::updateDuration(packet.currentTime, packet.duration);
    });
    connect(_connectionHandler, &ConnectionHandler::action, _settingsActionHandler, &SettingsActionHandler::media_action);
    connect(_connectionHandler, &ConnectionHandler::inputConnectionChange, this, [this](ConnectionChangedSignal event) {
        auto selectedInputDevice = _connectionHandler->getSelectedInputDevice();
        if(event.type == DeviceType::Input && (!selectedInputDevice || selectedInputDevice->name() == event.deviceName))
        {
            _syncHandler->on_input_device_change(selectedInputDevice);
            if(event.status == ConnectionStatus::Connected)
            {
               _syncHandler->stopAll();
            }
        }
    });
    connect(_connectionHandler, &ConnectionHandler::outputConnectionChange, this, [this](ConnectionChangedSignal event) {
        auto selectedOutputDevice = _connectionHandler->getSelectedOutputDevice();
        if(event.type == DeviceType::Output && (!selectedOutputDevice || selectedOutputDevice->name() == event.deviceName))
        {
            _syncHandler->on_output_device_change(selectedOutputDevice);
            if(event.status == ConnectionStatus::Connected)
            {
                _connectionHandler->sendTCode(_tcodeHandler->getAllHome());
            }
        }
    });

    connect(_syncHandler, &SyncHandler::sendTCode, _connectionHandler, &ConnectionHandler::sendTCode, Qt::QueuedConnection);
    connect(_syncHandler, &SyncHandler::funscriptLoaded, this, [this](QString funscriptPath) {
        // Generate first load moneyshot based off heatmap if not already set.
        auto funscriptHandler = _syncHandler->getFunscriptHandler();
        if(funscriptHandler)
        {
            auto funscript = funscriptHandler->currentFunscript();
            auto playingItem = XMediaStateHandler::getPlaying();
            if(playingItem && playingItem->metadata.moneyShotMillis > 0)
                return;

            if(funscript) {
                m_heatmap->getMaxHeatAsync(funscript->actions);
            }
        }
    });
    connect(_syncHandler, &SyncHandler::funscriptSearchResult, this, &XTEngine::onFunscriptSearchResult);

    connect(_settingsActionHandler, &SettingsActionHandler::tcode_action, this, [this](QString tcode) {
        if(tcode == "DHOME") {
            tcode = _tcodeHandler->getAllHome();
        }
        _connectionHandler->sendTCode(tcode);
    });
    _connectionHandler->init();

    _mediaLibraryHandler->loadLibraryAsync();
}

void XTEngine::scheduleLibraryLoadEnableChange(bool enabled)
{
    enabled ?
        m_scheduler->startLibraryLoadSchedule() :
        m_scheduler->stopLibraryLoadSchedule();
}

void XTEngine::onFunscriptSearchResult(QString mediaPath, QString funscriptPath, qint64 mediaDuration)
{
    bool error = false;
    if(funscriptPath.isEmpty())
    {
        LogHandler::Warn("Funscript path was empty when starting sync");
        error = true;
    }
    if(mediaPath.isEmpty())
    {
        LogHandler::Warn("Media path was empty when starting sync");
        error = true;
    }
    if(_mediaLibraryHandler->isLoadingMediaPaths())
    {
        LogHandler::Warn("Please wait for library paths has been loaded before starting sync...");
        error = true;
    }
    if(error)
        return;
    LogHandler::Debug("Starting sync: "+funscriptPath);
    auto fileName = QUrl(mediaPath).fileName();
    auto itemRef = _mediaLibraryHandler->findItemByName(fileName);
    if(!itemRef) {
        LogHandler::Error("No external item found in media library. Setting up temporary item for: "+mediaPath);
        _mediaLibraryHandler->setupTempExternalItem(mediaPath, funscriptPath, mediaDuration);
        itemRef = _mediaLibraryHandler->findItemByName(fileName);
        XMediaStateHandler::setPlaying(itemRef);
    } else {
        XMediaStateHandler::setPlaying(itemRef);
    }
    _syncHandler->syncInputDeviceFunscript(itemRef);
}

void XTEngine::skipToNextAction()
{
    if(SettingsHandler::getEnableHttpServer() && _syncHandler->isPlayingVR())
    {
        qint64 nextActionMillis = _syncHandler->getFunscriptNext();
        if(nextActionMillis > 1500)
        {
            _httpHandler->sendWebSocketTextMessage("skipToNextAction", QString::number((nextActionMillis / 1000) - 1, 'f', 1));
        }
    }
}

void XTEngine::skipToMoneyShot()
{
    _syncHandler->skipToMoneyShot();
    if(SettingsHandler::getEnableHttpServer())
        _httpHandler->sendWebSocketTextMessage("skipToMoneyShot");
}

//void XTEngine::processVRMetaData(QString videoPath, QString funscriptPath, qint64 duration)
//{
//    QFileInfo videoFile(videoPath);
//    QString fileNameTemp = videoFile.fileName();
//    QString videoPathTemp = videoFile.fileName();
//    QString mediaExtension = "*" + fileNameTemp.remove(0, fileNameTemp.length() - (fileNameTemp.length() - fileNameTemp.lastIndexOf('.')));
//    QString scriptNoExtension = videoPathTemp.remove(videoPathTemp.lastIndexOf('.'), videoPathTemp.length() - 1);
//    QString zipFile;
//    if(funscriptPath.endsWith(".zip"))
//        zipFile = funscriptPath;
//    LibraryListItem27 vrItem ;
//    vrItem.type = LibraryListItemType::VR;
//    vrItem.path = videoPath; // path
//    vrItem.name = videoFile.fileName(); // name
//    vrItem.nameNoExtension = scriptNoExtension; //nameNoExtension
//    vrItem.script = funscriptPath; // script
//    vrItem.scriptNoExtension = scriptNoExtension;
//    vrItem.mediaExtension = mediaExtension;
//    vrItem.zipFile = zipFile;
//    vrItem.modifiedDate = videoFile.birthTime().isValid() ? videoFile.birthTime().date() : videoFile.created().date();
//    vrItem.duration = (unsigned)duration;
//    _mediaLibraryHandler->setLiveProperties(vrItem);

//    auto itemRef = _mediaLibraryHandler->findItemByName(QUrl(videoPath).fileName());
//    XMediaStateHandler::setPlaying(itemRef ? LibraryListItem27(itemRef) : vrItem);
//    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(XMediaStateHandler::getPlaying().path);
//    SettingsHandler::setLiveOffset(libraryListItemMetaData.offset);
//}
