#include "httphandler.h"
#include "../tool/imagefactory.h"

HttpHandler::HttpHandler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent):
    HttpRequestHandler(parent)
{
    _mediaLibraryHandler = mediaLibraryHandler;
    _webSocketHandler = new WebSocketHandler(this);
    connect(_webSocketHandler, &WebSocketHandler::connectOutputDevice, this, &HttpHandler::connectOutputDevice);
    connect(_webSocketHandler, &WebSocketHandler::connectInputDevice, this, &HttpHandler::connectInputDevice);
    connect(_webSocketHandler, &WebSocketHandler::tcode, this, &HttpHandler::tcode);
    connect(_webSocketHandler, &WebSocketHandler::newWebSocketConnected, this, &HttpHandler::on_webSocketClient_Connected);
    connect(_webSocketHandler, &WebSocketHandler::restartService, this, &HttpHandler::restartService);
    connect(_webSocketHandler, &WebSocketHandler::skipToMoneyShot, this, &HttpHandler::skipToMoneyShot);
    connect(_webSocketHandler, &WebSocketHandler::skipToNextAction, this, &HttpHandler::skipToNextAction);
    connect(_webSocketHandler, &WebSocketHandler::saveSingleThumb, this, [this](QString itemID, qint64 pos) {
        _mediaLibraryHandler->saveSingleThumb(itemID, pos);
    });

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
    connect(_mediaLibraryHandler, &MediaLibraryHandler::saveNewThumbLoading, this, [this](LibraryListItem27 item) {_webSocketHandler->sendUpdateThumb(item.ID, item.thumbFileLoadingCurrent);});
    // connect(_mediaLibraryHandler, &MediaLibraryHandler::thumbProcessBegin, this, [this]() {onLibraryLoadingStatusChange("Loading thumbs...");});

    config.port = SettingsHandler::getHTTPPort();
    config.requestTimeout = 20;
    if(LogHandler::getUserDebug())
        config.verbosity = HttpServerConfig::Verbose::All;
    config.maxMultipartSize = 512 * 1024 * 1024;
//    config.errorDocumentMap[HttpStatus::NotFound] = "data/404_2.html";
//    config.errorDocumentMap[HttpStatus::InternalServerError] = "data/404_2.html";
//    config.errorDocumentMap[HttpStatus::BadGateway] = "data/404_2.html";
    _server = new HttpServer(config, this);

    QString extensions;
    extensions += SettingsHandler::getVideoExtensions().join("|");
    extensions += "|";
    extensions += SettingsHandler::getAudioExtensions().join("|");
    router.addRoute("GET", "^/media/(.*\\.(("+extensions+")$))?[.]*$", this, &HttpHandler::handleVideoStream);
    router.addRoute("GET", "^/media$", this, &HttpHandler::handleVideoList);
    router.addRoute("GET", "^/thumb/.*$", this, &HttpHandler::handleThumbFile);
    router.addRoute("GET", "^/funscript/(.*\\.((funscript)$))?[.]*$", this, &HttpHandler::handleFunscriptFile);
    router.addRoute("GET", "^/deotest$", this, &HttpHandler::handleDeo);
    router.addRoute("GET", "^/settings$", this, &HttpHandler::handleSettings);
    router.addRoute("GET", "^/settings/availableSerialPorts$", this, &HttpHandler::handleAvailableSerialPorts);
    router.addRoute("POST", "^/settings$", this, &HttpHandler::handleSettingsUpdate);
    router.addRoute("POST", "^/xtpweb$", this, &HttpHandler::handleWebTimeUpdate);
    router.addRoute("POST", "^/heresphere$", this, &HttpHandler::handleHereSphere);
}
bool HttpHandler::listen()
{
    if(!_server->listen()) {
        emit error("Error listening on port "+ QString::number(config.port) + ": " + _server->errorString());
        return false;
    }
    return true;
}

HttpHandler::~HttpHandler()
{

}


