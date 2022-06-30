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
    GamepadAxisNames gamepadAxisNames;
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
        executeAction(gamepadAxisNames.LeftXAxis, _gamepad->axisLeftX(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.LeftYAxis, _gamepad->axisLeftY(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.RightXAxis, _gamepad->axisRightX(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.RightYAxis, _gamepad->axisRightY(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.RightTrigger, _gamepad->buttonR2(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.LeftTrigger, _gamepad->buttonL2(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.A, _gamepad->buttonA(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.B, _gamepad->buttonB(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.X, _gamepad->buttonX(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.Y, _gamepad->buttonY(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.RightBumper, _gamepad->buttonR1(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.LeftBumper, _gamepad->buttonL1(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.Start, _gamepad->buttonStart(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.Select, _gamepad->buttonSelect(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.DPadUp, _gamepad->buttonUp(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.DPadDown, _gamepad->buttonDown(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.DPadLeft, _gamepad->buttonLeft(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.DPadRight, _gamepad->buttonRight(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.RightAxisButton, _gamepad->buttonR3(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.LeftAxisButton, _gamepad->buttonL3(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.Center, _gamepad->buttonCenter(), axisValues, mediaActions, leftXAxisTimer);
        executeAction(gamepadAxisNames.Guide, _gamepad->buttonGuide(), axisValues, mediaActions, leftXAxisTimer);

        QString currentTCode = _tcodeFactory->formatTCode(&axisValues);
        if (lastTCode != currentTCode)
        {
            lastTCode = currentTCode;
            emit emitTCode(currentTCode);
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
    if (!AxisActions.empty())
    {
//        QStringList currentTCodeActions = subtract(AxisActions, mediaActions.Values.keys());
//        foreach(QString action, currentTCodeActions) {
//            _tcodeFactory->calculate(action, calculateDeadZone(gamepadValue), axisValues);
//        }

//        if(actionTimer.remainingTime() <= 0 && gamepadValue != 0)
//        {
//            foreach(QString action, AxisActions) {
//                if(mediaActions.Values.contains(axisName))
//                    emit emitAction(action);
//            }
//            actionTimer.init(500);
//        }
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

QStringList GamepadHandler::subtract(QList<QString> minuend, QList<QString> subtrahend)
{
    QStringList equal;
    foreach(QString action, minuend)
    {
        if(!subtrahend.contains(action))
            equal << action;
    }
    return equal;
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
