#include "thumbextractor.h"
#include <QThread>
#include <QThreadPool>

#include "../handler/loghandler.h"
#include "xmath.h"

///
/// \brief ThumbExtractor::ThumbExtractor This class should be constructed IN THE main thread
/// \param parent
///
ThumbExtractor::ThumbExtractor(QObject *parent)
    : QObject{parent},
    m_mediaPlayer(0)
{
    createMediaPlayer();
    m_debouncer.setSingleShot(true);
    connect(&m_debouncer, &QTimer::timeout, this, [this]() {
        // Snapshot the request under the lock and pass it by value to the pool job:
        // extractDebounce() may be called from any thread.
        QString file;
        qint64 time = 0;
        qint64 timeout = 0;
        {
            QMutexLocker locker(&m_stateMutex);
            file = m_file;
            time = m_lastTime;
            timeout = m_lastTimeout;
        }
        QThreadPool::globalInstance()->start([this, file, time, timeout](){
            LogHandler::Debug("[ThumbExtractor::m_debouncer] Extract");
            QImage image = extract(file, time, timeout);
            if(!image.isNull())
            {
                emit frameExtracted(image.copy());
                image = QImage();
            }
            else
            {
                QString error;
                {
                    QMutexLocker locker(&m_stateMutex);
                    error = m_lastError;
                    m_lastError.clear();
                }
                emit frameExtractionError(error);
            }
        });
    });
}

///
/// \brief ThumbExtractor::extractDebounce Extracts a fram from a video file with a debounce. CAN be called from ANY QThread. I think...
/// \param file
/// \param time
/// \param timeout
///
void ThumbExtractor::extractDebounce(QString file, qint64 time, qint64 timeout)
{
    LogHandler::Debug("[ThumbExtractor::extractDebounce] at: " + QString::number(time) + " from: "+ file);
    {
        QMutexLocker locker(&m_stateMutex);
        if(!m_fileChanged)
            m_fileChanged = m_file != file;
        if(m_fileChanged)
            m_file = file;
        m_lastTime = time;
        m_lastTimeout = timeout;
    }
    m_debouncer.start(100);
}

