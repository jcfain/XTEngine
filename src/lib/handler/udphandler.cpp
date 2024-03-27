#include "udphandler.h"
#include <QtConcurrent/QtConcurrent>
#include <QNetworkInterface>
#include "loghandler.h"
#include "settingshandler.h"

UdpHandler::UdpHandler(QObject *parent) :
    NetworkDevice(parent),
    m_udpSocket(new QUdpSocket(this))
{
    qRegisterMetaType<ConnectionChangedSignal>();
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpHandler::onReadyRead, Qt::QueuedConnection);
}
UdpHandler::~UdpHandler() {}

void UdpHandler::init(NetworkAddress address, int waitTimeout)
{
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connecting, "Connecting..."});
    _address = address;

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
        LogHandler::Debug("IP resolved: "+m_hostAddress.toString());
    }
    tryConnectDevice(waitTimeout);
}

void UdpHandler::sendTCode(const QString &tcode)
{
    LogHandler::Debug("Sending TCode UDP: "+tcode);

    QByteArray currentRequest;
    auto formattedTcode = tcode + "\n";
    currentRequest.append(formattedTcode.toUtf8());
    m_udpSocket->writeDatagram(currentRequest, m_hostAddress, _address.port);
}

void UdpHandler::onReadyRead()
{
    QString recieved;
    while (m_udpSocket->hasPendingDatagrams())
    {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        recieved += QString::fromUtf8(datagram.data());
    }
    LogHandler::Debug("Recieved UDP: "+recieved);
    processDeviceInput(recieved);
}

void UdpHandler::dispose()
{
    LogHandler::Debug("Udp dispose "+ _address.address);
    m_udpSocket->close();
    OutputDeviceHandler::dispose();
}

