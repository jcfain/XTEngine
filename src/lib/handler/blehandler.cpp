#include "blehandler.h"

#include <QLowEnergyController>
#include <QTimer>

OutputBLEConnectionHandler::OutputBLEConnectionHandler(QObject *parent)
    : OutputConnectionHandler{ConnectionInterface::BLE, parent},
    m_serviceUUID(QUuid("{ff1b451d-3070-4276-9c81-5dc5ea1043bc}")),
    m_characteristicUUID(QUuid("{c5f1543e-338d-47a0-8525-01e3c621359d}"))
{
}

void OutputBLEConnectionHandler::init(int waitTimeout)
{
    if(m_deviceDiscoveryAgent)
        return;
    m_waitTimeout = waitTimeout;
    emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Connecting, tr("BLE connecting") });
    m_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
    m_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(waitTimeout);

    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered, this, &OutputBLEConnectionHandler::foundDevice);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred, this, &OutputBLEConnectionHandler::scanError);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished, this, &OutputBLEConnectionHandler::scanFinished);
    connect(m_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled, this, &OutputBLEConnectionHandler::scanFinished);
    m_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}

void OutputBLEConnectionHandler::sendTCode(const QString &tcode)
{
    if(m_service && m_service->state() == QLowEnergyService::ServiceState::RemoteServiceDiscovered) {
        LogHandler::Debug(tr("Sending TCode BLE: ")+ tcode);
        QString tcodeSend = tcode + "\n";
        m_service->writeCharacteristic(m_characteristic, tcodeSend.toUtf8(), QLowEnergyService::WriteWithoutResponse);
    }
}

void OutputBLEConnectionHandler::dispose()
{
    LogHandler::Debug(tr("BLE dispose"));
    if(m_deviceDiscoveryAgent) {
        // Detach signals first so a late `canceled` from stop() can't kick off
        // a new connection attempt while we're tearing down.
        disconnect(m_deviceDiscoveryAgent, nullptr, this, nullptr);
        m_deviceDiscoveryAgent->stop();
        delete m_deviceDiscoveryAgent;
        m_deviceDiscoveryAgent = nullptr;
    }
    cleanupController();
    m_candidates.clear();
}

void OutputBLEConnectionHandler::foundDevice(const QBluetoothDeviceInfo &info)
{
    if (!(info.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
        return;
    const bool advertisesTCode = info.serviceUuids().contains(m_serviceUUID);
    LogHandler::Debug(tr("BLE Device found: ")+info.name() + tr(" address: ")+ info.address().toString() + tr(" tcodeService: ")+(advertisesTCode ? tr("yes") : tr("no")));
    if (!m_candidates.contains(info))
        m_candidates.append(info);
}

void OutputBLEConnectionHandler::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    Q_UNUSED(error);
    LogHandler::Debug("scanError");
    emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Error, tr("BLE error") });
}

void OutputBLEConnectionHandler::scanFinished()
{
    LogHandler::Debug("scanFinished");
    if (m_control)
        return; // already connecting

    // Order candidates by confidence so we connect to the right device first and
    // only probe (connect + verify by service) as a last resort:
    //   1. Advertises the TCode service UUID (most reliable -- the firmware does this).
    //   2. Advertised name matches TCODE-ESP32.
    //   3. Everything else (probed and verified in serviceDiscovered/serviceScanDone).
    // This matters on Windows, where the advertised name is often empty.
    QList<QBluetoothDeviceInfo> ordered;
    for (const auto &dev : m_candidates)
        if (dev.serviceUuids().contains(m_serviceUUID))
            ordered.append(dev);
    for (const auto &dev : m_candidates)
        if (!dev.serviceUuids().contains(m_serviceUUID) && dev.name() == m_bleDeviceName)
            ordered.append(dev);
    for (const auto &dev : m_candidates)
        if (!dev.serviceUuids().contains(m_serviceUUID) && dev.name() != m_bleDeviceName)
            ordered.append(dev);
    m_candidates = ordered;

    tryConnectNext();
}

void OutputBLEConnectionHandler::tryConnectNext()
{
    if (m_control)
        return;
    if (m_candidates.isEmpty()) {
        emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Error, tr("BLE ")+m_bleDeviceName+tr(" not found") });
        return;
    }

    QBluetoothDeviceInfo info = m_candidates.takeFirst();
    connectToCandidate(info);
}

