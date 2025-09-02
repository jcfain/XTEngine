#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QJsonDocument>
#include "QtWebSockets/qwebsocketserver.h"
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
    void newWebSocketConnected(QWebSocket* client);
    void restartService();
    void cleanupThumbs();
    void cleanupMetadata();
    void skipToMoneyShot();
    void skipToNextAction();
    void saveSingleThumb(QString itemID, qint64 pos);
    void reloadLibrary();
    void startMetadataProcess();
    void processMetadata(QString itemID);
    void mediaAction(QString action);
    void swapScript(ScriptInfo value);
    void clean1024();
    void deleteMediaItem(QString itemID);

public:
    explicit WebSocketHandler(QObject *parent = nullptr);
    ~WebSocketHandler();
    QHostAddress getAddress();
    QUrl getUrl();
    QString getServerName();
    int getServerPort();
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
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    ConnectionChangedSignal _outputDeviceStatus = {ConnectionDirection::Output, ConnectionInterface::Serial, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _inputDeviceStatus = {ConnectionDirection::Input, ConnectionInterface::None, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _gamepadStatus = {ConnectionDirection::Input, ConnectionInterface::Gamepad, ConnectionStatus::Disconnected, "Disconnected"};
    bool scriptPaused = false;

    void closed();
    void onNewConnection();
    void processBinaryMessage(QByteArray message);
    void processTextMessage(QString message);
    void socketDisconnected();
    void initNewClient(QWebSocket* client);
};

#endif // WEBSOCKETHANDLER_H
