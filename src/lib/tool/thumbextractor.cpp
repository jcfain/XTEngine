#include "thumbextractor.h"
#include <QThread>
#include <QThreadPool>

#include "../handler/loghandler.h"
#include "../handler/settingshandler.h"
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
        // Track the job so the destructor can guarantee it has finished before
        // this object is torn down (the job captures `this`).
        m_activeJobs.storeRelease(m_activeJobs.loadRelaxed() + 1);
        QThreadPool::globalInstance()->start([this, file, time, timeout](){
            LogHandler::Debug("[ThumbExtractor::m_debouncer] Extract");
            QImage image = extract(file, time, timeout, &m_stopRequested);
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
            m_activeJobs.storeRelease(m_activeJobs.loadRelaxed() - 1);
        });
    });
}

ThumbExtractor::~ThumbExtractor()
{
    // Arm the stop flag so any in-flight extract() bails out of its blocking
    // loops within ~100ms, then wait for every in-flight debounce job to finish
    // so none of them touch this object (or its QMediaPlayer) after destruction.
    m_stopRequested.storeRelease(1);
    while(m_activeJobs.loadRelaxed() > 0)
        QThread::msleep(10);
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
        timeout = timeout > 0 ? timeout : SettingsHandler::getThumbExtractionTimeoutMS();
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
QImage ThumbExtractor::extract(QString file, qint64 time, qint64 timeout, QAtomicInt* stopFlag)
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
    timeout = timeout > 0 ? timeout : SettingsHandler::getThumbExtractionTimeoutMS();
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
            if(stopFlag && stopFlag->loadRelaxed())
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Extraction cancelled";
                return QImage();
            }
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
            if(stopFlag && stopFlag->loadRelaxed())
            {
                QMutexLocker locker(&m_stateMutex);
                m_lastError = "Extraction cancelled";
                return QImage();
            }
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
        if(stopFlag && stopFlag->loadRelaxed())
        {
            QMutexLocker locker(&m_stateMutex);
            m_lastError = "Extraction cancelled";
            return QImage();
        }
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
        if(stopFlag && stopFlag->loadRelaxed())
        {
            LogHandler::Debug("[ThumbExtractor::extractSync] Cancelled mediaStatus: " + QString::number(m_mediaPlayer->mediaStatus()));
            QMutexLocker locker(&m_stateMutex);
            m_lastError = "Extraction cancelled";
            m_extracting.storeRelease(false);
            return QImage();
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

    // 1) Fast path: toImage() works for 8-bit formats
    QImage temp = frame.toImage();

    // 2) Fallback: manual YUV->RGB for 10-bit (P010) and other formats
    //    where toImage() returns null (no RHI -> CPU path drops 10-bit).
    if(temp.isNull())
        temp = convertYuvFrameToImage(frame);

    if(!temp.isNull())
    {
        // Runs on the GUI thread - m_lastImage is read by the worker thread in
        // extract(), so the assignment must be synchronized.
        QMutexLocker locker(&m_stateMutex);
        m_lastImage = temp;
    }
}

QImage ThumbExtractor::convertYuvFrameToImage(const QVideoFrame &frame)
{
    QVideoFrame mutableFrame = frame; // map() is non-const
    if(!mutableFrame.map(QVideoFrame::ReadOnly))
        return QImage();

    const int w = mutableFrame.width();
    const int h = mutableFrame.height();
    const int planes = mutableFrame.planeCount();
    QImage temp(w, h, QImage::Format_RGB32);
    temp.fill(Qt::black);

    if(planes >= 2)
    {
        // YUV 4:2:0 (P010 / NV12 / NV21 / YUV420P)
        const auto pixFmt = mutableFrame.surfaceFormat().pixelFormat();
        const bool isNV21 = (pixFmt == QVideoFrameFormat::Format_NV21);
        const bool isPlanar = (planes >= 3); // YUV420P: separate U/V planes
        const uchar* yData  = mutableFrame.bits(0);
        const uchar* uData  = mutableFrame.bits(1);
        const uchar* vData  = isPlanar ? mutableFrame.bits(2) : nullptr;
        const int yStride  = mutableFrame.bytesPerLine(0);
        const int uStride  = mutableFrame.bytesPerLine(1);
        const int vStride  = isPlanar ? mutableFrame.bytesPerLine(2) : 0;
        const bool is10bit = (yStride >= w * 2); // 16-bit samples (stride may have padding)
        // BT.709 for 720p+, BT.601 for 480p/576p
        const bool isBT709 = (h >= 720);
        const int kr = 298, kg = 298, kb = 298;
        const int kv = isBT709 ? 459 : 409;
        const int ku1 = isBT709 ? 55 : 100;
        const int kv1 = isBT709 ? 136 : 208;
        const int ku2 = isBT709 ? 541 : 516;

        for(int y = 0; y < h; ++y)
        {
            QRgb* row = reinterpret_cast<QRgb*>(temp.scanLine(y));
            for(int x = 0; x < w; ++x)
            {
                int yv, u, v;
                if(is10bit)
                {
                    yv = (reinterpret_cast<const quint16*>(yData)[y * yStride / 2 + x]) >> 2;
                    if(isPlanar)
                    {
                        u = (reinterpret_cast<const quint16*>(uData)[(y/2) * uStride/2 + x/2]) >> 2;
                        v = (reinterpret_cast<const quint16*>(vData)[(y/2) * vStride/2 + x/2]) >> 2;
                    }
                    else
                    {
                        int uvIdx = (y / 2) * (uStride / 2) + (x / 2) * 2;
                        auto uv = reinterpret_cast<const quint16*>(uData);
                        u = isNV21 ? uv[uvIdx + 1] >> 2 : uv[uvIdx] >> 2;
                        v = isNV21 ? uv[uvIdx] >> 2     : uv[uvIdx + 1] >> 2;
                    }
                }
                else
                {
                    yv = yData[y * yStride + x];
                    if(isPlanar)
                    {
                        u = uData[(y/2) * uStride + x/2];
                        v = vData[(y/2) * vStride + x/2];
                    }
                    else
                    {
                        int uvIdx = (y / 2) * uStride + (x / 2) * 2;
                        u = isNV21 ? uData[uvIdx + 1] : uData[uvIdx];
                        v = isNV21 ? uData[uvIdx]     : uData[uvIdx + 1];
                    }
                }
                // YUV -> RGB
                int c = yv - 16;
                int d = u - 128;
                int e = v - 128;
                int r = (kr * c + kv * e + 128) >> 8;
                int g = (kg * c - ku1 * d - kv1 * e + 128) >> 8;
                int b = (kb * c + ku2 * d + 128) >> 8;
                row[x] = qRgb(
                    qBound(0, r, 255),
                    qBound(0, g, 255),
                    qBound(0, b, 255));
            }
        }
    }
    mutableFrame.unmap();
    return temp;
}

void ThumbExtractor::on_mediaError(QMediaPlayer::Error error, const QString &errorString)
{
    LogHandler::Error("[ThumbExtractor::on_mediaError] "+ m_file);
    QMutexLocker locker(&m_stateMutex);
    m_lastError = errorString;
}
