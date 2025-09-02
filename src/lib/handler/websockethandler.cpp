#include "websockethandler.h"
#include "settingshandler.h"

#include "xmediastatehandler.h"

WebSocketHandler::WebSocketHandler(QObject *parent):
    QObject(parent),
    m_pWebSocketServer(new QWebSocketServer(QStringLiteral("XTP Websocket"),
                                            QWebSocketServer::NonSecureMode, this))
{
//    QNetworkProxy proxy;
//    proxy.setType(QNetworkProxy::Socks5Proxy);
    //proxy.setHostName("proxy.example.com");
//    proxy.setPort(80);
//    proxy.setUser("username");
//    proxy.setPassword("password");
//    QNetworkProxy::setApplicationProxy(proxy);
//    m_pWebSocketServer->setProxy(proxy);
    // m_pWebSocketServer->setProxy(QNetworkProxy::ProxyType::DefaultProxy);
    if (m_pWebSocketServer->listen(QHostAddress::Any, SettingsHandler::getWebSocketPort()))
    {
        LogHandler::Debug("Websocket listening on port " + QString::number(m_pWebSocketServer->serverPort()));
        connect(m_pWebSocketServer, &QWebSocketServer::newConnection, this, &WebSocketHandler::onNewConnection);
        connect(m_pWebSocketServer, &QWebSocketServer::closed, this, &WebSocketHandler::closed);
    }
    else
        LogHandler::Error("Websocket start fail on port: "+QString::number(SettingsHandler::getWebSocketPort()));
}

WebSocketHandler::~WebSocketHandler()
{
    sendCommand("connectionClosed");
    m_pWebSocketServer->close();
    //qDeleteAll(m_clients.begin(), m_clients.end());
}

QHostAddress WebSocketHandler::getAddress()
{
    return m_pWebSocketServer->serverAddress();
}
QUrl WebSocketHandler::getUrl()
{
    return m_pWebSocketServer->serverUrl();
}
QString WebSocketHandler::getServerName()
{
    return m_pWebSocketServer->serverName();
}
int WebSocketHandler::getServerPort()
{
    return m_pWebSocketServer->serverPort();
}

void WebSocketHandler::onSettingChange(QString setting, QVariant value)
{
    QJsonObject obj;
    obj[setting] = QJsonValue::fromVariant(value);
    sendCommand("settingChange", obj);
}

void WebSocketHandler::sendCommand(QString command, QString message, QWebSocket* client)
{
    QString commandJson;
    if(message.isEmpty())
        commandJson = "{ \"command\": \""+command+"\"}";
    else if( message.contains("{"))
        commandJson = "{ \"command\": \""+command+"\", \"message\": "+message+" }";
    else
        commandJson = "{ \"command\": \""+command+"\", \"message\": \""+message+"\"}";
    if(client)
        client->sendTextMessage(commandJson);
    else
        for (QWebSocket *pClient : qAsConst(m_clients))
        pClient->sendTextMessage(commandJson);
}

void WebSocketHandler::sendCommand(QString command, QJsonObject message, QWebSocket* client)
{
    QJsonDocument doc;
    QJsonObject obj;
    obj["command"] = command;
    obj["message"] = message;
    doc.setObject(obj);
    QString fullMessage = QString(doc.toJson(QJsonDocument::Compact));
    if(client)
        client->sendTextMessage(fullMessage);
    else
        for (QWebSocket *pClient : qAsConst(m_clients))
            pClient->sendTextMessage(fullMessage);
}

void WebSocketHandler::sendUserError(QString message)
{
    sendCommand("userError", message);
}

void WebSocketHandler::sendUserWarning(QString message)
{
    sendCommand("userWarning", message);
}

void WebSocketHandler::sendError(QString message)
{
    sendCommand("systemError", message);
}

void WebSocketHandler::sendWarning(QString message)
{
    sendCommand("systemWarning", message);
}

void WebSocketHandler::onNewConnection()
{
    QWebSocket *pSocket = m_pWebSocketServer->nextPendingConnection();

    connect(pSocket, &QWebSocket::textMessageReceived, this, &WebSocketHandler::processTextMessage);
    connect(pSocket, &QWebSocket::binaryMessageReceived, this, &WebSocketHandler::processBinaryMessage);
    connect(pSocket, &QWebSocket::disconnected, this, &WebSocketHandler::socketDisconnected);

    m_clients << pSocket;
    initNewClient(pSocket);

    emit newWebSocketConnected(pSocket);
}

void WebSocketHandler::initNewClient(QWebSocket* client)
{
    sendDeviceConnectionStatus(_inputDeviceStatus);
    sendDeviceConnectionStatus(_gamepadStatus);
    sendDeviceConnectionStatus(_outputDeviceStatus);
    sendScriptPaused(scriptPaused);
}

