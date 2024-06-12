#include "websocketdevicehandler.h"

WebsocketDeviceHandler::WebsocketDeviceHandler(QObject *parent)
    : NetworkDevice{parent}
{
    qRegisterMetaType<ConnectionChangedSignal>();
    //qRegisterMetaType<QAbstractSocket::SocketState>();

}

WebsocketDeviceHandler::~WebsocketDeviceHandler()
{
}

void WebsocketDeviceHandler::init(NetworkAddress address, int waitTimeout)
{
    _address = address;
    if(_isListening || isConnected()) {
        m_webSocket.close();
    }
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connecting, "Connecting..."});
    connect(&m_webSocket, &QWebSocket::connected, this, &WebsocketDeviceHandler::onConnected);
    connect(&m_webSocket, &QWebSocket::disconnected, this, &WebsocketDeviceHandler::onClosed);
    connect(&m_webSocket, &QWebSocket::stateChanged, this, &WebsocketDeviceHandler::onSocketStateChange);
    connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, &WebsocketDeviceHandler::onErrorOccured);
    m_webSocket.open(QUrl("ws://" + address.address + ":" + QString::number(address.port) + "/ws"));
    _isListening = true;
}

void WebsocketDeviceHandler::onConnected() {
    connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebsocketDeviceHandler::onTextMessageReceived);
    if(SettingsHandler::getDisableTCodeValidation()) {
        emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: No validate"});
        setConnected(true);
    } else {
        emit sendHandShake();
    }

}

void WebsocketDeviceHandler::onClosed() {
    disconnect(&m_webSocket, nullptr, nullptr, nullptr);
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Disconnected, "Disconnected"});
    setConnected(false);
}

void WebsocketDeviceHandler::onTextMessageReceived(QString response)
{
    processDeviceInput(response);
    // QString version = "V?";
    // bool validated = false;
    // if(response.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v2)))
    // {
    //     version = "V2";
    //     validated = true;
    // }
    // else if (response.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v3)))
    // {
    //     version = "V3";
    //     validated = true;
    // }
    // if (validated)
    // {
    //     emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: "+version});
    //     _isConnected = true;
    // }
    // else
    // {
    //     emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "No " + TCodeChannelLookup::getSelectedTCodeVersionName()});
    //     dispose();
    // }
}

void WebsocketDeviceHandler::sendTCode(const QString &tcode)
{
    //const QMutexLocker locker(&_mutex);
    QString tcodeFormatted = tcode + "\n";
    LogHandler::Debug("Sending TCode websocket: "+tcode);
    m_webSocket.sendTextMessage(tcodeFormatted);
}

void WebsocketDeviceHandler::dispose()
{
    LogHandler::Debug("Websocket dispose "+ _address.address);
    m_webSocket.close();
    OutputDeviceHandler::dispose();
}


