#include "udphandler.h"
#include <QtConcurrent/QtConcurrent>

UdpHandler::UdpHandler(QObject *parent) :
    NetworkDevice(parent)
{
    qRegisterMetaType<ConnectionChangedSignal>();
}
UdpHandler::~UdpHandler()
{
}

void UdpHandler::init(NetworkAddress address, int waitTimeout)
{
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Connecting, "Connecting..."});
    connect(this, &UdpHandler::sendHandShake, this, &UdpHandler::onSendHandShake);
    //_mutex.lock();
    _stop = false;
    _isSelected = true;
    _waitTimeout = waitTimeout;
    _address = address;
    //_mutex.unlock();
    if(_initFuture.isRunning())
    {
        _stop = true;
        _initFuture.cancel();
        _initFuture.waitForFinished();
    }
    _initFuture = QtConcurrent::run([this]() {
        int timeouttracker = 0;
        QMutex mutex;
        QWaitCondition cond;
        QElapsedTimer mSecTimer;
        qint64 time1 = 0;
        qint64 time2 = 0;
        mSecTimer.start();
        while(!_isConnected && !_stop && timeouttracker <= 3)
        {
            if (time2 - time1 >= _waitTimeout + 1000 || timeouttracker == 0)
            {
                time1 = time2;
                emit sendHandShake();
                ++timeouttracker;
            }
            time2 = (round(mSecTimer.nsecsElapsed() / 1000000));
            mutex.lock();
            cond.wait(&mutex, 1);
            mutex.unlock();
        }
        if (!_stop && timeouttracker > 3)
        {
            _stop = true;
            _isConnected = false;
            emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "Timed out"});
        }
    });
}

void UdpHandler::onSendHandShake() {
    sendTCode("D1");
}

void UdpHandler::sendTCode(const QString &tcode)
{
    const QMutexLocker locker(&_mutex);
    _tcode = tcode + "\n";
    LogHandler::Debug("Sending TCode UDP: "+tcode);
    if (!isRunning())
        start();
    else
        _cond.wakeOne();
}

void UdpHandler::run()
{
    bool currentAddressChanged = false;

    _mutex.lock();
    _tcode = "";
    QByteArray currentRequest;
    currentRequest.append(_tcode);

    QString currentAddress;
    int currentPort = 0;
    QHostAddress addressObj;
    addressObj.setAddress(_address.address);
    currentAddress = _address.address;
    currentPort = _address.port;
    _isConnected = false;
    currentAddressChanged = true;

    _mutex.unlock();

    QScopedPointer<QUdpSocket> udpSocketSend(new QUdpSocket(this));

    //QScopedPointer<QUdpSocket> udpSocketRecieve(new QUdpSocket(this));
    //if (!udpSocketSend->bind(QHostAddress::Any, 54000))
    //{
        //emit connectionChange({DeviceName::Network, ConnectionStatus::Error, "Error opening handshake"});
    //}
    while (!_stop)
    {
        if (currentAddressChanged)
        {
            udpSocketSend->connectToHost(currentAddress, currentPort);
            if(!udpSocketSend->waitForConnected(_waitTimeout))
            {
                emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "Can't connect"});
            }
        }
        if(udpSocketSend->isWritable())
        {
            udpSocketSend->write(currentRequest);
            if (!_isConnected)
            {
                QString version = "V?";
                if(!SettingsHandler::getDisableTCodeValidation())
                {
                    if(udpSocketSend->waitForReadyRead(_waitTimeout))
                    {
                        QNetworkDatagram datagram = udpSocketSend->receiveDatagram();

                        QString response = QString(datagram.data());
                        bool validated = false;
                        if(response.contains(SettingsHandler::SupportedTCodeVersions.value(TCodeVersion::v2)))
                        {
                            version = "V2";
                            validated = true;
                        }
                        else if (response.contains(SettingsHandler::SupportedTCodeVersions.value(TCodeVersion::v3)))
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
                            emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Error, "No " + SettingsHandler::getSelectedTCodeVersion()});
                        }
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


        if (!_stop)
        {
            _mutex.lock();
            _cond.wait(&_mutex);
            if (currentAddress != _address.address || currentPort != _address.port)
            {
                currentAddress = _address.address;
                addressObj.setAddress(_address.address);
                currentPort = _address.port;
                currentAddressChanged = true;
                _isConnected = false;
            }
            else
            {
                currentAddressChanged = false;
            }

            currentRequest.clear();
            currentRequest.append(_tcode.toUtf8());
            _mutex.unlock();
        }
    }
}

void UdpHandler::dispose()
{
    LogHandler::Debug("Udp dispose "+ _address.address);
    _mutex.lock();
    _isConnected = false;
    _stop = true;
    _mutex.unlock();
    _cond.wakeOne();
    emit connectionChange({DeviceType::Output, DeviceName::Network, ConnectionStatus::Disconnected, "Disconnected"});
    if(isRunning())
    {
        quit();
        wait();
    }
}

bool UdpHandler::isConnected()
{
    QMutexLocker locker(&_mutex);
    return _isConnected;
}

