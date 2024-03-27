#ifndef WEBSOCKETDEVICEHANDLER_H
#define WEBSOCKETDEVICEHANDLER_H

#include <QWebSocket>
#include <QWebSocketServer>
#include <QNetworkDatagram>
#include <QHostInfo>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include "lib/interface/networkdevice.h"
#include <math.h>
#include "loghandler.h"
#include "settingshandler.h"
#include "../struct/NetworkAddress.h"

class WebsocketDeviceHandler : public NetworkDevice
{
    Q_OBJECT
public:
    explicit WebsocketDeviceHandler(QObject *parent = nullptr);
    ~WebsocketDeviceHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void dispose() override;
    void sendTCode(const QString &tcode) override;

private slots:
    void onConnected();
    void onClosed();

private:
    QWebSocket m_webSocket;
//    QWebSocketServer *m_pWebSocketServer;
//    QList<QWebSocket *> m_clients;
    NetworkAddress _address;
    // QWaitCondition _cond;
    // QMutex _mutex;
    // QString _tcode;
    // int _waitTimeout = 0;
    bool _isListening = false;

    void onSocketStateChange (QAbstractSocket::SocketState state);
    void onTextMessageReceived(QString message);
    void onErrorOccured(QAbstractSocket::SocketError state);
};

#endif // WEBSOCKETDEVICEHANDLER_H
