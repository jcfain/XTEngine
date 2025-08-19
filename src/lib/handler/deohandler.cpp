#include "deohandler.h"

InputHeresphereConnectionHandler::InputHeresphereConnectionHandler(QObject *parent) :
    InputConnectionHandler(parent),
    m_connectTries(0)
{
    qRegisterMetaType<ConnectionChangedSignal>();
    qRegisterMetaType<InputConnectionPacket>();
}

InputHeresphereConnectionHandler::~InputHeresphereConnectionHandler()
{
    _isConnected = false;
}

ConnectionInterface InputHeresphereConnectionHandler::name() {
    return ConnectionInterface::HereSphere;
}

void InputHeresphereConnectionHandler::init(NetworkAddress address, int waitTimeout)
{
    emit connectionChange({ConnectionDirection::Input, ConnectionInterface::HereSphere, ConnectionStatus::Connecting, "Waiting..."});

    _waitTimeout = waitTimeout;
    _address = address;

    QHostAddress addressObj;
    addressObj.setAddress(_address.address);
    connect(&tcpSocket, &QTcpSocket::stateChanged, this, &InputHeresphereConnectionHandler::onSocketStateChange);
    connect(&tcpSocket, &QTcpSocket::errorOccurred, this, &InputHeresphereConnectionHandler::tcpErrorOccured);
    m_connectTries++;
    tcpSocket.connectToHost(addressObj, _address.port);
    currentPacket =
    {
        nullptr,
        0,
        0,
        1.0,
        0,
        0
    };
}

void InputHeresphereConnectionHandler::sendKeepAlive()
{
    if (_isConnected)
    {
        //LogHandler::Debug("Sending keepalive: "+ QString::number(QTime::currentTime().msecsSinceStartOfDay()));
        send(nullptr);
    }
    else
    {
        keepAliveTimer.stop();
    }
}

void InputHeresphereConnectionHandler::send(const QString &command)
{
    _sendCommand = command;
    if (!command.isEmpty())
    {
        LogHandler::Debug("Sending to Deo/Heresphere: "+command);
        QByteArray currentRequest(4, '\0');
        currentRequest.append(command.toUtf8());
        tcpSocket.write(currentRequest, currentRequest.size());
        tcpSocket.waitForBytesWritten();
    }
    else
    {
        //LogHandler::Debug("Sending Deo/Heresphere keep alive");
        QByteArray data(4, '\0');
        tcpSocket.write(data, data.size());
        tcpSocket.waitForBytesWritten();
    }
    tcpSocket.flush();
}

void InputHeresphereConnectionHandler::sendPacket(InputConnectionPacket packet) {
    QJsonObject jsonObject;
    jsonObject["path"] = packet.path;
    jsonObject["currentTime"] = packet.currentTime;
    jsonObject["playbackSpeed"] = packet.playbackSpeed;
    send(QJsonDocument(jsonObject).toJson(QJsonDocument::Compact).toStdString().c_str());
}

void InputHeresphereConnectionHandler::dispose()
{
    LogHandler::Debug("Deo/HereSphere: dispose");
    tearDown();
    emit connectionChange({ConnectionDirection::Input, ConnectionInterface::HereSphere, ConnectionStatus::Disconnected, "Disconnected"});
}

void InputHeresphereConnectionHandler::tearDown()
{

    _isConnected = false;
    _isPlaying = false;
    disconnect(&keepAliveTimer, &QTimer::timeout, this, &InputHeresphereConnectionHandler::sendKeepAlive);
    if (keepAliveTimer.isActive())
        keepAliveTimer.stop();
    disconnect(&tcpSocket, &QTcpSocket::stateChanged, this, &InputHeresphereConnectionHandler::onSocketStateChange);
    disconnect(&tcpSocket, &QTcpSocket::errorOccurred, this, &InputHeresphereConnectionHandler::tcpErrorOccured);
    if (tcpSocket.isOpen())
        tcpSocket.disconnectFromHost();
}

