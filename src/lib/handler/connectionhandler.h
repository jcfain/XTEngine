#ifndef CONNECTIONHANDLER_H
#define CONNECTIONHANDLER_H

#include <QObject>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

#include "outputconnectionhandler.h"
#include "lib/interface/outputnetworkconnectionhandler.h"
#include "deohandler.h"
#include "whirligighandler.h"
#include "xtpwebhandler.h"
#include "udphandler.h"
#include "websocketdevicehandler.h"
#include "serialhandler.h"
#include "blehandler.h"
#include "gamepadhandler.h"
#include "devicehandler.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include "lib/struct/InputConnectionPacket.h"
#include "lib/struct/OutputConnectionPacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT ConnectionHandler : public QObject
{
    Q_OBJECT
signals:
    void connectionChange(ConnectionChangedSignal status);
    void inputConnectionChange(ConnectionChangedSignal status);
    void outputConnectionChange(ConnectionChangedSignal status);
    void inputMessageReceived(InputConnectionPacket packet);
    void outputMessageReceived(OutputConnectionPacket packet);
    void action(QString action);
    void serialPortFound(QString portFriendlyName, QString portName);
    void gamepadConnectionChange(ConnectionChangedSignal event);
public slots:
    void inputMessageSend(QByteArray message);
private slots:
    void on_output_connectionChanged(ConnectionChangedSignal event);
    void on_input_connectionChanged(ConnectionChangedSignal event);
    void on_gamepad_connectionChanged(ConnectionChangedSignal event);

public:
    explicit ConnectionHandler(QObject *parent = nullptr);
    void init();
    void sendTCode(QString tcode);
    void stopOutputConnection();

    bool isOutputConnected();
    bool isInputConnected();
    /** @brief Initializes the device that is stored in SettingsHandler */
    void initOutputConnection(ConnectionInterface outputConnection);
    /** @brief Initializes the device that is stored in SettingsHandler */
    void initInputConnection(ConnectionInterface inputConnection);
    void disposeOutputConnection(ConnectionInterface outputConnection);
    void disposeInputConnection(ConnectionInterface inputConnection);
    void dispose();
    OutputNetworkConnectionHandler* getNetworkHandler();
    OutputSerialConnectionHandler* getSerialHandler();
    InputHeresphereConnectionHandler* getHereSphereHandler();
    InputXTPWebConnectionHandler* getXTPWebHandler();
    InputWhirligigConnectionHandler* getWhirligigHandler();
    GamepadHandler* getGamepadHandler();
    OutputConnectionHandler* getSelectedOutputConnection();
    InputConnectionHandler* getSelectedInputConnection();

private:
    QFuture<void> _initFuture;

    OutputConnectionHandler* m_outputConnection = 0;
    InputConnectionHandler* m_inputConnection = 0;

    OutputSerialConnectionHandler* _serialHandler;
    OutputNetworkConnectionHandler* _networkConnection;
    OutputUdpConnectionHandler* _udpHandler;
    OutputBLEConnectionHandler* m_bleHandler;
    OutputWebsocketConnectionHandler* _webSocketHandler;

    InputHeresphereConnectionHandler* m_hereSphereHandler;
    InputWhirligigConnectionHandler* _whirligigHandler;
    InputXTPWebConnectionHandler* _xtpWebHandler;

    GamepadHandler* _gamepadHandler;

    ConnectionStatus m_outConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _deoConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _whirligigConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _xtpWebConnectionStatus = ConnectionStatus::Disconnected;
    ConnectionStatus _gamepadConnectionStatus = ConnectionStatus::Disconnected;

    // DeviceHandler* m_deviceHandler;

    void setOutputConnection(OutputConnectionHandler* connection);
    void setInputConnection(InputConnectionHandler* connection);

};

#endif // CONNECTIONHANDLER_H
