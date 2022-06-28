#include "connectionhandler.h"

ConnectionHandler::ConnectionHandler(QObject *parent)
    : QObject(parent)
{
    _serialHandler = new SerialHandler(this);
    _udpHandler = new UdpHandler(this);
    _deoHandler = new DeoHandler(this);
    _xtpWebHandler = new XTPWebHandler(this);
    _whirligigHandler = new WhirligigHandler(this);
    _gamepadHandler = new GamepadHandler(this);
}

void ConnectionHandler::init()
{
    if(SettingsHandler::getSelectedOutputDevice() == DeviceName::Serial)
    {
        setOutputDevice(_serialHandler);
    }
    else if (SettingsHandler::getSelectedOutputDevice() == DeviceName::Network)
    {
        setOutputDevice(_udpHandler);
    }
    if(SettingsHandler::getGamepadEnabled())
    {
        connect(_gamepadHandler, &GamepadHandler::connectionChange, this, &ConnectionHandler::on_gamepad_connectionChanged);
        _gamepadHandler->init();
    }
}

void ConnectionHandler::dispose()
{
    disposeInputDevice(DeviceName::Deo);
    disposeInputDevice(DeviceName::Whirligig);
    disposeInputDevice(DeviceName::XTPWeb);
    disposeInputDevice(DeviceName::Gamepad);
    disposeOutputDevice(DeviceName::Serial);
    disposeOutputDevice(DeviceName::Network);
    if(_initFuture.isRunning())
    {
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
//    delete _serialHandler;
//    delete _udpHandler;
//    delete _deoHandler;
//    delete _whirligigHandler;
//    delete _xtpWebHandler;
//    delete _gamepadHandler;
}

bool ConnectionHandler::isOutputDeviceConnected()
{
    return _outputDevice && _outputDevice->isConnected();
}

bool ConnectionHandler::isInputDeviceConnected()
{
    return _inputDevice && _inputDevice->isConnected();
}

void ConnectionHandler::setOutputDevice(OutputDeviceHandler* device)
{
    if(!device && _outputDevice) {
        disconnect(_outputDevice, nullptr, nullptr, nullptr);
        _outputDevice->dispose();
    }
    _outputDevice = device;
    if(_outputDevice)
        connect(_outputDevice, &OutputDeviceHandler::connectionChange, this, &ConnectionHandler::on_output_connectionChanged, Qt::UniqueConnection);
}
void ConnectionHandler::setInputDevice(InputDeviceHandler* device)
{
    if(!device && _inputDevice) {
        disconnect(_inputDevice, nullptr, nullptr, nullptr);
        _inputDevice->dispose();
    }
    _inputDevice = device;
    if(_inputDevice) {
        connect(_inputDevice, &InputDeviceHandler::connectionChange, this, &ConnectionHandler::on_input_connectionChanged, Qt::UniqueConnection);
        connect(_inputDevice, &InputDeviceHandler::messageRecieved, this, [this](InputDevicePacket packet){
            emit messageRecieved(packet);
        });
    }
}
OutputDeviceHandler* ConnectionHandler::getSelectedOutputDevice() {
    return _outputDevice;
}
InputDeviceHandler* ConnectionHandler::getSelectedInputDevice() {
    return _inputDevice;
}
UdpHandler* ConnectionHandler::getNetworkHandler() {
    return _udpHandler;
}
SerialHandler* ConnectionHandler::getSerialHandler() {
    return _serialHandler;
}
DeoHandler* ConnectionHandler::getDeoHandler() {
   return _deoHandler;
}
XTPWebHandler* ConnectionHandler::getXTPWebHandler() {
    return _xtpWebHandler;
}
WhirligigHandler* ConnectionHandler::getWhirligigHandler() {
    return _whirligigHandler;
}
GamepadHandler* ConnectionHandler::getGamepadHandler() {
    return _gamepadHandler;
}

void ConnectionHandler::inputMessageSend(QByteArray message) {
    if(_inputDevice)
        _inputDevice->messageSend(message);
}
void ConnectionHandler::sendTCode(QString tcode)
{
    if(!tcode.isEmpty() && _outputDevice && _outputDevice->isConnected())
        _outputDevice->sendTCode(tcode);
}

void ConnectionHandler::initOutputDevice(DeviceName outputDevice)
{
    if (_outputDevice && _outputDevice->isRunning())
    {
        disconnect(_outputDevice, nullptr, nullptr, nullptr);
        _outputDevice->dispose();
    }
    if(_initFuture.isRunning())
    {
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
    if(outputDevice == DeviceName::Serial)
    {
        setOutputDevice(_serialHandler);
        _initFuture = QtConcurrent::run([this]() {
            _serialHandler->init(SettingsHandler::getSerialPort());
        });
    }
    else if (outputDevice == DeviceName::Network)
    {
        setOutputDevice(_udpHandler);
        if(!SettingsHandler::getServerAddress().isEmpty() && !SettingsHandler::getServerPort().isEmpty() &&
            SettingsHandler::getServerAddress() != "0" && SettingsHandler::getServerPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getServerAddress(), SettingsHandler::getServerPort().toInt() };
            _initFuture = QtConcurrent::run([this, address]() {
                _udpHandler->init(address);
            });
        }
    }
}

void ConnectionHandler::disposeOutputDevice(DeviceName outputDevice)
{
    if(outputDevice == DeviceName::Serial)
    {
        _serialHandler->dispose();
    }
    else if (outputDevice == DeviceName::Network)
    {
        _udpHandler->dispose();
    }
}

void ConnectionHandler::initInputDevice(DeviceName inputDevice)
{
    if(inputDevice == DeviceName::Gamepad)
    {
        if (SettingsHandler::getGamepadEnabled())
        {
            connect(_gamepadHandler, &GamepadHandler::emitTCode, this, [this](QString tcode){ emit gamepadTCode(tcode); });
            connect(_gamepadHandler, &GamepadHandler::emitAction, this, [this](QString action){ emit gamepadAction(action); });
            _gamepadHandler->init();
        }
        else
        {
            disconnect(_gamepadHandler, nullptr, nullptr, nullptr);
            _gamepadHandler->dispose();
        }
        return;
    }
    if (_inputDevice && _inputDevice->isConnected())
    {
        disconnect(_inputDevice, nullptr, nullptr, nullptr);
        _inputDevice->dispose();
    }
    if(inputDevice == DeviceName::Deo)
    {
        setInputDevice(_deoHandler);
        if(!SettingsHandler::getDeoAddress().isEmpty() && !SettingsHandler::getDeoPort().isEmpty() &&
            SettingsHandler::getDeoAddress() != "0" && SettingsHandler::getDeoPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getDeoAddress(), SettingsHandler::getDeoPort().toInt() };
            _deoHandler->init(address);
        }
    }
    else if(inputDevice == DeviceName::Whirligig)
    {
        setInputDevice(_whirligigHandler);
        if(!SettingsHandler::getWhirligigAddress().isEmpty() && !SettingsHandler::getWhirligigPort().isEmpty() &&
            SettingsHandler::getWhirligigAddress() != "0" && SettingsHandler::getWhirligigPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getWhirligigAddress(), SettingsHandler::getWhirligigPort().toInt() };
            _whirligigHandler->init(address);
        }
    }
    else if(inputDevice == DeviceName::XTPWeb)
    {
        setInputDevice(_xtpWebHandler);
        if(SettingsHandler::getEnableHttpServer())
            _xtpWebHandler->init();
    }
}

