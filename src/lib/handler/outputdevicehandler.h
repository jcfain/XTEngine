#ifndef OUTPUTDEVICEHANDLER_H
#define OUTPUTDEVICEHANDLER_H
#include <QTime>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include "settingshandler.h"
#include "loghandler.h"
#include "lib/lookup/enum.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputDeviceHandler : public QThread
{
    Q_OBJECT

signals:
    void connectionChange(ConnectionChangedSignal status);

public:
    explicit OutputDeviceHandler(QObject *parent = nullptr) :
        QThread(parent)
    {

    }
    ~OutputDeviceHandler() {};
    virtual void sendTCode(const QString &tcode) = 0;
    virtual void dispose() = 0;
    virtual bool isConnected() = 0;
    virtual DeviceName name() = 0;

private:
    virtual void run() override = 0;
};

#endif // OUTPUTDEVICEHANDLER_H
