#ifndef WHIRLIGIGHANDLER_H
#define WHIRLIGIGHANDLER_H
#include <QNetworkDatagram>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include "loghandler.h"
#include "outputconnectionhandler.h"
#include "settingshandler.h"
#include "../struct/NetworkAddress.h"
#include "../struct/ConnectionChangedSignal.h"
#include "../struct/InputConnectionPacket.h"
#include "inputconnectionhandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT InputWhirligigConnectionHandler : public InputConnectionHandler
{
    Q_OBJECT
public slots:
    void messageSend(QByteArray message) override;

public:
    explicit InputWhirligigConnectionHandler(QObject *parent = nullptr);
    ~InputWhirligigConnectionHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void dispose() override;
    void send(const QString &command) override;
    bool isConnected() override;
    bool isPlaying() override;
    //void togglePause();
    InputConnectionPacket getCurrentPacket() override;
    void sendPacket(InputConnectionPacket packet) override;
    ConnectionInterface name() override;

private:
    void readData();
    void onSocketStateChange (QAbstractSocket::SocketState state);
    void tcpErrorOccured(QAbstractSocket::SocketError);

    InputConnectionPacket* currentVRPacket = nullptr;
    QTcpSocket* tcpSocket = nullptr;
    QMutex _mutex;
    NetworkAddress _address;
    QString _sendCommand;
    int _waitTimeout = 0;
    bool _isConnected = false;
    bool _isPlaying = false;
    bool _isSelected = false;

    QString path = nullptr;
    qint64 duration = 0;
    QElapsedTimer mSecTimer;
    qint64 time1 = 0;
    qint64 time2 = 0;
};

#endif // WHIRLIGIGHANDLER_H
