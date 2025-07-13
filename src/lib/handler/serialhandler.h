#ifndef SERIALHANDLER_H
#define SERIALHANDLER_H
#include <QSerialPort>
#include <QSerialPortInfo>
#include "../struct/SerialComboboxItem.h"
#include "outputconnectionhandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputSerialConnectionHandler : public OutputConnectionHandler
{
    Q_OBJECT

public:
    explicit OutputSerialConnectionHandler(QObject *parent = nullptr);
    ~OutputSerialConnectionHandler();

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
