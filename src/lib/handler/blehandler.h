#ifndef BLEHANDLER_H
#define BLEHANDLER_H

#include "outputdevicehandler.h"

#include <QBluetoothDeviceDiscoveryAgent>
#include <QLowEnergyController>

class BLEHandler : public OutputDeviceHandler
{
    Q_OBJECT
public:
    explicit BLEHandler(QObject *parent = nullptr);
    void init(int waitTimeout = 5000);
    void sendTCode(const QString &tcode) override;
    void dispose() override;
private:
    int m_waitTimeout;
    const QString m_deviceName = "TCODE-ESP32";
    QBluetoothDeviceDiscoveryAgent * m_deviceDiscoveryAgent = 0;
    QLowEnergyController* m_control = 0;
    QLowEnergyService* m_service = 0;
    QLowEnergyCharacteristic m_characteristic;
    const QBluetoothUuid m_serviceUUID;
    const QBluetoothUuid m_characteristicUUID;

private slots:
    void foundDevice(const QBluetoothDeviceInfo &info);
    void updateDevice(const QBluetoothDeviceInfo &info, QBluetoothDeviceInfo::Fields updatedFields);
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void scanFinished();
    void serviceDiscovered(const QBluetoothUuid &newService);
    void serviceScanDone();
    void serviceStateChanged(QLowEnergyService::ServiceState newState);
    void characteristicChanged(const QLowEnergyCharacteristic &info,
                               const QByteArray &value);
    void characteristicRead(const QLowEnergyCharacteristic &info,
                            const QByteArray &value);
    void characteristicWritten(const QLowEnergyCharacteristic &info,
                               const QByteArray &value);
    void descriptorWritten(const QLowEnergyDescriptor &info,
                           const QByteArray &value);
};

#endif // BLEHANDLER_H
