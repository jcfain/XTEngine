#ifndef DEOHANDLER_H
#define DEOHANDLER_H
#include <QTcpSocket>
#include <QNetworkDatagram>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include "inputdevicehandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT HereSphereHandler : public InputDeviceHandler
{

    Q_OBJECT
public slots:
    void messageSend(QByteArray message) override;

public:
    explicit HereSphereHandler(QObject *parent = nullptr);
    ~HereSphereHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void dispose() override;
    void send(const QString &command) override;
    bool isConnected() override;
    bool isPlaying() override;
    //void togglePause();
    InputDevicePacket getCurrentPacket() override;
    void sendPacket(InputDevicePacket packet) override;

    DeviceName name() override;

private:
    void readData();
    void onSocketStateChange (QAbstractSocket::SocketState state);
    void tcpErrorOccured(QAbstractSocket::SocketError);
    void sendKeepAlive();
    void tearDown();

    InputDevicePacket currentPacket;
    InputDevicePacket blankPacket = {
        NULL,
        0,
        0,
        0,
        0,
        0
    };
    QTcpSocket tcpSocket;
    QTimer keepAliveTimer;
    QMutex _mutex;
    NetworkAddress _address;
    QString _sendCommand;
    int _waitTimeout = 0;
    bool _isConnected = false;
    bool _isPlaying = false;
    bool _isSelected = false;
    qint64 _currentTime;
    uint8_t m_connectTries;
};

#endif // DEOHANDLER_H