void OutputBLEConnectionHandler::connectToCandidate(const QBluetoothDeviceInfo &info)
{
    // Always start from a clean slate: a QLowEnergyController that has been
    // disconnected (e.g. by our CCC re-discovery retry) can be left in a bad state
    // where connectToDevice() never completes, so we create a fresh controller.
    cleanupController();
    m_currentDevice = info;
    if (!m_isRetrying) {
        m_cccRetryCount = 0;
        m_isRetrying = false;
    }
    LogHandler::Debug(tr("BLE connecting to candidate: name='")+info.name()+tr("' address=")+info.address().toString());

    m_control = QLowEnergyController::createCentral(info, this);
    connect(m_control, &QLowEnergyController::serviceDiscovered,
            this, &OutputBLEConnectionHandler::serviceDiscovered);
    connect(m_control, &QLowEnergyController::discoveryFinished,
            this, &OutputBLEConnectionHandler::serviceScanDone);
    connect(m_control, &QLowEnergyController::errorOccurred,
            this, &OutputBLEConnectionHandler::controllerError);
    connect(m_control, &QLowEnergyController::connected, this, [this]() {
        m_control->discoverServices();
    });
    connect(m_control, &QLowEnergyController::disconnected, this, [this]() {
        // If we deliberately disconnected to force a GATT re-discovery (BlueZ didn't
        // expose the CCC), reconnect with a fresh controller. Otherwise report a real
        // disconnect.
        if (m_cccRetryCount > 0 && m_cccRetryCount < kMaxCccRetries) {
            LogHandler::Debug("BLE controller disconnected; reconnecting for re-discovery");
            QTimer::singleShot(500, this, [this]() {
                if (m_currentDevice.isValid())
                    connectToCandidate(m_currentDevice);
            });
        } else if (m_service) {
            emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Disconnected, tr("BLE disconnected") });
        }
    });

    m_control->connectToDevice();
}

void OutputBLEConnectionHandler::serviceDiscovered(const QBluetoothUuid &newService)
{
    if(newService == m_serviceUUID) {
        LogHandler::Debug("Service discovered!");
        m_service = m_control->createServiceObject(m_serviceUUID, m_control);

        if (m_service) {
            connect(m_service, &QLowEnergyService::stateChanged, this, &OutputBLEConnectionHandler::serviceStateChanged);
            connect(m_service, &QLowEnergyService::characteristicChanged, this, &OutputBLEConnectionHandler::characteristicChanged);
            connect(m_service, &QLowEnergyService::characteristicWritten, this, &OutputBLEConnectionHandler::characteristicWritten);
            connect(m_service, &QLowEnergyService::descriptorWritten, this, &OutputBLEConnectionHandler::descriptorWritten);
            m_service->discoverDetails();
        } else {
            LogHandler::Error("BLE Error: TCode service not found");
            emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Error, tr("BLE error: TCode service not found") });
        }
    }
}

void OutputBLEConnectionHandler::serviceScanDone()
{
    LogHandler::Debug("Service scan done!");
    if (!m_service) {
        // This device did not expose the TCode service; skip it and try the next candidate.
        LogHandler::Debug("TCode service not present on this device; trying next candidate.");
        cleanupController();
        tryConnectNext();
    }
}

void OutputBLEConnectionHandler::controllerError(QLowEnergyController::Error error)
{
    Q_UNUSED(error);
    LogHandler::Debug("BLE controller error");
    if (m_isRetrying && m_cccRetryCount < kMaxCccRetries) {
        // We're in a CCC re-discovery retry; retry the same device with a fresh controller.
        cleanupController();
        QTimer::singleShot(500, this, [this]() {
            if (m_currentDevice.isValid())
                connectToCandidate(m_currentDevice);
        });
    } else if (!m_service) {
        // A candidate failed to connect (busy/out of range/etc.); move on to the next.
        cleanupController();
        tryConnectNext();
    }
}

