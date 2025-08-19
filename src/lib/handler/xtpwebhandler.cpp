#include "xtpwebhandler.h"

InputXTPWebConnectionHandler::InputXTPWebConnectionHandler(QObject *parent) :
    InputConnectionHandler(parent)
{
}

InputXTPWebConnectionHandler::~InputXTPWebConnectionHandler()
{
    _isConnected = false;
//    if (keepAliveTimer != nullptr)
//        delete keepAliveTimer;
    if (_currentPacket)
        delete _currentPacket;
}

ConnectionInterface InputXTPWebConnectionHandler::name() {
    return ConnectionInterface::XTPWeb;
}

void InputXTPWebConnectionHandler::init(NetworkAddress address, int waitTimeout)
{
}

void InputXTPWebConnectionHandler::init()
{
    qRegisterMetaType<ConnectionChangedSignal>();
    qRegisterMetaType<InputConnectionPacket>();
    emit connectionChange({ConnectionDirection::Input, ConnectionInterface::XTPWeb, ConnectionStatus::Connecting, "Waiting..."});

    if (_currentPacket)
        delete _currentPacket;
    _currentPacket = new InputConnectionPacket
    {
        nullptr,
        0,
        0,
        0,
        0,
        0
    };
}

void InputXTPWebConnectionHandler::send(const QString &command)
{

}
void InputXTPWebConnectionHandler::sendPacket(InputConnectionPacket packet) {

}

void InputXTPWebConnectionHandler::dispose()
{
    LogHandler::Debug("XTP Web: dispose");
    _isConnected = false;
    emit connectionChange({ConnectionDirection::Input, ConnectionInterface::XTPWeb, ConnectionStatus::Disconnected, "Disconnected"});
//    if(_httpHandler)
//        disconnect(_httpHandler, &HttpHandler::readyRead, this, &XTPWebHandler::readData);
}

void InputXTPWebConnectionHandler::messageSend(QByteArray message) {
    readData(message);
}
void InputXTPWebConnectionHandler::readData(QByteArray data)
{
    if(!_isConnected)
    {
        _isConnected = true;
        emit connectionChange({ConnectionDirection::Input, ConnectionInterface::XTPWeb, ConnectionStatus::Connected, "Connected"});
    }
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (doc.isNull())
    {
        LogHandler::Error("XTP Web json response error: "+error.errorString());
        LogHandler::Error("data: "+data);
    }
    else
    {
        QJsonObject jsonObject = doc.object();
        QString path = jsonObject["path"].toString();
        qint64 duration = jsonObject["duration"].toDouble() * 1000;
        qint64 currentTime = jsonObject["currentTime"].toDouble() * 1000;
        float playbackSpeed = jsonObject["playbackSpeed"].toDouble() * 1.0;
        bool playing = jsonObject["playing"].toBool();
        bool stopped = jsonObject["stopped"].toBool();;
//        LogHandler::Debug("XTP Web path: "+path);
//        LogHandler::Debug("XTP Web duration: "+QString::number(duration));
//        LogHandler::Debug("XTP Web currentTime------------------------------------------------> "+QString::number(currentTime));
//        LogHandler::Debug("XTP Web _currentTime------------------------------------------------> "+QString::number(_currentTime));
//        LogHandler::Debug("XTP Web playbackSpeed: "+QString::number(playbackSpeed));
//        LogHandler::Debug("XTP Web playing: "+QString::number(playing));
        _mutex.lock();
        _currentPacket = new InputConnectionPacket
        {
            path,
            duration,
            currentTime,
            playbackSpeed,
            playing,
            stopped
       };
        _isPlaying = playing;
        _currentTime = currentTime;
        //LogHandler::Debug("XTP Web _isPlaying: "+QString::number(_isPlaying));
        _mutex.unlock();
        emit messageReceived(*_currentPacket);

    }
}

bool InputXTPWebConnectionHandler::isConnected()
{
    return _isConnected;
}

bool InputXTPWebConnectionHandler::isPlaying()
{
    const QMutexLocker locker(&_mutex);
    return _isPlaying;
}

InputConnectionPacket InputXTPWebConnectionHandler::getCurrentPacket()
{
    const QMutexLocker locker(&_mutex);
    InputConnectionPacket blankPacket = {
        NULL,
        0,
        0,
        0,
        0
    };
    return (_currentPacket == nullptr) ? blankPacket : *_currentPacket;
}
