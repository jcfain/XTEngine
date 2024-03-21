#include "udphandler.h"
#include <QtConcurrent/QtConcurrent>
#include <QNetworkInterface>

UdpHandler::UdpHandler(QObject *parent) :
    NetworkDevice(parent),
    m_udpSocket(new QUdpSocket(this))
{
    qRegisterMetaType<ConnectionChangedSignal>();
    connect(this, &UdpHandler::sendHandShake, this, &UdpHandler::onSendHandShake);
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
}
UdpHandler::~UdpHandler() {}

DeviceName UdpHandler::name() {
    return DeviceName::Network;
}

void UdpHandler::init(NetworkAddress address, int waitTimeout)
{
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connecting, "Connecting..."});
    //_mutex.lock();
    _stop = false;
    _isConnected = false;
    _isSelected = true;
    _waitTimeout = waitTimeout;
    _address = address;
    //_mutex.unlock();
    if(_initFuture.isRunning())
    {
        _stop = true;
        _initFuture.cancel();
        _initFuture.waitForFinished();
        _stop = false;
    }
    _initFuture = QtConcurrent::run([this]() {
        m_hostAddress = QHostAddress(_address.address);
        if (QAbstractSocket::IPv4Protocol == m_hostAddress.protocol() || QAbstractSocket::IPv6Protocol == m_hostAddress.protocol())
        {
            LogHandler::Debug("IP address passed in: "+_address.address);
        }
        else
        {
            LogHandler::Debug("Hostname passed in: "+_address.address);
            QHostInfo info = QHostInfo::fromName(_address.address);
            if(info.error() != QHostInfo::NoError) {
                emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, info.errorString()});
                return;
            }
            if (!info.addresses().isEmpty()) {
                m_hostAddress = QHostAddress(info.addresses().constFirst());
            } else {
                emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "No IP for: " + _address.address + " found"});
                return;
            }
        }
        uint8_t timeouttracker = 0;
        const uint8_t maxTries = 3;
        while(!_isConnected && !_stop && timeouttracker <= maxTries)
        {
            emit sendHandShake();
            timeouttracker++;
            QThread::msleep(_waitTimeout);
        }
        if (!_stop && timeouttracker > maxTries)
        {
            _isConnected = false;
            emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "Timed out"});
        }
    });
}

void UdpHandler::onSendHandShake() {
    sendTCode("D1");
}

void UdpHandler::onReadyRead()
{
    QString recieved;
    while (m_udpSocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        recieved += QString::fromUtf8(datagram.data());
    }
    if(recieved.isEmpty()) {
        return;
    }
    LogHandler::Debug("Recieved UDP: "+recieved);
    if(_isConnected) {
        emit commandRecieve(recieved);
    } else {
        QString version = "V?";
        if(!SettingsHandler::getDisableTCodeValidation())
        {
            bool validated = false;
            if(recieved.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v2)))
            {
                version = "V2";
                validated = true;
            }
            else if (recieved.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v3)))
            {
                version = "V3";
                validated = true;
            }
            if (validated)
            {
                emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: "+version});
                _mutex.lock();
                _isConnected = true;
                _mutex.unlock();
            }
            else
            {
                emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "No " + TCodeChannelLookup::getSelectedTCodeVersionName()});
            }
        }
        else
        {
            emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: "+version});
            _mutex.lock();
            _isConnected = true;
            _mutex.unlock();
        }
    }
}


void UdpHandler::sendTCode(const QString &tcode)
{
    const QMutexLocker locker(&_mutex);
    auto formattedTcode = tcode + "\n";
    LogHandler::Debug("Sending TCode UDP: "+tcode);

    QByteArray currentRequest;
    currentRequest.append(formattedTcode.toUtf8());
    m_udpSocket->writeDatagram(currentRequest, m_hostAddress, _address.port);
    // if (!isRunning())
    //     start();
    // else
    //     _cond.wakeOne();
}

