#ifndef XVIDEOPREVIEW_H
#define XVIDEOPREVIEW_H
#include "XTEngine_global.h"

#include <QMediaPlayer>
#include <QTimer>
#include <QVideoSink>
#include <QImage>
#include <QVideoFrame>

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
    void stop();

private:
    void extract();
    QMediaPlayer* _thumbPlayer;
    QString _file;
    qint64 _time;
    bool _loadingInfo = false;
    bool _extracting = false;
    qint64 _lastDuration;
    QImage _lastImage;
    QString _lastError;
    QVideoSink m_videoSink;
    QTimer m_debouncer;
    bool m_processed;

    void setUpThumbPlayer();
    void setUpInfoPlayer();
    void tearDownPlayer();
    void on_thumbCapture(const QVideoFrame &thumb);
    void on_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void on_mediaStateChange(QMediaPlayer::PlaybackState state);
    void on_thumbError(QString error);
    void on_durationChanged(qint64 duration);
    void process();
};

#endif // XVIDEOPREVIEW_H
