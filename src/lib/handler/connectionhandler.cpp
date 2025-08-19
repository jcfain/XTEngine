#include "connectionhandler.h"

ConnectionHandler::ConnectionHandler(QObject *parent)
    : QObject(parent)
{
    _serialHandler = new OutputSerialConnectionHandler(this);
    _udpHandler = new OutputUdpConnectionHandler(this);
    _webSocketHandler = new OutputWebsocketConnectionHandler(this);
    _networkConnection = _udpHandler;
    m_hereSphereHandler = new InputHeresphereConnectionHandler(this);
    _xtpWebHandler = new InputXTPWebConnectionHandler(this);
    _whirligigHandler = new InputWhirligigConnectionHandler(this);
    _gamepadHandler = new GamepadHandler(this);
    m_bleHandler = new OutputBLEConnectionHandler(this);
}

void ConnectionHandler::init()
{
    ConnectionInterface outputDeviceName = SettingsHandler::getSelectedOutputDevice();
    if(outputDeviceName != ConnectionInterface::None)
        initOutputConnection(outputDeviceName);

    ConnectionInterface inputDeviceName = SettingsHandler::getSelectedInputDevice();
    if(inputDeviceName != ConnectionInterface::None)
        initInputConnection(inputDeviceName);

    if(SettingsHandler::getGamepadEnabled())
    {
        initInputConnection(ConnectionInterface::Gamepad);
    }
}

