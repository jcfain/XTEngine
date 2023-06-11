#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QtPromise>
#include <QTimer>
#include <QBuffer>
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QtConcurrent/QtConcurrent>

#include "settingshandler.h"
#include "websockethandler.h"
#include "xtpwebhandler.h"
#include "httpServer/httpServer.h"
#include "httpServer/httpRequestHandler.h"
#include "httpServer/httpRequestRouter.h"
#include "medialibraryhandler.h"
#include "../struct/ConnectionChangedSignal.h"
#include "../tool/videoformat.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT HttpHandler : public HttpRequestHandler
{
    Q_OBJECT
signals:
    void error(QString error);
    void xtpWebPacketRecieve(QByteArray data);
    void tcode(QString tcode);
    void connectOutputDevice(DeviceName deviceName, bool checked);
    void connectInputDevice(DeviceName deviceName, bool checked);
    void restartService();
    void skipToMoneyShot();
    void skipToNextAction();

public slots:
    void on_DeviceConnection_StateChange(ConnectionChangedSignal status);

public:
    HttpHandler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent = nullptr);
    ~HttpHandler();
    bool listen();
    HttpPromise handle(HttpDataPtr data);
    HttpPromise handleAuth(HttpDataPtr data);
    HttpPromise handleActiveSessions(HttpDataPtr data);
    HttpPromise handleExpireSession(HttpDataPtr data);
    HttpPromise handleLogout(HttpDataPtr data);
    HttpPromise handleVideoStream(HttpDataPtr data);
    HttpPromise handleVideoList(HttpDataPtr data);
    HttpPromise handleThumbFile(HttpDataPtr data);
    HttpPromise handleFunscriptFile(HttpDataPtr data);
    HttpPromise handleSettings(HttpDataPtr data);
    HttpPromise handleChannels(HttpDataPtr data);
    HttpPromise handleAvailableSerialPorts(HttpDataPtr data);
    HttpPromise handleSettingsUpdate(HttpDataPtr data);
    //HttpPromise handleChannelsUpdate(HttpDataPtr data);
    HttpPromise handleDeviceConnected(HttpDataPtr data);
    HttpPromise handleConnectDevice(HttpDataPtr data);
    HttpPromise handleTCodeIn(HttpDataPtr data);
    HttpPromise handleDeo(HttpDataPtr data);
    HttpPromise handleHereSphere(HttpDataPtr data);
    HttpPromise handleWebTimeUpdate(HttpDataPtr data);


    void sendWebSocketTextMessage(QString command, QString message = nullptr);

private:
    HttpServerConfig config;
    HttpRequestRouter router;
    HttpServer* _server = 0;
    QMimeDatabase mimeDatabase;
    WebSocketHandler* _webSocketHandler = 0;
    MediaLibraryHandler* _mediaLibraryHandler = 0;
    bool _libraryLoaded = false;
    QString _libraryLoadingStatus = "Loading...";
    QMutex _mutex;
    QHash<QString, QDateTime> m_authenticated;
    QStringList m_authenticatedForMedia;
    QTimer m_sessionPolice;
    int m_sessionTimeout = 900; // 15 Min

    QJsonObject createMediaObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createDeoObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createHeresphereObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createSelectedChannels();
    void on_webSocketClient_Connected(QWebSocket* client);
    void onSetLibraryLoaded();
    void onSetLibraryLoading();
    void onLibraryLoadingStatusChange(QString message);
    bool isAuthenticated(HttpDataPtr data);

};

#endif // HTTPHANDLER_H
