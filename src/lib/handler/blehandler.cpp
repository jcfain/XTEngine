#include "blehandler.h"

#include <QLowEnergyController>

BLEHandler::BLEHandler(QObject *parent)
    : OutputDeviceHandler{DeviceName::BLE, parent},
    m_serviceUUID(QUuid("{ff1b451d-3070-4276-9c81-5dc5ea1043bc}")),
    m_characteristicUUID(QUuid("{c5f1543e-338d-47a0-8525-01e3c621359d}"))
{
    // connect(this, &BLEHandler::connectionChange, this, [this] (const ConnectionChangedSignal signal) {
    //     if(signal.status == ConnectionStatus::Error) {
    //         dispose();
    //     }
    // });
}

void BLEHandler::init(int waitTimeout)
{
    if(m_deviceDiscoveryAgent)
        return;
    m_waitTimeout = waitTimeout;
    emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Connecting, "BLE connecting" });
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(waitTimeout);

    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &BLEHandler::foundDevice);
    connect(m_deviceDiscoveryAgent,
            static_cast<void (QBluetoothDeviceDiscoveryAgent::*)(QBluetoothDeviceDiscoveryAgent::Error)>(&QBluetoothDeviceDiscoveryAgent::error),
            this, &BLEHandler::scanError);

    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &BLEHandler::scanFinished);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &BLEHandler::scanFinished);
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void BLEHandler::sendTCode(const QString &tcode)
{
    if(m_service && m_service->state() == QLowEnergyService::ServiceState::ServiceDiscovered) {
        LogHandler::Debug("Sending TCode BLE: "+ tcode);
        QString tcodeSend = tcode + "\n";
        m_service->writeCharacteristic(m_characteristic, tcodeSend.toUtf8(), QLowEnergyService::WriteWithoutResponse);
    }
}

void BLEHandler::dispose()
{
    LogHandler::Debug("BLE dispose");
    if(m_control)
        m_control->disconnectFromDevice();
    if(m_deviceDiscoveryAgent) {
        m_deviceDiscoveryAgent->stop();
        delete m_deviceDiscoveryAgent;
        m_deviceDiscoveryAgent = 0;
        m_service = 0;
        m_control = 0;
    }
}

void BLEHandler::foundDevice(const QBluetoothDeviceInfo &info)
{
    LogHandler::Debug("BLE Device found: "+info.name() + " address: "+ info.address().toString());
    if (info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration && info.name() == m_deviceName)
    {
        m_control = QLowEnergyController::createCentral(info, m_deviceDiscoveryAgent);
        connect(m_control, &QLowEnergyController::serviceDiscovered,
                this, &BLEHandler::serviceDiscovered);
        connect(m_control, &QLowEnergyController::discoveryFinished,
                this, &BLEHandler::serviceScanDone);

        connect(m_control, static_cast<void (QLowEnergyController::*)(QLowEnergyController::Error)>(&QLowEnergyController::error),
            this, [this](QLowEnergyController::Error error) {
                Q_UNUSED(error);
                //setError("Cannot connect to remote device.");
                LogHandler::Error("BLE Error");
                emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Error, "BLE error" });
        });
        connect(m_control, &QLowEnergyController::connected, this, [this]() {
            //setInfo("Controller connected. Search services...");
            m_control->discoverServices();
        });
        connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
            //setError("LowEnergy controller disconnected");
            emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Disconnected, "BLE disconnected" });
        });

        // Connect
        m_control->connectToDevice();
    }
}

void BLEHandler::updateDevice(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields)
{
    LogHandler::Debug("updateDevice");
}

void BLEHandler::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    LogHandler::Debug("scanError");
    emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Error, "BLE error" });
}

void BLEHandler::scanFinished()
{
    LogHandler::Debug("scanFinished");
    if(!m_control) {
        emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Error, "BLE "+m_deviceName+" not found" });
    }
}

void BLEHandler::serviceDiscovered(const QBluetoothUuid &newService)
{
    if(newService == m_serviceUUID) {
        LogHandler::Debug("Service discovered!");
        // if (m_service) {
        //     delete m_service;
        //     m_service = 0;
        // }
        // emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Connected, "BLE connected" });
        m_service = m_control->createServiceObject(m_serviceUUID, m_control);

        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged, this, &BLEHandler::serviceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged, this, &BLEHandler::characteristicChanged);
            connect(m_service, &QLowEnergyService::characteristicWritten, this, &BLEHandler::characteristicChanged);
            connect(m_service, &QLowEnergyService::descriptorWritten, this, &BLEHandler::descriptorWritten);
            m_service->discoverDetails();
        } else {
            LogHandler::Error("BLE Error: TCode service not found");
            emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Error, "BLE error: TCode service not found" });
        }
    }
}

void BLEHandler::serviceScanDone()
{
    LogHandler::Debug("Service scan done!");
}

void BLEHandler::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    switch (newState) {
        case QLowEnergyService::DiscoveringServices:
            LogHandler::Debug(tr("Service State change to: Discovering services..."));
            break;
        case QLowEnergyService::ServiceDiscovered:
        {
            LogHandler::Debug(tr("Service State change to: Service discovered."));

            m_characteristic = m_service->characteristic(m_characteristicUUID);
            if (!m_characteristic.isValid()) {
                LogHandler::Error("TCode characteristic not found.");
                emit connectionChange({DeviceType::Output, DeviceName::BLE, ConnectionStatus::Error, "BLE error: TCode characteristic not found" });
            } else {
                tryConnectDevice(m_waitTimeout);
            }

            // m_notificationDesc = hrChar.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
            // if (m_notificationDesc.isValid())
            //     m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));

            break;
        }
        default:
            //nothing for now
            break;
    }
}

void BLEHandler::characteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    LogHandler::Debug("characteristicChanged: "+ QString(value));
}

void BLEHandler::characteristicRead(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    LogHandler::Debug("characteristicRead: "+ QString(value));
}

void BLEHandler::characteristicWritten(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    LogHandler::Debug("characteristicWritten: "+ QString(value));
}

void BLEHandler::descriptorWritten(const QLowEnergyDescriptor &info, const QByteArray &value)
{
    LogHandler::Debug("descriptorWritten: "+ QString(value));
}
