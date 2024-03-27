#ifndef OUTPUTDEVICEHANDLER_H
#define OUTPUTDEVICEHANDLER_H
#include <QTime>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QtConcurrent/QtConcurrent>
#include "settingshandler.h"
#include "loghandler.h"
#include "lib/lookup/enum.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT OutputDeviceHandler : public QObject
{
    Q_OBJECT

signals:
    void connectionChange(ConnectionChangedSignal status);
    void commandRecieve(QString command);
    void sendHandShake();

public:
    explicit OutputDeviceHandler(DeviceName deviceName, QObject *parent = nullptr) :
                                                                        QObject(parent),
                                                                        m_deviceName(deviceName),
                                                                        m_newline("(\r\n|\r|\n)")
    {
        connect(this, &OutputDeviceHandler::sendHandShake, this, &OutputDeviceHandler::onSendHandShake);
    }
    ~OutputDeviceHandler() {};
    virtual void sendTCode(const QString &tcode) = 0;
    virtual void dispose()
    {
        tryConnectStop();
        setConnected(false);
        emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Disconnected, "Disconnected"});
    };

    DeviceName name()
    {
        return m_deviceName;
    };
    bool isConnected()
    {
        const QMutexLocker locker(&m_mutex);
        return m_isConnected;
    }

protected slots:
    void onSendHandShake()
    {
        sendTCode("D1");
    };

protected:
    DeviceName m_deviceName;
    void tryConnectStop() {
        if(m_initFuture.isRunning())
        {
            m_stop = true;
            m_initFuture.cancel();
            m_initFuture.waitForFinished();
        }
    }
    void tryConnectDevice(unsigned long waitTimeout = 5000)
    {
        tryConnectStop();

        m_stop = false;

        m_initFuture = QtConcurrent::run([this, waitTimeout]() {
            if(!SettingsHandler::getDisableTCodeValidation())
            {
                uint8_t timeouttracker = 0;
                const uint8_t maxTries = 3;
                while(!isConnected() && !m_stop && timeouttracker <= maxTries)
                {
                    emit sendHandShake();
                    timeouttracker++;
                    QThread::msleep(waitTimeout);
                }
                if (!m_stop && timeouttracker > maxTries)
                {
                    setConnected(false);
                    emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Error, "Timed out"});
                }
            } else {
                emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Connected, "Connected: No Validate"});
                m_stop = true;
                setConnected(true);
            }
        });
    }

    void processDeviceInput(QString buffer)
    {
        m_readBuffer += buffer;
        if(m_readBuffer.isEmpty())// Must end with newline char
            return;
        bool isCommand = m_readBuffer.startsWith('#') || m_readBuffer.startsWith('@');
        if(isConnected() && isCommand)
        {
            if(!m_readBuffer.endsWith('\n'))// Must end with newline char
                return;
            m_readBuffer.remove(m_newline);
            LogHandler::Debug(tr("Recieved device command: ")+m_readBuffer);
            emit commandRecieve(m_readBuffer);
            m_readBuffer.clear();
        }
        else if(!isConnected())
        {
            if (!SettingsHandler::getDisableTCodeValidation())
            {
                if(!m_readBuffer.endsWith('\n'))// Must end with newline char
                    return;
                QString version = "V?";

                bool validated = false;
                if(m_readBuffer.contains(TCodeChannelLookup::SupportedTCodeVersions.value(TCodeVersion::v2)))
                {
                    version = "V2";
                    validated = true;
                }
                else if (m_readBuffer.contains(TCodeChannelLookup::SupportedTCodeVersions.value(TCodeVersion::v3)))
                {
                    version = "V3";
                    validated = true;
                }
                if (validated)
                {
                    if(!isConnected()) //temp
                    { // temp
                        setConnected(true);
                        emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Connected, "Connected: "+version});
                    } // temp
                }
                else
                {
                    //emit connectionChange({DeviceName::Serial, ConnectionStatus::Error, "No TCode"});
                    // Due to issue with connecting to some romeos with validation. Do not block them from using it.
                    setConnected(true);
                    emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Connected, "Connected: "+version});
                    LogHandler::Error("An INVALID response recieved: ");
                    LogHandler::Error("response: "+m_readBuffer);
                    //emit errorOccurred("Warning! You should be able to keep using the program if you have the correct port selected\n\nIt would be greatly appreciated if you could run the program in debug mode.\nSend the console output file to Khrull on patreon or discord. Thanks!");
                }
                // }
                // else if (currentPortNameChanged || !_isConnected)
                // {

                //     LogHandler::Error(tr("Read serial handshake timeout %1")
                //                           .arg(QTime::currentTime().toString()));
                //     // Due to issue with connecting to some romeos with validation. Do not block them from using it.
                //     emit connectionChange({DeviceType::Output, DeviceName::Serial, ConnectionStatus::Connected, "Connected: V?"});
                //     _mutex.lock();
                //     _isConnected = true;
                //     _mutex.unlock();
                // }
                LogHandler::Debug(tr("Recieved device validation: ")+m_readBuffer);
                m_readBuffer.clear();
            }
            else
            {
                emit connectionChange({DeviceType::Output, m_deviceName, ConnectionStatus::Connected, "Connected: No Validate"});
                setConnected(true);
            }
        } else {
            LogHandler::Debug(tr("Recieved other data: ")+m_readBuffer);
            // emit debug log event?
            m_readBuffer.clear();
        }
    }

    void setConnected(bool connected)
    {
        m_mutex.lock();
        m_isConnected = connected;
        m_mutex.unlock();
    }
private:
    QMutex m_mutex;
    bool m_isConnected = false;
    QString m_readBuffer;
    QRegExp m_newline;
    QFuture<void> m_initFuture;
    bool m_stop = false;
};

#endif // OUTPUTDEVICEHANDLER_H
