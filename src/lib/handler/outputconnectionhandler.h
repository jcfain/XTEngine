#ifndef OUTPUTCONNECTIONHANDLER_H
#define OUTPUTCONNECTIONHANDLER_H
#include <QTime>
#include <QMutex>
#include <QRegularExpression>
#include <QThread>
#include <QWaitCondition>
#include <QtConcurrent/QtConcurrent>
#include "settingshandler.h"
#include "loghandler.h"
#include "lib/lookup/enum.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "lib/struct/OutputConnectionPacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputConnectionHandler : public QObject
{
    Q_OBJECT

signals:
    void connectionChange(ConnectionChangedSignal status);
    void commandRecieve(OutputConnectionPacket packet);
    void sendHandShake();

public:
    explicit OutputConnectionHandler(ConnectionInterface deviceName,
                                 QObject *parent = nullptr);
    ~OutputConnectionHandler() {};
    virtual void sendTCode(const QString &tcode) = 0;
    virtual void dispose();

    ConnectionInterface name();
    bool isConnected();

protected slots:
    void onSendHandShake();

protected:
    ConnectionInterface m_deviceName;
    void tryConnectStop();
    void tryConnectDevice(unsigned long waitTimeout = 5000);

    void processDeviceInput(QString buffer);

    void setConnected(bool connected);

private:
    QMutex m_mutex;
    bool m_isConnected = false;
    QString m_readBuffer;
    QRegularExpression m_newline;
    QFuture<void> m_initFuture;
    bool m_stop = false;

    void processCommand(QString data);
};

#endif // OUTPUTCONNECTIONHANDLER_H