void InputHeresphereConnectionHandler::messageSend(QByteArray message) {
    //readData(message);
}
void InputHeresphereConnectionHandler::readData()
{
    QByteArray datagram = tcpSocket.readAll();
    datagram.remove(0, 4);
    int blockOpenIndex = datagram.indexOf('{');
    int blockCloseIndex = datagram.indexOf('}') +1;
    if(blockOpenIndex > -1 && blockCloseIndex > -1)
        datagram = datagram.mid(blockOpenIndex, blockCloseIndex <= datagram.length() ? blockCloseIndex : datagram.length());
    //datagram = datagram.remove(QRegularExpression("{(.*)}"));
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(datagram, &error);
    if (doc.isNull())
    {
        LogHandler::Error("Heresphere/Deo json response error: "+error.errorString());
        LogHandler::Error("datagram: "+datagram);
        //emit connectionChange({DeviceName::Deo, ConnectionStatus::Error, "Read error: " + error.errorString()});
//        if(currentPacket != nullptr)
//        {
//            currentPacket->playing = false;
//        }
    }
    else
    {
        QJsonObject jsonObject = doc.object();
        QString path = jsonObject["path"].toString();
        qint64 duration = jsonObject["duration"].toDouble() * 1000;
        qint64 currentTime = jsonObject["currentTime"].toDouble() * 1000;
        double playbackSpeed = jsonObject["playbackSpeed"].toDouble() * 1.0;
        bool playing = jsonObject["playerState"].toInt() == 0; // 0 == true? right? I know...
        bool stopped = false;
//        LogHandler::Debug("Deo path: "+path);
//        LogHandler::Debug("Deo duration: "+QString::number(duration));
//        LogHandler::Debug("Deo currentTime------------------------------------------------> "+QString::number(currentTime));
//        LogHandler::Debug("Deo _currentTime------------------------------------------------> "+QString::number(_currentTime));
//        LogHandler::Debug("Deo playbackSpeed: "+QString::number(playbackSpeed));
//        LogHandler::Debug("Deo playing: "+QString::number(playing));
        _mutex.lock();
        currentPacket =
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
        //LogHandler::Debug("Deo _isPlaying: "+QString::number(_isPlaying));
        _mutex.unlock();
        emit messageReceived(currentPacket);

    }
}

bool InputHeresphereConnectionHandler::isConnected()
{
    return _isConnected;
}
bool InputHeresphereConnectionHandler::isPlaying()
{
    const QMutexLocker locker(&_mutex);
    //LogHandler::Error("DeoHandler::isPlaying(): "+ QString::number(_isPlaying));
    return _isPlaying;
}
//void DeoHandler::togglePause()
//{
//    bool isPaused = false;
//    if(!getCurrentDeoPacket()->playing)
//    {
//        isPaused = true;
//    }
//    QJsonObject pausePacket{
//        {"playerState",isPaused}
//    };
//    QJsonDocument jsonResponse = QJsonDocument(pausePacket);
//    send(QString::fromLatin1(jsonResponse.toJson()));
//}
InputConnectionPacket InputHeresphereConnectionHandler::getCurrentPacket()
{
    const QMutexLocker locker(&_mutex);
    return !_isConnected ? blankPacket : currentPacket;
}

