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
    HttpPromise handleVideoStream(HttpDataPtr data);
    HttpPromise handleVideoList(HttpDataPtr data);
    HttpPromise handleThumbFile(HttpDataPtr data);
    HttpPromise handleFunscriptFile(HttpDataPtr data);
    HttpPromise handleSettings(HttpDataPtr data);
    HttpPromise handleAvailableSerialPorts(HttpDataPtr data);
    HttpPromise handleSettingsUpdate(HttpDataPtr data);
    HttpPromise handleDeviceConnected(HttpDataPtr data);
    HttpPromise handleConnectDevice(HttpDataPtr data);
    HttpPromise handleTCodeIn(HttpDataPtr data);
    HttpPromise handleDeo(HttpDataPtr data);
    HttpPromise handleWebTimeUpdate(HttpDataPtr data);

    void sendWebSocketTextMessage(QString command, QString message = nullptr);

private:
    HttpServerConfig config;
    HttpRequestRouter router;
    HttpServer* _server;
    QMimeDatabase mimeDatabase;
    WebSocketHandler* _webSocketHandler;
    MediaLibraryHandler* _mediaLibraryHandler;
    bool _libraryLoaded = false;
    QString _libraryLoadingStatus = "Loading...";
    QMutex _mutex;

    QJsonObject createMediaObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createDeoObject(LibraryListItem27 libraryListItem, QString hostAddress);
    void on_webSocketClient_Connected(QWebSocket* client);
    void onSetLibraryLoaded();
    void onSetLibraryLoading();
    void onLibraryLoadingStatusChange(QString message);

};

#endif // HTTPHANDLER_H
