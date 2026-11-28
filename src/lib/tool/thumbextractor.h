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
    ~ThumbExtractor();

    QImage extract(QString videoPath, qint64 time = -1, qint64 timeout = -1, QAtomicInt* stopFlag = nullptr);
    void extractDebounce(QString videoPath, qint64 time = -1, qint64 timeout = -1);
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
    // Atomic: written by the worker thread (extract), read by the GUI thread (videoFrameChanged)
    QAtomicInt m_extracting = 0;
    // Shutdown guard for the debounce pool jobs. The destructor arms this so an
    // in-flight extract() bails out quickly, then waits on m_activeJobs so no
    // pool job outlives this object (prevents a use-after-free when the owning
    // XVideoPreviewWidget is destroyed mid-extraction).
    QAtomicInt m_stopRequested = 0;
    QAtomicInt m_activeJobs = 0;
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
    // Set by the worker thread (extract) right before it queues a setSource for a
    // NEW file, and cleared by on_mediaStatusChanged() (GUI thread) once that new
    // file reaches LoadedMedia. The QMediaPlayer is reused across library items, so
    // mediaStatus() is already LoadedMedia from the previous file; polling it would
    // let extract() read the old file's duration and seek out of range ("Seek timeout").
    QAtomicInt m_pendingLoad = 0;
    QTimer m_debouncer;
    MediaFormat* m_mediaFormat;
    void createMediaPlayer();
    bool stopAndWait();
    bool mediaHasError();
    void videoFrameChanged(const QVideoFrame &thumb);
    void on_mediaError(QMediaPlayer::Error error, const QString &errorString);
    // GUI-thread slot: clears m_pendingLoad when the source requested in extract()
    // reaches LoadedMedia, so the worker proceeds with the NEW file's state.
    void on_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    // Converts a YUV 4:2:0 QVideoFrame (8-bit or 10-bit, semi-planar or planar)
    // to an 8-bit RGB32 QImage. Handles NV12, NV21, YUV420P, P010, and
    // YUV420P10 with BT.601/BT.709 auto-detect by resolution.
    // Returns a null QImage if the frame cannot be converted.
    static QImage convertYuvFrameToImage(const QVideoFrame &frame);
};

#endif // THUMBEXTRACTOR_H