///
/// \brief ThumbExtractor::extract Extracts a fram from a video file, Should be called from a NON GUI thread.
/// \param file
/// \param time
/// \param timeout
/// \return
///
QImage ThumbExtractor::extract(QString file, qint64 time, qint64 timeout)
{
    LogHandler::Debug("[ThumbExtractor::extract] at: " + QString::number(time) + " from: "+ file);
    auto currentTime = QTime::currentTime().msecsSinceStartOfDay();
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = "";
        m_lastImage = QImage();
    }
    if(file.isNull())
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = "Invalid file path.";
        return QImage();
    }
    if(!QFile::exists(file))
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = "File: "+file+" does not exist.";
        return QImage();
    }
    bool fileChanged;
    QString currentFile;
    {
        QMutexLocker locker(&m_stateMutex);
        if(!m_fileChanged)
            m_fileChanged = file != m_file;
        if(m_fileChanged)
            m_file = file;
        m_lastTime = time;
        m_lastTimeout = timeout;
        fileChanged = m_fileChanged;
        currentFile = m_file;
    }

    if(fileChanged)
    {
        LogHandler::Debug("[ThumbExtractor::extract] File changed: " + currentFile);
        QUrl mediaUrl = QUrl::fromLocalFile(currentFile);
        // QMediaPlayer lives on the GUI thread - route the call through a queued signal
        emit setMediaSource(mediaUrl);
        m_lastDuration = -1;
        {
            QMutexLocker locker(&m_stateMutex);
            m_fileChanged = false;
        }
        while(m_mediaPlayer->mediaStatus() != QMediaPlayer::LoadedMedia)
        {
            if(mediaHasError())
            {
                stopAndWait();
                return QImage();
            }
            if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Load timeout";
                return QImage();
            }
            QThread::msleep(100);
        }
    }
    if(!m_mediaPlayer->hasVideo())
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = "No video";
        return QImage();
    }
    // This didnt work on the orangepi5 but did on main pc...
    // if(!m_mediaFormat->isCodecSupported(m_lastError)) {
    //     return QImage();
    // }
    LogHandler::Debug("[ThumbExtractor::extract] Loaded media mediaStatus: " + QString::number(m_mediaPlayer->mediaStatus()));
    currentTime = QTime::currentTime().msecsSinceStartOfDay();
    // Do not start/stop QMediaPlayer in a non GUI thread.
    emit startPlaying();
    if(time == -1 || m_lastDuration == -1)
    {
        while(m_lastDuration <= 0)
        {
            // LogHandler::Debug("[ThumbExtractor::extractSync] mediaStatus: " + QString::number(m_mediaPlayer->mediaStatus()));
            if(mediaHasError())
            {
                stopAndWait();
                return QImage();
            }
            auto duration = m_mediaPlayer->duration();
            m_lastDuration = duration > 0 ? duration : -1;
            if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
            {
                {
                    QMutexLocker locker(&m_stateMutex);
                    m_lastError = "Duration load timeout";
                }
                stopAndWait();
                return QImage();
            }
            QThread::msleep(100);
        }
    }
    qint64 position = time > 0 ? time : XMath::random((qint64)1, m_lastDuration);
    LogHandler::Debug("[ThumbExtractor::extract] extract at: " + QString::number(position));
    emit setPosition(position);
    currentTime = QTime::currentTime().msecsSinceStartOfDay();
    while(m_mediaPlayer->position() < position)
    {
        // LogHandler::Debug("[ThumbExtractor::extract] Waiting for seek: " + QString::number(m_mediaPlayer->position()));
        if(!m_mediaPlayer->isSeekable())
        {
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Cant seek media";
            }
            stopAndWait();
            return QImage();
        }
        if(mediaHasError())
        {
            stopAndWait();
            return QImage();
        }
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
        {
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Seek timeout";
            }
            stopAndWait();
            return QImage();
        }
        QThread::msleep(100);
    }
    m_extracting.storeRelease(true);
    currentTime = QTime::currentTime().msecsSinceStartOfDay();
    while(true)
    {
        // Copy the frame under the lock: videoFrameChanged() (GUI thread) may replace
        // m_lastImage at any moment and QImage is not thread-safe.
        QImage image;
        {
            QMutexLocker locker(&m_stateMutex);
            if(!m_lastImage.isNull())
                image = m_lastImage.copy();
        }
        if(!image.isNull())
        {
            m_extracting.storeRelease(false);
            stopAndWait();
            return image;
        }
        if(mediaHasError())
        {
            m_extracting.storeRelease(false);
            stopAndWait();
            return QImage();
        }
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
        {
            LogHandler::Debug("[ThumbExtractor::extractSync] Timeout mediaStatus: " + QString::number(m_mediaPlayer->mediaStatus()));
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Image extraction timeout";
            }
            m_extracting.storeRelease(false);
            stopAndWait();
            return QImage();
        }
        QThread::msleep(100);
    }
}

QString ThumbExtractor::lastError()
{
    QMutexLocker locker(&m_stateMutex);
    return m_lastError;
}

void ThumbExtractor::reset()
{
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastImage = QImage();
        m_lastError = "";
    }
    m_extracting.storeRelease(false);
    stopAndWait();
    emit setMediaSource(QUrl());
    // createMediaPlayer(); Doesnt seem to help much with memory
}

void ThumbExtractor::createMediaPlayer()
{
    if(m_mediaPlayer)
    {
        delete m_mediaPlayer;
    }
    m_mediaPlayer = new QMediaPlayer(this);
    m_mediaPlayer->setVideoSink(&m_videoSink);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &ThumbExtractor::on_mediaError);
    // Explicit Qt::QueuedConnection: these signals are emitted from the worker thread
    // (extract()) and QMediaPlayer must only be touched from the GUI thread. With the
    // default AutoConnection both objects have GUI-thread affinity, which would make
    // these DirectConnections and run the QMediaPlayer slots on the worker thread.
    connect(this, &ThumbExtractor::startPlaying, m_mediaPlayer, &QMediaPlayer::play, Qt::QueuedConnection);
    connect(this, &ThumbExtractor::stopPlaying, m_mediaPlayer, &QMediaPlayer::stop, Qt::QueuedConnection);
    connect(this, &ThumbExtractor::pausePlaying, m_mediaPlayer, &QMediaPlayer::pause, Qt::QueuedConnection);
    connect(this, &ThumbExtractor::setPosition, m_mediaPlayer, &QMediaPlayer::setPosition, Qt::QueuedConnection);
    connect(this, &ThumbExtractor::setMediaSource, m_mediaPlayer, &QMediaPlayer::setSource, Qt::QueuedConnection);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this, &ThumbExtractor::videoFrameChanged);
    m_mediaFormat = new MediaFormat(m_mediaPlayer, m_mediaPlayer);
}

