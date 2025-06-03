#ifndef HTTPHANDLER_H
#define HTTPHANDLER_H

#include <QHttpServer>
#include <QTcpServer>
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
#include "../lookup/AxisType.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT HttpHandler : public QObject
{
    Q_OBJECT
signals:
    void error(QString error);
    void settingChange(QString settingName, QVariant value);
    void mediaAction(QString action);
    void actionExecuted(QString action, QString spokenText, QVariant value);
    void xtpWebPacketRecieve(QByteArray data);
    void tcode(QString tcode);
    void connectOutputDevice(DeviceName deviceName, bool checked);
    void connectInputDevice(DeviceName deviceName, bool checked);
    void restartService();
    void cleanupThumbs();
    void skipToMoneyShot();
    void skipToNextAction();
    void channelPositionChange(QString channel, int position, int time, ChannelTimeType timeType);
    void scriptTogglePaused(bool paused);
    void swapScript(ScriptInfo script);
    void clean1024();

public slots:
    void on_DeviceConnection_StateChange(ConnectionChangedSignal status);
    void stopAllMedia();
    void onSettingChange(QString settingName, QVariant value);

public:
    HttpHandler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent = nullptr);
    ~HttpHandler();
    bool listen();
    void handleRoot(const QHttpServerRequest &req, QHttpServerResponder &responder);
    QHttpServerResponse handleFile(const QUrl &url, const QHttpServerRequest &request);
    void handleAuth(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleActiveSessions(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleExpireSession(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleLogout(const QHttpServerRequest &req, QHttpServerResponder &responder);
    QFuture<QHttpServerResponse> handleVideoStream(const QHttpServerRequest &req);
    void handleVideoList(const QHttpServerRequest &req, QHttpServerResponder &responder);
    QHttpServerResponse handleThumbFile(const QHttpServerRequest &req);
    void handleFunscriptFile(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleSettings(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleChannels(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleAvailableSerialPorts(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleMediaActions(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleSettingsUpdate(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleMediaItemMetadataUpdate(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleSubtitle(const QHttpServerRequest &req, QHttpServerResponder &responder);

    void handleChannelsUpdate(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleDeviceConnected(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleConnectDevice(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleTCodeIn(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleDeo(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleHereSphere(const QHttpServerRequest &req, QHttpServerResponder &responder);
    void handleWebTimeUpdate(const QHttpServerRequest &req, QHttpServerResponder &responder);


    void sendWebSocketTextMessage(QString command, QString message = nullptr);

private:
    QHttpServer* _server = 0;
    QMimeDatabase mimeDatabase;
    WebSocketHandler* _webSocketHandler = 0;
    MediaLibraryHandler* _mediaLibraryHandler = 0;
    bool _libraryLoaded = false;
    QString _libraryLoadingStatus = "Loading...";
    // QMutex _mutex;
    QMutex m_fileMutex;
    QHash<QString, QDateTime> m_authenticated;
    QStringList m_authenticatedForMedia;
    QTimer m_sessionPolice;
    int m_sessionTimeout = 900; // 15 Min
    QString m_hostAddress;
    QFuture<QHttpServerResponse> m_hlsFuture;

    QJsonObject createMediaObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createDeoObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createHeresphereObject(LibraryListItem27 libraryListItem, QString hostAddress);
    QJsonObject createSelectedChannels();
    QJsonDocument createError(QString message);
    void on_webSocketClient_Connected(QWebSocket* client);
    void onSetLibraryLoaded();
    void onSetLibraryLoading();
    void onLibraryLoadingStatusChange(QString message);
    bool isAuthenticated(const QHttpServerRequest &req);
    QString getURL(const QHttpServerRequest &request);
    QString getCookie(const QHttpServerRequest &request, const QString& name);
    void sendFile(QHttpServerResponder &responder, const QString &path);
    void sendFile(QHttpServerResponder &responder, QHttpHeaders& headers, const QString &path);
    // void sendFile(const QByteArray &byte);
    // void sendFile(const QIODevice &device);
    QHttpServerResponse sendFile(const QString &path);
    QHttpServerResponse sendFile(const QString &path, QHttpHeaders& headers);

};

#endif // HTTPHANDLER_H
