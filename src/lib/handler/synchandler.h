#ifndef SYNCHANDLER_H
#define SYNCHANDLER_H

#include <QObject>
#include <QFuture>
#include <QtCompress/qzipwriter.h>
#include <QtCompress/qzipreader.h>
#include "tcodehandler.h"
#include "funscripthandler.h"
#include "lib/lookup/XMedia.h"
#include "lib/struct/InputDevicePacket.h"
#include "lib/struct/ConnectionChangedSignal.h"
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include "lib/handler/inputdevicehandler.h"
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
    void funscriptSearchResult(QString mediaPath, QString funscriptPath, qint64 mediaDuration);

public slots:
    void on_output_device_status_change(ConnectionChangedSignal state);
    void on_input_device_change(InputDeviceHandler* connectedVRDeviceHandler);
    void on_other_media_state_change(XMediaState state);
    void searchForFunscript(InputDevicePacket packet);

public:
    SyncHandler(QObject *parent = nullptr);
    ~SyncHandler();
    void togglePause();
    void setPause(bool paused);
    bool isPaused();
    void playStandAlone(QString funscript = nullptr);
    void skipToMoneyShot();
    void setStandAloneLoop(bool enabled);
    void syncInputDeviceFunscript(QString funscript, InputDeviceHandler* connectedInputDeviceHandler);
    void syncOtherMediaFunscript(std::function<qint64()> getMediaPosition);
    void setFunscriptTime(qint64 secs);
    qint64 getFunscriptTime();
    qint64 getFunscriptMin();
    qint64 getFunscriptMax();
    void stopStandAloneFunscript();
    void stopOtherMediaFunscript();
    void stopInputDeviceFunscript();
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
    FunscriptHandler* _funscriptHandler;
    QList<FunscriptHandler*> _funscriptHandlers;
    InputDeviceHandler* _connectedInputDeviceHandler = 0;

    QMutex _mutex;
    QString _playingStandAloneFunscript;
    bool _isOtherMediaPlaying = false;
    bool _isDeviceConnected = false;
    bool _isMediaFunscriptPlaying = false;
    bool _isVRFunscriptPlaying = false;
    bool _isStandAloneFunscriptPlaying = false;
    bool _isPaused = false;
    bool _standAloneLoop;
    qint64 _currentTime = 0;
    qint64 _currentPulseTime = 0;
    qint64 _seekTime = -1;
    qint64 _currentLocalVideoTime = 0;
    QFuture<void> _funscriptMediaFuture;
    QFuture<void> _funscriptVRFuture;
    QFuture<void> _funscriptStandAloneFuture;
    QList<QString> _invalidScripts;

    bool _funscriptSearchRunning = false;
    bool _funscriptSearchNotFound = false;
    QString _lastSearchedMediaPath;

    bool load(QByteArray funscript);
    void loadMFS(QString funscript);
    bool loadMFS(QString channel, QString funscript);
    bool loadMFS(QString channel, QByteArray funscript);

    void sendPulse(qint64 currentMsecs, qint64 &nextPulseTime);
};

#endif // SYNCHANDLER_H