HttpPromise HttpHandler::handle(HttpDataPtr data)
{
    bool foundRoute;
    HttpPromise promise = router.route(data, &foundRoute);
    if (foundRoute)
        return promise;

    auto path = data->request->uri().path();
    auto root = SettingsHandler::getHttpServerRoot();
    if(path == "/") {
        LogHandler::Debug("Sending root index-min.html");
        if(!QFileInfo(root+"/index-min.html").exists())
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
        QFile file(localPath);
        if(file.exists())
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

HttpPromise HttpHandler::handleWebTimeUpdate(HttpDataPtr data)
{
    auto body = data->request->body();
    LogHandler::Debug("HTTP time sync update: "+QString(body));
    emit xtpWebPacketRecieve(body);
    data->response->setStatus(HttpStatus::Ok);
    return HttpPromise::resolve(data);
}
HttpPromise HttpHandler::handleAvailableSerialPorts(HttpDataPtr data) {
    QJsonArray root;
    //root = Sett
    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
    data->response->compressBody();
    return HttpPromise::resolve(data);
}
HttpPromise HttpHandler::handleSettings(HttpDataPtr data) {
    QJsonObject root;
    root["webSocketServerPort"] = _webSocketHandler->getServerPort();

    QJsonObject availableAxisJson;
    auto availableAxis = TCodeChannelLookup::getChannels();
    foreach(auto channelName, availableAxis)
    {
        QJsonObject value;
        auto channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type != AxisType::HalfRange && channel->Type != AxisType::None)
        {
            value["axisName"] = channel->AxisName;
            value["channel"] = channel->Channel;
            value["damperEnabled"] = channel->DamperEnabled;
            value["damperValue"] = channel->DamperValue;
            value["dimension"] = (int)channel->Dimension;
            value["friendlyName"] = channel->FriendlyName;
            value["funscriptInverted"] = channel->FunscriptInverted;
            value["gamepadInverted"] = channel->GamepadInverted;
            value["linkToRelatedMFS"] = channel->LinkToRelatedMFS;
            value["max"] = channel->Max;
            value["mid"] = channel->Mid;
            value["min"] = channel->Min;
            value["multiplierEnabled"] = channel->MultiplierEnabled;
            value["multiplierValue"] = channel->MultiplierValue;
            value["relatedChannel"] = channel->RelatedChannel;
            value["trackName"] = channel->TrackName;
            value["type"] = (int)channel->Type;
            value["userMax"] = channel->UserMax;
            value["userMid"] = channel->UserMid;
            value["userMin"] = channel->UserMin;
            availableAxisJson[channelName] = value;
        }
    }
    root["availableAxis"] = availableAxisJson;

    root["multiplierEnabled"] = SettingsHandler::getMultiplierEnabled();

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

    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
    data->response->compressBody();
    return HttpPromise::resolve(data);
}

HttpPromise HttpHandler::handleSettingsUpdate(HttpDataPtr data)
{
    auto body = data->request->body();
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(body, &error);
    if (doc.isEmpty())
    {
        LogHandler::Error("XTP Web json response error: "+error.errorString());
        LogHandler::Error("data: "+body);
        data->response->setStatus(HttpStatus::BadRequest);
        return HttpPromise::resolve(data);
    }
    else
    {
        auto channels = TCodeChannelLookup::getChannels();
        foreach(auto channelName, channels)
        {
            auto channel = TCodeChannelLookup::getChannel(channelName);
            if(channel->Type == AxisType::HalfRange || channel->Type == AxisType::None)
                continue;
            auto value = doc["availableAxis"][channelName];
            ChannelModel33 channelModel = {
                value["friendlyName"].toString(),//QString FriendlyName;
                value["axisName"].toString(),//QString AxisName;
                value["channel"].toString(),//QString Channel;
                value["min"].toInt(),//int Min;
                value["mid"].toInt(),//int Mid;
                value["max"].toInt(),//int Max;
                value["userMin"].toInt(),//int UserMin;
                value["userMid"].toInt(),//int UserMid;
                value["userMax"].toInt(),//int UserMax;
                (AxisDimension)(value["dimension"].toInt()),//AxisDimension Dimension;
                (AxisType)(value["type"].toInt()),//AxisType Type;
                value["trackName"].toString(),//QString TrackName;
                value["multiplierEnabled"].toBool(),//bool MultiplierEnabled;
                float(value["multiplierValue"].toDouble()),//float MultiplierValue;
                value["damperEnabled"].toBool(),//bool DamperEnabled;
                float(value["damperValue"].toDouble()),//float DamperValue;
                value["funscriptInverted"].toBool(),//bool FunscriptInverted;
                value["gamepadInverted"].toBool(),//bool GamepadInverted;
                value["linkToRelatedMFS"].toBool(),//bool LinkToRelatedMFS;
                value["relatedChannel"].toString()//QString RelatedChannel;
            };
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

    }
    data->response->setStatus(HttpStatus::Ok);
    return HttpPromise::resolve(data);
}

//HttpPromise HttpHandler::handleDeviceConnected(HttpDataPtr data)
//{
//    QJsonObject root;
////    root["status"] = _tcodeDeviceStatus.status;
////    root["deviceType"] = _tcodeDeviceStatus.deviceType;
////    root["message"] = _tcodeDeviceStatus.message;
//    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
//    return HttpPromise::resolve(data);
//}
//HttpPromise HttpHandler::handleConnectDevice(HttpDataPtr data)
//{
//    emit connectTCodeDevice();
//    data->response->setStatus(HttpStatus::Ok);
//    return HttpPromise::resolve(data);
//}

//HttpPromise HttpHandler::handleTCodeIn(HttpDataPtr data)
//{
//    data->response->setStatus(HttpStatus::Ok);
//    QString tcodeData(data->request->body());
//    if(!tcodeData.isEmpty())
//        emit tcode(tcodeData);
//    else
//        data->response->setStatus(HttpStatus::BadRequest);
//    return HttpPromise::resolve(data);
//}

HttpPromise HttpHandler::handleVideoList(HttpDataPtr data)
{
    QJsonArray media;
    QString hostAddress = "http://" + data->request->headerDefault("Host", "") + "/";
    foreach(auto item, _mediaLibraryHandler->getLibraryCache())
    {
        QJsonObject object;
        if(item.type == LibraryListItemType::PlaylistInternal || item.type == LibraryListItemType::FunscriptType)
            continue;
        media.append(createMediaObject(item, hostAddress));
    }
    data->response->setStatus(HttpStatus::Ok, QJsonDocument(media));
    data->response->compressBody();
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
    QString relativePath = item.path.replace(item.libraryPath +"/", "");
    object["path"] = hostAddress + "media/" + QString(QUrl::toPercentEncoding(relativePath));
    object["relativePath"] = "/" + QString(QUrl::toPercentEncoding(relativePath));
    QString scriptNoExtensionRelativePath = item.scriptNoExtension.replace(item.libraryPath, "");
    object["scriptNoExtensionRelativePath"] = "funscript/" + QString(QUrl::toPercentEncoding(scriptNoExtensionRelativePath));
    QString thumbFile = item.thumbFile.replace(SettingsHandler::getSelectedThumbsDir(), "");
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

    auto metaData = SettingsHandler::getLibraryListItemMetaData(item.path);
    object["metaData"] = LibraryListItemMetaData258::toJson(metaData);
    if(item.isMFS)
        object["displayName"] = "(MFS) " + item.nameNoExtension;

    return object;
}


HttpPromise HttpHandler::handleHereSphere(HttpDataPtr data)
{
    QString hostAddress = "http://" + data->request->headerDefault("Host", "") + "/";
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
    data->response->header("HereSphere-JSON-Version", new QString("1"));
    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
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

HttpPromise HttpHandler::handleDeo(HttpDataPtr data)
{
    QString hostAddress = "http://" + data->request->headerDefault("Host", "") + "/";
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
    data->response->setStatus(HttpStatus::Ok, QJsonDocument(root));
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

HttpPromise HttpHandler::handleFunscriptFile(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString parameter = match.captured();

    QString funscriptName = parameter.remove("/funscript/");
    if(funscriptName.contains("../"))
    {
        data->response->setStatus(HttpStatus::Forbidden);
        return HttpPromise::resolve(data);
    }
//    QString filePath = SettingsHandler::getSelectedLibrary() + funscriptName;
//    if(!QFile(filePath).exists())
//    {
//        data->response->setStatus(HttpStatus::NotFound);
//        return HttpPromise::resolve(data);
//    }
//    data->response->sendFile(filePath, "text/json", "", -1, Z_DEFAULT_COMPRESSION);
//    data->response->setStatus(HttpStatus::Ok);
    return HttpPromise::resolve(data);
}
HttpPromise HttpHandler::handleThumbFile(HttpDataPtr data)
{
    auto match = data->state["match"].value<QRegularExpressionMatch>();
    QString parameter = match.captured();
    QString thumbName = parameter.remove("/thumb/");
    if(thumbName.contains("../"))
    {
        data->response->setStatus(HttpStatus::Forbidden);
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
            data->response->setStatus(HttpStatus::NotFound);
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
        data->response->sendFile(newObj, "image/webp", "", -1, Z_DEFAULT_COMPRESSION);
        buffer.close();
        newObj->close();
        delete newObj;
    }
    else {
        QString mimeType = mimeDatabase.mimeTypeForFile(thumbToSend, QMimeDatabase::MatchExtension).name();
        data->response->sendFile(thumbToSend, mimeType, "", -1, Z_DEFAULT_COMPRESSION);
    }
    data->response->setStatus(HttpStatus::Ok);
    //buffer->deleteLater();
    return HttpPromise::resolve(data);
}

HttpPromise HttpHandler::handleVideoStream(HttpDataPtr data)
{
    return QPromise<HttpDataPtr> {[&](
        const QtPromise::QPromiseResolve<HttpDataPtr> &resolve,
        const QtPromise::QPromiseReject<HttpDataPtr> &reject)
        {

            QtConcurrent::run([=]()
            {
                try
                {
                    QElapsedTimer timer;
                    LogHandler::Debug("Enter Video stream");
                    timer.start();
                    auto match = data->state["match"].value<QRegularExpressionMatch>();
                    QString parameter = match.captured();
                    QString apiStr("/media/");
                    QString mediaName = parameter.replace(parameter.indexOf(apiStr), apiStr.size(), "");
                    if(mediaName.contains("../"))
                    {
                        data->response->setStatus(HttpStatus::Forbidden);
                        resolve(data);
                        return;
                    }
                    LogHandler::Debug("Looking for media in library: " + mediaName);
                    LibraryListItem27* libraryItem = _mediaLibraryHandler->findItemByPartialMediaPath(mediaName);
                    if(!libraryItem) {
                        LogHandler::Error(QString("Media item not found (%1)").arg(mediaName));
                        data->response->setStatus(HttpStatus::NotFound);
                        resolve(data);
                        return;
                    }
                    QString filename = libraryItem->path;
                    //filename = _videoHandler->transcode(filename);
                    QFile file(filename);
                    if (!file.open(QIODevice::ReadOnly))
                    {
                        LogHandler::Error(QString("Unable to open file to be sent (%1): %2").arg(filename).arg(file.errorString()));
                        data->response->setStatus(HttpStatus::Forbidden);
                        resolve(data);
                        return;
                    }
                    qint64 bytesAvailable = file.bytesAvailable();

                    QString range;
                    data->request->header<QString>("range", &range);
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
                        data->response->setStatus(HttpStatus::RequestRangeNotSatisfiable);
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

                        data->response->setStatus(HttpStatus::Forbidden);
                        delete byteArray;
                        file.close();
                        resolve(data);
                        return;
                    }

                    data->response->setStatus(HttpStatus::PartialContent);
                    qint64 contentLength = (endByte - startByte) + 1;
                    //LogHandler::Debug("Start bytes: " + QString::number(startByte));
                    //LogHandler::Debug("End bytes: " + QString::number(endByte));
                    //LogHandler::Debug("Content length: " + QString::number(contentLength));
                    data->response->setHeader("Accept-Ranges", "bytes");
                    data->response->setHeader("Content-Range", requestBytes);
                    data->response->setHeader("Content-Length", contentLength);
                    //LogHandler::Debug("Video stream send chunk: "+ QString::number(timer.elapsed()));
                    QString mimeType = mimeDatabase.mimeTypeForFile(filename, QMimeDatabase::MatchExtension).name();
                    data->response->sendFile(&buffer, mimeType);
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
