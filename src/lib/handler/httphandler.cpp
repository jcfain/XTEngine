#include "httphandler.h"
#include <QUuid>
#include <QNetworkCookie>
#include "../tool/imagefactory.h"
#include "settingshandler.h"
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
        QJsonDocument doc(obj);
        QString itemJson = QString(doc.toJson(QJsonDocument::Compact));
        _webSocketHandler->sendCommand("tagsUpdate", itemJson);
    });

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
    connect(_webSocketHandler, &WebSocketHandler::reloadLibrary, _mediaLibraryHandler, &MediaLibraryHandler::loadLibraryAsync);

    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoading, this, &HttpHandler::onSetLibraryLoading);
    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoadingStatus, this, &HttpHandler::onLibraryLoadingStatusChange);
    connect(_mediaLibraryHandler, &MediaLibraryHandler::libraryLoaded, this, &HttpHandler::onSetLibraryLoaded);

    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveThumbError, this, [this](LibraryListItem27 item, bool vrMode, QString error) {_webSocketHandler->sendUpdateThumb(item.ID, item.thumbFileError, error);});
    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveNewThumb, this, [this](LibraryListItem27 item, bool vrMode, QString thumbFile) {
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
        if(_mediaLibraryHandler->isLibraryLoading())
            return;
        auto item = _mediaLibraryHandler->getLibraryCache().value(index);

        QString roleslist;
        foreach (int role, roles) {
            roleslist += QString::number(role);
            if(roles.indexOf(role) < roles.length() - 1)
                roleslist +=",";
        }
        QJsonDocument doc(createMediaObject(item, m_hostAddress));
        QString itemJson = QString(doc.toJson(QJsonDocument::Compact));
        _webSocketHandler->sendUpdateItem(itemJson, roleslist);
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::backgroundProcessStateChange, this, [this](QString message, float percentage) {
        QJsonObject obj;
        obj["message"] = message;
        obj["percentage"] = percentage;
        QJsonDocument doc(obj);
        _webSocketHandler->sendCommand("statusOutput", QString(doc.toJson(QJsonDocument::Compact)));
    });
    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveNewThumbLoading, this, [this](LibraryListItem27 item) {_webSocketHandler->sendUpdateThumb(item.ID, item.thumbFileLoadingCurrent);});
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

    QString extensions;
    extensions += SettingsHandler::getVideoExtensions().join("|");
    extensions += "|";
    extensions += SettingsHandler::getAudioExtensions().join("|");
    // _server->route("GET", "^/home", this, &HttpHandler::handleHome);
    _server->route("^/media/(.*\\.(("+extensions+")$))?[.]*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleVideoStream);
    router.addRoute("GET", "^/media/(.*\\.(("+SettingsHandler::getSubtitleExtensions().join("|")+")$))?[.]*$", this, &HttpHandler::handleSubtitle);
    _server->route("^/media$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleVideoList);
    _server->route("^/thumb/.*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleThumbFile);
    _server->route("^/funscript/(.*\\.((funscript)$))?[.]*$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleFunscriptFile);
    _server->route("^/deotest$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleDeo);
    _server->route("^/settings$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleSettings);
    _server->route("^/channels$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleChannels);
    _server->route("^/settings/availableSerialPorts$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleAvailableSerialPorts);
    _server->route("^/logout$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleLogout);
    _server->route("^/activeSessions$", QHttpServerRequest::Method::Get, this, &HttpHandler::handleActiveSessions);
    _server->route("POST", "^/settings$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleSettingsUpdate);
    _server->route("POST", "^/mediaItemMetadata$", this, &HttpHandler::handleMediaItemMetadataUpdate);
    // _server->route("POST", "^/channels$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleChannelsUpdate);
    _server->route("^/xtpweb$", this, &HttpHandler::handleWebTimeUpdate);
    _server->route("^/heresphere$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleHereSphere);
    _server->route("^/auth$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleAuth);
    _server->route("^/expireSession$", QHttpServerRequest::Method::Post, this, &HttpHandler::handleExpireSession);

}
bool HttpHandler::listen()
{
    if(!_server->listen(QHostAddress::Any, SettingsHandler::getHTTPPort())) {
        emit error("Error listening on port "+ QString::number(SettingsHandler::getHTTPPort()));
        return false;
    }
    return true;
}

HttpHandler::~HttpHandler()
{

}


QHttpServerResponse HttpHandler::handle(const QHttpServerRequest& request)
{
    auto hashedWebPass = SettingsHandler::hashedWebPass();
    QString urlPath = request.url().path();
    if(!hashedWebPass.isEmpty() && !isAuthenticated(request)) {
            QString path = urlPath;
            auto root = SettingsHandler::getHttpServerRoot();
            auto response = QHttpServerResponse::StatusCode::Ok;
            QString fileToSend;
            QString mimeType;
            if(path =="/common-min.js" || path =="/auth-min.js" ||
                path =="/js-sha3-min.js" ||
                path == "/styles-min.css"  ||
                path == "/faviicon.ico" ||
                path.startsWith("/://images"))
            {
                QString localPath;
                if(path.startsWith("/:"))
                {
                    localPath = path.removeFirst();
                }
                else
                {
                    localPath = root + path;
                }
                if(QFile::exists(localPath))
                {
                    mimeType = mimeDatabase.mimeTypeForFile(localPath, QMimeDatabase::MatchExtension).name();
                    fileToSend = localPath;
                    response = QHttpServerResponse::StatusCode::Ok;
                }
                else
                    response = QHttpServerResponse::StatusCode::BadRequest;
            }
            else if(path == "/")
            {
                LogHandler::Debug("Sending root auth-min.html");
                if(!QFileInfo::exists(root+"/auth-min.html"))
                {
                    LogHandler::Debug("file does not exist: "+root+"/auth-min.html");
                    response = QHttpServerResponse::StatusCode::BadRequest;
                }
                else
                {
                    fileToSend = root+"/auth-min.html";
                    mimeType = "text/html";
                    response = QHttpServerResponse::StatusCode::Ok;
                }
            }
            auto httpResponse = QHttpServerResponse(response);
            //, mimeType, "", -1, Z_DEFAULT_COMPRESSION)
            httpResponse.fromFile(fileToSend);
            return httpResponse;
    }

        // bool foundRoute = false;
        // QFuture<QHttpServerResponse> promise = router.route(data, &foundRoute);
        // if (foundRoute)
        //     return promise;

        QString path = urlPath;
        auto root = SettingsHandler::getHttpServerRoot();
        auto response = QHttpServerResponse::StatusCode::Ok;
        QString fileToSend;
        QString mimeType;

        if(path == "/") {
            LogHandler::Debug("Sending root index-min.html");
            if(!QFileInfo::exists(root+"/index-min.html"))
            {
                LogHandler::Debug("file does not exist: "+root+"/index-min.html");
                response = QHttpServerResponse::StatusCode::BadRequest;
            }
            else
            {
                fileToSend = root+"/index-min.html";
                mimeType = "text/html";
                response = QHttpServerResponse::StatusCode::Ok;
            }
        }
        else if(path.contains("favicon.ico"))
        {
            fileToSend = root+"/favicon.ico";
            mimeType = "image/x-icon";
            response = QHttpServerResponse::StatusCode::Ok;
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
                mimeType = mimeDatabase.mimeTypeForFile(localPath, QMimeDatabase::MatchExtension).name();
                fileToSend = localPath;
                response = QHttpServerResponse::StatusCode::Ok;
            }
            else
                response = QHttpServerResponse::StatusCode::BadRequest;
        }

        // TODO fix this shit
        auto httpResponse = QHttpServerResponse(response);
        //, mimeType, "", -1, Z_DEFAULT_COMPRESSION)
        httpResponse.fromFile(fileToSend);
        return httpResponse;
        else if(path == "/")
        {
            m_hostAddress = "http://" + data->request->headerDefault("Host", "") + "/";
            LogHandler::Debug("Sending root auth-min.html");
            if(!QFileInfo::exists(root+"/auth-min.html"))
            {
                LogHandler::Debug("file does not exist: "+root+"/auth-min.html");
                data->response->setStatus(HttpStatus::BadRequest);
            }
            else
            {
                data->response->sendFile(root+"/auth-min.html", "text/html", "", -1, Z_DEFAULT_COMPRESSION);
                data->response->setStatus(HttpStatus::Ok);
            }
        }

    } else if(path == "/") {
        m_hostAddress = "http://" + data->request->headerDefault("Host", "") + "/";
        LogHandler::Debug("Sending root index-min.html");
        if(!QFileInfo::exists(root+"/index-min.html"))
        {
            LogHandler::Debug("file does not exist: "+root+"/index-min.html");
            data->response->setStatus(HttpStatus::BadRequest);
        }
        else
        {
            data->response->sendFile(root+"/index-min.html", "text/html", "", -1, Z_DEFAULT_COMPRESSION);
            data->response->setStatus(HttpStatus::Ok);
        }
    }
    else if(path.contains("favicon.ico"))
    {
        data->response->sendFile(root+"/favicon.ico", "image/x-icon", "", -1, Z_DEFAULT_COMPRESSION);
        data->response->setStatus(HttpStatus::Ok);
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
            QString mimeType = mimeDatabase.mimeTypeForFile(localPath, QMimeDatabase::MatchExtension).name();
            data->response->sendFile(localPath, mimeType, "", -1, Z_DEFAULT_COMPRESSION);
            data->response->setStatus(HttpStatus::Ok);
        }
        else
            data->response->setStatus(HttpStatus::BadRequest);
    }
    return HttpPromise::resolve(data);
}

QHttpServerResponse HttpHandler::handleAuth(const QHttpServerRequest& request)
{
    auto body = request.body();
    auto response = QHttpServerResponse::StatusCode::Ok;
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("XTP Web json response error: "+error.errorString());
        LogHandler::Error("data: "+body);
        return QHttpServerResponse(QHttpServerResponse::StatusCode::BadRequest);
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
            QString lastSessionID = request.request->cookie("sessionID");
            if(!lastSessionID.isEmpty())
                m_authenticated.remove(lastSessionID);
            request.response->setCookie(cookie);
            request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
            request.response->compressBody();
            m_authenticated.insert(sessionID, createDate);
        } else {
            response = QHttpServerResponse::StatusCode::Unauthorized;
        }
    }
    auto httpResponse = QHttpServerResponse(response);
    return httpResponse;
}

QFuture<QHttpServerResponse> HttpHandler::handleActiveSessions(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }
    QJsonArray activeSessions;
    foreach(QString sessionID, m_authenticated.keys())
    {
        QJsonObject session;
        session["id"] = sessionID;
        session["lastAccessed"] = m_authenticated.value(sessionID).toString();
        session["expire"] = m_authenticated.value(sessionID).addSecs(m_sessionTimeout).toString();
        session["current"] = sessionID == request.request->cookie("sessionID");
        activeSessions.append(session);
    }
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(activeSessions));
    request.response->compressBody();
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleExpireSession(const QHttpServerRequest& request)
{
    if(request.request->uriQuery().hasQueryItem("sessionID")) {
        LogHandler::Debug("Auth from query.");
        QString sessionID = request.request->uriQuery().queryItemValue("sessionID");
        if(m_authenticated.contains(sessionID))
            m_authenticated.remove(sessionID);
        else {
            request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
            return HttpPromise::resolve(data);
        }
    }
    else {
        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
        return HttpPromise::resolve(data);
    }
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleLogout(const QHttpServerRequest& request)
{
    QString sessionID = request.request->cookie("sessionID");
    m_authenticated.remove(sessionID);
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleWebTimeUpdate(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }
    auto body = request.request->body();
    LogHandler::Debug("HTTP time sync update: "+QString(body));
    emit xtpWebPacketRecieve(body);
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return HttpPromise::resolve(data);
}
QFuture<QHttpServerResponse> HttpHandler::handleAvailableSerialPorts(const QHttpServerRequest& request) {
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }
    QJsonArray root;
    //root = Sett
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
    request.response->compressBody();
    return HttpPromise::resolve(data);
}
QFuture<QHttpServerResponse> HttpHandler::handleSettings(const QHttpServerRequest& request) {
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    QJsonObject root;
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

    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
    data->response->compressBody();
    
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
    request.response->compressBody();
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleSettingsUpdate(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    auto body = request.request->body();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("data: "+body);
        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
        return HttpPromise::resolve(data);
    }
    else
    {
        auto channels = TCodeChannelLookup::getChannels();
        foreach(auto channelName, channels)
        {
            auto channel = TCodeChannelLookup::getChannel(channelName);
            if(channel->Type == AxisType::HalfOscillate || channel->Type == AxisType::None)
                continue;
            auto value = doc["availableChannels"][channelName].toObject();
            ChannelModel33 channelModel = ChannelModel33::fromJson(value);
            SettingsHandler::setAxis(channelName, channelModel);
        }

        SettingsHandler::setMultiplierEnabled(doc["multiplierEnabled"].toBool());

        QJsonObject connection = doc["connection"].toObject();
        QJsonObject input = connection["input"].toObject();
        QJsonObject output = connection["output"].toObject();

        DeviceName selectedInputDevice = (DeviceName)input["selectedDevice"].toInt();
        QString deoAddress = input["deoAddress"].toString();
        QString deoPort = input["deoPort"].toString();
        if(deoAddress != SettingsHandler::getDeoAddress() || deoPort != SettingsHandler::getDeoPort())
        {
            SettingsHandler::setDeoAddress(deoAddress);
            SettingsHandler::setDeoPort(deoPort);
            if(selectedInputDevice == DeviceName::HereSphere)
                emit connectInputDevice(DeviceName::HereSphere, true);
        }
        DeviceName selectedOutputDevice = (DeviceName)output["selectedDevice"].toInt();
        QString networkAddress = output["networkAddress"].toString();
        QString networkPort = output["networkPort"].toString();
        if(!networkAddress.isEmpty() && (networkAddress != SettingsHandler::getServerAddress() || networkPort != SettingsHandler::getServerPort()))
        {
            SettingsHandler::setServerAddress(networkAddress);
            SettingsHandler::setServerPort(networkPort);
            if(selectedOutputDevice == DeviceName::Network)
                emit connectOutputDevice(DeviceName::Network, true);
        }
        QString serialPort = output["serialPort"].toString();
        if(!serialPort.isEmpty() && serialPort != SettingsHandler::getSerialPort())
        {
            SettingsHandler::setSerialPort(serialPort);
            if(selectedOutputDevice == DeviceName::Serial)
                emit connectOutputDevice(DeviceName::Serial, true);
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
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleMediaItemMetadataUpdate(const QHttpServerRequest& request)
{

    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    auto body = request.request->body();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("data: "+body);
        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
        return HttpPromise::resolve(data);
    }
    else
    {
        auto metaData = LibraryListItemMetaData258::fromJson(doc.object());
        SettingsHandler::updateLibraryListItemMetaData(metaData);
        SettingsHandler::setLiveOffset(metaData.offset);
    }
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleChannels(const QHttpServerRequest& request) {
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
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
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
    request.response->compressBody();
    return HttpPromise::resolve(data);
}

//QFuture<QHttpServerResponse> HttpHandler::handleChannelsUpdate(const QHttpServerRequest& request)
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
//QFuture<QHttpServerResponse> HttpHandler::handleDeviceConnected(const QHttpServerRequest& request)
//{
//    QJsonObject root;
////    root["status"] = _tcodeDeviceStatus.status;
////    root["deviceType"] = _tcodeDeviceStatus.deviceType;
////    root["message"] = _tcodeDeviceStatus.message;
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
//    return HttpPromise::resolve(data);
//}
//QFuture<QHttpServerResponse> HttpHandler::handleConnectDevice(const QHttpServerRequest& request)
//{
//    emit connectTCodeDevice();
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
//    return HttpPromise::resolve(data);
//}

//QFuture<QHttpServerResponse> HttpHandler::handleTCodeIn(const QHttpServerRequest& request)
//{
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
//    QString tcodeData(request.request->body());
//    if(!tcodeData.isEmpty())
//        emit tcode(tcodeData);
//    else
//        request.response->setStatus(QHttpServerResponse::StatusCode::BadRequest);
//    return HttpPromise::resolve(data);
//}

QFuture<QHttpServerResponse> HttpHandler::handleVideoList(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    QJsonArray media;
    // TODO conflict?
    QString hostAddress = "http://" + request.request->headerDefault("Host", "") + "/";
    foreach(auto item, _mediaLibraryHandler->getLibraryCache())
    {
        QJsonObject object;
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        media.append(createMediaObject(item, m_hostAddress));
    }
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(media));
    request.response->compressBody();
    return HttpPromise::resolve(data);
}

QJsonObject HttpHandler::createMediaObject(LibraryListItem27 item, QString hostAddress)
{
    //VideoFormat videoFormat;
    QJsonObject object;
    object["id"] = item.ID;
    object["type"] = (int)item.type;
    object["name"] = item.nameNoExtension;
    if(item.isMFS)
        object["displayName"] = "(MFS) " + item.nameNoExtension;
    else
        object["displayName"] = item.nameNoExtension;
    QString relativePath = item.path;
    relativePath = relativePath.replace(item.libraryPath +"/", "");
    object["path"] = hostAddress + "media/" + QString(QUrl::toPercentEncoding(relativePath));
    object["relativePath"] = "/" + QString(QUrl::toPercentEncoding(relativePath));
    if(!item.subtitle.isEmpty())
    {
        QString relativeSubtitlePath = item.subtitle;
        relativeSubtitlePath = relativeSubtitlePath.replace(item.libraryPath +"/", "");
        object["subtitle"] = hostAddress + "media/" + QString(QUrl::toPercentEncoding(relativeSubtitlePath));
        object["subtitleRelative"] = "/" + QString(QUrl::toPercentEncoding(relativeSubtitlePath));
    }
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
    object["isMFS"] = item.isMFS;
    object["tooltip"] = item.toolTip;
    object["hasScript"] = !item.script.isEmpty() || !item.zipFile.isEmpty();
    object["thumbState"] = (int)item.thumbState;
    object["thumbFileLoading"] = item.thumbFileLoading;
    object["thumbFileLoadingCurrent"] = item.thumbFileLoadingCurrent;
    object["thumbFileError"] = item.thumbFileError;
    object["thumbFileExists"] = item.thumbFileExists;
    object["loaded"] = false;
    object["playing"] = false;
    object["managedThumb"] = item.managedThumb;

    auto metaData = SettingsHandler::getLibraryListItemMetaData(item);
    object["metaData"] = LibraryListItemMetaData258::toJson(metaData);
    if(item.isMFS)
        object["displayName"] = "(MFS) " + item.nameNoExtension;

    return object;
}


QFuture<QHttpServerResponse> HttpHandler::handleHereSphere(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    QString hostAddress = "http://" + request.request->headerDefault("Host", "") + "/";
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

    foreach(auto item, _mediaLibraryHandler->getLibraryCache())
    {
        QJsonObject object;
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        list.append(createHeresphereObject(item, hostAddress));
    }

    root["library"] = list;
    request.response->header("HereSphere-JSON-Version", new QString("1"));
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
    return HttpPromise::resolve(data);
}
QJsonObject HttpHandler::createHeresphereObject(LibraryListItem27 item, QString hostAddress)
{
    QJsonObject root;
    QJsonArray encodings;
    QJsonObject encodingsObj;
    QJsonArray videoSources;
    QJsonObject videoSource;
    QString relativePath = item.path.replace(item.libraryPath, "");
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
    QString relativeThumb = item.thumbFile.isEmpty() ? "://images/icons/error.png" : item.thumbFile.replace(SettingsHandler::getSelectedThumbsDir(), "");
    root["thumbnailImage"] = hostAddress + "thumb/" + relativeThumb;
    //root["id"] = item.nameNoExtension;
    root["projection"] = _mediaLibraryHandler->getScreenType(item.path);
    //fisheye" - 180 degrees fisheye mesh, mkx200, "mkx200" - 200 degrees fisheye mesh
    root["stereo"] = _mediaLibraryHandler->getStereoMode(item.path);
//    root["skipIntro"] = 0;
    return root;
}
QJsonObject HttpHandler::createSelectedChannels() {
    QJsonObject availableChannelsJson;
    auto channels = TCodeChannelLookup::getChannels();
    foreach(auto channelName, channels)
    {
        auto channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type != AxisType::HalfOscillate && channel->Type != AxisType::None)
        {
            availableChannelsJson[channelName] = ChannelModel33::toJson(*channel);
        }
    }
    return availableChannelsJson;
}
QFuture<QHttpServerResponse> HttpHandler::handleDeo(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    QString hostAddress = "http://" + request.request->headerDefault("Host", "") + "/";
    QJsonObject root;
    QJsonArray scenes;
    QJsonObject library;
    QJsonArray list;

    foreach(auto item, _mediaLibraryHandler->getLibraryCache())
    {
        QJsonObject object;
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        list.append(createDeoObject(item, hostAddress));
    }

    library["libraryData"] = list;
    scenes.append(library);
    root["scenes"] = scenes;
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok, QJsonDocument(root));
    return HttpPromise::resolve(data);
}
QJsonObject HttpHandler::createDeoObject(LibraryListItem27 item, QString hostAddress)
{
    QJsonObject root;
    QJsonArray encodings;
    QJsonObject encodingsObj;
    QJsonArray videoSources;
    QJsonObject videoSource;
    QString relativePath = item.path.replace(item.libraryPath, "");
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
    QString relativeThumb = item.thumbFile.isEmpty() ? "://images/icons/error.png" : item.thumbFile.replace(SettingsHandler::getSelectedThumbsDir(), "");
    root["thumbnailUrl"] = hostAddress + "thumb/" + relativeThumb;
    return root;
}

QHttpServerResponse HttpHandler::handleFunscriptFile(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Unauthorized);
    }
    auto status = QHttpServerResponse::StatusCode::Ok

    auto match = request.url().state["match"].value<QRegularExpressionMatch>();
    QString parameter = match.captured();

    QString funscriptName = parameter.remove("/funscript/");
    if(funscriptName.contains("../"))
    {
        return QHttpServerResponse(QHttpServerResponse::StatusCode::Forbidden);
    }
//    QString filePath = SettingsHandler::getSelectedLibrary() + funscriptName;
//    if(!QFile(filePath).exists())
//    {
//        request.response->setStatus(QHttpServerResponse::StatusCode::NotFound);
//        return HttpPromise::resolve(data);
//    }
//    request.response->sendFile(filePath, "text/json", "", -1, Z_DEFAULT_COMPRESSION);
//    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    return QHttpServerResponse(QHttpServerResponse::StatusCode::Ok, funscriptName.constData());
}
QFuture<QHttpServerResponse> HttpHandler::handleThumbFile(const QHttpServerRequest& request)
{
    if(!isAuthenticated(request)) {
        request.response->setStatus(QHttpServerResponse::StatusCode::Unauthorized);
        return HttpPromise::resolve(data);
    }

    auto match = request.state["match"].value<QRegularExpressionMatch>();
    QString parameter = match.captured();
    QString thumbName = parameter.remove("/thumb/");
    if(thumbName.contains("../"))
    {
        request.response->setStatus(QHttpServerResponse::StatusCode::Forbidden);
        return HttpPromise::resolve(data);
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
            request.response->setStatus(QHttpServerResponse::StatusCode::NotFound);
            return HttpPromise::resolve(data);
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
        buffer.open(QIODevice::ReadWrite);
        pixmap.save(&buffer, "WEBP", quality);
        auto newObj = new QBuffer(&bytes);
        //LogHandler::Debug("Image resized: "+QString::number(bytes.length()));
        newObj->open(QIODevice::ReadOnly);
        request.response->sendFile(newObj, "image/webp", "", -1, Z_DEFAULT_COMPRESSION);
        buffer.close();
        newObj->close();
        delete newObj;
    }
    else {
        QString mimeType = mimeDatabase.mimeTypeForFile(thumbToSend, QMimeDatabase::MatchExtension).name();
        request.response->sendFile(thumbToSend, mimeType, "", -1, Z_DEFAULT_COMPRESSION);
    }
    request.response->setStatus(QHttpServerResponse::StatusCode::Ok);
    //buffer->deleteLater();
    return HttpPromise::resolve(data);
}

HttpPromise HttpHandler::handleSubtitle(HttpDataPtr data)
{
    if(!isAuthenticated(data)) {
        data->response->setStatus(HttpStatus::Unauthorized);
        return HttpPromise::resolve(data);
    }
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString parameter = match.captured();
    QString apiStr("/media/");
    QString subtitleFileName = parameter.replace(parameter.indexOf(apiStr), apiStr.size(), "");
    LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialSubtitle(subtitleFileName);
    if(!libraryItem || libraryItem->subtitle.isEmpty() || !QFileInfo::exists(libraryItem->subtitle))
    {
        data->response->setStatus(HttpStatus::NotFound);
        return HttpPromise::resolve(data);
    }
    QFileInfo fileInfo(libraryItem->subtitle);
    data->response->setHeader("Content-Disposition", "attachment");
    data->response->setHeader("filename", fileInfo.fileName());
    QString mimeType = mimeDatabase.mimeTypeForFile(libraryItem->subtitle, QMimeDatabase::MatchExtension).name();
    data->response->sendFile(libraryItem->subtitle, mimeType, "", -1, Z_DEFAULT_COMPRESSION);
    data->response->setStatus(HttpStatus::Ok);
    //buffer->deleteLater();
    return HttpPromise::resolve(data);
}

QFuture<QHttpServerResponse> HttpHandler::handleVideoStream(const QHttpServerRequest& request)
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

    return QPromise<const QHttpServerRequest&> {[&](
        const QtPromise::QPromiseResolve<const QHttpServerRequest&> &resolve,
        const QtPromise::QPromiseReject<const QHttpServerRequest&> &reject)
        {
            m_hlsFuture = QtConcurrent::run([=]()
            {
                try
                {
                    QElapsedTimer timer;
                    LogHandler::Debug("Enter Video stream");
                    timer.start();
                    auto match = request.state["match"].value<QRegularExpressionMatch>();
                    QString parameter = match.captured();
                    QString apiStr("/media/");
                    QString mediaName = parameter.replace(parameter.indexOf(apiStr), apiStr.size(), "");
                    if(mediaName.contains("../"))
                    {
                        request.response->setStatus(QHttpServerResponse::StatusCode::Forbidden);
                        resolve(data);
                        return;
                    }
                    LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialMediaPath(mediaName);
                    if(!libraryItem) {
                        LogHandler::Error(QString("Media item not found (%1)").arg(mediaName));
                        request.response->setStatus(QHttpServerResponse::StatusCode::NotFound);
                        resolve(data);
                        return;
                    }
                    QString filename = libraryItem->path;
                    //filename = _videoHandler->transcode(filename);
                    QFile file(filename);
                    if (!file.open(QIODevice::ReadOnly))
                    {
                        LogHandler::Error(QString("Unable to open file to be sent (%1): %2").arg(filename).arg(file.errorString()));
                        request.response->setStatus(QHttpServerResponse::StatusCode::Forbidden);
                        resolve(data);
                        return;
                    }
                    qint64 bytesAvailable = file.bytesAvailable();

                    QString range;
                    request.request->header<QString>("range", &range);
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
                       if((startByte == 0 && endByte == 1) || (endByte && (startByte + endByte) <= chunkSize))
                           chunkSize = startByte + endByte;
                       else
                           endByte = startByte + chunkSize;

                    if (startByte >= bytesAvailable){
                        LogHandler::Debug("RequestRangeNotSatisfiable: startByte: "+ QString::number(startByte));
                        LogHandler::Debug("RequestRangeNotSatisfiable: bytesAvailable: "+ QString::number(bytesAvailable));
                        request.response->setStatus(QHttpServerResponse::StatusCode::RequestRangeNotSatisfiable);
                        file.close();
                        resolve(data);
                        return;
                    }
                    if(endByte > bytesAvailable)
                        endByte = bytesAvailable;
                    LogHandler::Debug("chunkSize: "+ QString::number(chunkSize));
                    QString requestBytes = "bytes " + QString::number(startByte) + "-" + QString::number(endByte) + "/" + QString::number(bytesAvailable +1);
                    LogHandler::Debug("Request bytes: "+requestBytes);
                    if(startByte)
                        file.skip(startByte);
                    LogHandler::Debug("Video stream read start: "+ QString::number(timer.elapsed()));
                    QByteArray* byteArray = new QByteArray(file.read(chunkSize));
                    LogHandler::Debug("Video stream read end: "+ QString::number(timer.elapsed()));
                    QBuffer buffer(byteArray);
                    //LogHandler::Debug("Chunk bytes: "+ QString::number(buffer.bytesAvailable()));
                    //LogHandler::Debug("Video stream open buffer: "+ QString::number(timer.elapsed()));
                    if (!buffer.open(QIODevice::ReadOnly))
                    {
                        LogHandler::Error(QString("Unable to open buffer to be sent (%1): %2").arg(filename).arg(file.errorString()));

                        request.response->setStatus(QHttpServerResponse::StatusCode::Forbidden);
                        delete byteArray;
                        file.close();
                        resolve(data);
                        return;
                    }

                    request.response->setStatus(QHttpServerResponse::StatusCode::PartialContent);
                    qint64 contentLength = (endByte - startByte) + 1;
                    //LogHandler::Debug("Start bytes: " + QString::number(startByte));
                    //LogHandler::Debug("End bytes: " + QString::number(endByte));
                    //LogHandler::Debug("Content length: " + QString::number(contentLength));
                    request.response->setHeader("Accept-Ranges", "bytes");
                    request.response->setHeader("Content-Range", requestBytes);
                    request.response->setHeader("Content-Length", contentLength);
                    //LogHandler::Debug("Video stream send chunk: "+ QString::number(timer.elapsed()));
                    QString mimeType = mimeDatabase.mimeTypeForFile(filename, QMimeDatabase::MatchExtension).name();
                    request.response->sendFile(&buffer, mimeType);
                    LogHandler::Debug("Video stream send chunk finish: "+ QString::number(timer.elapsed()));
                    delete byteArray;
                    file.close();

                    resolve(data);
                }
                catch (...)
                {
                    reject(std::current_exception());
                }
            });
        }
    };
}


void HttpHandler::sendWebSocketTextMessage(QString command, QString message)
{
    if(_webSocketHandler)
        _webSocketHandler->sendCommand(command, message);
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

bool HttpHandler::isAuthenticated(const QHttpServerRequest& request)
{
    if(SettingsHandler::hashedWebPass().isEmpty()) {
        return true;
    }
    LogHandler::Debug("Auth from cookie.");
    QString sessionID = request.request->cookie("sessionID");
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
