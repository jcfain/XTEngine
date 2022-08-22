#ifndef XTPWEBHANDLER_H
#define XTPWEBHANDLER_H

#include <QJsonDocument>
#include <QMutex>
#include <QTimer>
#include "inputdevicehandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT XTPWebHandler : public InputDeviceHandler
{
    Q_OBJECT
public slots:
    void messageSend(QByteArray message) override;
public:
    explicit XTPWebHandler(QObject *parent = nullptr);
    ~XTPWebHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void init();
    void dispose() override;
    void send(const QString &command) override;
    bool isConnected() override;
    bool isPlaying() override;
    //void togglePause();
    InputDevicePacket getCurrentPacket() override;
    void sendPacket(InputDevicePacket packet) override;
    void readData(QByteArray data);
    DeviceName name() override;

private:
    InputDevicePacket* _currentPacket = 0;
    QMutex _mutex;
    NetworkAddress _address;
    QString _sendCommand;
    int _waitTimeout = 0;
    bool _isConnected = false;
    bool _isPlaying = false;
    bool _isSelected = false;
    qint64 _currentTime;
};

#endif // XTPWEBHANDLER_H
