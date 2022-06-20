#ifndef VRDEVICEHANDLER_H
#define VRDEVICEHANDLER_H
#include <QObject>
#include <QTcpSocket>
#include "loghandler.h"
#include "settingshandler.h"
#include "../struct/NetworkAddress.h"
#include "../struct/ConnectionChangedSignal.h"
#include "../struct/VRPacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT VRDeviceHandler : public QObject
{
    Q_OBJECT
public:
    explicit VRDeviceHandler(QObject *parent = nullptr);
    virtual void init(NetworkAddress _address, int waitTimeout = 5000);
    virtual void dispose();
    virtual void send(const QString &command);
    virtual bool isConnected();
    virtual bool isPlaying();
    //void togglePause();
    virtual VRPacket getCurrentPacket();
signals:
    void errorOccurred(QString error);
    void connectionChange(ConnectionChangedSignal status);
    void messageRecieved(VRPacket message);
public slots:
    virtual void messageSend(QByteArray message);
};

#endif // VRDEVICEHANDLER_H
