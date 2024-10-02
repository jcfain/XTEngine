#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "outputdevicehandler.h"
#include "lib/interface/networkdevice.h"
#include "deohandler.h"
#include "whirligighandler.h"
#include "xtpwebhandler.h"
#include "udphandler.h"
#include "websocketdevicehandler.h"
#include "serialhandler.h"
#include "blehandler.h"
#include "gamepadhandler.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "lib/struct/InputDevicePacket.h"
#include "lib/struct/OutputDevicePacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT ConnectionHandler : public QObject
{
    Q_OBJECT
signals:
    void connectionChange(ConnectionChangedSignal status);
    void inputConnectionChange(ConnectionChangedSignal status);
    void outputConnectionChange(ConnectionChangedSignal status);
    void inputMessageRecieved(InputDevicePacket packet);
    void outputMessageRecieved(OutputDevicePacket packet);
    void action(QString action);
    void serialPortFound(QString portFriendlyName, QString portName);
    void gamepadConnectionChange(ConnectionChangedSignal event);
public slots:
    void inputMessageSend(QByteArray message);
private slots:
    void on_output_connectionChanged(ConnectionChangedSignal event);
    void on_input_connectionChanged(ConnectionChangedSignal event);
    void on_gamepad_connectionChanged(ConnectionChangedSignal event);

public:
    explicit ConnectionHandler(QObject *parent = nullptr);
    void init();
    void sendTCode(QString tcode);
    void stopOutputDevice();

    bool isOutputDeviceConnected();
    bool isInputDeviceConnected();
    /** @brief Initializes the device that is stored in SettingsHandler */
    void initOutputDevice(DeviceName outputDevice);
    /** @brief Initializes the device that is stored in SettingsHandler */
    void initInputDevice(DeviceName inputDevice);
    void disposeOutputDevice(DeviceName outputDevice);
    void disposeInputDevice(DeviceName inputDevice);
    void dispose();
    NetworkDevice* getNetworkHandler();
    SerialHandler* getSerialHandler();
    HereSphereHandler* getHereSphereHandler();
    XTPWebHandler* getXTPWebHandler();
    WhirligigHandler* getWhirligigHandler();
    GamepadHandler* getGamepadHandler();
    OutputDeviceHandler* getSelectedOutputDevice();
    InputDeviceHandler* getSelectedInputDevice();

private:
    QFuture<void> _initFuture;
    OutputDeviceHandler* _outputDevice = 0;
    InputDeviceHandler* _inputDevice = 0;
    SerialHandler* _serialHandler;
    NetworkDevice* _networkDevice;
    UdpHandler* _udpHandler;
    BLEHandler* m_bleHandler;
    WebsocketDeviceHandler* _webSocketHandler;
    HereSphereHandler* m_hereSphereHandler;
    WhirligigHandler* _whirligigHandler;
    XTPWebHandler* _xtpWebHandler;
    GamepadHandler* _gamepadHandler;
    ConnectionStatus _outDeviceConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _deoConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _whirligigConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _xtpWebConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _gamepadConnectionStatus = ConnectionStatus::Disconnected;


    void setOutputDevice(OutputDeviceHandler* device);
    void setInputDevice(InputDeviceHandler* device);

};

#endif // CONNECTIONHANDLER_H
