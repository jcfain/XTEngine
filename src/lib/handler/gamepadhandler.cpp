#include "gamepadhandler.h"

GamepadHandler::GamepadHandler(QObject *parent):
    QThread(parent)
{
    qRegisterMetaType<ConnectionChangedSignal>();
    _tcodeFactory = new TCodeFactory(0.0, 1.0);
}

GamepadHandler::~GamepadHandler()
{
    if (_gamepadState != nullptr)
        delete _gamepadState;
    if (_gamepad != nullptr)
        delete _gamepad;
    delete _tcodeFactory;
}

void GamepadHandler::init()
{
    _stop = false;
    emit connectionChange({DeviceType::Input, DeviceName::Gamepad, ConnectionStatus::Connecting, "Connecting..."});
    connect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this, &GamepadHandler::connectedGamepadsChanged);
    connectedGamepadsChanged();
}

void GamepadHandler::connectedGamepadsChanged()
{
    _gamepads = QGamepadManager::instance()->connectedGamepads();
    if(_gamepads.count() > 0 && (_gamepad == nullptr || !_isConnected))
    {
        LogHandler::Debug("Gamepad connected: "+ QString::number(*_gamepads.begin()));
        LogHandler::Debug("Gamepads connected count: "+ QString::number(_gamepads.count()));
        _stop = false;
        _isConnected = true;
        _gamepad = new QGamepad(*_gamepads.begin(), this);
        emit connectionChange({DeviceType::Input, DeviceName::Gamepad, ConnectionStatus::Connected, "Connected"});
        connect(_gamepad, &QGamepad::connectedChanged, this, &GamepadHandler::gamePadConnectionChanged);
        if(!isRunning())
            start();
    }
    else if (_gamepad != nullptr && !_gamepad->isConnected())
    {
        LogHandler::Debug("Gamepads disconnected: "+ QString::number(_gamepads.count()));
        _isConnected = false;
        disposeInternal();
    }
    else
    {
        LogHandler::Debug("Gamepad connectionchange count: "+ QString::number(_gamepads.count()));
    }
}