void UdpHandler::run()
{
    // bool currentAddressChanged = false;

    // _mutex.lock();
    // //_tcode = "";
    // QByteArray currentRequest;
    // currentRequest.append(_tcode.toUtf8());

    // QString currentAddress;
    // int currentPort = 0;
    // QHostAddress addressObj;
    // addressObj.setAddress(_address.address);
    // currentAddress = _address.address;
    // currentPort = _address.port;
    // _isConnected = false;
    // currentAddressChanged = true;

    // _mutex.unlock();
    // //QScopedPointer<QUdpSocket> udpSocketRecieve(new QUdpSocket(this));
    // //if (!udpSocketSend->bind(QHostAddress::Any, 54000))
    // //{
    //     //emit connectionChange({DeviceName::Network, ConnectionStatus::Error, "Error opening handshake"});
    // //}
    // while (!_stop)
    // {
    //     if (currentAddressChanged)
    //     {

    //         //udpSocket->bind(QHostAddress::LocalHost, currentPort);
    //         udpSocket->connectToHost(currentAddress, currentPort);
    //         connect(udpSocket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead);
    //         if(!udpSocket->waitForConnected(_waitTimeout))
    //         {
    //             emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "Can't connect"});
    //         }
    //     }
    //     if(udpSocket->isWritable())
    //     {
    //         udpSocket->write(currentRequest);
    //         // if (!_isConnected)
    //         // {
    //         //     QString version = "V?";
    //         //     if(!SettingsHandler::getDisableTCodeValidation())
    //         //     {
    //         //         if(udpSocket->waitForReadyRead(_waitTimeout))
    //         //         {
    //         //             QString recieved;
    //         //             while (udpSocket->hasPendingDatagrams()) {
    //         //                 QNetworkDatagram datagram = udpSocket->receiveDatagram();
    //         //                 recieved += QString::fromUtf8(datagram.data());
    //         //             }

    //         //             bool validated = false;
    //         //             if(recieved.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v2)))
    //         //             {
    //         //                 version = "V2";
    //         //                 validated = true;
    //         //             }
    //         //             else if (recieved.contains(TCodeChannelLookup::getTCodeVersionName(TCodeVersion::v3)))
    //         //             {
    //         //                 version = "V3";
    //         //                 validated = true;
    //         //             }
    //         //             if (validated)
    //         //             {
    //         //                 emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: "+version});
    //         //                 _mutex.lock();
    //         //                 _isConnected = true;
    //         //                 _mutex.unlock();
    //         //             }
    //         //             else
    //         //             {
    //         //                 emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "No " + TCodeChannelLookup::getSelectedTCodeVersionName()});
    //         //             }
    //         //         }
    //         //     }
    //         //     else
    //         //     {
    //         //         emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connected, "Connected: "+version});
    //         //         _mutex.lock();
    //         //         _isConnected = true;
    //         //         _mutex.unlock();
    //         //     }
    //         // }
    //         // else if(_isConnected && udpSocket->bytesAvailable()) {
    //         //     QString recieved;
    //         //     while (udpSocket->hasPendingDatagrams()) {
    //         //         QNetworkDatagram datagram = udpSocket->receiveDatagram();
    //         //         recieved += QString::fromUtf8(datagram.data());
    //         //     }
    //         //     emit commandRecieve(recieved);
    //         // }
    //     }

    //     if (!_stop)
    //     {
    //         _mutex.lock();
    //         _cond.wait(&_mutex);
    //         if (currentAddress != _address.address || currentPort != _address.port)
    //         {
    //             currentAddress = _address.address;
    //             addressObj.setAddress(_address.address);
    //             currentPort = _address.port;
    //             currentAddressChanged = true;
    //             _isConnected = false;
    //         }
    //         else
    //         {
    //             currentAddressChanged = false;
    //         }

    //         currentRequest.clear();
    //         currentRequest.append(_tcode.toUtf8());
    //         _mutex.unlock();
    //     }
    // }
}

void UdpHandler::dispose()
{
    LogHandler::Debug("Udp dispose "+ _address.address);
    _mutex.lock();
    _isConnected = false;
    _stop = true;
     _mutex.unlock();
    // _cond.wakeOne();
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Disconnected, "Disconnected"});
    // if(isRunning())
    // {
    //     quit();
    //     wait();
    // }
}

bool UdpHandler::isConnected()
{
    QMutexLocker locker(&_mutex);
    return _isConnected;
}

