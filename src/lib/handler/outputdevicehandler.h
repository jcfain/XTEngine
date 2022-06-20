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
    void errorOccurred(QString error);
    void connectionChange(ConnectionChangedSignal status);
    void emitTCode(QString tcode);
    void emitAction(QString tcode);

public:
    explicit OutputDeviceHandler(QObject *parent = nullptr);
    ~OutputDeviceHandler();
    virtual void sendTCode(const QString &tcode);
    virtual void dispose();
    virtual bool isConnected();

private:
    virtual void run() override;
};

#endif // OUTPUTDEVICEHANDLER_H
