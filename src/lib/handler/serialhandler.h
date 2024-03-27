#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H
#include <QSerialPort>
#include <QSerialPortInfo>
#include "../struct/SerialComboboxItem.h"
#include "outputdevicehandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT SerialHandler : public OutputDeviceHandler
{
    Q_OBJECT

public:
    explicit SerialHandler(QObject *parent = nullptr);
    ~SerialHandler();

    QList<SerialComboboxItem> getPorts();
    void sendTCode(const QString &tcode) override;
    void init(const QString &portNameOrFriendlyName, int waitTimeout = 5000);
    void dispose() override;

private slots:
    void onReadyRead();

private:
    QSerialPort* m_serial;
    QString _portName;

};

#endif // SERIALHANDLER_H