void WebSocketHandler::processTextMessage(QString message)
{
    LogHandler::Debug("WebSocket text message recieved: "+message);
    QJsonObject json;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if(!doc.isNull())
        if(doc.isObject())
            json = doc.object();
        else
            LogHandler::Error("Document is not an object");
    else
        LogHandler::Error("Invalid JSON...");
    QString command = json["command"].toString();
    if (command == "tcode") {
        QString commandMessage = json["message"].toString();
        emit tcode(commandMessage);
    } else if (command == "settingChange") {
        QJsonObject obj = json["message"].toObject();
        emit settingChange(obj["key"].toString(), obj["value"].toVariant());
    } else if (command == "setChannelRange") {
        QJsonObject obj = json["message"].toObject();
        emit setChannelRange(obj["channelName"].toString(), obj["min"].toInt(), obj["max"].toInt());
    } else if (command == "changeChannelProfile") {
        emit changeChannelProfile(json["message"].toString());
    } else if (command == "connectOutputDevice") {
        QJsonObject obj = json["message"].toObject();
        emit connectOutputDevice((ConnectionInterface)obj["deviceName"].toInt(), obj["enabled"].toBool());
    } else if (command == "connectInputDevice") {
        QJsonObject obj = json["message"].toObject();
        emit connectInputDevice((ConnectionInterface)obj["deviceName"].toInt(), obj["enabled"].toBool());
    } else if (command == "restartService") {
        emit restartService();
    } else if (command == "cleanupThumbs") {
        emit cleanupThumbs();
    } else if (command == "cleanupMetadata") {
        emit cleanupMetadata();
    } else if (command == "skipToMoneyShot") {
        emit skipToMoneyShot();
    } else if (command == "skipToNextAction") {
        emit skipToNextAction();
    } else if (command == "saveSingleThumb") {
        QJsonObject obj = json["message"].toObject();
        QString itemID = obj["itemID"].toString();
        qint64 pos = obj["pos"].toDouble() * 1000;
        emit saveSingleThumb(itemID, pos);
    } else if (command == "reloadLibrary") {
        emit reloadLibrary();
    } else if(command == "startMetadataProcess") {
        emit startMetadataProcess();
    } else if(command == "processMetadata") {
        QString metadataKey = json["message"].toString();
        emit processMetadata(metadataKey);
    } else if(command == "mediaAction") {
        QString action = json["message"].toString();
        emit mediaAction(action);
    } else if (command == "swapScript") {
        QJsonObject obj = json["message"].toObject();
        ScriptInfo scriptInfo = ScriptInfo::fromJson(obj);
        emit swapScript(scriptInfo);
    } else if (command == "setPlaybackRate") {
        auto value = json["message"].toString().toDouble();
        XMediaStateHandler::setPlaybackSpeed(value);
    } else if(command == "clean1024") {
        emit clean1024();
    } else if(command == "deleteMediaItem") {
        QJsonObject obj = json["message"].toObject();
        QString itemID = obj["itemID"].toString();
        emit deleteMediaItem(itemID);
    }

}

void WebSocketHandler::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    LogHandler::Debug("WEBSOCKET Binary Message received: " + message);
    if (pClient)
        pClient->sendBinaryMessage(message);
}

void WebSocketHandler::socketDisconnected()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    LogHandler::Debug("WEBSOCKET socketDisconnected: " + pClient->objectName());
    if (pClient)
    {
        m_clients.removeAll(pClient);
        pClient->deleteLater();
    }
}

void WebSocketHandler::closed()
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    LogHandler::Debug("WEBSOCKET closed: " + pClient->objectName());
}

void  WebSocketHandler::sendDeviceConnectionStatus(ConnectionChangedSignal status, QWebSocket* client)
{
    QString messageJson = "{ \"status\": "+QString::number(status.status)+", \"deviceName\": "+QString::number(status.connectionName)+", \"message\": \""+status.message+"\" }";
    if(status.type == ConnectionDirection::Output)
    {
        _outputDeviceStatus = status;
        sendCommand("outputDeviceStatus", messageJson, client);
    }
    else if(status.connectionName == ConnectionInterface::Gamepad)
    {
        _gamepadStatus = status;
        sendCommand("inputDeviceStatus", messageJson, client);
    }
    else
    {
        _inputDeviceStatus = status;
        sendCommand("inputDeviceStatus", messageJson, client);
    }
}
void  WebSocketHandler::sendUpdateThumb(QString id, QString thumb, QString errorMessage)
{
    QString messageJson;
    if(errorMessage.isEmpty())
        messageJson = "{ \"id\": \""+id+"\", \"thumb\": \""+thumb+"\"}";
    else
        messageJson = "{ \"id\": \""+id+"\", \"thumb\": \""+thumb+"\", \"errorMessage\": \""+errorMessage+"\"}";
    sendCommand("updateThumb", messageJson);
}

void WebSocketHandler::sendUpdateItem(QString itemJson, QString roleslist, QString error)
{
    QString messageJson;
    if(error.isEmpty())
        messageJson = "{ \"item\": "+itemJson+", \"roles\": ["+roleslist+"]}";
    else
        messageJson = "{ \"item\": "+itemJson+", \"roles\": \""+roleslist+"\", \"errorMessage\": \""+error+"\"}";
    sendCommand("updateItem", messageJson);
}

void WebSocketHandler::sendAddItem(QString itemJson, QString error)
{
    QString messageJson;
    if(error.isEmpty())
        messageJson = "{ \"item\": "+itemJson+"}";
    else
        messageJson = "{ \"item\": "+itemJson+", \"errorMessage\": \""+error+"\"}";
    sendCommand("addItem", messageJson);
}

void WebSocketHandler::sendScriptPaused(bool isPaused)
{
    QJsonObject obj;
    obj["isPaused"] = isPaused;
    scriptPaused = isPaused;
    sendCommand("scriptTogglePaused", obj);
}
