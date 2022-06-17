#include "websockethandler.h"

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
    } else if (command == "connectOutputDevice") {
        emit connectOutputDevice();
    } else if (command == "connectInputDevice") {
        QJsonObject obj = json["message"].toObject();
        emit connectInputDevice((DeviceType)obj["deviceType"].toInt(), obj["enabled"].toBool());
    } else if (command == "restartService") {
        emit restartService();
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
    QString messageJson = "{ \"status\": "+QString::number(status.status)+", \"deviceType\": "+QString::number(status.deviceType)+", \"message\": \""+status.message+"\" }";
    if(status.deviceType == DeviceType::Serial || status.deviceType == DeviceType::Network)
    {
        _outputDeviceStatus = status;
        sendCommand("outputDeviceStatus", messageJson, client);
    }
    else if(status.deviceType == DeviceType::Gamepad)
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
