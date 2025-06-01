#ifndef OUTPUTDEVICEHANDLER_H
#define OUTPUTDEVICEHANDLER_H
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
#include "lib/struct/OutputDevicePacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputDeviceHandler : public QObject
{
    Q_OBJECT

signals:
    void connectionChange(ConnectionChangedSignal status);
    void commandRecieve(OutputDevicePacket packet);
    void sendHandShake();

public:
    explicit OutputDeviceHandler(DeviceName deviceName,
                                 QObject *parent = nullptr);
    ~OutputDeviceHandler() {};
    virtual void sendTCode(const QString &tcode) = 0;
    virtual void dispose();

    DeviceName name();
    bool isConnected();

protected slots:
    void onSendHandShake();

protected:
    DeviceName m_deviceName;
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

#endif // OUTPUTDEVICEHANDLER_H
