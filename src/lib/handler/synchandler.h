#ifndef SYNCHANDLER_H
#define SYNCHANDLER_H

#include <QObject>
#include <QFuture>
// #include <QtCompress/qzipwriter.h>
// #include <QtCompress/qzipreader.h>
#include "tcodehandler.h"
#include "funscripthandler.h"
#include "lib/lookup/XMedia.h"
#include "lib/struct/InputConnectionPacket.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include "lib/handler/inputconnectionhandler.h"
#include "lib/handler/outputconnectionhandler.h"
#include "lib/struct/ScriptInfo.h"
#include "XTEngine_global.h"

struct SyncLoadState {
    bool hasScript;
    QString mainScript;
    QStringList invalidScripts;
};

class XTENGINE_EXPORT SyncHandler: public QObject
{
    Q_OBJECT
signals:
    void funscriptPositionChanged(qint64 msecs);
    void funscriptStandaloneDurationChanged(qint64 duration);
    void funscriptStatusChanged(XMediaStatus status);
    void syncEnd();
    void syncStart();
    void syncStopping();
    /// For standalone funscripts
    void funscriptStopped();
    void funscriptStarted();
    void funscriptEnded();
    void funscriptLoaded(QString funscriptPath);
    //void funscriptVREnded(QString videoPath, QString funscriptPath, qint64 duration);
    void togglePaused(bool paused);
    void sendTCode(QString tcode);
    void channelPositionChange(QString channel, int position, int time, ChannelTimeType timeType);
    void funscriptSearchResult(QString mediaPath, QString funscriptPath, qint64 mediaDuration);
    void updateMetadata(LibraryListItemMetaData258 value);

public slots:
    void on_output_device_change(OutputConnectionHandler* outputDeviceHandler);
    void on_input_device_change(InputConnectionHandler* inputDeviceHandler);
    void on_other_media_state_change(XMediaState state);
    void searchForFunscript(InputConnectionPacket packet);
    SyncLoadState swap(const ScriptInfo &script);

public:
    SyncHandler(QObject *parent = nullptr);
    ~SyncHandler();
    void togglePause();
    void setPause(bool paused);
    bool isPaused();
    void playStandAlone();
    void skipToMoneyShot();
    void setStandAloneLoop(bool enabled);
    void syncInputDeviceFunscript(const LibraryListItem27 &libraryItem);
    void syncOtherMediaFunscript(std::function<qint64()> getMediaPosition);
    void setFunscriptTime(qint64 secs);
    qint64 getFunscriptTime();
    qint64 getFunscriptMin();
    qint64 getFunscriptMax();
    qint64 getFunscriptNext();
    void stopStandAloneFunscript();
    void stopOtherMediaFunscript();
    void stopInputDeviceFunscript();
    void stopAll();
    void clear();
    void reset();
    SyncLoadState load(const LibraryListItem27 &libraryItem);
    SyncLoadState swap(const LibraryListItem27 &libraryItem, const ScriptInfo &script);
    bool isLoaded();
    bool isPlaying();
    bool isPlayingInternal();
    bool isPlayingStandAlone();
    bool isPlayingVR();
    QString getPlayingStandAloneScript();

    const Funscript *getFunscript();
    const Funscript *getFunscript(Track channel);

    void buildScriptItem(LibraryListItem27 &item, QString altScript);

private:
    TCodeHandler* _tcodeHandler;
    FunscriptHandler m_funscriptHandler;
    // QList<FunscriptHandler*> _funscriptHandlers;
    InputConnectionHandler* _inputDeviceHandler = 0;
    //OutputDeviceHandler* _outputDeviceHandler = 0;


    QMutex _mutex;
    QString _playingStandAloneFunscript;
    bool _isPaused = false;
    bool _standAloneLoop;
    bool _isOtherMediaPlaying = false;
    double _standAloneFunscriptCurrentTime = 0;
    qint64 _currentPulseTime = 0;
    qint64 _seekTime = -1;
    qint64 _currentLocalVideoTime = 0;
    QFuture<void> _funscriptMediaFuture;
    QFuture<void> _funscriptVRFuture;
    QFuture<void> _funscriptStandAloneFuture;
    QFuture<void> _funscriptSearchFuture;

    bool _funscriptSearchNotFound = false;
    bool m_stopFunscript = false;
    QString _lastSearchedMediaPath;

    SyncLoadState load(const LibraryListItem27 &libraryItem, bool reset);
    SyncLoadState load(const QString& funscript);
    //bool load(QByteArray funscript);
    bool loadFunscripts(const LibraryListItem27 &libraryItem, SyncLoadState &loadState);
    // FunscriptHandler* createFunscriptHandler(QString channel, QString funscript);
    // FunscriptHandler* createFunscriptHandler(QString channel, QByteArray funscript);

    QString buildChannelActions(qint64 time);

    void sendPulse(qint64 currentMsecs, qint64 &nextPulseTime);

    QString searchForFunscript(QString videoPath, QStringList extensions, QStringList libraryPaths);
    QString searchForFunscriptDeep(QString videoPath, QStringList extensions, QStringList libraryPaths);
    QString searchForFunscript(QString videoPath, QStringList extensions, QString pathToSearch);
    QString searchForFunscriptHttp(QString videoPath, QStringList extensions, QString pathToSearch);
    QString searchForFunscriptMFS(QString mediaPath, QStringList libraryPaths);
    QString searchForFunscriptMFSDeep(QString mediaPath, QStringList libraryPaths);


    // QStringList getFunscriptPaths(QString videoPath, QStringList extensions, QString path);
    // QString getNameNoExtension(QString file);
};

#endif // SYNCHANDLER_H
