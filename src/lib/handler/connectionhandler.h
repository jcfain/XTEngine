#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "outputdevicehandler.h"
#include "deohandler.h"
#include "whirligighandler.h"
#include "xtpwebhandler.h"
#include "udphandler.h"
#include "serialhandler.h"
#include "gamepadhandler.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "lib/struct/InputDevicePacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT ConnectionHandler : public QObject
{
    Q_OBJECT
signals:
    void connectionChange(ConnectionChangedSignal status);
    void inputConnectionChange(ConnectionChangedSignal status);
    void inputConnectionError(QString error);
    void outputConnectionChange(ConnectionChangedSignal status);
    void outputConnectionError(QString error);
    void messageRecieved(InputDevicePacket packet);
    void gamepadAction(QString action);
    void gamepadTCode(QString tcode);
//    void serialDeviceError(QString error);
//    void serialDeviceConnectionChange(ConnectionChangedSignal event);
//    void udpDeviceError(QString error);
//    void udpDeviceConnectionChange(ConnectionChangedSignal event);
//    void deoDeviceError(QString error);
//    void deoDeviceConnectionChange(ConnectionChangedSignal event);
//    void whirligigDeviceError(QString error);
//    void whirligigDeviceConnectionChange(ConnectionChangedSignal event);
//    void xtpWebDeviceError(QString error);
//    void xtpWebDeviceConnectionChange(ConnectionChangedSignal event);
    void gamepadConnectionChange(ConnectionChangedSignal event);
public slots:
    void inputMessageSend(QByteArray message);
private slots:
    void on_output_connectionChanged(ConnectionChangedSignal event);
    void on_output_error(QString error);
    void on_input_connectionChanged(ConnectionChangedSignal event);
    void on_input_error(QString error);
    void on_gamepad_connectionChanged(ConnectionChangedSignal event);

public:
    explicit ConnectionHandler(QObject *parent = nullptr);
    void init();
    void sendTCode(QString tcode);

    bool isOutputDeviceConnected();
    bool isInputDeviceConnected();
    void initOutputDevice(DeviceName outputDevice);
    void initInputDevice(DeviceName inputDevice);
    void disposeOutputDevice(DeviceName outputDevice);
    void disposeInputDevice(DeviceName inputDevice);
    void dispose();
    UdpHandler* getNetworkHandler();
    SerialHandler* getSerialHandler();
    DeoHandler* getDeoHandler();
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
    UdpHandler* _udpHandler;
    DeoHandler* _deoHandler;
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
