#ifndef UDPHANDLER_H
#define UDPHANDLER_H
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QHostInfo>
#include <QThread>
#include <QMutex>
#include <QFuture>
#include <QWaitCondition>
#include "lib/interface/networkdevice.h"
#include "../struct/NetworkAddress.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT  UdpHandler : public NetworkDevice
{
    Q_OBJECT
signals:
    // Made this signal so the hearbeat thread can dispose this object.
    void disposeMe();
public:
    explicit UdpHandler(QObject *parent = nullptr);
    ~UdpHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void dispose() override;
    void sendTCode(const QString &tcode) override;

private slots:
    void onReadyRead();
    void onConnected();

private:
    void readData();
    void onSocketStateChange (QAbstractSocket::SocketState state);
    void sendHeartbeat();
    bool hasAddress(const QHostAddress &address);

    QUdpSocket* m_udpSocket = 0;
    NetworkAddress _address;
    QHostAddress m_hostAddress;
    QTimer m_heartBeatTimer;
    QMutex m_socketMutex;
    QList<QHostAddress> m_connectedHosts;
};

#endif // UDPHANDLER_H
