#ifndef WEBSOCKETHANDLER_H
#define WEBSOCKETHANDLER_H

#include <QObject>
#include <QJsonDocument>
#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"

#include "settingshandler.h"
#include "../struct/ConnectionChangedSignal.h"

class WebSocketHandler: public QObject
{
    Q_OBJECT
signals:
    void connectOutputDevice(DeviceName deviceName);
    void connectInputDevice(DeviceName deviceName, bool checked);
    void tcode(QString tcode);
    void newWebSocketConnected(QWebSocket* client);
    void restartService();
public:
    WebSocketHandler(QObject *parent = nullptr);
    ~WebSocketHandler();
    QHostAddress getAddress();
    QUrl getUrl();
    QString getServerName();
    int getServerPort();
    void sendCommand(QString command, QString message = nullptr, QWebSocket* client = 0);
    void sendDeviceConnectionStatus(ConnectionChangedSignal status, QWebSocket* client = 0);
    void sendUpdateThumb(QString id, QString thumb, QString error = nullptr);

private:
    QWebSocketServer *m_pWebSocketServer;
    QList<QWebSocket *> m_clients;
    ConnectionChangedSignal _outputDeviceStatus = {DeviceType::Output, DeviceName::Serial, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _inputDeviceStatus = {DeviceType::Input, DeviceName::None, ConnectionStatus::Disconnected, "Disconnected"};
    ConnectionChangedSignal _gamepadStatus = {DeviceType::Input, DeviceName::Gamepad, ConnectionStatus::Disconnected, "Disconnected"};
private:
    void closed();
    void onNewConnection();
    void processBinaryMessage(QByteArray message);
    void processTextMessage(QString message);
    void socketDisconnected();
    void initNewClient(QWebSocket* client);
};

#endif // WEBSOCKETHANDLER_H