void ConnectionHandler::dispose()
{
    disposeInputConnection(ConnectionInterface::HereSphere);
    disposeInputConnection(ConnectionInterface::Whirligig);
    disposeInputConnection(ConnectionInterface::XTPWeb);
    disposeInputConnection(ConnectionInterface::Gamepad);
    disposeOutputConnection(ConnectionInterface::Serial);
    disposeOutputConnection(ConnectionInterface::Network);
    disposeOutputConnection(ConnectionInterface::BLE);
    if(_initFuture.isRunning())
    {
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
}

bool ConnectionHandler::isOutputConnected()
{
    return m_outputConnection && m_outputConnection->isConnected();
}

bool ConnectionHandler::isInputConnected()
{
    return m_inputConnection && m_inputConnection->isConnected();
}

void ConnectionHandler::setOutputConnection(OutputConnectionHandler* device)
{
    if(!device && m_outputConnection) {
        disconnect(m_outputConnection, &OutputConnectionHandler::connectionChange, this, nullptr);
        disconnect(m_outputConnection, &OutputConnectionHandler::commandReceive, this, nullptr);
        m_outputConnection->dispose();
    }
    m_outputConnection = device;
    if(m_outputConnection) {
        connect(m_outputConnection, &OutputConnectionHandler::connectionChange, this, &ConnectionHandler::on_output_connectionChanged, Qt::UniqueConnection);
        connect(m_outputConnection, &OutputConnectionHandler::commandReceive, this, &ConnectionHandler::outputMessageReceived);
        connect(m_outputConnection, &OutputConnectionHandler::commandReceive, this, [this](OutputConnectionPacket packet) {
            if(!packet.original.isEmpty()) {
                auto actions = SettingsHandler::getTCodeCommandMapCommands(packet.original);
                foreach(QString actionValue, actions) {
                    emit action(actionValue);
                }
            }
        });
    }
}
void ConnectionHandler::setInputConnection(InputConnectionHandler* device)
{
    if(!device && m_inputConnection) {
        disconnect(m_inputConnection, &InputConnectionHandler::connectionChange, this, nullptr);
        disconnect(m_inputConnection, &InputConnectionHandler::messageReceived, this, nullptr);
        m_inputConnection->dispose();
    }
    m_inputConnection = device;
    if(m_inputConnection) {
        connect(m_inputConnection, &InputConnectionHandler::connectionChange, this, &ConnectionHandler::on_input_connectionChanged, Qt::UniqueConnection);
        connect(m_inputConnection, &InputConnectionHandler::messageReceived, this, [this](InputConnectionPacket packet){
            if(m_inputConnection->isConnected())
                emit inputMessageReceived(packet);
        });
    }
}

OutputConnectionHandler* ConnectionHandler::getSelectedOutputConnection() {
    return m_outputConnection;
}
InputConnectionHandler* ConnectionHandler::getSelectedInputConnection() {
    return m_inputConnection;
}
OutputNetworkConnectionHandler* ConnectionHandler::getNetworkHandler() {
    return _networkConnection;
}
OutputSerialConnectionHandler* ConnectionHandler::getSerialHandler() {
    return _serialHandler;
}
InputHeresphereConnectionHandler* ConnectionHandler::getHereSphereHandler() {
   return m_hereSphereHandler;
}
InputXTPWebConnectionHandler* ConnectionHandler::getXTPWebHandler() {
    return _xtpWebHandler;
}
InputWhirligigConnectionHandler* ConnectionHandler::getWhirligigHandler() {
    return _whirligigHandler;
}
GamepadHandler* ConnectionHandler::getGamepadHandler() {
    return _gamepadHandler;
}

void ConnectionHandler::inputMessageSend(QByteArray message) {
    if(m_inputConnection)
        m_inputConnection->messageSend(message);
}
void ConnectionHandler::sendTCode(QString tcode)
{
    if(!tcode.isEmpty() && m_outputConnection && m_outputConnection->isConnected())
        m_outputConnection->sendTCode(tcode);
}

void ConnectionHandler::stopOutputConnection()
{
    sendTCode("DSTOP");
}

void ConnectionHandler::initOutputConnection(ConnectionInterface outputDevice)
{
    if (m_outputConnection && m_outputConnection->isConnected())
    {
        m_outputConnection->dispose();
        disconnect(m_outputConnection, &OutputConnectionHandler::connectionChange, nullptr, nullptr);
    }
    if(_initFuture.isRunning())
    {
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
    if(outputDevice == ConnectionInterface::None)
    {
        return;
    }
    else if(outputDevice == ConnectionInterface::Serial)
    {
        setOutputConnection(_serialHandler);
        _serialHandler->init(SettingsHandler::getSerialPort());
    }
    else if (outputDevice == ConnectionInterface::Network)
    {
        if(SettingsHandler::getSelectedNetworkProtocol() == NetworkProtocol::WEBSOCKET) {
           _networkConnection = _webSocketHandler;
        } else {
           _networkConnection = _udpHandler;
        }
        setOutputConnection(_networkConnection);
        if(!SettingsHandler::getServerAddress().isEmpty() && !SettingsHandler::getServerPort().isEmpty() &&
            SettingsHandler::getServerAddress() != "0" && SettingsHandler::getServerPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getServerAddress(), SettingsHandler::getServerPort().toInt() };

            _networkConnection->init(address);
//            _initFuture = QtConcurrent::run([this, address]() {
//                _webSocketHandler->init(address);
//            });
        }
    }
    else if(outputDevice == ConnectionInterface::BLE)
    {
        setOutputConnection(m_bleHandler);
        m_bleHandler->init();
    }
}

void ConnectionHandler::disposeOutputConnection(ConnectionInterface outputDevice)
{
    if(outputDevice == ConnectionInterface::Serial)
    {
        _serialHandler->dispose();
    }
    else if (outputDevice == ConnectionInterface::Network)
    {
        _udpHandler->dispose();
    }
    else if (outputDevice == ConnectionInterface::BLE)
    {
        m_bleHandler->dispose();
    }
}

void ConnectionHandler::initInputConnection(ConnectionInterface inputConnection)
{
    if(inputConnection == ConnectionInterface::Gamepad)
    {
        connect(_gamepadHandler, &GamepadHandler::connectionChange, this, &ConnectionHandler::on_gamepad_connectionChanged);
        connect(_gamepadHandler, &GamepadHandler::emitTCode, this, &ConnectionHandler::sendTCode);
        connect(_gamepadHandler, &GamepadHandler::emitAction, this, &ConnectionHandler::action);
        _gamepadHandler->init();
        return;
    }
    if (m_inputConnection)
    {
        disconnect(m_inputConnection, &InputConnectionHandler::messageReceived, nullptr, nullptr);
        m_inputConnection->dispose();
        disconnect(m_inputConnection, &InputConnectionHandler::connectionChange, nullptr, nullptr);
    }
    if(inputConnection == ConnectionInterface::None)
    {
        return;
    }
    else if(inputConnection == ConnectionInterface::HereSphere)
    {
        setInputConnection(m_hereSphereHandler);
        if(!SettingsHandler::getDeoAddress().isEmpty() && !SettingsHandler::getDeoPort().isEmpty() &&
            SettingsHandler::getDeoAddress() != "0" && SettingsHandler::getDeoPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getDeoAddress(), SettingsHandler::getDeoPort().toInt() };
            m_hereSphereHandler->init(address);
        }
    }
    else if(inputConnection == ConnectionInterface::Whirligig)
    {
        setInputConnection(_whirligigHandler);
        if(!SettingsHandler::getWhirligigAddress().isEmpty() && !SettingsHandler::getWhirligigPort().isEmpty() &&
            SettingsHandler::getWhirligigAddress() != "0" && SettingsHandler::getWhirligigPort() != "0")
        {
            NetworkAddress address { SettingsHandler::getWhirligigAddress(), SettingsHandler::getWhirligigPort().toInt() };
            _whirligigHandler->init(address);
        }
    }
    else if(inputConnection == ConnectionInterface::XTPWeb)
    {
        setInputConnection(_xtpWebHandler);
        if(SettingsHandler::getEnableHttpServer())
            _xtpWebHandler->init();
    }
}

