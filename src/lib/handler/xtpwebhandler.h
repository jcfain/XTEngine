#ifndef XTPWEBHANDLER_H
#define XTPWEBHANDLER_H

#include <QJsonDocument>
#include <QMutex>
#include <QTimer>
#include "inputconnectionhandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT InputXTPWebConnectionHandler : public InputConnectionHandler
{
    Q_OBJECT
public slots:
    void messageSend(QByteArray message) override;
public:
    explicit InputXTPWebConnectionHandler(QObject *parent = nullptr);
    ~InputXTPWebConnectionHandler();
    void init(NetworkAddress _address, int waitTimeout = 5000) override;
    void init();
    void dispose() override;
    void send(const QString &command) override;
    bool isConnected() override;
    bool isPlaying() override;
    //void togglePause();
    InputConnectionPacket getCurrentPacket() override;
    void sendPacket(InputConnectionPacket packet) override;
    void readData(QByteArray data);
    ConnectionInterface name() override;

private:
    InputConnectionPacket* _currentPacket = 0;
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