bool ThumbExtractor::stopAndWait()
{
    emit stopPlaying();
    auto currentTime = QTime::currentTime().msecsSinceStartOfDay();
    while(m_mediaPlayer->playbackState() != QMediaPlayer::StoppedState)
    {
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= 2000)
        {
            LogHandler::Debug("[ThumbExtractor::extractSync] Timeout media stop: " + QString::number(m_mediaPlayer->playbackState()));
            QMutexLocker locker(&m_stateMutex);
            m_lastError = tr("Timed out waiting for media to stop");
            return false;
        }
        QThread::msleep(100);
    }
    return true;
}

bool ThumbExtractor::mediaHasError()
{
    if(m_mediaPlayer->error() != QMediaPlayer::Error::NoError)
    {
        QMutexLocker locker(&m_stateMutex);
        m_lastError = m_mediaPlayer->errorString();
        return true;
    }
    return false;
}

// Private
void ThumbExtractor::videoFrameChanged(const QVideoFrame &frame)
{
    if(!m_extracting.loadAcquire() || !frame.isValid())
        return;

    QImage temp;

    // 1) Fast path: toImage() works for 8-bit formats
    temp = frame.toImage();

    // 2) Fallback: manual YUV->RGB for 10-bit (P010) and other formats
    //    where toImage() returns null (no RHI -> CPU path drops 10-bit).
    if(temp.isNull())
    {
        QVideoFrame mutableFrame = frame; // map() is non-const
        if(mutableFrame.map(QVideoFrame::ReadOnly))
        {
            const int w = mutableFrame.width();
            const int h = mutableFrame.height();
            const int planes = mutableFrame.planeCount();
            temp = QImage(w, h, QImage::Format_RGB32);
            temp.fill(Qt::black);

            if(planes >= 2)
            {
                // YUV 4:2:0 (P010 / NV12 / YUV420P)
                const uchar* yData  = mutableFrame.bits(0);
                const uchar* uvData = mutableFrame.bits(1);
                const int yStride  = mutableFrame.bytesPerLine(0);
                const int uvStride = mutableFrame.bytesPerLine(1);
                const bool is10bit = (yStride == w * 2); // 16-bit samples

                for(int y = 0; y < h; ++y)
                {
                    QRgb* row = reinterpret_cast<QRgb*>(temp.scanLine(y));
                    for(int x = 0; x < w; ++x)
                    {
                        int yv, u, v;
                        if(is10bit)
                        {
                            // P010: 16-bit samples, upper 6 bits are zero
                            yv = (reinterpret_cast<const quint16*>(yData)[y * yStride / 2 + x]) >> 6;
                            int uvIdx = (y / 2) * (uvStride / 2) + (x / 2) * 2;
                            u = (reinterpret_cast<const quint16*>(uvData)[uvIdx])     >> 6;
                            v = (reinterpret_cast<const quint16*>(uvData)[uvIdx + 1]) >> 6;
                        }
                        else
                        {
                            // 8-bit YUV 4:2:0
                            yv = yData[y * yStride + x];
                            int uvIdx = (y / 2) * uvStride + (x / 2) * 2;
                            u = uvData[uvIdx];
                            v = uvData[uvIdx + 1];
                        }
                        // YUV (BT.601) -> RGB
                        int c = yv - 16;
                        int d = u - 128;
                        int e = v - 128;
                        int r = (298 * c + 409 * e + 128) >> 8;
                        int g = (298 * c - 100 * d - 208 * e + 128) >> 8;
                        int b = (298 * c + 516 * d + 128) >> 8;
                        row[x] = qRgb(
                            qBound(0, r, 255),
                            qBound(0, g, 255),
                            qBound(0, b, 255));
                    }
                }
            }
            mutableFrame.unmap();
        }
    }

    if(!temp.isNull())
    {
        // Runs on the GUI thread - m_lastImage is read by the worker thread in
        // extract(), so the assignment must be synchronized.
        QMutexLocker locker(&m_stateMutex);
        m_lastImage = temp;
    }
}

void ThumbExtractor::on_mediaError(QMediaPlayer::Error error, const QString &errorString)
{
    LogHandler::Error("[ThumbExtractor::on_mediaError] "+ m_file);
    QMutexLocker locker(&m_stateMutex);
    m_lastError = errorString;
}
