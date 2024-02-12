#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QHttpServer>
#include <QTimer>
#include <QBuffer>
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include <QtConcurrent/QtConcurrent>

#include "websockethandler.h"
#include "medialibraryhandler.h"
#include "../struct/ConnectionChangedSignal.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT HttpHandler : QObject
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
    // QHttpServerResponse handle(const QHttpServerRequest& request);
    // QHttpServerResponse handleAuth(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleActiveSessions(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleExpireSession(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleLogout(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleVideoStream(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleVideoList(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleThumbFile(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleFunscriptFile(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleSettings(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleChannels(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleAvailableSerialPorts(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleSettingsUpdate(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleMediaItemMetadataUpdate(const QHttpServerRequest& request);

    // //HttpPromise handleChannelsUpdate(HttpDataPtr data);
    // QFuture<QHttpServerResponse> handleDeviceConnected(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleConnectDevice(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleTCodeIn(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleDeo(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleHereSphere(const QHttpServerRequest& request);
    // QFuture<QHttpServerResponse> handleWebTimeUpdate(const QHttpServerRequest& request);


    void sendWebSocketTextMessage(QString command, QString message = nullptr);

private:
    QHttpServer* _server = 0;
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
    bool isAuthenticated(const QHttpServerRequest& request);

};

#endif // HTTPHANDLER_H