void InputHeresphereConnectionHandler::onSocketStateChange (QAbstractSocket::SocketState state)
{
    //const QMutexLocker locker(&_mutex);
    switch(state) {
        case QAbstractSocket::SocketState::ConnectedState:
        {
            _isConnected = true;
            m_connectTries = 0;
            LogHandler::Debug("Deo/HereSphere connected");
            send(nullptr);
            if (keepAliveTimer.isActive())
                keepAliveTimer.stop();
            connect(&keepAliveTimer, &QTimer::timeout, this, &InputHeresphereConnectionHandler::sendKeepAlive);
            keepAliveTimer.start(1000);
            connect(&tcpSocket, &QTcpSocket::readyRead, this, &InputHeresphereConnectionHandler::readData);
            emit connectionChange({ConnectionDirection::Input, ConnectionInterface::HereSphere, ConnectionStatus::Connected, "Connected"});
            break;
        }
        case QAbstractSocket::SocketState::UnconnectedState:
        {
            _isConnected = false;
            if(SettingsHandler::getSelectedInputDevice() == ConnectionInterface::HereSphere)
            {
                LogHandler::Debug("HereSphere retrying: " +QString::number(m_connectTries) + " " + _address.address);
                tearDown();
                QTimer::singleShot(2000, [this] () {
                    init(_address, _waitTimeout);
                });
            }
            else
            {
                LogHandler::Debug("Deo/HereSphere disconnected");
                emit connectionChange({ConnectionDirection::Input, ConnectionInterface::HereSphere, ConnectionStatus::Disconnected, "Disconnected"});
            }
            break;
        }
        case QAbstractSocket::SocketState::ConnectingState:
        {
            LogHandler::Debug("Deo/HereSphere connecting");
            emit connectionChange({ConnectionDirection::Input, ConnectionInterface::HereSphere, ConnectionStatus::Connecting, "Waiting..."});
            break;
        }
        case QAbstractSocket::SocketState::BoundState:
        {
            LogHandler::Debug("Deo/HereSphere bound");
            break;
        }
        case QAbstractSocket::SocketState::ListeningState:
        {
            LogHandler::Debug("Deo/HereSphere listening");
            break;
        }
        case QAbstractSocket::SocketState::HostLookupState:
        {
            LogHandler::Debug("Deo/HereSphere host look up");
            break;
        }
        case QAbstractSocket::SocketState::ClosingState:
        {
            LogHandler::Debug("Deo/HereSphere closing");
            break;
        }
    }
}

