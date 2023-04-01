#include "connectionhandler.h"

ConnectionHandler::ConnectionHandler(QObject *parent)
    : QObject(parent)
{
    _serialHandler = new SerialHandler(this);
    _udpHandler = new UdpHandler(this);
    _webSocketHandler = new WebsocketDeviceHandler(this);
    _networkDevice = _udpHandler;
    m_hereSphereHandler = new HereSphereHandler(this);
    _xtpWebHandler = new XTPWebHandler(this);
    _whirligigHandler = new WhirligigHandler(this);
    _gamepadHandler = new GamepadHandler(this);
}

void ConnectionHandler::init()
{
    DeviceName outputDeviceName = SettingsHandler::getSelectedOutputDevice();
    if(outputDeviceName != DeviceName::None)
        initOutputDevice(outputDeviceName);

    DeviceName inputDeviceName = SettingsHandler::getSelectedInputDevice();
    if(inputDeviceName != DeviceName::None)
        initInputDevice(inputDeviceName);

    if(SettingsHandler::getGamepadEnabled())
    {
        initInputDevice(DeviceName::Gamepad);
    }
}

void ConnectionHandler::dispose()
{
    disposeInputDevice(DeviceName::HereSphere);
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
        disconnect(_outputDevice, &OutputDeviceHandler::connectionChange, nullptr, nullptr);
        _outputDevice->dispose();
    }
    _outputDevice = device;
    if(_outputDevice)
        connect(_outputDevice, &OutputDeviceHandler::connectionChange, this, &ConnectionHandler::on_output_connectionChanged, Qt::UniqueConnection);
}
void ConnectionHandler::setInputDevice(InputDeviceHandler* device)
{
    if(!device && _inputDevice) {
        disconnect(_inputDevice, &InputDeviceHandler::connectionChange, nullptr, nullptr);
        disconnect(_inputDevice, &InputDeviceHandler::messageRecieved, nullptr, nullptr);
        _inputDevice->dispose();
    }
    _inputDevice = device;
    if(_inputDevice) {
        connect(_inputDevice, &InputDeviceHandler::connectionChange, this, &ConnectionHandler::on_input_connectionChanged, Qt::UniqueConnection);
        connect(_inputDevice, &InputDeviceHandler::messageRecieved, this, [this](InputDevicePacket packet){
            if(_inputDevice->isConnected())
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
NetworkDevice* ConnectionHandler::getNetworkHandler() {
    return _networkDevice;
}
SerialHandler* ConnectionHandler::getSerialHandler() {
    return _serialHandler;
}
HereSphereHandler* ConnectionHandler::getHereSphereHandler() {
   return m_hereSphereHandler;
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
    if(!SettingsHandler::getLiveActionPaused() && !tcode.isEmpty() && _outputDevice && _outputDevice->isConnected())
        _outputDevice->sendTCode(tcode);
}

void ConnectionHandler::stopOutputDevice()
{
    sendTCode("DSTOP");
}

void ConnectionHandler::initOutputDevice(DeviceName outputDevice)
{
    if (_outputDevice && _outputDevice->isRunning())
    {
        _outputDevice->dispose();
        disconnect(_outputDevice, &OutputDeviceHandler::connectionChange, nullptr, nullptr);
    }
    if(_initFuture.isRunning())
    {
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
    if(outputDevice == DeviceName::None)
    {
        return;
    }
    else if(outputDevice == DeviceName::Serial)
    {
        setOutputDevice(_serialHandler);
        _initFuture = QtConcurrent::run([this]() {
            _serialHandler->init(SettingsHandler::getSerialPort());
        });
    }
    else if (outputDevice == DeviceName::Network)
    {
        if(SettingsHandler::getSelectedNetworkDevice() == NetworkDeviceType::WEBSOCKET) {
           _networkDevice = _webSocketHandler;
        } else {
           _networkDevice = _udpHandler;
        }
        setOutputDevice(_networkDevice);
        if(!SettingsHandler::getServerAddress().isEmpty() && !SettingsHandler::getServerPort().isEmpty() &&
            SettingsHandler::getServerAddress() != "0" && SettingsHandler::getServerPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getServerAddress(), SettingsHandler::getServerPort().toInt() };

            _networkDevice->init(address);
//            _initFuture = QtConcurrent::run([this, address]() {
//                _webSocketHandler->init(address);
//            });
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
        connect(_gamepadHandler, &GamepadHandler::connectionChange, this, &ConnectionHandler::on_gamepad_connectionChanged);
        connect(_gamepadHandler, &GamepadHandler::emitTCode, this, &ConnectionHandler::sendTCode);
        connect(_gamepadHandler, &GamepadHandler::emitAction, this, &ConnectionHandler::gamepadAction);
        _gamepadHandler->init();
        return;
    }
    if (_inputDevice)
    {
        disconnect(_inputDevice, &InputDeviceHandler::messageRecieved, nullptr, nullptr);
        _inputDevice->dispose();
        disconnect(_inputDevice, &InputDeviceHandler::connectionChange, nullptr, nullptr);
    }
    if(inputDevice == DeviceName::None)
    {
        return;
    }
    else if(inputDevice == DeviceName::HereSphere)
    {
        setInputDevice(m_hereSphereHandler);
        if(!SettingsHandler::getDeoAddress().isEmpty() && !SettingsHandler::getDeoPort().isEmpty() &&
            SettingsHandler::getDeoAddress() != "0" && SettingsHandler::getDeoPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getDeoAddress(), SettingsHandler::getDeoPort().toInt() };
            m_hereSphereHandler->init(address);
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
        _gamepadHandler->dispose();
        disconnect(_gamepadHandler, &GamepadHandler::emitTCode, nullptr, nullptr);
        disconnect(_gamepadHandler, &GamepadHandler::emitAction, nullptr, nullptr);
        disconnect(_gamepadHandler, &GamepadHandler::connectionChange, nullptr, nullptr);
    }
    else if(inputDevice == DeviceName::HereSphere)
    {
        m_hereSphereHandler->dispose();
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
        emit inputConnectionChange(event);
    }
    else
    {
        SettingsHandler::setSelectedInputDevice(event.deviceName);
    }
    ConnectionChangedSignal status = {event.type, event.deviceName, event.status, event.message};
    emit inputConnectionChange(status);
    emit connectionChange(status);
}
void ConnectionHandler::on_output_connectionChanged(ConnectionChangedSignal event)
{
    _deoConnectionStatus = event.status;
    if (event.status == ConnectionStatus::Error)
    {
        setOutputDevice(0);
        emit outputConnectionChange(event);
    }
    else
    {
        SettingsHandler::setSelectedOutputDevice(event.deviceName);
    }
    ConnectionChangedSignal status = {event.type, event.deviceName, event.status, event.message};
    emit outputConnectionChange(status);
    emit connectionChange(status);
}
void ConnectionHandler::on_gamepad_connectionChanged(ConnectionChangedSignal status)
{
    emit gamepadConnectionChange(status);
    emit connectionChange(status);
}
