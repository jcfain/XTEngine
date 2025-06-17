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
    : QObject{parent}
{
    m_mediaPlayer = new QMediaPlayer(this);
    m_mediaPlayer->setVideoSink(&m_videoSink);
    connect(m_mediaPlayer, &QMediaPlayer::errorOccurred, this, &ThumbExtractor::on_mediaError);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this, &ThumbExtractor::videoFrameChanged);
    connect(this, &ThumbExtractor::startPlaying, m_mediaPlayer, &QMediaPlayer::play);
    connect(this, &ThumbExtractor::stopPlaying, m_mediaPlayer, &QMediaPlayer::stop);
    connect(this, &ThumbExtractor::pausePlaying, m_mediaPlayer, &QMediaPlayer::pause);
    connect(this, &ThumbExtractor::setPosition, m_mediaPlayer, &QMediaPlayer::setPosition);
    m_debouncer.setSingleShot(true);
    connect(&m_debouncer, &QTimer::timeout, this, [this]() {
        QThreadPool::globalInstance()->start([this](){
            LogHandler::Debug("[ThumbExtractor::m_debouncer] Extract");
            QImage image = extract(m_file, m_lastTime, m_lastTimeout);
            if(!image.isNull())
            {
                emit frameExtracted(image.copy());
                image = QImage();
            }
            else
            {
                emit frameExtractionError(m_lastError);
                m_lastError.clear();
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
    if(!m_fileChanged)
        m_fileChanged = m_file != file;
    if(m_fileChanged)
        m_file = file;
    m_lastTime = time;
    m_lastTimeout = timeout;
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
    m_lastError = "";
    m_lastImage = QImage();
    if(file.isNull())
    {
        m_lastError = "Invalid file path.";
        return m_lastImage;
    }
    if(!QFile::exists(file))
    {
        m_lastError = "File: "+file+" does not exist.";
        return m_lastImage;
    }
    if(!m_fileChanged)
        m_fileChanged = file != m_file;
    if(m_fileChanged)
        m_file = file;
    m_lastTime = time;
    m_lastTimeout = timeout;

    if(m_fileChanged)
    {
        QUrl mediaUrl = QUrl::fromLocalFile(m_file);
        m_mediaPlayer->setSource(mediaUrl);
        m_lastDuration = -1;
        m_fileChanged = false;
    }
    // Do not start/stop QMediaPlayer in a non GUI thread.
    emit startPlaying();
    if(time == -1 || m_lastDuration == -1)
    {
        while(m_lastDuration <= 0)
        {
            auto duration = m_mediaPlayer->duration();
            // LogHandler::Debug("[XVideoPreview::extractSync] mediaStatus: " + QString::number(m_mediaPlayer->mediaStatus()));
            m_lastDuration = duration > 0 ? duration : -1;
            if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
            {
                m_lastError = "Duration load timeout";
                emit stopPlaying();
                return m_lastImage;
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
        LogHandler::Debug("[ThumbExtractor::extract] Waiting for seek: " + QString::number(m_mediaPlayer->position()));
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
        {
            m_lastError = "Seek timeout";
            emit stopPlaying();
            return m_lastImage;
        }
        QThread::msleep(100);
    }
    m_extracting = true;
    currentTime = QTime::currentTime().msecsSinceStartOfDay();
    while(m_lastImage.isNull())
    {
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
        {
            m_lastError = "Image extraction timeout";
            m_extracting = false;
            emit stopPlaying();
            return m_lastImage;
        }
        QThread::msleep(100);
    }
    emit stopPlaying();
    while(m_mediaPlayer->playbackState() != QMediaPlayer::StoppedState)
        QThread::msleep(100);
    m_extracting = false;
    return m_lastImage;
}

QString ThumbExtractor::lastError()
{
    return m_lastError;
}

// Private
void ThumbExtractor::videoFrameChanged(const QVideoFrame &frame)
{
    if(m_extracting) {
        LogHandler::Debug("[ThumbExtractor::videoFrameChanged]: "+ m_file);
        QImage temp = frame.toImage();
        if(!temp.isNull())
            m_lastImage = temp;
    }
}

void ThumbExtractor::on_mediaError(QMediaPlayer::Error error, const QString &errorString)
{
    LogHandler::Debug("[ThumbExtractor::on_mediaError] "+ m_file);
    m_lastError = errorString;
}
