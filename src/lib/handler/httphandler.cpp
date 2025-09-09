#include "httphandler.h"
#include <QUuid>
#include <QNetworkCookie>
#include "../tool/imagefactory.h"
#include "settingshandler.h"
#include "funscripthandler.h"
#include "../tool/medialibrarycache.h"
#include "xmediastatehandler.h"
// #include "xtpwebhandler.h"
// #include "../tool/videoformat.h"

HttpHandler::HttpHandler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent) : QObject(parent)
{
    _mediaLibraryHandler = mediaLibraryHandler;



    _webSocketHandler = new WebSocketHandler(this);

    connect(SettingsHandler::instance(), &SettingsHandler::tagsChanged, this, [this]() {
        QJsonObject obj;
        QJsonArray userTags;
        QJsonArray smartTags;
        foreach(auto tag, SettingsHandler::getUserTags()) {
            userTags.append(tag);
        }
        foreach(auto tag, SettingsHandler::getUserSmartTags()) {
            smartTags.append(tag);
        }
        obj["userTags"] = userTags;
        obj["smartTags"] = smartTags;
        _webSocketHandler->sendCommand("tagsUpdate", obj);
    });

    connect(_webSocketHandler, &WebSocketHandler::clean1024, this, &HttpHandler::clean1024);
    connect(_webSocketHandler, &WebSocketHandler::connectOutputDevice, this, &HttpHandler::connectOutputDevice);
    connect(_webSocketHandler, &WebSocketHandler::connectInputDevice, this, &HttpHandler::connectInputDevice);
    connect(_webSocketHandler, &WebSocketHandler::tcode, this, &HttpHandler::tcode);
    connect(_webSocketHandler, &WebSocketHandler::setChannelRange, this, [](QString channelName, int min, int max) {
        TCodeChannelLookup::setChannelRange(channelName, min, max);
    });
    connect(_webSocketHandler, &WebSocketHandler::changeChannelProfile, this, [](QString profileName) {
        if(TCodeChannelLookup::getSelectedChannelProfile() != profileName)
            TCodeChannelLookup::setSelectedChannelProfile(profileName);
    });
    connect(_webSocketHandler, &WebSocketHandler::newWebSocketConnected, this, &HttpHandler::on_webSocketClient_Connected);
    connect(_webSocketHandler, &WebSocketHandler::restartService, this, &HttpHandler::restartService);
    connect(_webSocketHandler, &WebSocketHandler::skipToMoneyShot, this, &HttpHandler::skipToMoneyShot);
    connect(_webSocketHandler, &WebSocketHandler::skipToNextAction, this, &HttpHandler::skipToNextAction);
    connect(_webSocketHandler, &WebSocketHandler::saveSingleThumb, this, [this](QString itemID, qint64 pos) {
        _mediaLibraryHandler->saveSingleThumb(itemID, pos);
    });
    connect(_webSocketHandler, &WebSocketHandler::cleanupThumbs, this, &HttpHandler::cleanupThumbs);
    connect(_webSocketHandler, &WebSocketHandler::reloadLibrary, _mediaLibraryHandler, &MediaLibraryHandler::loadLibraryAsync);
    connect(_webSocketHandler, &WebSocketHandler::startMetadataProcess, this, [this]() {
        if(!_mediaLibraryHandler->isLibraryProcessing() &&
            !_mediaLibraryHandler->metadataProcessing() &&
            !_mediaLibraryHandler->thumbProcessRunning())
        {
            connect(_mediaLibraryHandler, &MediaLibraryHandler::metadataProcessEnd, this, [this]() {
                disconnect(_mediaLibraryHandler, &MediaLibraryHandler::metadataProcessEnd, this, nullptr);
                _webSocketHandler->sendCommand("metadataProcessingFinished");
            });
            _mediaLibraryHandler->startMetadataProcess(true);
        }
    });
    connect(_webSocketHandler, &WebSocketHandler::cleanupMetadata, this, [this]() {
        if(!_mediaLibraryHandler->isLibraryProcessing() &&
            !_mediaLibraryHandler->metadataProcessing() &&
            !_mediaLibraryHandler->thumbProcessRunning())
        {
            _mediaLibraryHandler->startMetadataCleanProcess();
        }
    });
    connect(_webSocketHandler, &WebSocketHandler::processMetadata, this, [this](QString metadataKey) {
        if(!_mediaLibraryHandler->metadataProcessing()) {
            auto item = _mediaLibraryHandler->findItemByMetadataKey(metadataKey);
            if(item) {
                _mediaLibraryHandler->processMetadata(*item);
            } else
                _webSocketHandler->sendError("Unknown library item");
        } else {
            _webSocketHandler->sendUserWarning("Please wait for metadata process to complete!");
        }
    });
    connect(_webSocketHandler, &WebSocketHandler::mediaAction, this, &HttpHandler::mediaAction);
    connect(_webSocketHandler, &WebSocketHandler::swapScript, this, &HttpHandler::swapScript);



    connect(_webSocketHandler, &WebSocketHandler::settingChange, this, &HttpHandler::settingChange);
    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoading, this, &HttpHandler::onSetLibraryLoading);
    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoadingStatus, this, &HttpHandler::onLibraryLoadingStatusChange);
    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoaded, this, &HttpHandler::onSetLibraryLoaded);

    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveThumbError, this, [this](LibraryListItem27 item, QString error) {_webSocketHandler->sendUpdateThumb(item.ID, ERROR_IMAGE, error);});
    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveNewThumb, this, [this](LibraryListItem27 item, QString thumbFile) {
        QString relativeThumb;
        if(thumbFile.startsWith(SettingsHandler::getSelectedThumbsDir())) {
            relativeThumb = thumbFile.replace(SettingsHandler::getSelectedThumbsDir(), "");
            _webSocketHandler->sendUpdateThumb(item.ID, relativeThumb);
            return;
        }
        auto selectLibraryPaths = SettingsHandler::getSelectedLibrary();
        selectLibraryPaths.append(SettingsHandler::getVRLibrary());
        foreach (auto path, selectLibraryPaths) {
            if(thumbFile.startsWith(path)) {
                relativeThumb = thumbFile.replace(path, "");
                break;
            }
        }
        _webSocketHandler->sendUpdateThumb(item.ID, relativeThumb);
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::itemUpdated, this, [this](int index, QVector<int> roles) {
        if(_mediaLibraryHandler->isLibraryProcessing() || index < 0)
            return;
        _mediaLibraryHandler->getLibraryCache()->lockForRead();
        auto item = _mediaLibraryHandler->getLibraryCache()->at(index);
        itemUpdated(&item, roles);
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::itemAdded, this, [this](int index, int newSize) {
        if(_mediaLibraryHandler->isLibraryProcessing() || index < 0)
            return;
        _mediaLibraryHandler->getLibraryCache()->lockForRead();
        auto item = _mediaLibraryHandler->getLibraryCache()->at(index);
        QJsonDocument doc(createMediaObject(item, m_hostAddress));
        _mediaLibraryHandler->getLibraryCache()->unlock();
        QString itemJson = QString(doc.toJson(QJsonDocument::Compact));
        _webSocketHandler->sendAddItem(itemJson);
    });
    connect(_mediaLibraryHandler, QOverload<QString>::of(&MediaLibraryHandler::itemRemoved), this, [this](QString itemID) {
        QJsonObject ret;
        ret["itemID"] = itemID;
        _webSocketHandler->sendCommand("deleteItem", ret);
    });
    connect(_webSocketHandler, &WebSocketHandler::deleteMediaItem, this, [this](QString itemID) {
        QStringList errors;
        _mediaLibraryHandler->deleteItem(itemID, errors);
        if(!errors.empty()) {
            QString errorString = "There was errors when deleteing the media item.<br>";
            errorString += errors.join("<br>");
            _webSocketHandler->sendError(errorString);
        }
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::backgroundProcessStateChange, this, [this](QString message, float percentage) {
        QJsonObject obj;
        obj["message"] = message;
        obj["percentage"] = percentage;
        _webSocketHandler->sendCommand("statusOutput", obj);
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveNewThumbLoading, this, [this](LibraryListItem27 item) {_webSocketHandler->sendUpdateThumb(item.ID, LOADING_CURRENT_IMAGE);});

    // connect(_mediaLibraryHandler, &MediaLibraryHandler::thumbProcessBegin, this, [this]() {onLibraryLoadingStatusChange("Loading thumbs...");});

    connect(TCodeChannelLookup::instance(), &TCodeChannelLookup::channelProfileChanged, this, [this](QMap<QString, ChannelModel33>* channelProfile) {
        sendWebSocketTextMessage("channelProfileChanged", TCodeChannelLookup::getSelectedChannelProfile());
    });

    // config.port = SettingsHandler::getHTTPPort();
    // config.requestTimeout = 20;
    // if(LogHandler::getUserDebug())
    //     config.verbosity = HttpServerConfig::Verbose::All;
    // config.maxMultipartSize = 512 * 1024 * 1024;
    _server = new QHttpServer(this);
    connect(this, &HttpHandler::channelPositionChange, this, [this](QString channel, int position, int time, ChannelTimeType timeType){
        QJsonObject obj;
        obj["channel"] = channel;
        obj["position"] = position;
        obj["time"] = time;
        obj["timeType"] = (uint8_t)timeType;
        _webSocketHandler->sendCommand("channelPositionChange", obj);
    });

    connect(this, &HttpHandler::scriptTogglePaused, this, [this](bool paused){
        _webSocketHandler->sendScriptPaused(paused);
    });

    connect(this, &HttpHandler::actionExecuted, this, [this](QString action, QString spokenText, QVariant value){
        QJsonObject obj;
        obj["mediaAction"] = action;
        obj["spokenText"] = spokenText;
        switch(value.metaType().id())
        {
            case QMetaType::Bool:
                obj["value"] = value.toBool();
                break;
            case QMetaType::QString:
                obj["value"] = value.toString();
                break;
            case QMetaType::Double:
                obj["value"] = value.toDouble();
                break;
            case QMetaType::LongLong:
            case QMetaType::ULongLong:
                obj["value"] = value.toString();
                break;
            case QMetaType::Int:
            case QMetaType::UInt:
                obj["value"] = value.toInt();
                break;
        }

        _webSocketHandler->sendCommand("mediaAction", obj);
    });

    _server = new QHttpServer(this);
    // config.port = SettingsHandler::getHTTPPort();
    // // config.maxRequestSize =
    // config.requestTimeout = 20;
    // if(LogHandler::getUserDebug())
    //     config.verbosity = HttpServerConfig::Verbose::All;
    // config.maxMultipartSize = 512 * 1024 * 1024;



//    config.errorDocumentMap[HttpStatus::NotFound] = "data/404_2.html";
//    config.errorDocumentMap[HttpStatus::InternalServerError] = "data/404_2.html";
//    config.errorDocumentMap[HttpStatus::BadGateway] = "data/404_2.html";

    QString extensions;
    extensions += SettingsHandler::getVideoExtensions().join("|");
    extensions += "|";
    extensions += SettingsHandler::getAudioExtensions().join("|");
    _server->route("/", QHttpServerRequest::Method::Get,  this, &HttpHandler::handleRoot);
    _server->route("/auth", QHttpServerRequest::Method::Post, this, &HttpHandler::handleAuth);
    _server->route("/settings", QHttpServerRequest::Method::Get, this, &HttpHandler::handleSettings);
    _server->route("^/media/(.*\\.(("+extensions+")$))?[.]*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleVideoStream);
    _server->route("^/media/(.*\\.(("+SettingsHandler::getSubtitleExtensions().join("|")+")$))?[.]*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleSubtitle);
    _server->route("^/media$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleVideoList);
    _server->route("^/thumb/.*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleThumbFile);
    _server->route("^/funscript/(.*\\.((funscript)$))?[.]*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleFunscriptFile);
    _server->route("^/deotest$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleDeo);
    _server->route("^/channels$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleChannels);
    _server->route("^/availableSerialPorts$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleAvailableSerialPorts);
    _server->route("^/mediaActions$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleMediaActions);
    _server->route("^/logout$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleLogout);
    _server->route("^/activeSessions$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleActiveSessions);
    _server->route("^/settings$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleSettingsUpdate);
    _server->route("^/mediaItemMetadata$", QHttpServerRequest::Method::Post,  this, &HttpHandler::handleMediaItemMetadataUpdate);
    // _server->route("POST", "^/channels$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleChannelsUpdate);
    _server->route("^/xtpweb$",QHttpServerRequest::Method::Post, this, &HttpHandler::handleWebTimeUpdate);
    _server->route("^/heresphere$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleHereSphere);
    _server->route("^/expireSession$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleExpireSession);

    _server->route("/<arg>", QHttpServerRequest::Method::Get,  this, &HttpHandler::handleFile);

}
bool HttpHandler::listen()
{
    // if(!_server->listen(QHostAddress::Any, SettingsHandler::getHTTPPort())) {
    //     emit error("Error listening on port "+ QString::number(SettingsHandler::getHTTPPort()));
    //     return false;
    // }

    auto tcpserver = std::make_unique<QTcpServer>();
    if (!tcpserver->listen(QHostAddress::Any, SettingsHandler::getHTTPPort()) || !_server->bind(tcpserver.get())) {
        LogHandler::Warn(QCoreApplication::translate("QHttpServer",
                                                     "Http Server failed to listen on a port."));
        return false;
    }

    tcpserver.release();
    return true;
}

HttpHandler::~HttpHandler()
{

}


void  HttpHandler::handleRoot(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    auto root = SettingsHandler::getHttpServerRoot();
    auto hashedWebPass = SettingsHandler::hashedWebPass();
    m_hostAddress = "http://" + request.headers().value("Host", "") + "/";
    if(!hashedWebPass.isEmpty() && !isAuthenticated(request))
    {
        LogHandler::Debug("Sending root auth-min.html");
        if(!QFileInfo::exists(root+"/auth-min.html"))
        {
            LogHandler::Debug("file does not exist: "+root+"/auth-min.html");
            responder.write(QHttpServerResponse::StatusCode::BadRequest);
        }
        else
        {
            sendFile(responder, root+"/auth-min.html");
        }

    }
    else
    {
        LogHandler::Debug("Sending root index-min.html");
        if(!QFileInfo::exists(root+"/index-min.html"))
        {
            LogHandler::Debug("file does not exist: "+root+"/index-min.html");
            responder.write(QHttpServerResponse::StatusCode::BadRequest);
        }
        else
        {
            sendFile(responder, root+"/index-min.html");
        }
    }
}

QHttpServerResponse HttpHandler::handleFile(const QUrl &url, const QHttpServerRequest &request)
{
    auto root = SettingsHandler::getHttpServerRoot();
    auto hashedWebPass = SettingsHandler::hashedWebPass();
    auto path = request.url().path();
    if(!hashedWebPass.isEmpty() && !isAuthenticated(request)) {// Not authorized to see root with few exceptions
        if(path =="/common-min.js" || path =="/auth-min.js" ||
            path =="/js-sha3-min.js" ||
            path == "/styles-min.css"  ||
            path == "/faviicon.ico" ||
            path.startsWith("/://images"))
        {
            QString localPath;
            if(path.startsWith("/:"))
            {
                localPath = path.remove(0,1);
            }
            else
            {
                localPath = root + path;
            }
            if(QFile::exists(localPath))
            {
                return sendFile(localPath);
            }
            else
                return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
        }
        else
        {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
        }
    }
    else // Is authorized to see root.
    {
        if(path.contains("favicon.ico"))
        {
            return sendFile(root+"/favicon.ico");
        }
        else
        {
            QString localPath;
            if(path.startsWith("/:"))
            {
                localPath = path.remove(0,1);
            }
            else
            {
                localPath = root + path;
            }
            if(QFile::exists(localPath))
            {
                return sendFile(localPath);
            }
            else
                return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
        }
    }
}

void HttpHandler::handleAuth(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    auto body = request.body();
    auto response = QHttpServerResponse::StatusCode::Ok;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("XTP Web json response error: "+error.errorString());
        LogHandler::Error("data: "+body);
        responder.write(QHttpServerResponse::StatusCode::BadRequest);
    }
    else
    {
        if(SettingsHandler::hashedWebPass() == doc["hashedPass"].toString()) {
            QString sessionID = QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces);

            if(!m_sessionPolice.isActive()) {
                connect(&m_sessionPolice, &QTimer::timeout, this, [this]()
                {
                    QStringList expiredIDs;
                    foreach(QString sessionID, m_authenticated.keys())
                    {
                        if(m_authenticated.value(sessionID).addSecs(m_sessionTimeout) < QDateTime::currentDateTime())// Exipre if not accessed in 15min
                        {
                            expiredIDs << sessionID;
                        }
                    }
                    foreach (QString sessionID, expiredIDs)
                    {
                        m_authenticated.remove(sessionID);
                    }
                });
                // Run every 5 mins.
                m_sessionPolice.start(300);
            }

//            root["sessionID"] = sessionID;
            QDateTime dateTime;
            QDateTime createDate = QDateTime::currentDateTime();
            QNetworkCookie cookie("sessionID", sessionID.toUtf8());
            if(!doc["remember"].toBool(false))
                cookie.setExpirationDate(createDate.addDays(1));
            cookie.setPath("/");
            QString lastSessionID = getCookie(request, "sessionID");
            if(!lastSessionID.isEmpty())
                m_authenticated.remove(lastSessionID);
            // responder.->setCookie(cookie);
            // request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
            // request.response->compressBody();
            QHttpHeaders headers;
            headers.append(QHttpHeaders::WellKnownHeader::SetCookie, "sessionID="+sessionID);

            m_authenticated.insert(sessionID, createDate);
            responder.write(headers, QHttpServerResponse::StatusCode::Ok);
        } else {
            response = QHttpServerResponse::StatusCode::Unauthorized;
        }
    }
    responder.write(response);
}

void HttpHandler::handleActiveSessions(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    QJsonArray activeSessions;
    foreach(QString sessionID, m_authenticated.keys())
    {
        QJsonObject session;
        session["id"] = sessionID;
        session["lastAccessed"] = m_authenticated.value(sessionID).toString();
        session["expire"] = m_authenticated.value(sessionID).addSecs(m_sessionTimeout).toString();
        session["current"] = sessionID == getCookie(request, "sessionID");
        activeSessions.append(session);
    }
    QHttpHeaders headers;
    responder.write(QJsonDocument(activeSessions), headers, QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleExpireSession(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(request.query().hasQueryItem("sessionID")) {
        LogHandler::Debug("Auth from query.");
        QString sessionID = getCookie(request, "sessionID");
        if(m_authenticated.contains(sessionID))
            m_authenticated.remove(sessionID);
        else {
            responder.write(QHttpServerResponse::StatusCode::BadRequest);
            return;
        }
    }
    else {
        responder.write(QHttpServerResponse::StatusCode::BadRequest);
        return;
    }
    responder.write(QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleLogout(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    QString sessionID = getCookie(request, "sessionID");
    m_authenticated.remove(sessionID);
    responder.write(QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleWebTimeUpdate(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    auto body = request.body();
    //LogHandler::Debug("HTTP time sync update: "+QString(body));
    emit xtpWebPacketReceive(body);
    responder.write(QHttpServerResponse::StatusCode::Ok);
}
void HttpHandler::handleAvailableSerialPorts(const QHttpServerRequest &request, QHttpServerResponder &responder) {
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    QJsonArray root;
    //root = Sett
    QHttpHeaders headers;
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleMediaActions(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    MediaActions actions;
    QJsonObject root;
    foreach(QString action, actions.Values.keys())
    {
        root[action] = action;
    }
    QHttpHeaders headers;
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleSettings(const QHttpServerRequest &request, QHttpServerResponder &responder) {
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
// Use enum?
    QJsonObject root;
#if defined(Q_OS_WIN)
    root["OS"] = "WIN32";
#elif defined(Q_OS_MAC)
    root["OS"] = "MAC";
#elif defined(Q_OS_ANDROID)
    root["OS"] = "ANDROID";
#elif defined(Q_OS_LINUX)
    root["OS"] = "LINUX";
#endif
    root["xteVersion"] = SettingsHandler::XTEVersion;
    root["webSocketServerPort"] = _webSocketHandler->getServerPort();

    root["availableChannels"] = createSelectedChannels();

    root["multiplierEnabled"] = SettingsHandler::getMultiplierEnabled();

    root["selectedChannelProfile"] = TCodeChannelLookup::getSelectedChannelProfile();
    QJsonArray allChannelProfileNames;
    foreach (auto profile, TCodeChannelLookup::getChannelProfiles()) {
        allChannelProfileNames.append(profile);
    }
    root["allChannelProfileNames"] = allChannelProfileNames;

    QJsonObject connectionSettingsJson;
    QJsonObject connectionInputSettingsJson;
    QJsonObject connectionOutputSettingsJson;
    connectionInputSettingsJson["selectedDevice"] = SettingsHandler::getSelectedInputDevice();
    connectionInputSettingsJson["gamePadEnabled"] = SettingsHandler::getGamepadEnabled();
    connectionInputSettingsJson["deoAddress"] = SettingsHandler::getDeoAddress();
    connectionInputSettingsJson["deoPort"] = SettingsHandler::getDeoPort();
    connectionSettingsJson["input"] = connectionInputSettingsJson;

    connectionOutputSettingsJson["selectedDevice"] = SettingsHandler::getSelectedOutputDevice();
    connectionOutputSettingsJson["networkAddress"] = SettingsHandler::getServerAddress();
    connectionOutputSettingsJson["networkPort"] = SettingsHandler::getServerPort();
    connectionOutputSettingsJson["serialPort"] = SettingsHandler::getSerialPort();
    connectionSettingsJson["output"] = connectionOutputSettingsJson;

    root["connection"] = connectionSettingsJson;
    root["hasLaunchPass"] = !SettingsHandler::GetHashedPass().isEmpty();

    QJsonArray tags;
    QStringList allTags = SettingsHandler::getTags();
    foreach (QString tag, allTags) {
        tags.append(tag);
    }
    root["allTags"] = tags;

    QJsonArray userTagsArray;
    QStringList userTags = SettingsHandler::getUserTags();
    foreach (QString tag, userTags) {
        userTagsArray.append(tag);
    }
    root["userTags"] = userTagsArray;

    QJsonArray smartTagsArray;
    QStringList smartTags = SettingsHandler::getUserSmartTags();
    foreach (QString tag, smartTags) {
        smartTagsArray.append(tag);
    }
    root["smartTags"] = smartTagsArray;

    // root["scheduleLibraryLoadEnabled"] = SettingsHandler::scheduleLibraryLoadEnabled();
    // root["scheduleLibraryLoadTime"] = SettingsHandler::scheduleLibraryLoadTime().toString("hh:mm");
    // root["scheduleLibraryLoadFullProcess"] = SettingsHandler::scheduleLibraryLoadFullProcess();
    // root["processMetadataOnStart"] = SettingsHandler::processMetadataOnStart();
    QJsonArray settingsArray;
    foreach (SettingMap map, XSettingsMap::SettingsList) {
        QJsonObject settingsObj = map.tojson();
        QJsonObject value;
        SettingsHandler::getSetting(map.key, value);
        settingsObj["value"] = value[map.key];
        settingsArray.append(settingsObj);
    }
    root["settings"] = settingsArray;

    // root["APPIMAGE1"] = QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE"));
    // root["APPIMAGE2"] = QString(qgetenv("APPIMAGE"));
    QJsonArray args;
    foreach (auto arg, qApp->arguments()) {
        args.append(arg);
    }
    root["args"] = args;
    QHttpHeaders headers;
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleSettingsUpdate(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }

    auto body = request.body();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("data: "+body);
        responder.write(QHttpServerResponse::StatusCode::BadRequest);
        return;
    }
    else
    {
        auto channels = TCodeChannelLookup::getChannels();
        foreach(auto channelName, channels)
        {
            auto channel = TCodeChannelLookup::getChannel(channelName);
            if(channel->Type == ChannelType::HalfOscillate || channel->Type == ChannelType::None)
                continue;
            auto value = doc["availableChannels"][channelName].toObject();
            ChannelModel33 channelModel = ChannelModel33::fromJson(value);
            SettingsHandler::setAxis(channelName, channelModel);
        }

        SettingsHandler::setMultiplierEnabled(doc["multiplierEnabled"].toBool());

        QJsonObject connection = doc["connection"].toObject();
        QJsonObject input = connection["input"].toObject();
        QJsonObject output = connection["output"].toObject();

        ConnectionInterface selectedInputDevice = (ConnectionInterface)input["selectedDevice"].toInt();
        QString deoAddress = input["deoAddress"].toString();
        QString deoPort = input["deoPort"].toString();
        if(deoAddress != SettingsHandler::getDeoAddress() || deoPort != SettingsHandler::getDeoPort())
        {
            SettingsHandler::setDeoAddress(deoAddress);
            SettingsHandler::setDeoPort(deoPort);
            if(selectedInputDevice == ConnectionInterface::HereSphere)
                emit connectInputDevice(ConnectionInterface::HereSphere, true);
        }
        ConnectionInterface selectedOutputDevice = (ConnectionInterface)output["selectedDevice"].toInt();
        QString networkAddress = output["networkAddress"].toString();
        QString networkPort = output["networkPort"].toString();
        if(!networkAddress.isEmpty() && (networkAddress != SettingsHandler::getServerAddress() || networkPort != SettingsHandler::getServerPort()))
        {
            SettingsHandler::setServerAddress(networkAddress);
            SettingsHandler::setServerPort(networkPort);
            if(selectedOutputDevice == ConnectionInterface::Network)
                emit connectOutputDevice(ConnectionInterface::Network, true);
        }
        QString serialPort = output["serialPort"].toString();
        if(!serialPort.isEmpty() && serialPort != SettingsHandler::getSerialPort())
        {
            SettingsHandler::setSerialPort(serialPort);
            if(selectedOutputDevice == ConnectionInterface::Serial)
                emit connectOutputDevice(ConnectionInterface::Serial, true);
        }
        if(!doc["tagsToRemove"].isNull())
        {
            QJsonArray tagsToRemove = doc["tagsToRemove"].toArray();
            foreach (auto tag, tagsToRemove) {
                QString tagString = tag.toString();
                SettingsHandler::removeUserTag(tagString);
            }
        }
        if(!doc["tagsToAdd"].isNull())
        {
            QJsonArray tagsToAdd = doc["tagsToAdd"].toArray();
            foreach (auto tag, tagsToAdd) {
                QString tagString = tag.toString();
                SettingsHandler::addUserTag(tagString);
            }
        }
        if(!doc["smartTagsToAdd"].isNull())
        {
            QJsonArray smartTagsToAdd = doc["smartTagsToAdd"].toArray();
            foreach (auto tag, smartTagsToAdd) {
                QString tagString = tag.toString();
                SettingsHandler::addUserSmartTag(tagString);
            }
        }
        if(!doc["smartTagsToRemove"].isNull())
        {
            QJsonArray smartTagsToRemove = doc["smartTagsToRemove"].toArray();
            foreach (auto tag, smartTagsToRemove) {
                QString tagString = tag.toString();
                SettingsHandler::removeUserSmartTag(tagString);
            }
        }
        SettingsHandler::Save();
    }
    responder.write(QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleMediaItemMetadataUpdate(const QHttpServerRequest &request, QHttpServerResponder &responder)
{

    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    if(_mediaLibraryHandler->metadataProcessing()) {
        _webSocketHandler->sendUserWarning("Please wait for metadata process to complete!");
        responder.write(QHttpServerResponse::StatusCode::Processing);
    }

    auto body = request.body();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("data: "+body);
        responder.write(QHttpServerResponse::StatusCode::BadRequest);
        return ;
    }
    else
    {
        auto metadata = LibraryListItemMetaData258::fromJson(doc.object());
        auto libraryItem = _mediaLibraryHandler->findItemByNameNoExtension(metadata.key);
        if(libraryItem)
        {
            libraryItem->metadata = metadata;
            SettingsHandler::updateLibraryListItemMetaData(*libraryItem);
            emit updateMetadata(libraryItem->metadata);
        } else {
            SettingsHandler::setForceMetaDataFullProcess(true);
            responder.write(createError("Invalid metadata item please process metadata<br> In System tab under settings."), QHttpServerResponse::StatusCode::Conflict);
            return ;
        }
    }
    responder.write(QHttpServerResponse::StatusCode::Ok);
}

void HttpHandler::handleChannels(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
    }
    QJsonObject root;
//    QJsonObject allChannelProfiles;
//    auto profiles = TCodeChannelLookup::getChannelProfiles();
//    QJsonArray allChannelProfileNames;
//    foreach(auto profileName, profiles)
//    {
//        allChannelProfileNames.append(profileName);
//        auto channels = TCodeChannelLookup::getChannels(profileName);
//        QJsonObject availableChannelsJson;
//        foreach(auto channelName, channels)
//        {
//            auto channel = TCodeChannelLookup::getChannel(channelName);
//            if(channel->Type != AxisType::HalfOscillate && channel->Type != AxisType::None)
//            {
//                availableChannelsJson[channelName] = ChannelModel33::toJson(*channel);
//            }
//        }
//        allChannelProfiles[profileName] = availableChannelsJson;
//    }
//    root["selectedProfile"] = TCodeChannelLookup::getSelectedChannelProfile();
//    root["allChannelProfiles"] = allChannelProfiles;
//    root["allChannelProfileNames"] = allChannelProfileNames;

    root["availableChannels"] = createSelectedChannels();
    root["selectedChannelProfile"] = TCodeChannelLookup::getSelectedChannelProfile();
    QJsonArray allChannelProfileNames;
    foreach (auto profile, TCodeChannelLookup::getChannelProfiles()) {
        allChannelProfileNames.append(profile);
    }
    root["allChannelProfileNames"] = allChannelProfileNames;
    QHttpHeaders headers;
    headers.append("HereSphere-JSON-Version", QString("1"));
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}

//QHttpServerResponse HttpHandler::handleChannelsUpdate(const QHttpServerRequest &request, QHttpServerResponder &responder)
//{
//    if(!isAuthenticated(request)) {
//        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
//        return HttpPromise::resolve(data);
//    }

//    auto body = request.request->body();
//    QJsonParseError error;
//    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
//    if (doc.isEmpty())
//    {
//        LogHandler::Error("XTP Web json response error (handleChannelsUpdate): "+error.errorString());
//        LogHandler::Error("data: "+body);
//        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
//        return HttpPromise::resolve(data);
//    }
//    else
//    {
//        auto profiles = TCodeChannelLookup::getChannelProfiles();
//        QJsonObject availableChannelProfilesJson = doc["availableChannels"].toObject();
//        QJsonArray deletedProfiles = availableChannelProfilesJson["deleted"].toArray();
//        QStringList updatedProfiles;
//        QJsonArray docArray = availableChannelProfilesJson["updated"].toArray();
//        foreach(auto update, docArray) {
//            updatedProfiles << update.toString();
//        }

//        QJsonArray addedProfiles = availableChannelProfilesJson["added"].toArray();
//        foreach(auto profileName, updatedProfiles)
//        {
//            auto channels = TCodeChannelLookup::getChannels(profileName);
//            auto incomingProfile = availableChannelProfilesJson[profileName].toObject();
//            foreach(auto channelName, channels)
//            {
//                auto channel = TCodeChannelLookup::getChannel(channelName);
//                if(channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None)
//                    continue;
//                auto value = incomingProfile[channelName].toObject();
//                ChannelModel33 channelModel  = ChannelModel33::fromJson(value);
//                TCodeChannelLookup::setChannel(channelName, channelModel, profileName);
//            }
//        }
//        SettingsHandler::Save();
//        request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
//        return HttpPromise::resolve(data);
//    }
//}
//QHttpServerResponse HttpHandler::handleDeviceConnected(const QHttpServerRequest &request, QHttpServerResponder &responder)
//{
//    QJsonObject root;
////    root["status"] = _tcodeDeviceStatus.status;
////    root["deviceType"] = _tcodeDeviceStatus.deviceType;
////    root["message"] = _tcodeDeviceStatus.message;
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
//    return HttpPromise::resolve(data);
//}
//QHttpServerResponse HttpHandler::handleConnectDevice(const QHttpServerRequest &request, QHttpServerResponder &responder)
//{
//    emit connectTCodeDevice();
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
//    return HttpPromise::resolve(data);
//}

//QHttpServerResponse HttpHandler::handleTCodeIn(const QHttpServerRequest &request, QHttpServerResponder &responder)
//{
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
//    QString tcodeData(request.request->body());
//    if(!tcodeData.isEmpty())
//        emit tcode(tcodeData);
//    else
//        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
//    return HttpPromise::resolve(data);
//}

void HttpHandler::handleVideoList(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }

    QJsonArray media;
    // TODO conflict?
    QString hostAddress = "http://" + request.headers().value("Host", "") + "/";
    auto libraryCache = _mediaLibraryHandler->getLibraryCache();
    libraryCache->lockForRead();
    for(int i=0; i< libraryCache->length(); i++)
    {
        QJsonObject object;
        auto item = libraryCache->at(i);
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        media.append(createMediaObject(item, m_hostAddress));
    }
    libraryCache->unlock();
    // request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(media));
    // request.response->compressBody();
    // return HttpPromise::resolve(data);
    QHttpHeaders headers;
    headers.append("HereSphere-JSON-Version", QString("1"));
    responder.write(QJsonDocument(media), headers, QHttpServerResponse::StatusCode::Ok);
}

QJsonObject HttpHandler::createMediaObject(const LibraryListItem27& item, QString hostAddress)
{
    //VideoFormat videoFormat;
    QJsonObject object;
    object["id"] = item.ID;
    object["type"] = (int)item.type;
    object["name"] = item.nameNoExtension;
    QString displayText = (item.metadata.isSFMA && item.metadata.isMFS ? "(SFMA/MFS) " : item.metadata.isSFMA ? "(SFMA) " : item.metadata.isMFS ? "(MFS) " : "") + item.nameNoExtension;
    object["displayName"] = displayText;
    QString relativePath = item.path;
    relativePath = relativePath.replace(item.libraryPath +"/", "");
    object["path"] = hostAddress + "media/" + QString(QUrl::toPercentEncoding(relativePath));
    object["relativePath"] = "/" + QString(QUrl::toPercentEncoding(relativePath));
    if(!item.metadata.subtitle.isEmpty())
    {
        QString relativeSubtitlePath = item.metadata.subtitle;
        relativeSubtitlePath = relativeSubtitlePath.replace(item.libraryPath +"/", "");
        object["subtitle"] = hostAddress + "media/" + QString(QUrl::toPercentEncoding(relativeSubtitlePath));
        object["subtitleRelative"] = "/" + QString(QUrl::toPercentEncoding(relativeSubtitlePath));
    }
    object["script"] = item.script;
    QString scriptNoExtensionRelativePath = item.pathNoExtension;
    scriptNoExtensionRelativePath = scriptNoExtensionRelativePath.replace(item.libraryPath, "");
    object["scriptNoExtensionRelativePath"] = "funscript/" + QString(QUrl::toPercentEncoding(scriptNoExtensionRelativePath));
    QString thumbFile = item.thumbFile;
    thumbFile = thumbFile.replace(SettingsHandler::getSelectedThumbsDir(), "");
    thumbFile = thumbFile.replace(item.libraryPath, "");
    QString relativeThumb = thumbFile;
    object["thumb"] = hostAddress + "thumb/" + QString(QUrl::toPercentEncoding(relativeThumb));
    object["relativeThumb"] = QString(QUrl::toPercentEncoding(relativeThumb));
    object["thumbSize"] = SettingsHandler::getThumbSize();
    object["duration"] = QJsonValue::fromVariant(item.duration);
    object["modifiedDate"] = item.modifiedDate.toString(Qt::DateFormat::ISODate);
    object["isMFS"] = item.metadata.isMFS;
    object["tooltip"] = item.metadata.toolTip;
    object["hasScript"] = !item.script.isEmpty() || !item.zipFile.isEmpty();
    object["thumbState"] = (int)item.thumbState;
    object["thumbFileLoading"] = LOADING_IMAGE;
    object["thumbFileLoadingCurrent"] = LOADING_CURRENT_IMAGE;
    object["thumbFileError"] = ERROR_IMAGE;
    object["thumbFileUnknown"] = UNKNOWN_IMAGE;
    object["thumbFileExists"] = item.thumbFileExists;
    object["loaded"] = false;
    object["playing"] = false;
    object["managedThumb"] = item.managedThumb;
    // Metadata

    QJsonObject metadata = LibraryListItemMetaData258::toJson(item.metadata);
    // auto altScripts = _mediaLibraryHandler->filterAlternateFunscriptsForMediaItem(item.metadata.scripts);
    // QJsonArray altScriptsArray;
    // foreach (ScriptInfo info, altScripts) {
    //     altScriptsArray.append(ScriptInfo::toJson(info));
    // }
    // metadata["altScripts"] = altScriptsArray;
    object["metaData"] = metadata;

    return object;
}


void HttpHandler::handleHereSphere(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }

    QString hostAddress = "http://" + request.headers().value("Host", "") + "/";
    QJsonObject root;
    QJsonObject banner;
    root["access"] = 1;
    banner["image"] = "";
    banner["alpha"] = "";
    banner["link"] = "";
    root["banner"] = banner;
    QJsonObject library;
    library["name"] = "XTP Library";
    QJsonArray list;

    auto libraryCache = _mediaLibraryHandler->getLibraryCache();
    libraryCache->lockForRead();
    for(int i=0; i< libraryCache->length(); i++)
    {
        QJsonObject object;
        auto item = libraryCache->at(i);
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        list.append(createHeresphereObject(item, hostAddress));
    }
    libraryCache->unlock();

    root["library"] = list;
    QHttpHeaders headers;
    headers.append("HereSphere-JSON-Version", QString("1"));
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}
QJsonObject HttpHandler::createHeresphereObject(const LibraryListItem27& item, QString hostAddress)
{
    QJsonObject root;
    // QJsonArray encodings;
    // QJsonObject encodingsObj;
    // QJsonArray videoSources;
    // QJsonObject videoSource;
    QString path = item.path;
    QString relativePath = path.replace(item.libraryPath, "");
//    videoSource["resolution"] = 1080;
//    videoSource["url"] = hostAddress + "video" + QString(QUrl::toPercentEncoding(relativePath));
//    videoSources.append(videoSource);
//    //encodingsObj["name"] = "h264";
//    encodingsObj["videoSources"] = videoSources;
//    encodings.append(encodingsObj);
//    root["encodings"] = encodings;

    root["HereSphere-JSON-Version"] = 1;
    root["access"] = 1;
    root["title"] = item.nameNoExtension;
    root["dateAdded"] = item.modifiedDate.toString("YYYY-MM-DD");
    root["duration"] = (double)item.duration;
    root["url"] = hostAddress + "video/" + relativePath;
    QString relativeThumb = item.thumbFile.isEmpty() ? "://images/icons/error.png" : QString(item.thumbFile).replace(SettingsHandler::getSelectedThumbsDir(), "");
    root["thumbnailImage"] = hostAddress + "thumb/" + relativeThumb;
    //root["id"] = item.nameNoExtension;
    root["projection"] = _mediaLibraryHandler->getScreenType(item.path);
    //fisheye" - 180 degrees fisheye mesh, mkx200, "mkx200" - 200 degrees fisheye mesh
    root["stereo"] = _mediaLibraryHandler->getStereoMode(item.path);
//    root["skipIntro"] = 0;
    return root;
}
QJsonObject HttpHandler::createSelectedChannels() 
{
    QJsonObject availableChannelsJson;
    auto channels = TCodeChannelLookup::getChannels();
    foreach(auto channelName, channels)
    {
        auto channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type != ChannelType::None)
        {
            availableChannelsJson[channelName] = ChannelModel33::toJson(*channel);
        }
    }
    return availableChannelsJson;
}

QJsonDocument HttpHandler::createError(QString message)
{
    QJsonDocument doc;
    QJsonObject error;
    error["message"] = message;
    doc.setObject(error);
    return doc;
}

void HttpHandler::handleDeo(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }

    QString hostAddress = "http://" + request.headers().value("Host", "") + "/";
    QJsonObject root;
    QJsonArray scenes;
    QJsonObject library;
    QJsonArray list;

    auto libraryCache = _mediaLibraryHandler->getLibraryCache();
    libraryCache->lockForRead();
    for(int i=0; i< libraryCache->length(); i++)
    {
        QJsonObject object;
        auto item = libraryCache->at(i);
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        list.append(createDeoObject(item, hostAddress));
    }
    libraryCache->unlock();

    library["libraryData"] = list;
    scenes.append(library);
    root["scenes"] = scenes;

    QHttpHeaders headers;
    responder.write(QJsonDocument(root), headers, QHttpServerResponse::StatusCode::Ok);
}
QJsonObject HttpHandler::createDeoObject(const LibraryListItem27& item, QString hostAddress)
{
    QJsonObject root;
    // QJsonArray encodings;
    // QJsonObject encodingsObj;
    // QJsonArray videoSources;
    // QJsonObject videoSource;
    QString relativePath = QString(item.path).replace(item.libraryPath, "");
//    videoSource["resolution"] = 1080;
//    videoSource["url"] = hostAddress + "video" + QString(QUrl::toPercentEncoding(relativePath));
//    videoSources.append(videoSource);
//    //encodingsObj["name"] = "h264";
//    encodingsObj["videoSources"] = videoSources;
//    encodings.append(encodingsObj);
//    root["encodings"] = encodings;

    root["title"] = item.nameNoExtension;
    root["video_url"] = hostAddress + "video/" + relativePath;
    //root["id"] = item.nameNoExtension;
    root["videoLength"] = (int)(item.duration / 1000);
    root["is3d"] = true;
    root["screenType"] = _mediaLibraryHandler->getScreenType(item.path);
    //fisheye" - 180 degrees fisheye mesh, mkx200, "mkx200" - 200 degrees fisheye mesh
    root["stereoMode"] = _mediaLibraryHandler->getStereoMode(item.path);
//    root["skipIntro"] = 0;
    QString relativeThumb = item.thumbFile.isEmpty() ? "://images/icons/error.png" : QString(item.thumbFile).replace(SettingsHandler::getSelectedThumbsDir(), "");
    root["thumbnailUrl"] = hostAddress + "thumb/" + relativeThumb;
    return root;
}

void HttpHandler::handleFunscriptFile(const QHttpServerRequest &request, QHttpServerResponder &responder)
{
    if(!isAuthenticated(request)) {
        responder.write(QHttpServerResponse::StatusCode::Unauthorized);
        return;
    }
    auto status = QHttpServerResponse::StatusCode::Ok;

    QString parameter = getURL(request);

    QString funscriptName = parameter.remove("/funscript/");
    if(funscriptName.contains("../"))
    {
        responder.write(QHttpServerResponse::StatusCode::Forbidden);
    }
//    QString filePath = SettingsHandler::getSelectedLibrary() + funscriptName;
//    if(!QFile(filePath).exists())
//    {
//        request.response->setStatus(QHttpServerResponse::StatusCode::NotFound);
//        return HttpPromise::resolve(data);
//    }
//    request.response->sendFile(filePath, "text/json", "", -1, Z_DEFAULT_COMPRESSION);
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    // QHttpHeaders headers;
    // responder.write(funscriptName.data(), headers, QHttpServerResponse::StatusCode::Ok);
    responder.write(QHttpServerResponse::StatusCode::NotImplemented);
}
QHttpServerResponse HttpHandler::handleThumbFile(const QHttpServerRequest &request)
{
    if(!isAuthenticated(request)) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
    }

    QString parameter = getURL(request);
    QString thumbName = parameter.remove("/thumb/");
    if(thumbName.contains("../"))
    {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
    }

    QString thumbToSend;
    if(thumbName.startsWith(":"))
        thumbToSend = thumbName;
    else {
        LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialThumbPath(thumbName);
        if(libraryItem) {
            thumbToSend = libraryItem->thumbFile;
        }
        else
        {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
        }
    }
//    QString thumbDirFile = SettingsHandler::getSelectedThumbsDir() + thumbName;
//    QString libraryThumbDirFile = SettingsHandler::getSelectedLibrary() + "/" + thumbName;
//    if(thumbName.startsWith(":") || (thumbName.startsWith(SettingsHandler::getSelectedLibrary()) && QFileInfo::exists(thumbName)) ||
//            (thumbName.startsWith(SettingsHandler::getVRLibrary()) && QFileInfo::exists(thumbName)))
//    {
//        // System resource thumbs
//        thumbToSend = thumbName;
//    }
//    else if(QFileInfo::exists(libraryThumbDirFile))
//    {
//        // VR media thumbs
//        thumbToSend = libraryThumbDirFile;
//    }
//    else if(QFileInfo::exists(thumbDirFile))
//    {
//        // Global thumb directory thumbs.
//        thumbToSend = thumbDirFile;
//    }
    int quality = SettingsHandler::getHttpThumbQuality();
    if(quality > -1)
    {
        QPixmap pixmap = ImageFactory::resize(thumbToSend, {500, 500});
        QByteArray bytes;
        QBuffer buffer(&bytes);
        if(!buffer.open(QIODevice::ReadWrite) || pixmap.isNull()) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::InternalServerError);
        }
        if(!pixmap.save(&buffer, "JPG", quality)) {
            return QHttpServerResponse(QHttpServerResponse::StatusCode::InternalServerError);
        }
        // auto newObj = new QBuffer(&bytes);
        LogHandler::Debug("Image resized: "+QString::number(bytes.length()));
        // newObj->open(QIODevice::ReadOnly);
        // request.response->sendFile(newObj, "image/webp", "", -1, Z_DEFAULT_COMPRESSION);
        QHttpHeaders headers;
        // QString mimeType = mimeDatabase.mimeTypeForFile(thumbToSend, QMimeDatabase::MatchExtension).name();
        headers.append(QHttpHeaders::WellKnownHeader::ContentType, "image/jpeg");
        headers.append(QHttpHeaders::WellKnownHeader::ContentLength, QString::number(bytes.length()));
        // responder.write(newObj, headers, QHttpServerResponse::StatusCode::Ok);
        auto response = QHttpServerResponse(bytes);
        response.setHeaders(headers);
        buffer.close();
        // newObj->close();
        // delete newObj;
        return response;
    }
    else {
        return sendFile(thumbToSend);
    }
    // responder.write(QHttpServerResponse::StatusCode::Ok);
}

QHttpServerResponse HttpHandler::handleSubtitle(const QHttpServerRequest &request)
{
    if(!isAuthenticated(request)) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
    }
    QString parameter = getURL(request);
    //QString parameter = getURL(request);
    QString apiStr("/media/");
    QString subtitleFileName = parameter.replace(parameter.indexOf(apiStr), apiStr.size(), "");
    LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialSubtitle(subtitleFileName);
    if(!libraryItem || libraryItem->metadata.subtitle.isEmpty() || !QFileInfo::exists(libraryItem->metadata.subtitle))
    {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
    }

    QHttpHeaders headers;
    QFile fileInfo(libraryItem->metadata.subtitle);
    headers.append(QHttpHeaders::WellKnownHeader::ContentDisposition, "attachment");
    headers.append("filename", fileInfo.fileName());
    QString mimeType = mimeDatabase.mimeTypeForFile(libraryItem->metadata.subtitle, QMimeDatabase::MatchExtension).name();
    headers.append(QHttpHeaders::WellKnownHeader::ContentType, mimeType);
    headers.append(QHttpHeaders::WellKnownHeader::ContentLength, QString::number(fileInfo.size()));
    return sendFile(libraryItem->metadata.subtitle, headers);
}

QFuture<QHttpServerResponse> HttpHandler::handleVideoStream(const QHttpServerRequest &request)
{
//    if(!SettingsHandler::hashedWebPass().isEmpty()) {
//        LogHandler::Debug("Has session query: "+ request.request->uriStr());
//        if(request.request->uriQuery().hasQueryItem("sessionID")) {//Workaround for external streaming where the cookie isnt passed to the app.
//            LogHandler::Debug("Auth from query.");
//            QString sessionID = request.request->uriQuery().queryItemValue("sessionID");
//            if(sessionID.isEmpty() || !m_authenticated.contains(sessionID)) {
//                request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
//                LogHandler::Debug("Auth from query denied.");
//                return HttpPromise::resolve(data);
//            }
//        } else if(!isAuthenticated(request)) {
//            request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
//            LogHandler::Debug("Auth from cookie denied.");
//            return HttpPromise::resolve(data);
//        }
//    }
    auto match = getURL(request);//.state["match"].value<QRegularExpressionMatch>();
    auto requestHeaders = request.headers();

    m_hlsFuture = QtConcurrent::run([this, match, requestHeaders]()
    {
        try
        {
            QElapsedTimer timer;
            LogHandler::Debug("Enter Video stream");
            timer.start();
            QString parameter = match;
            QString apiStr("/media/");
            QString mediaName = parameter.replace(parameter.indexOf(apiStr), apiStr.size(), "");
            if(mediaName.contains("../"))
            {
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
            }
            LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialMediaPath(mediaName);
            if(!libraryItem) {
                LogHandler::Error(QString("Media item not found (%1)").arg(mediaName));
                return QHttpServerResponse(QHttpServerResponse::StatusCode::NotFound);
            }
            QString filename = libraryItem->path;
            //filename = _videoHandler->transcode(filename);
            QFile file(filename);
            if (!file.open(QIODevice::ReadOnly))
            {
                LogHandler::Error(QString("Unable to open file to be sent (%1): %2").arg(filename).arg(file.errorString()));
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
            }
            qint64 bytesAvailable = file.bytesAvailable();

            QString range = QString(requestHeaders.value(QHttpHeaders::WellKnownHeader::Range).toByteArray());
            LogHandler::Debug("Requested range: "+range);
            QStringList rangeKeyValue = range.split('=');
            qint64 startByte = 0;
            qint64 endByte = 0;
            if(rangeKeyValue.length() > 1)
            {
                QStringList rangeEnds = rangeKeyValue[1].split('-');
                if(rangeEnds.length() > 0)
                {
                    startByte = rangeEnds[0].toLongLong();
                    if(rangeEnds.length() > 1)
                        endByte = rangeEnds[1].toLongLong();
                }

            }
            LogHandler::Debug("startByte: "+ QString::number(startByte));
            LogHandler::Debug("endByte: "+ QString::number(endByte));
            qint64 chunkSize = SettingsHandler::getHTTPChunkSize();
            //if((startByte == 0 && endByte == 1) || (endByte && (startByte + endByte) <= chunkSize))
            if(endByte)
               chunkSize = startByte + endByte;
            else
               endByte = startByte + chunkSize;

            if (startByte >= bytesAvailable){
                LogHandler::Debug("RequestRangeNotSatisfiable: startByte: "+ QString::number(startByte));
                LogHandler::Debug("RequestRangeNotSatisfiable: bytesAvailable: "+ QString::number(bytesAvailable));
                return QHttpServerResponse(QHttpServerResponse::StatusCode::RequestRangeNotSatisfiable);
            }
            if(endByte > bytesAvailable)
                endByte = bytesAvailable;
            LogHandler::Debug("chunkSize: "+ QString::number(chunkSize));
            QString requestBytes = "bytes " + QString::number(startByte) + "-" + QString::number(endByte) + "/" + QString::number(bytesAvailable +1);
            LogHandler::Debug("Request bytes: "+requestBytes);
            if(startByte)
                file.skip(startByte);
            LogHandler::Debug("Video stream read start: "+ QString::number(timer.elapsed()));
            QByteArray byteArray(file.read(chunkSize));
            LogHandler::Debug("Video stream read end: "+ QString::number(timer.elapsed()));
            QBuffer buffer(&byteArray);
            //LogHandler::Debug("Chunk bytes: "+ QString::number(buffer.bytesAvailable()));
            //LogHandler::Debug("Video stream open buffer: "+ QString::number(timer.elapsed()));
            if (!buffer.open(QIODevice::ReadOnly))
            {
                LogHandler::Error(QString("Unable to open buffer to be sent (%1): %2").arg(filename).arg(file.errorString()));

                file.close();
                return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
            }
            qint64 contentLength = (endByte - startByte) + 1;
            //LogHandler::Debug("Start bytes: " + QString::number(startByte));
            //LogHandler::Debug("End bytes: " + QString::number(endByte));
            //LogHandler::Debug("Content length: " + QString::number(contentLength));
            QHttpHeaders headers;
            headers.append(QHttpHeaders::WellKnownHeader::AcceptRanges, "bytes");
            headers.append(QHttpHeaders::WellKnownHeader::ContentRange, requestBytes);
            // headers.append(QHttpHeaders::WellKnownHeader::ContentLength, QString::number(contentLength));
            //LogHandler::Debug("Video stream send chunk: "+ QString::number(timer.elapsed()));
            QString mimeType = mimeDatabase.mimeTypeForFile(filename, QMimeDatabase::MatchExtension).name();
            headers.append(QHttpHeaders::WellKnownHeader::ContentType, mimeType);
            // response->sendFile(&buffer, mimeType);
            auto response = QHttpServerResponse(byteArray, QHttpServerResponse::StatusCode::PartialContent);
            response.setHeaders(headers);
            LogHandler::Debug("Video stream send chunk finish: "+ QString::number(timer.elapsed()));
            file.close();
            return response;
        }
        catch (...)
        {
            // reject(std::current_exception());
            return QHttpServerResponse(QHttpServerResponse::StatusCode::InternalServerError);
        }
    });
    return m_hlsFuture;
}


void HttpHandler::sendWebSocketTextMessage(QString command, QString message)
{
    if(_webSocketHandler)
        _webSocketHandler->sendCommand(command, message);
}

void HttpHandler::stopAllMedia()
{
    sendWebSocketTextMessage("stopAllMedia");
}

void HttpHandler::onSetLibraryLoaded()
{
    _libraryLoaded = true;
    if(_webSocketHandler)
        _webSocketHandler->sendCommand("mediaLoaded");
}
void HttpHandler::onSetLibraryLoading()
{
    _libraryLoaded = false;
    if(_webSocketHandler)
        _webSocketHandler->sendCommand("mediaLoading");
}

void HttpHandler::onLibraryLoadingStatusChange(QString message) {
    _libraryLoadingStatus = message;
    if(_webSocketHandler)
        _webSocketHandler->sendCommand("mediaLoadingStatus", message);
}

bool HttpHandler::isAuthenticated(const QHttpServerRequest &request)
{
    if(SettingsHandler::hashedWebPass().isEmpty()) {
        return true;
    }
    LogHandler::Debug("Auth from cookie.");
    QString sessionID = getCookie(request, "sessionID");
    if(sessionID.isEmpty())
        return false;
    bool isAuthed = m_authenticated.contains(sessionID);
    if(!isAuthed) {
        LogHandler::Debug("Auth from cookie failed");
    } else {
        // Keep date last accessed.
        m_authenticated[sessionID] = QDateTime::currentDateTime();
    }
    return isAuthed;
}

QString HttpHandler::getURL(const QHttpServerRequest &request)
{
    auto match = request.url();;//.state["match"].value<QRegularExpressionMatch>();
    return match.path();
}

QString HttpHandler::getCookie(const QHttpServerRequest &request, const QString& name)
{
    auto headers = request.headers();
    auto cookieHeader = headers.wellKnownHeaderName(QHttpHeaders::WellKnownHeader::Cookie);
    auto header = request.headers().value(cookieHeader);
    QString cookie = QString(header.constData());
    if(cookie.contains("="))
    {
        auto arr = cookie.split("=");
        for (int i=0; i<arr.length(); i++) {
            if(arr[i] == name) {
                return arr[i+1];
            }
        }
    }
    return cookie;
}

void HttpHandler::sendFile(QHttpServerResponder &responder, const QString &path)
{
    QHttpHeaders headers;
    sendFile(responder, headers, path);
}

void HttpHandler::sendFile(QHttpServerResponder &responder, QHttpHeaders& headers, const QString &path)
{
    QFile fileInfo(path);
    // headers.append("filename", fileInfo.fileName());
    // QString mimeType = mimeDatabase.mimeTypeForFile(path, QMimeDatabase::MatchExtension).name();
    // if(mimeType.contains("empty")) {
    //     LogHandler::Debug("");
    // }
    // headers.append("Content-Type", mimeType);
    // // responder.write(fileInfo.readAll(), headers, QHttpServerResponse::StatusCode::Ok);
    QHttpServerResponse response = QHttpServerResponse::fromFile(path);
    // response.headers().append("filename", fileInfo.fileName());
    response.headers().append(QHttpHeaders::WellKnownHeader::ContentLength, QString::number(fileInfo.size()));
    responder.sendResponse(response);
    // if(response.mimeType() == "application/x-empty") {
    //     LogHandler::Debug("");
    // }

}

// void HttpHandler::sendFile(const QByteArray &data, const QByteArray &mimeType)
// {
//     // const QByteArray mimeType = QMimeDatabase().mimeTypeForFileNameAndData(fileName, data).name().toLocal8Bit();
//     return QHttpServerResponse(mimeType, data);
// }

// void HttpHandler::sendFile(const QIODevice &device)
// {

// }

QHttpServerResponse HttpHandler::sendFile(const QString &path)
{
    QHttpHeaders headers;
    return sendFile(path, headers);
}

QHttpServerResponse HttpHandler::sendFile(const QString &path, QHttpHeaders &headers)
{
    QFile fileInfo(path);
    // headers.append("filename", fileInfo.fileName());
    // QString mimeType = mimeDatabase.mimeTypeForFile(path, QMimeDatabase::MatchExtension).name();
    // if(mimeType.contains("empty")) {
    //     LogHandler::Debug("");
    // }
    // headers.append("Content-Type", mimeType);
    auto response = QHttpServerResponse::fromFile(path);
    // response.headers().append("filename", fileInfo.fileName());
    return response;
}

void HttpHandler::on_webSocketClient_Connected(QWebSocket* client)
{
    if(_webSocketHandler) {
        QString command = _libraryLoaded ? "mediaLoaded" : "mediaLoading";
        client->sendTextMessage("{ \"command\": \""+command+"\"}");
        client->sendTextMessage("{ \"command\": \"mediaLoadingStatus\", \"message\": \""+_libraryLoadingStatus+"\"}");
    }
}

void HttpHandler::on_DeviceConnection_StateChange(ConnectionChangedSignal status)
{
    if(_webSocketHandler)
        _webSocketHandler->sendDeviceConnectionStatus(status);
}

void HttpHandler::onSettingChange(QString settingName, QVariant value)
{
    _webSocketHandler->onSettingChange(settingName, value);
}

void HttpHandler::itemUpdated(const LibraryListItem27 *item, const QVector<int>& roles)
{
    if(!item)
        return;
    QString roleslist;
    foreach (int role, roles) {
        roleslist += QString::number(role);
        if(roles.indexOf(role) < roles.length() - 1)
            roleslist +=",";
    }
    QJsonDocument doc(createMediaObject(*item, m_hostAddress));
    _mediaLibraryHandler->getLibraryCache()->unlock();
    QString itemJson = QString(doc.toJson(QJsonDocument::Compact));
    _webSocketHandler->sendUpdateItem(itemJson, roleslist);
}
