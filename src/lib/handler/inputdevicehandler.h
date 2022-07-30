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
public:
    explicit InputDeviceHandler(QObject *parent = nullptr);
    virtual void init(NetworkAddress _address, int waitTimeout = 5000);
    virtual void dispose();
    virtual void send(const QString &command);
    virtual bool isConnected();
    virtual bool isPlaying();
    //void togglePause();
    virtual InputDevicePacket getCurrentPacket();
    virtual void sendPacket(InputDevicePacket packet);
signals:
    void errorOccurred(QString error);
    void connectionChange(ConnectionChangedSignal status);
    void messageRecieved(InputDevicePacket message);
    // For gamepad
    void emitTCode(QString tcode);
    void emitAction(QStringList actions);
    ///
public slots:
    virtual void messageSend(QByteArray message);
};

#endif // INPUTDEVICEHANDLER_H
