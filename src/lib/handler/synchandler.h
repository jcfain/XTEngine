#ifndef SYNCHANDLER_H
#define SYNCHANDLER_H

#include <QObject>
#include <QFuture>
#include <QtCompress/qzipwriter.h>
#include <QtCompress/qzipreader.h>
#include "tcodehandler.h"
#include "funscripthandler.h"
#include "lib/lookup/XMedia.h"
#include "lib/struct/VRPacket.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include "lib/handler/vrdevicehandler.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT SyncHandler: public QObject
{
    Q_OBJECT
signals:
    void funscriptPositionChanged(qint64 msecs);
    void funscriptStatusChanged(XMediaStatus status);
    void funscriptStopped();
    void funscriptStarted();
    void funscriptEnded();
    void funscriptVREnded(QString videoPath, QString funscriptPath, qint64 duration);
    void togglePaused(bool paused);
    void sendTCode(QString tcode);
    void channelPositionChange(QString channel, int position);
public slots:
    void on_device_status_change(ConnectionChangedSignal state);
    void on_vr_device_status_change(VRDeviceHandler* connectedVRDeviceHandler);
    void on_local_video_state_change(XMediaState state);
public:
    SyncHandler(QObject *parent = nullptr);
    ~SyncHandler();
    void togglePause();
    void setPause(bool paused);
    bool isPaused();
    void playStandAlone(QString funscript = nullptr);
    void skipToMoneyShot();
    void setStandAloneLoop(bool enabled);
    void syncVRFunscript(QString funscript);
    void syncFunscript();
    void setFunscriptTime(qint64 secs);
    qint64 getFunscriptTime();
    qint64 getFunscriptMin();
    qint64 getFunscriptMax();
    void stopStandAloneFunscript();
    void stopMediaFunscript();
    void stopVRFunscript();
    void stopAll();
    void clear();
    void reset();
    QList<QString> load(QString funscript);
    bool isLoaded();
    bool isPlaying();
    bool isPlayingStandAlone();
    bool isPlayingVR();
    QString getPlayingStandAloneScript();
private:
    TCodeHandler* _tcodeHandler;

    QMutex _mutex;
    QString _playingStandAloneFunscript;
    bool _isLocalVideoPlaying = false;
    bool _isDeviceConnected = false;
    bool _isMediaFunscriptPlaying = false;
    bool _isVRFunscriptPlaying = false;
    bool _isStandAloneFunscriptPlaying = false;
    bool _isPaused = false;
    bool _standAloneLoop;
    qint64 _currentTime = 0;
    qint64 _currentPulseTime = 0;
    qint64 _seekTime = -1;
    QFuture<void> _funscriptMediaFuture;
    QFuture<void> _funscriptVRFuture;
    QFuture<void> _funscriptStandAloneFuture;
    FunscriptHandler* _funscriptHandler;
    QList<FunscriptHandler*> _funscriptHandlers;
    QList<QString> _invalidScripts;
    VRDeviceHandler* _connectedVRDeviceHandler = 0;
    bool load(QByteArray funscript);
    void loadMFS(QString funscript);
    bool loadMFS(QString channel, QString funscript);
    bool loadMFS(QString channel, QByteArray funscript);

    void sendPulse(qint64 currentMsecs, qint64 &nextPulseTime);
};

#endif // SYNCHANDLER_H
