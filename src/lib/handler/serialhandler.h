#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H
#include <QSerialPort>
#include <QSerialPortInfo>
#include <math.h>
#include "../struct/SerialComboboxItem.h"
#include "outputdevicehandler.h"
#include "../tool/boolinq.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT SerialHandler : public OutputDeviceHandler
{

public:
    explicit SerialHandler(QObject *parent = nullptr);
    ~SerialHandler();

    QList<SerialComboboxItem> getPorts();
    void sendTCode(const QString &tcode) override;
    void init(const QString &portNameOrFriendlyName, int waitTimeout = 30000);
    void dispose() override;
    bool isConnected() override;
    DeviceName name() override;


private:
    void run() override;
    QString _portName;
    QString _tcode;
    QMutex _mutex;
    QWaitCondition _cond;
    int _waitTimeout = 0;
    bool _stop = false;
    bool _isConnected = false;
};

#endif // SERIALHANDLER_H