void WebsocketDeviceHandler::onSocketStateChange (QAbstractSocket::SocketState state)
{
    //const QMutexLocker locker(&_mutex);
    switch(state) {
        case QAbstractSocket::SocketState::ConnectedState:
        {
//            //_mutex.lock();
//            _isConnected = true;
            LogHandler::Debug("Websocket connected");
//            send(nullptr);
//            if (keepAliveTimer != nullptr && keepAliveTimer->isActive())
//                keepAliveTimer->stop();
//            keepAliveTimer = new QTimer(this);
//            //_mutex.unlock();
//            connect(keepAliveTimer, &QTimer::timeout, this, &WebsocketDeviceHandler::sendKeepAlive);
//            keepAliveTimer->start(1000);
//            connect(m_webSocket, &Qm_webSocket::readyRead, this, &WebsocketDeviceHandler::readData);
//            emit connectionChange({DeviceType::Input, DeviceName::Deo, ConnectionStatus::Connected, "Connected"});
            break;
        }
        case QAbstractSocket::SocketState::UnconnectedState:
        {
//            //_mutex.lock();
//            _isConnected = false;
//            _isPlaying = false;
//            //_mutex.unlock();
//            if (keepAliveTimer != nullptr)
//            {
//                disconnect(keepAliveTimer, &QTimer::timeout, this, &WebsocketDeviceHandler::sendKeepAlive);
//            }
//            if (keepAliveTimer != nullptr && keepAliveTimer->isActive())
//            {
//                keepAliveTimer->stop();
//            }
//            if(SettingsHandler::getSelectedInputDevice() == DeviceName::Deo)
//            {
                //LogHandler::Debug("Websocket retrying: " + _address.address);
//                tearDown();
//                QTimer::singleShot(2000, [this] () {
//                    init(_address, _waitTimeout);
//                });
//            }
//            else
//            {
                LogHandler::Debug("Websocket disconnected");
                //emit connectionChange({DeviceType::Input, DeviceName::Deo, ConnectionStatus::Disconnected, "Disconnected"});
//            }
            break;
        }
        case QAbstractSocket::SocketState::ConnectingState:
        {
            LogHandler::Debug("Websocket connecting");
            //emit connectionChange({DeviceType::Input, DeviceName::Deo, ConnectionStatus::Connecting, "Waiting..."});
            break;
        }
        case QAbstractSocket::SocketState::BoundState:
        {
            LogHandler::Debug("Websocket bound");
            break;
        }
        case QAbstractSocket::SocketState::ListeningState:
        {
            LogHandler::Debug("Websocket listening");
            break;
        }
        case QAbstractSocket::SocketState::HostLookupState:
        {
            LogHandler::Debug("Websocket host look up");
            break;
        }
        case QAbstractSocket::SocketState::ClosingState:
        {
            LogHandler::Debug("Websocket closing");
            break;
        }
    }
}

void WebsocketDeviceHandler::onErrorOccured(QAbstractSocket::SocketError state)
{

    switch(state)
    {
        case QAbstractSocket::SocketError::AddressInUseError:
        {
            LogHandler::Error("Websocket AddressInUseError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ConnectionRefusedError:
        {
            LogHandler::Error("Websocket ConnectionRefusedError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::DatagramTooLargeError:
        {
            LogHandler::Error("Websocket DatagramTooLargeError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::HostNotFoundError:
        {
            LogHandler::Error("Websocket HostNotFoundError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::NetworkError:
        {
            LogHandler::Error("Websocket NetworkError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::OperationError:
        {
            LogHandler::Error("Websocket OperationError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyAuthenticationRequiredError:
        {
            LogHandler::Error("Websocket ProxyAuthenticationRequiredError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionClosedError:
        {
            LogHandler::Error("Websocket ProxyConnectionClosedError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionRefusedError:
        {
            LogHandler::Error("Websocket ProxyConnectionRefusedError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyConnectionTimeoutError:
        {
            LogHandler::Error("Websocket ProxyConnectionTimeoutError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyNotFoundError:
        {
            LogHandler::Error("Websocket ProxyNotFoundError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::ProxyProtocolError:
        {
            LogHandler::Error("Websocket ProxyProtocolError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::RemoteHostClosedError:
        {
            LogHandler::Error("Websocket RemoteHostClosedError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketAccessError:
        {
            LogHandler::Error("Websocket SocketAccessError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketAddressNotAvailableError:
        {
            LogHandler::Error("Websocket SocketAddressNotAvailableError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketResourceError:
        {
            LogHandler::Error("Websocket SocketResourceError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SocketTimeoutError:
        {
            LogHandler::Error("Websocket SocketTimeoutError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslHandshakeFailedError:
        {
            LogHandler::Error("Websocket SslHandshakeFailedError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslInternalError:
        {
            LogHandler::Error("Websocket SslInternalError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::SslInvalidUserDataError:
        {
            LogHandler::Error("Websocket SslInvalidUserDataError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::TemporaryError:
        {
            LogHandler::Error("Websocket TemporaryError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnfinishedSocketOperationError:
        {
            LogHandler::Error("Websocket UnfinishedSocketOperationError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnknownSocketError:
        {
            LogHandler::Error("Websocket UnknownSocketError: "+m_webSocket.errorString());
            break;
        }
        case QAbstractSocket::SocketError::UnsupportedSocketOperationError:
        {
            LogHandler::Error("Websocket UnsupportedSocketOperationError: "+m_webSocket.errorString());
            break;
        }
    }
}
