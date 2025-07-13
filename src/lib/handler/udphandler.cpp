#include "udphandler.h"
#include <QtConcurrent/QtConcurrent>
#include <QNetworkInterface>
#include "loghandler.h"
#include "../tool/xnetwork.h"
#include "settingshandler.h"

OutputUdpConnectionHandler::OutputUdpConnectionHandler(QObject *parent) :
    OutputNetworkConnectionHandler(parent),
    m_udpSocket(new QUdpSocket(this))
{
    qRegisterMetaType<ConnectionChangedSignal>();
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &OutputUdpConnectionHandler::onReadyRead, Qt::QueuedConnection);
    connect(this, &OutputUdpConnectionHandler::connectionChange, this, [this](ConnectionChangedSignal signal) {
        if(signal.status == ConnectionStatus::Connected) {
            onConnected();
        } else if(signal.status == ConnectionStatus::Disconnected) {
            // Do NOT call dispose! it is called from OutputDeviceHandler::dispose();
            // Infinitloop danger
        }
    });
    connect(&m_heartBeatTimer, &QTimer::timeout, this, &OutputUdpConnectionHandler::sendHeartbeat, Qt::QueuedConnection);
    connect(this, &OutputUdpConnectionHandler::disposeMe, this, &OutputUdpConnectionHandler::dispose);
}
OutputUdpConnectionHandler::~OutputUdpConnectionHandler() {}

void OutputUdpConnectionHandler::init(NetworkAddress address, int waitTimeout)
{
    emit connectionChange({ConnectionDirection::Output, ConnectionInterface::Network, ConnectionStatus::Connecting, "Connecting..."});
    _address = address;

    m_hostAddress = QHostAddress(_address.address);
    if (QAbstractSocket::IPv4Protocol == m_hostAddress.protocol() || QAbstractSocket::IPv6Protocol == m_hostAddress.protocol())
    {
        LogHandler::Debug("IP address passed in to UDP: "+_address.address);
    }
    else
    {
        LogHandler::Debug("Hostname passed in to UDP: "+_address.address);
        QHostInfo info = QHostInfo::fromName(_address.address);
        if(info.error() != QHostInfo::NoError) {
            emit connectionChange({ConnectionDirection::Output, ConnectionInterface::Network, ConnectionStatus::Error, info.errorString()});
            return;
        }
        if (!info.addresses().isEmpty()) {
            m_hostAddress = QHostAddress(info.addresses().constFirst());
        } else {
            emit connectionChange({ConnectionDirection::Output, ConnectionInterface::Network, ConnectionStatus::Error, "No IP for: " + _address.address + " found"});
            return;
        }
        LogHandler::Debug("IP resolved: "+m_hostAddress.toString());
    }

    tryConnectDevice(waitTimeout);

}

void OutputUdpConnectionHandler::sendTCode(const QString &tcode)
{
    LogHandler::Debug("Sending TCode UDP: "+tcode);
    QMutexLocker locker(&m_socketMutex);
    QByteArray currentRequest;
    auto formattedTcode = tcode + "\n";
    currentRequest.append(formattedTcode.toUtf8());
    m_udpSocket->writeDatagram(currentRequest, m_hostAddress, _address.port);
    //m_udpSocket->write(currentRequest);
}

void OutputUdpConnectionHandler::onReadyRead()
{
    QString recieved;
    while (m_udpSocket->waitForReadyRead(100))
    {
        QNetworkDatagram datagram = m_udpSocket->receiveDatagram();
        // auto senderAddress = datagram.senderAddress();
        // if(!hasAddress(senderAddress))
        //     m_connectedHosts.append(senderAddress);
        if(datagram.isValid()) {
            recieved += QString::fromUtf8(datagram.data());
        } else {
            LogHandler::Warn("Bad datagram");
        }
    }
    LogHandler::Debug("Recieved UDP: "+recieved);
    processDeviceInput(recieved);
}

void OutputUdpConnectionHandler::onConnected()
{
    if(!hasAddress(m_hostAddress))
        m_connectedHosts.append(m_hostAddress);
    if(!SettingsHandler::getDisableHeartBeat())
    {
        sendHeartbeat();
        m_heartBeatTimer.start(30000);
    }
}

void OutputUdpConnectionHandler::sendHeartbeat()
{
    if(!SettingsHandler::getDisableHeartBeat())
    {
        QtConcurrent::run([this]() {
            LogHandler::Debug("UDP heart beat check");
            int replyTimeInMS = 0;
            QList<QHostAddress> hostsToremove;
            foreach (QHostAddress address, m_connectedHosts) {
                if (XNetwork::ping(address, replyTimeInMS)) {
                    LogHandler::Debug("UDP address detected!");
                    int roundTrip = replyTimeInMS * 2;
                    if(SettingsHandler::isSmartOffSet())
                        LogHandler::Debug("Adjusting live offset by: "+QString::number(roundTrip) + "ms");
                    SettingsHandler::setSmartOffset(roundTrip);
                } else {
                    LogHandler::Debug("UDP heart beat check failed!");
                    hostsToremove.append(address);
                }
            }
            foreach (QHostAddress address, hostsToremove) {
                m_connectedHosts.removeAll(address);
            }
            if(m_connectedHosts.empty())
            {
                emit disposeMe();
                return;
            }
        });
    }
}

bool OutputUdpConnectionHandler::hasAddress(const QHostAddress &addressIn)
{
    auto it = std::find_if(m_connectedHosts.begin(), m_connectedHosts.end(),
                           [addressIn](const QHostAddress &address) {
                               return addressIn == address;
                           });
    return it != m_connectedHosts.end();
}

void OutputUdpConnectionHandler::dispose()
{
    LogHandler::Debug("Udp dispose "+ _address.address);
    m_udpSocket->close();
    m_heartBeatTimer.stop();
    OutputConnectionHandler::dispose();
}

