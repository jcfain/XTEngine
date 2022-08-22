#ifndef UDPHANDLER_H
#define UDPHANDLER_H
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QHostInfo>
#include <QThread>
#include <QMutex>
#include <QFuture>
#include <QWaitCondition>
#include <math.h>
#include "loghandler.h"
#include "lib/interface/networkdevice.h"
#include "settingshandler.h"
#include "../struct/NetworkAddress.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT  UdpHandler : public NetworkDevice
{
    Q_OBJECT

signals:
    void sendHandShake();

public:
    explicit UdpHandler(QObject *parent = nullptr);
    ~UdpHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void dispose() override;
    void sendTCode(const QString &tcode) override;
    bool isConnected() override;
    DeviceName name() override;

private slots:
    void onSendHandShake();

private:
    void run() override;
    void readData();
    void onSocketStateChange (QAbstractSocket::SocketState state);

    QFuture<void> _initFuture;
    NetworkAddress _address;
    QWaitCondition _cond;
    QMutex _mutex;
    QString _tcode;
    int _waitTimeout = 0;
    bool _stop = false;
    bool _isConnected = false;
    bool _isSelected = false;

};

#endif // UDPHANDLER_H
