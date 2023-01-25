#ifndef XVIDEOPREVIEW_H
#define XVIDEOPREVIEW_H
#include "XTEngine_global.h"

#include <QMediaPlayer>
#include <QTimer>
//#include <QProcess>
//#include <QCoreApplication>
//#include <QFileInfo>
#include "xvideosurface.h"
#include "loghandler.h"

class XTENGINE_EXPORT XVideoPreview : public QObject
{
    Q_OBJECT
signals:
    void mediaLoaded();
    void durationChanged(qint64 duration);
    void frameExtracted(QImage frame);
    void frameExtractionError(QString error);

public:
    XVideoPreview(QObject* parent = nullptr);
    ~XVideoPreview();
    void extract(QString videoPath, qint64 time = -1);
    void load(QString file);

private:
    void extract();
    XVideoSurface* _thumbNailVideoSurface;
    QMediaPlayer* _thumbPlayer;
//    QProcess* m_ffmpeg;
//    QProcess* m_ffmprobe;
    QString _file;
    qint64 _time;
    bool _loadingInfo = false;
    bool _extracting = false;
    qint64 _lastDuration;
    QImage _lastImage;
    QString _lastError;
    QVideoSurfaceFormat m_format;
    QTimer m_debouncer;

    void setUpThumbPlayer();
    void setUpInfoPlayer();
    void tearDownPlayer();
    void on_thumbCapture(QImage thumb);
    void on_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void on_mediaStateChange(QMediaPlayer::State state);
    void on_thumbError(QString error);
    void on_durationChanged(qint64 duration);
//    bool checkForFFMpeg();
//    void getDurationFromFFmpeg();
//    void onFFMpegDuration();
//    void getThumbFromFFmpeg();
//    void onFFMpegThumbSave();
};

#endif // XVIDEOPREVIEW_H
