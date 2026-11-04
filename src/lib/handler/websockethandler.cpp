#include "websockethandler.h"
#include "settingshandler.h"
#include <QAbstractSocket>

#include "xmediastatehandler.h"

WebSocketHandler::WebSocketHandler(QObject *parent):
    QObject(parent)
{
    // The WebSocket server is no longer a separate QWebSocketServer. The HTTP
    // server (Qt 6.8+) handles WebSocket upgrades on the same port and forwards
    // each accepted socket to handleNewWebSocket().
}

WebSocketHandler::~WebSocketHandler()
{
    sendCommand("connectionClosed");
    //qDeleteAll(m_clients.begin(), m_clients.end());
}

int WebSocketHandler::getServerPort()
{
    // WebSocket now shares the HTTP port, so report that port to the client.
    return SettingsHandler::getHTTPPort();
}

void WebSocketHandler::onSettingChange(QString setting, QVariant value)
{
    QJsonObject obj;
    obj["key"] = setting;
    obj["value"] = QJsonValue::fromVariant(value);
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
    {
        // Only write to sockets that are still connected. A client that just
        // closed (e.g. a page refresh) may not have been removed from m_clients
        // yet; writing to it raises EPIPE/SIGPIPE.
        if(client->state() == QAbstractSocket::ConnectedState)
            client->sendTextMessage(commandJson);
    }
    else
        for (QWebSocket *pClient : qAsConst(m_clients))
            if(pClient->state() == QAbstractSocket::ConnectedState)
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
    {
        if(client->state() == QAbstractSocket::ConnectedState)
            client->sendTextMessage(fullMessage);
    }
    else
        for (QWebSocket *pClient : qAsConst(m_clients))
            if(pClient->state() == QAbstractSocket::ConnectedState)
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

void WebSocketHandler::handleNewWebSocket(QWebSocket* client)
{
    // The upgrade (and session auth) was already performed by the HTTP server via
    // addWebSocketUpgradeVerifier(); the socket arrives fully connected.
    connect(client, &QWebSocket::textMessageReceived, this, &WebSocketHandler::processTextMessage);
    connect(client, &QWebSocket::binaryMessageReceived, this, &WebSocketHandler::processBinaryMessage);
    connect(client, &QWebSocket::disconnected, this, &WebSocketHandler::socketDisconnected);

    m_clients << client;
    initNewClient(client);

    emit newWebSocketConnected(client);
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
    } else if (command == "systemReady") {
        SettingsHandler::systemReady();
    } else if (command == "playMedia") {
        QJsonObject obj = json["message"].toObject();
        QString id = obj["id"].toString();
        emit playMedia(id);
    } else if (command == "stopMedia") {
        emit stopMedia();
    } else if (command == "settingsQuickExport") {
        SettingsHandler::ExportQuick();
    } else if (command == "settingsQuickImport") {
        QJsonObject obj = json["message"].toObject();
        QString file = obj["filename"].toString();
        QSettings::Format format = file.endsWith("ini") ? QSettings::Format::IniFormat : JSONSettingsFormatter::JsonFormat;
        SettingsHandler::ImportQuick(file, format);
    } else if (command == "settingChange") {
        QJsonObject obj = json["message"].toObject();
        emit settingChange(obj["key"].toString(), obj["value"].toVariant());
    } else if (command == "setChannelRange") {
        QJsonObject obj = json["message"].toObject();
        emit setChannelRange(obj["channelName"].toString(), obj["min"].toInt(), obj["max"].toInt());
    } else if (command == "changeChannelProfile") {
        emit changeChannelProfile(json["message"].toString());
    } else if (command == "addChannelProfile") {
        emit addChannelProfile(json["message"].toString());
    } else if (command == "deleteChannelProfile") {
        emit deleteChannelProfile(json["message"].toString());
    } else if (command == "cloneChannelProfile") {
        QJsonObject obj = json["message"].toObject();
        emit cloneChannelProfile(obj["fromName"].toString(), obj["toName"].toString());
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

    } else if (command == "setMoneyShot") {
        QJsonObject obj = json["message"].toObject();
        QString itemID = obj["itemID"].toString();
        qint64 pos = obj["pos"].toDouble() * 1000;
        emit setMoneyShot(itemID, pos);
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
    } else {
        LogHandler::Warn("Command not defined in websocketHandler: "+command);
    }

}

void WebSocketHandler::processBinaryMessage(QByteArray message)
{
    QWebSocket *pClient = qobject_cast<QWebSocket *>(sender());
    LogHandler::Debug("WEBSOCKET Binary Message received: " + message);
    if (pClient && pClient->state() == QAbstractSocket::ConnectedState)
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