void ConnectionHandler::disposeInputConnection(ConnectionInterface inputConnection)
{
    if(inputConnection == ConnectionInterface::Gamepad)
    {
        _gamepadHandler->dispose();
        disconnect(_gamepadHandler, &GamepadHandler::emitTCode, nullptr, nullptr);
        disconnect(_gamepadHandler, &GamepadHandler::emitAction, nullptr, nullptr);
        disconnect(_gamepadHandler, &GamepadHandler::connectionChange, nullptr, nullptr);
    }
    else if(inputConnection == ConnectionInterface::HereSphere)
    {
        m_hereSphereHandler->dispose();
    }
    else if(inputConnection == ConnectionInterface::Whirligig)
    {
        _whirligigHandler->dispose();
    }
    else if(inputConnection == ConnectionInterface::XTPWeb)
    {
        _xtpWebHandler->dispose();
    }
}

void ConnectionHandler::on_input_connectionChanged(ConnectionChangedSignal event)
{
    _deoConnectionStatus = event.status;
    if (event.status == ConnectionStatus::Error)
    {
        setInputConnection(0);
        emit inputConnectionChange(event);
    }
    else
    {
        SettingsHandler::setSelectedInputConnection(event.connectionName);
    }
    ConnectionChangedSignal status = {event.type, event.connectionName, event.status, event.message};
    emit inputConnectionChange(status);
    emit connectionChange(status);
}
void ConnectionHandler::on_output_connectionChanged(ConnectionChangedSignal event)
{
    _deoConnectionStatus = event.status;
    if (event.status == ConnectionStatus::Error)
    {
        setOutputConnection(0);
        emit outputConnectionChange(event);
    }
    else
    {
        SettingsHandler::setSelectedOutputConnection(event.connectionName);
    }
    ConnectionChangedSignal status = {event.type, event.connectionName, event.status, event.message};
    emit outputConnectionChange(status);
    emit connectionChange(status);
}
void ConnectionHandler::on_gamepad_connectionChanged(ConnectionChangedSignal status)
{
    emit gamepadConnectionChange(status);
    emit connectionChange(status);
}
