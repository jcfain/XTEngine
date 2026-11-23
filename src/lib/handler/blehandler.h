#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include "outputconnectionhandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>

class OutputBLEConnectionHandler : public OutputConnectionHandler
{
    Q_OBJECT
public:
    explicit OutputBLEConnectionHandler(QObject *parent = nullptr);
    void init(int waitTimeout = 5000);
    void sendTCode(const QString &tcode) override;
    void dispose() override;

private:
    int m_waitTimeout;
    int m_cccRetryCount = 0;
    bool m_isRetrying = false;
    static constexpr int kMaxCccRetries = 6;
    // Advertised name of the TCode peripheral. NOTE: on some platforms
    // (notably Windows) the advertised name is empty during scanning, so this
    // is only a fast-path hint -- the service UUID is the authoritative match.
    const QString m_bleDeviceName = "TCODE-ESP32";
    QBluetoothDeviceDiscoveryAgent * m_deviceDiscoveryAgent = nullptr;
    QLowEnergyController* m_control = nullptr;
    QLowEnergyService* m_service = nullptr;
    QBluetoothDeviceInfo m_currentDevice;
    QLowEnergyCharacteristic m_characteristic;
    QLowEnergyDescriptor m_notificationDesc;
    const QBluetoothUuid m_serviceUUID;
    const QBluetoothUuid m_characteristicUUID;
    QList<QBluetoothDeviceInfo> m_candidates;

    void tryConnectNext();
    void cleanupController();
    void connectToCandidate(const QBluetoothDeviceInfo &info);

private slots:
    void foundDevice(const QBluetoothDeviceInfo &info);
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void scanFinished();
    void serviceDiscovered(const QBluetoothUuid &newService);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void controllerError(QLowEnergyController::Error error);
    void characteristicChanged(const QLowEnergyCharacteristic &info,
                               const QByteArray &value);
    void characteristicWritten(const QLowEnergyCharacteristic &info,
                               const QByteArray &value);
    void descriptorWritten(const QLowEnergyDescriptor &info,
                           const QByteArray &value);
};

#endif // BLEHANDLER_H