void OutputBLEConnectionHandler::serviceStateChanged(QLowEnergyService::ServiceState newState)
{
    switch (newState) {
        case QLowEnergyService::RemoteServiceDiscovering:
            LogHandler::Debug(tr("Service State change to: Discovering services..."));
            break;
        case QLowEnergyService::RemoteServiceDiscovered:
        {
            LogHandler::Debug(tr("Service State change to: Service discovered."));

            m_characteristic = m_service->characteristic(m_characteristicUUID);
            if (!m_characteristic.isValid()) {
                LogHandler::Error("TCode characteristic not found.");
                emit connectionChange({ConnectionDirection::Output, ConnectionInterface::BLE, ConnectionStatus::Error, tr("BLE error: TCode characteristic not found") });
                break;
            }

            // The firmware replies to commands via BLE notifications, so we must
            // subscribe to them by writing 0x0100 to the CCC descriptor. Without
            // this, characteristicChanged() never fires and the handshake can't complete.
            m_notificationDesc = m_characteristic.descriptor(QBluetoothUuid(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration));
            if (!m_notificationDesc.isValid())
                m_notificationDesc = m_characteristic.clientCharacteristicConfiguration();

            // Notifications (via the CCC descriptor) are only needed to receive the
            // D1 version reply when TCode validation is enabled. When validation is
            // disabled we don't need notifications at all, so skip the CCC entirely.
            // This also sidesteps the BlueZ descriptor-discovery bug on Linux, where
            // the CCC is never exposed and the retry loop would just waste ~15s.
            if (m_notificationDesc.isValid() || SettingsHandler::getDisableTCodeValidation()) {
                if (m_notificationDesc.isValid()) {
                    LogHandler::Debug(tr("Enabling BLE notifications"));
                    m_service->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
                }
                m_isRetrying = false;
                tryConnectDevice(m_waitTimeout);
            } else if (m_cccRetryCount < kMaxCccRetries) {
                // BlueZ often exposes the characteristic before its descriptors are
                // populated in the managed-object tree, so discoverDetails() can return
                // an empty descriptor list even though the CCC exists on the device.
                // discoverDetails() is a no-op once the service is RemoteServiceDiscovered,
                // so force a real re-discovery by disconnecting and reconnecting the
                // controller (the disconnected handler re-connects it).
                ++m_cccRetryCount;
                m_isRetrying = true;
                LogHandler::Debug(tr("CCC not found (attempt %1/%2); reconnecting to re-discover...")
                    .arg(m_cccRetryCount).arg(kMaxCccRetries));
                QTimer::singleShot(250, this, [this]() {
                    if (m_control)
                        m_control->disconnectFromDevice();
                });
            } else {
                LogHandler::Debug(tr("BLE characteristic has no CCC descriptor after %1 attempts; notifications may not be available").arg(kMaxCccRetries));
                m_isRetrying = false;
                tryConnectDevice(m_waitTimeout);
            }
            break;
        }
        default:
            //nothing for now
            break;
    }
}

void OutputBLEConnectionHandler::cleanupController()
{
    if (m_service) {
        m_service->disconnect();
        m_service->deleteLater();
        m_service = nullptr;
    }
    if (m_control) {
        m_control->disconnectFromDevice();
        m_control->disconnect();
        m_control->deleteLater();
        m_control = nullptr;
    }
    m_characteristic = QLowEnergyCharacteristic();
    m_notificationDesc = QLowEnergyDescriptor();
}

void OutputBLEConnectionHandler::characteristicChanged(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    Q_UNUSED(info);
    LogHandler::Debug("characteristicChanged: "+ QString::fromUtf8(value));
    // Feed device responses into the shared validation pipeline (same as serial/udp),
    // so the handshake/version check in tryConnectDevice() can succeed.
    processDeviceInput(QString::fromUtf8(value));
}

void OutputBLEConnectionHandler::characteristicWritten(const QLowEnergyCharacteristic &info, const QByteArray &value)
{
    Q_UNUSED(info);
    LogHandler::Debug("characteristicWritten: "+ QString::fromUtf8(value));
}

void OutputBLEConnectionHandler::descriptorWritten(const QLowEnergyDescriptor &info, const QByteArray &value)
{
    Q_UNUSED(info);
    LogHandler::Debug("descriptorWritten: "+ QString::fromUtf8(value));
}
