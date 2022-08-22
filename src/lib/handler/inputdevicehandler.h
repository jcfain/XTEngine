#ifndef INPUTDEVICEHANDLER_H
#define INPUTDEVICEHANDLER_H
#include <QObject>
#include <QTcpSocket>
#include "loghandler.h"
#include "settingshandler.h"
#include "../struct/NetworkAddress.h"
#include "../struct/ConnectionChangedSignal.h"
#include "../struct/InputDevicePacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT InputDeviceHandler : public QObject
{
    Q_OBJECT
signals:
    void errorOccurred(QString error);
    void connectionChange(ConnectionChangedSignal status);
    void messageRecieved(InputDevicePacket message);
    // For gamepad
    void emitTCode(QString tcode);
    void emitAction(QStringList actions);
    ///

public slots:
    virtual void messageSend(QByteArray message) = 0;

public:
    explicit InputDeviceHandler(QObject *parent = nullptr) : QObject(parent) { }
    virtual void init(NetworkAddress _address, int waitTimeout = 5000) = 0;
    virtual void dispose() = 0;
    virtual void send(const QString &command) = 0;
    virtual bool isConnected() = 0;
    virtual bool isPlaying() = 0;
    //void togglePause();
    virtual InputDevicePacket getCurrentPacket() = 0;
    virtual void sendPacket(InputDevicePacket packet) = 0;
    virtual DeviceName name() = 0;
};

#endif // INPUTDEVICEHANDLER_H
