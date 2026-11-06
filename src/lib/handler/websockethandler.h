#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QJsonDocument>
#include <QTimer>
#include <QHash>
#include "QtWebSockets/qwebsocket.h"

#include "../struct/ConnectionChangedSignal.h"
#include "../struct/ScriptInfo.h"

class WebSocketHandler: public QObject
{
    Q_OBJECT
signals:
    void settingChange(QString settingName, QVariant value);
    void connectOutputDevice(ConnectionInterface deviceName, bool checked);
    void connectInputDevice(ConnectionInterface deviceName, bool checked);
    void tcode(QString tcode);
    void setChannelRange(QString channelName, int min, int max);
    void changeChannelProfile(QString name);
    void addChannelProfile(QString name);
    void deleteChannelProfile(QString name);
    void cloneChannelProfile(QString fromName, QString toName);
    void newWebSocketConnected(QWebSocket* client);
    void clientActivity(QString sessionID);
    void restartService();
    void cleanupThumbs();
    void cleanupMetadata();
    void skipToMoneyShot();
    void skipToNextAction();
    void saveSingleThumb(QString itemID, qint64 pos);
    void setMoneyShot(QString itemID, qint64 pos);
    void reloadLibrary();
    void startMetadataProcess();
    void processMetadata(QString itemID);
    void mediaAction(QString action);
    void playMedia(QString id);
    void stopMedia();
    void swapScript(ScriptInfo value);
    void clean1024();
    void deleteMediaItem(QString itemID);

public:
    explicit WebSocketHandler(QObject *parent = nullptr);
    ~WebSocketHandler();
    int getServerPort();
    void handleNewWebSocket(QWebSocket* client, QString sessionID = QString());
    void expireSessionClients(const QString &sessionID);
    void onSettingChange(QString setting, QVariant value);
    void sendCommand(QString command, QString message = nullptr, QWebSocket* client = 0);
    void sendCommand(QString command, QJsonObject message, QWebSocket* client = 0);
    void sendUserError(QString message);
    void sendUserWarning(QString message);
    void sendError(QString message);
    void sendWarning(QString message);
    void sendDeviceConnectionStatus(ConnectionChangedSignal status, QWebSocket* client = 0);
    void sendUpdateThumb(QString id, QString thumb, QString error = nullptr);
    void sendUpdateItem(QString itemJson, QString rolesList, QString error = nullptr);
    void sendAddItem(QString itemJson, QString error = nullptr);
    void sendScriptPaused(bool isPaused);


private:
    QList<QWebSocket *> m_clients;
    ConnectionChangedSignal _outputDeviceStatus = {ConnectionDirection::Output, ConnectionInterface::Serial, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _inputDeviceStatus = {ConnectionDirection::Input, ConnectionInterface::None, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _gamepadStatus = {ConnectionDirection::Input, ConnectionInterface::Gamepad, ConnectionStatus::Disconnected, "Disconnected"};
    bool scriptPaused = false;

    // Keep-alive: proxies/firewalls/NATs drop idle WebSockets after a couple of
    // minutes. A periodic ping frame (browser auto-pongs) keeps the socket active.
    // m_lastPong tracks the last pong per client so we can close dead clients.
    QTimer m_keepAliveTimer;
    QHash<QWebSocket*, qint64> m_lastPong;
    // Maps each client to its HTTP session so WebSocket activity can keep the
    // session alive (emitted via clientActivity, handled by HttpHandler).
    QHash<QWebSocket*, QString> m_clientSessions;

    void processBinaryMessage(QByteArray message);
    void processTextMessage(QString message);
    void socketDisconnected();
    void initNewClient(QWebSocket* client);
    void sendKeepAlive();
    void onPongReceived();
};

#endif // WEBSOCKETHANDLER_H
