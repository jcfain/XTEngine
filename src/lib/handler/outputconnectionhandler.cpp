#include "outputconnectionhandler.h"

OutputConnectionHandler::OutputConnectionHandler(ConnectionInterface deviceName, QObject *parent)
    : QObject(parent), m_deviceName(deviceName), m_newline("(\r\n|\r|\n)") {
    connect(this, &OutputConnectionHandler::sendHandShake, this, &OutputConnectionHandler::onSendHandShake);
}
void OutputConnectionHandler::dispose() {
    tryConnectStop();
    setConnected(false);
    emit connectionChange({ConnectionDirection::Output, m_deviceName,
                           ConnectionStatus::Disconnected, "Disconnected"});
}
ConnectionInterface OutputConnectionHandler::name() { return m_deviceName; }
bool OutputConnectionHandler::isConnected() {
    const QMutexLocker locker(&m_mutex);
    return m_isConnected;
}
void OutputConnectionHandler::onSendHandShake() { sendTCode("D1"); }
void OutputConnectionHandler::tryConnectStop() {
    if (m_initFuture.isRunning()) {
        m_stop = true;
        m_initFuture.cancel();
        m_initFuture.waitForFinished();
    }
}
void OutputConnectionHandler::tryConnectDevice(unsigned long waitTimeout) {
    tryConnectStop();

    m_stop = false;

    m_initFuture = QtConcurrent::run([this, waitTimeout]() {
        if (!SettingsHandler::getDisableTCodeValidation()) {
            uint8_t timeouttracker = 0;
            const uint8_t maxTries = 3;
            while (!isConnected() && !m_stop && timeouttracker <= maxTries) {
                emit sendHandShake();
                timeouttracker++;
                QThread::msleep(waitTimeout);
            }
            if (!m_stop && timeouttracker > maxTries) {
                setConnected(false);
                emit connectionChange({ConnectionDirection::Output, m_deviceName,
                                       ConnectionStatus::Error, "Timed out"});
            }
        } else {
            emit connectionChange({ConnectionDirection::Output, m_deviceName,
                                   ConnectionStatus::Connected,
                                   "Connected: No Validate"});
            m_stop = true;
            setConnected(true);
        }
    });
}
void OutputConnectionHandler::processDeviceInput(QString buffer) {
    m_readBuffer += buffer;
    if (m_readBuffer.isEmpty()) // Must end with newline char
        return;
    if (!m_readBuffer.endsWith('\n')) // Must end with newline char
        return;
    LogHandler::Debug(tr("processDeviceInput: ") + buffer);
    bool isCommand = m_readBuffer.startsWith('#') || m_readBuffer.startsWith('@');
    if (isConnected() && isCommand) {
        if (!m_readBuffer.endsWith('\n')) // Must end with newline char
            return;
        m_readBuffer.remove(m_newline);
        LogHandler::Debug(tr("Received device command: ") + m_readBuffer);
        processCommand(m_readBuffer);
        m_readBuffer.clear();
    } else if (!isConnected()) {
        if (!SettingsHandler::getDisableTCodeValidation()) {
            if (!m_readBuffer.endsWith('\n')) // Must end with newline char
                return;
            QString version = "Unknown v?";

            bool validated = false;
            auto keys = TCodeChannelLookup::SupportedTCodeVersions.keys();
            for (auto supportedversion : keys) {
                auto tcodeVersion = TCodeChannelLookup::SupportedTCodeVersions.value(supportedversion);
                if (m_readBuffer.contains(tcodeVersion)) {
                    version = tcodeVersion;
                    validated = true;
                }
            }
            if (validated) {
                LogHandler::Debug(tr("Received device validation: ") + m_readBuffer);
                setConnected(true);
                emit connectionChange({ConnectionDirection::Output, m_deviceName,
                                       ConnectionStatus::Connected, version});
            }
            m_readBuffer.clear();
        } else {
            emit connectionChange({ConnectionDirection::Output, m_deviceName,
                                   ConnectionStatus::Connected,
                                   "Connected: No Validate"});
            setConnected(true);
        }
    } else {
        LogHandler::Debug(tr("Received other data: ") + m_readBuffer);
        // emit debug log event?
        m_readBuffer.clear();
    }
}
void OutputConnectionHandler::setConnected(bool connected) {
    m_mutex.lock();
    m_isConnected = connected;
    m_mutex.unlock();
}
void OutputConnectionHandler::processCommand(QString data) {
    QString command = data;
    auto indexOfValue = data.indexOf(":");
    double value = -1;
    if (indexOfValue > -1) {
        command = data.mid(0, indexOfValue);
        bool ok;
        value = data.mid(indexOfValue + 1).toDouble(&ok);
        if (!ok) {
            value = -1;
        }
    }
    // TODO: how to tell if is another type?
    emit commandReceive({OutputDeviceCommandType::BUTTON, command, value, data});
}