void GamepadHandler::gamePadConnectionChanged(bool connected)
{
    //_isConnected = connected;
    if(connected)
    {
        LogHandler::Debug("Gamepads connected event");
//        _stop = false;
//        emit connectionChange({DeviceName::Gamepad, ConnectionStatus::Connected, "Connected"});
//        start();
    }
    else
    {
        LogHandler::Debug("Gamepad disconnected event");
    }
}
void GamepadHandler::run()
{
    QString lastTCode;
    MediaActions mediaActions;
    QVector<ChannelValueModel> axisValues;
    GamepadAxisName gamepadAxisName;
    XTimer leftXAxisTimer;
    XTimer leftYAxisTimer;
    XTimer rightXAxisTimer;
    XTimer rightYAxisTimer;
    XTimer rightTriggerTimer;
    XTimer leftTriggerTimer;
    XTimer aTimer;
    XTimer bTimer;
    XTimer xTimer;
    XTimer yTimer;
    XTimer rightBumperTimer;
    XTimer leftBumperTimer;
    XTimer startTimer;
    XTimer selectTimer;
    XTimer dPadUpTimer;
    XTimer dPadDownTimer;
    XTimer dPadRightTimer;
    XTimer dPadLeftTimer;
    XTimer rightAxisButtonTimer;
    XTimer leftAxisButtonTimer;
    XTimer centerTimer;
    XTimer guideTimer;

    while(!_stop)
    {
        axisValues.clear();
        if(!_listeningForInput) {
            executeAction(gamepadAxisName.LeftXAxis, _gamepad->axisLeftX(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.LeftYAxis, _gamepad->axisLeftY(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.RightXAxis, _gamepad->axisRightX(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.RightYAxis, _gamepad->axisRightY(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.RightTrigger, _gamepad->buttonR2(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.LeftTrigger, _gamepad->buttonL2(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.A, _gamepad->buttonA(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.B, _gamepad->buttonB(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.X, _gamepad->buttonX(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.Y, _gamepad->buttonY(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.RightBumper, _gamepad->buttonR1(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.LeftBumper, _gamepad->buttonL1(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.Start, _gamepad->buttonStart(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.Select, _gamepad->buttonSelect(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.DPadUp, _gamepad->buttonUp(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.DPadDown, _gamepad->buttonDown(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.DPadLeft, _gamepad->buttonLeft(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.DPadRight, _gamepad->buttonRight(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.RightAxisButton, _gamepad->buttonR3(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.LeftAxisButton, _gamepad->buttonL3(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.Center, _gamepad->buttonCenter(), axisValues, mediaActions, leftXAxisTimer);
            executeAction(gamepadAxisName.Guide, _gamepad->buttonGuide(), axisValues, mediaActions, leftXAxisTimer);

            QString currentTCode = _tcodeFactory->formatTCode(&axisValues);
            if (lastTCode != currentTCode)
            {
                lastTCode = currentTCode;
                emit emitTCode(currentTCode);
            }
        }
        msleep(1);

    }
    leftXAxisTimer.stop();
    leftYAxisTimer.stop();
    rightXAxisTimer.stop();
    rightYAxisTimer.stop();
    rightTriggerTimer.stop();
    leftTriggerTimer.stop();
    aTimer.stop();
    bTimer.stop();
    xTimer.stop();
    yTimer.stop();
    rightBumperTimer.stop();
    leftBumperTimer.stop();
    startTimer.stop();
    selectTimer.stop();
    dPadUpTimer.stop();
    dPadDownTimer.stop();
    dPadRightTimer.stop();
    dPadLeftTimer.stop();
    rightAxisButtonTimer.stop();
    leftAxisButtonTimer.stop();
    centerTimer.stop();
    guideTimer.stop();
}

void GamepadHandler::executeAction(QString axisName, double gamepadValue, QVector<ChannelValueModel> &axisValues, MediaActions &mediaActions, XTimer &actionTimer) {

    QStringList AxisActions = SettingsHandler::getGamePadMapButton(axisName);
    if (!AxisActions.empty() && calculateDeadZone(gamepadValue) != 0.0)
    {
        bool actionExecuted = false;
        foreach(QString action, AxisActions) {
          if (!mediaActions.Values.contains(action))
              _tcodeFactory->calculate(action, calculateDeadZone(gamepadValue), axisValues);
          else if(actionTimer.remainingTime() <= 0 && gamepadValue != 0)
          {
              actionExecuted = true;
              emit emitAction(action);
          }
        }
        if(actionExecuted)
            actionTimer.init(500);
    }
}

void GamepadHandler::listenForInput()
{
    _listeningForInput = true;
    GamepadAxisName gamepadAxisName;
    connect(_gamepad, &QGamepad::axisLeftXChanged, this, [this, gamepadAxisName](double value) { listenForInputAxisChange(value, gamepadAxisName.LeftXAxis); });
    connect(_gamepad, &QGamepad::axisLeftYChanged, this, [this, gamepadAxisName](double value) { listenForInputAxisChange(value, gamepadAxisName.LeftYAxis); });
    connect(_gamepad, &QGamepad::axisRightXChanged, this, [this, gamepadAxisName](double value) { listenForInputAxisChange(value, gamepadAxisName.RightXAxis); });
    connect(_gamepad, &QGamepad::axisRightYChanged, this, [this, gamepadAxisName](double value) { listenForInputAxisChange(value, gamepadAxisName.RightYAxis); });
    connect(_gamepad, &QGamepad::buttonR2Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.RightTrigger); });
    connect(_gamepad, &QGamepad::buttonL2Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.LeftTrigger); });
    connect(_gamepad, &QGamepad::buttonAChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.A); });
    connect(_gamepad, &QGamepad::buttonBChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.B); });
    connect(_gamepad, &QGamepad::buttonXChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.X); });
    connect(_gamepad, &QGamepad::buttonYChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.Y); });
    connect(_gamepad, &QGamepad::buttonL1Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.RightBumper); });
    connect(_gamepad, &QGamepad::buttonR1Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.LeftBumper); });
    connect(_gamepad, &QGamepad::buttonStartChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.Start); });
    connect(_gamepad, &QGamepad::buttonSelectChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.Select); });
    connect(_gamepad, &QGamepad::buttonUpChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.DPadUp); });
    connect(_gamepad, &QGamepad::buttonDownChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.DPadDown); });
    connect(_gamepad, &QGamepad::buttonLeftChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.DPadLeft); });
    connect(_gamepad, &QGamepad::buttonRightChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.DPadRight); });
    connect(_gamepad, &QGamepad::buttonR3Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.RightAxisButton); });
    connect(_gamepad, &QGamepad::buttonL3Changed, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.LeftAxisButton); });
    connect(_gamepad, &QGamepad::buttonCenterChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.Center); });
    connect(_gamepad, &QGamepad::buttonGuideChanged, this, [this, gamepadAxisName]() {  onListenForInput(gamepadAxisName.Guide); });
}
void GamepadHandler::listenForInputAxisChange(double axisValue, QString channel)
{
    if(axisValue > 0.5 || axisValue < -0.5)
        onListenForInput(channel);
}
void GamepadHandler::cancelListenForInput()
{
    disconnect(_gamepad, &QGamepad::axisLeftXChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::axisLeftYChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::axisRightXChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::axisRightYChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonR2Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonL2Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonAChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonBChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonXChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonYChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonL1Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonR1Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonStartChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonSelectChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonUpChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonDownChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonLeftChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonRightChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonR3Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonL3Changed, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonCenterChanged, nullptr, nullptr);
    disconnect(_gamepad, &QGamepad::buttonGuideChanged, nullptr, nullptr);
    _listeningForInput = false;
}

void GamepadHandler::onListenForInput(QString button)
{
    emit onListenForInputRecieve(button);
    cancelListenForInput();
}


double GamepadHandler::calculateDeadZone(double gpIn)
{
    if ((gpIn < _deadzone && gpIn > 0) || (gpIn > -_deadzone && gpIn < 0))
    {
        return 0;
    }
    return gpIn;
}

void GamepadHandler::disposeInternal()
{
    _mutex.lock();
    _stop = true;
    _isConnected = false;
    if (_gamepad != nullptr && _gamepad->isConnected())
        _gamepad->disconnect();

    if (_gamepad != nullptr)
        disconnect(_gamepad, &QGamepad::connectedChanged, this, &GamepadHandler::gamePadConnectionChanged);
    _mutex.unlock();

    emit connectionChange({DeviceType::Input, DeviceName::Gamepad, ConnectionStatus::Disconnected, "Disconnected"});
    if(isRunning())
    {
        quit();
        wait();
    }
}

void GamepadHandler::dispose()
{
    disposeInternal();
    disconnect(QGamepadManager::instance(), &QGamepadManager::connectedGamepadsChanged, this, &GamepadHandler::connectedGamepadsChanged);
}

QHash<QString, QVariant>* GamepadHandler::getState()
{
    const QMutexLocker locker(&_mutex);
    return _gamepadState;
}

bool GamepadHandler::isConnected()
{
    return _isConnected;
}
