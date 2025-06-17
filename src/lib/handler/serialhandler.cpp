#include "serialhandler.h"

SerialHandler::SerialHandler(QObject *parent) :
    OutputDeviceHandler(DeviceName::Serial, parent),
    m_serial(new QSerialPort(this))
{
    qRegisterMetaType<ConnectionChangedSignal>();
    connect(m_serial, &QSerialPort::readyRead, this, &SerialHandler::onReadyRead, Qt::QueuedConnection);
}

SerialHandler::~SerialHandler() {
}

void SerialHandler::init(const QString &portNameOrFriendlyName, int waitTimeout)
{
    auto available = getPorts();
    LogHandler::Debug("Init port: "+ portNameOrFriendlyName);
    LogHandler::Debug("Availible ports length: "+ QString::number(available.length()));
    foreach(SerialComboboxItem port, available)
    {
        LogHandler::Debug("Port: "+ port.portName);
        if(!portNameOrFriendlyName.isEmpty() && (portNameOrFriendlyName == port.friendlyName || portNameOrFriendlyName == port.portName)) {
            _portName = port.portName;
            break;
        } else if(portNameOrFriendlyName.isEmpty()) {
            if(port.friendlyName.toLower().contains("arduino")
                || port.friendlyName.toLower().contains("cp210x")
                || port.friendlyName.toLower().contains("ch340"))
            {
                _portName = port.portName;
                SettingsHandler::setSerialPort(_portName);
                break;
            }
        }
    }
    if(_portName.isEmpty() && available.count() > 0)
    {
        emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Disconnected,  portNameOrFriendlyName.isEmpty() ? "No existing ports found with Arduino, CP210x or ch340 in the name. Select a port from the settings menu.": portNameOrFriendlyName+ " not found in available ports. Select a new port from the settings menu."});
        return;
    }
    else if(_portName.isEmpty() || available.length() == 0)
    {
        //LogHandler::Dialog("No portname specified", XLogLevel::Critical);
        emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Disconnected, "No COM"});
        return;
    }
    emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Connecting, "Connecting..."});


    LogHandler::Debug("Connecting to: "+ _portName);
    if(m_serial->isOpen())
        m_serial->close();

    LogHandler::Debug("Setting port params: ");
    m_serial->setPortName(_portName);
    // m_serial->setDataBits(QSerialPort::Data8);
    // // LogHandler::Debug("NoParity");
    // m_serial->setParity(QSerialPort::NoParity);
    // // LogHandler::Debug("OneStop");
    // m_serial->setStopBits(QSerialPort::OneStop);
    // // LogHandler::Debug("NoFlowControl");
    // m_serial->setFlowControl(QSerialPort::NoFlowControl);
    // LogHandler::Debug("Opening port");
    if (!m_serial->open(QIODevice::ReadWrite))
    {
        LogHandler::Error("Error opening: "+ _portName + ", Error: "+m_serial->errorString());
        emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Error, tr("Can't open %1, error %2")
                                                                                                    .arg(_portName).arg(m_serial->errorString())});
        return;
    }
    if(!m_serial->setBaudRate(QSerialPort::BaudRate::Baud115200)) {
        emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Error, tr("Error setting  %1 baud:\n115200, error %2")
                                                                                                    .arg(_portName).arg(m_serial->errorString())});
        m_serial->close();
        return;
    }

    if(SettingsHandler::getUseDTRAndRTS())
    {
        LogHandler::Debug("setRequestToSend");
         m_serial->setRequestToSend(true);
        LogHandler::Debug("setDataTerminalReady");
        m_serial->setDataTerminalReady(true);
    }

    tryConnectDevice(waitTimeout);
}


void SerialHandler::sendTCode(const QString &tcode)
{
    LogHandler::Debug("Sending TCode serial: "+ tcode);
    if(m_serial->isOpen())
    {
        QString tcodeOut = tcode + "\n";
        m_serial->write(tcodeOut.toUtf8());
        m_serial->flush();
    }
}

void SerialHandler::dispose()
{
    LogHandler::Debug("Serial dispose "+ _portName);
    m_serial->close();
    OutputDeviceHandler::dispose();
}

void SerialHandler::onReadyRead()
{
    QByteArray responseData = m_serial->readAll();
    while (m_serial->bytesAvailable())
        responseData += m_serial->readAll();
    processDeviceInput(QString::fromUtf8(responseData));
}

QList<SerialComboboxItem> SerialHandler::getPorts()
{
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    LogHandler::Debug("QSerialPortInfo::availablePorts() length: "+ QString::number(serialPortInfos.length()));
    QList<SerialComboboxItem> availablePorts;
    for (const QSerialPortInfo &serialPortInfo : serialPortInfos) {
        QString friendlyName = serialPortInfo.portName() + " " + serialPortInfo.description() ;
        QString portName = serialPortInfo.portName();
        if (!friendlyName.isEmpty() && !portName.isEmpty())
        {
            availablePorts.push_back({friendlyName, portName});
        }
        else if (!portName.isEmpty())
        {
            availablePorts.push_back({portName, portName});
        }
    }
    return availablePorts;

}