void ConnectionHandler::disposeInputDevice(DeviceName inputDevice)
{
    if(inputDevice == DeviceName::Gamepad)
    {
        disconnect(_gamepadHandler, nullptr, nullptr, nullptr);
        _gamepadHandler->dispose();
    }
    else if(inputDevice == DeviceName::Deo)
    {
        _deoHandler->dispose();
    }
    else if(inputDevice == DeviceName::Whirligig)
    {
        _whirligigHandler->dispose();
    }
    else if(inputDevice == DeviceName::XTPWeb)
    {
        _xtpWebHandler->dispose();
    }
}

void ConnectionHandler::on_input_connectionChanged(ConnectionChangedSignal event)
{
    _deoConnectionStatus = event.status;
    if (event.status == ConnectionStatus::Error)
    {
        setInputDevice(0);
        emit inputConnectionError(event.message);
    }
    else
    {
        SettingsHandler::setSelectedInputDevice(event.deviceName);
    }
    ConnectionChangedSignal status = {event.type, event.deviceName, event.status, event.message};
//    if(event.deviceName == DeviceName::Deo)
//        emit deoDeviceConnectionChange(status);
//    else if(event.deviceName == DeviceName::Whirligig)
//        emit whirligigDeviceConnectionChange(status);
//    else if(event.deviceName == DeviceName::XTPWeb)
//        emit xtpWebDeviceConnectionChange(status);
    emit inputConnectionChange(status);
    emit connectionChange(status);
}

void ConnectionHandler::on_input_error(QString error)
{
    setInputDevice(0);
    emit inputConnectionError(error);
}

void ConnectionHandler::on_output_connectionChanged(ConnectionChangedSignal event)
{
    _deoConnectionStatus = event.status;
    if (event.status == ConnectionStatus::Error)
    {
        setOutputDevice(0);
        emit outputConnectionError(event.message);
    }
    else
    {
        SettingsHandler::setSelectedOutputDevice(event.deviceName);
    }
    ConnectionChangedSignal status = {event.type, event.deviceName, event.status, event.message};
//    if(event.deviceName == DeviceName::Serial)
//        emit serialDeviceConnectionChange(status);
//    else if(event.deviceName == DeviceName::Network)
//        emit udpDeviceConnectionChange(status);
    emit outputConnectionChange(status);
    emit connectionChange(status);
}
void ConnectionHandler::on_output_error(QString error)
{
    setInputDevice(0);
    emit outputConnectionError(error);
}
void ConnectionHandler::on_gamepad_connectionChanged(ConnectionChangedSignal status)
{
    emit gamepadConnectionChange(status);
    emit connectionChange(status);
}