void InputHeresphereConnectionHandler::tcpErrorOccured(QAbstractSocket::SocketError state)
{

    switch(state)
    {
        case QAbstractSocket::SocketError::AddressInUseError:
        {
            LogHandler::Error("Deo/HereSphere AddressInUseError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ConnectionRefusedError:
        {
            LogHandler::Error("Deo/HereSphere ConnectionRefusedError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::DatagramTooLargeError:
        {
            LogHandler::Error("Deo/HereSphere DatagramTooLargeError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::HostNotFoundError:
        {
            LogHandler::Error("Deo/HereSphere HostNotFoundError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::NetworkError:
        {
            LogHandler::Error("Deo/HereSphere NetworkError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::OperationError:
        {
            LogHandler::Error("Deo/HereSphere OperationError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyAuthenticationRequiredError:
        {
            LogHandler::Error("Deo/HereSphere ProxyAuthenticationRequiredError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionClosedError:
        {
            LogHandler::Error("Deo/HereSphere ProxyConnectionClosedError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionRefusedError:
        {
            LogHandler::Error("Deo/HereSphere ProxyConnectionRefusedError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionTimeoutError:
        {
            LogHandler::Error("Deo/HereSphere ProxyConnectionTimeoutError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyNotFoundError:
        {
            LogHandler::Error("Deo/HereSphere ProxyNotFoundError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyProtocolError:
        {
            LogHandler::Error("Deo/HereSphere ProxyProtocolError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::RemoteHostClosedError:
        {
            LogHandler::Error("Deo/HereSphere RemoteHostClosedError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketAccessError:
        {
            LogHandler::Error("Deo/HereSphere SocketAccessError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketAddressNotAvailableError:
        {
            LogHandler::Error("Deo/HereSphere SocketAddressNotAvailableError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketResourceError:
        {
            LogHandler::Error("Deo/HereSphere SocketResourceError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketTimeoutError:
        {
            LogHandler::Error("Deo/HereSphere SocketTimeoutError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslHandshakeFailedError:
        {
            LogHandler::Error("Deo/HereSphere SslHandshakeFailedError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslInternalError:
        {
            LogHandler::Error("Deo/HereSphere SslInternalError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslInvalidUserDataError:
        {
            LogHandler::Error("Deo/HereSphere SslInvalidUserDataError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::TemporaryError:
        {
            LogHandler::Error("Deo/HereSphere TemporaryError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnfinishedSocketOperationError:
        {
            LogHandler::Error("Deo/HereSphere UnfinishedSocketOperationError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnknownSocketError:
        {
            LogHandler::Error("Deo/HereSphere UnknownSocketError: "+tcpSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnsupportedSocketOperationError:
        {
            LogHandler::Error("Deo/HereSphere UnsupportedSocketOperationError: "+tcpSocket.errorString());
            break;
        }
    }
}
//#include <QNetworkRequest>
//#include <QNetworkAccessManager>
//#include <QNetworkReply>
//#include <QEventLoop>
//QString m_device; //!< Device uuid.
//QNetworkReply::NetworkError m_error = QNetworkReply::NoError; //!< Network error.
//QNetworkAccessManager* m_naMgr = nullptr; //!< The current netword access manager. see CActionManager (QNetworkAccessManager* naMgr, QObject* parent).

//static int m_elapsedTime; //!< The time to execute the last action.
//static QString m_lastError; //!< The error generated by the last action.
//bool DeoHandler::post (QString const & device, QUrl const & url, int timeout)
//{
//    if (m_naMgr == nullptr)
//    {
//      m_naMgr = new QNetworkAccessManager (this);
//    }
//  m_lastError.clear ();
//  m_device     = device;
//  bool success = false;
//  if (!isRunning ())
//  {
//    QNetworkRequest req (url);
//    req.setPriority (QNetworkRequest::HighPriority);
//     // To fix a problem with DSM6 (Synology). If User-Agent exists DSM send only the full precision
//     // image not the thumbnails.
//    req.setHeader (QNetworkRequest::UserAgentHeader, " ");
//    req.setHeader (QNetworkRequest::ContentTypeHeader, QString ("text/xml; charset=\"utf-8\""));
//    req.setRawHeader ("Accept-Encoding", "*");
//    req.setRawHeader ("Accept-Language", "*");
//    req.setRawHeader ("Connection", "Close");
//    QString const & actionName    = info.actionName ();
//    QString         soapActionHdr = QString ("\"%1#%2\"").arg (info.serviceID (), actionName);
//    req.setRawHeader ("SOAPAction", soapActionHdr.toUtf8 ());

//    QTime time;
//    time.start ();

//    m_naMgr->setNetworkAccessible (QNetworkAccessManager::Accessible);
//    QNetworkReply* reply = m_naMgr->post (req, info.message ().toUtf8 ());
//    connect (reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error(QNetworkReply::NetworkError)));
//    connect (reply, SIGNAL(finished()), this, SLOT(finished()));

//    m_error     = QNetworkReply::NoError;
//    int idTimer = startTimer (timeout);
//    int retCode = exec (QEventLoop::ExcludeUserInputEvents/* | QEventLoop::ExcludeSocketNotifiers*/);
//    success     =  retCode == 0 && m_error == QNetworkReply::NoError;
//    killTimer (idTimer);
//    if (success)
//    {
//      info.setResponse (reply->readAll ());
//    }
//    else
//    {
//      if (m_error != QNetworkReply::NoError)
//      {
//        qint32 statusCode = reply->attribute (QNetworkRequest::HttpStatusCodeAttribute).toInt ();
//        m_lastError       = reply->attribute (QNetworkRequest::HttpReasonPhraseAttribute).toString ();
//        QString message   = QString ("Action failed. Response from the server: %1, %2").arg (statusCode).arg (m_lastError);
//        message          += '\n';
//        message          += url.toString () + '\n';
//        message          += info.message () + "\n\n";
//        qDebug () << "CActionManager::post:" << message;
//        m_lastError.prepend (QString ("(%1) ").arg (statusCode));
//      }
//    }

//    reply->deleteLater ();
//    m_elapsedTime = time.elapsed ();
//  }

//  return success;
//}
