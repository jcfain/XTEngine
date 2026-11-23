#ifndef THUMBEXTRACTOR_H
#define THUMBEXTRACTOR_H

#include <QObject>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QTimer>
#include <QMutex>
#include <QAtomicInt>

#include "mediaformat.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT ThumbExtractor : public QObject
{
    Q_OBJECT
public:
    explicit ThumbExtractor(QObject *parent = nullptr);

    QImage extract(QString videoPath, qint64 time = -1, qint64 timeout = m_defaultTimeout);
    void extractDebounce(QString videoPath, qint64 time = -1, qint64 timeout = m_defaultTimeout);
    QString lastError();
    void reset();

signals:
    void startPlaying();
    void stopPlaying();
    void pausePlaying();
    void setPosition(qint64 value);
    void setMediaSource(QUrl url);
    void frameExtracted(QImage frame);
    void frameExtractionError(QString error);

private:
    // 60s: AV1 software decode on SBCs (Orange Pi 5 / RK3588S, no HW accel in
    // the bundled FFmpeg) is slow enough that the default 5s times out during
    // seek+decode. Non-AV1 formats extract in <1s so the longer timeout is
    // harmless for them. The proper fix is RK3588S MPP hwaccel in FFmpeg,
    // but the rkmpp hwaccel in FFmpeg 6.1 does not support AV1 on this kernel.
    static constexpr int m_defaultTimeout = 30000;
    // Atomic: written by the worker thread (extract), read by the GUI thread (videoFrameChanged)
    QAtomicInt m_extracting = 0;
    bool m_fileChanged = false;
    QString m_file;
    QMediaPlayer* m_mediaPlayer;
    QVideoSink m_videoSink;
    QImage m_lastImage;
    QString m_lastError;
    qint64 m_lastTime;
    // Guards m_lastImage, m_lastError, m_file, m_fileChanged, m_lastTime and m_lastTimeout,
    // which are written by the GUI thread (videoFrameChanged/on_mediaError/reset) and by the
    // worker thread (extract) concurrently. QImage/QString are not thread-safe.
    QMutex m_stateMutex;
    qint64 m_lastTimeout;
    qint64 m_lastDuration = -1;
    QTimer m_debouncer;
    MediaFormat* m_mediaFormat;
    void createMediaPlayer();
    bool stopAndWait();
    bool mediaHasError();
    void videoFrameChanged(const QVideoFrame &thumb);
    void on_mediaError(QMediaPlayer::Error error, const QString &errorString);
};

#endif // THUMBEXTRACTOR_H
