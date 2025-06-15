#include "xvideopreview.h"
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>
#include "loghandler.h"
#include "QMediaMetaData"
#include "QThread"
#include "../tool/xmath.h"

XVideoPreview::XVideoPreview(QObject* parent) : QObject(parent),  _thumbPlayer(0)
{
    LogHandler::Debug("XVideoPreview");
    _thumbPlayer = new QMediaPlayer(this);
    // _thumbNailVideoSurface = new XVideoSurface(_thumbPlayer);
    _thumbPlayer->setVideoSink(&m_videoSink);
    m_debouncer.setSingleShot(true);
    connect(_thumbPlayer, &QMediaPlayer::playbackStateChanged, this, &XVideoPreview::on_mediaStateChange);
    connect(_thumbPlayer, &QMediaPlayer::mediaStatusChanged, this, &XVideoPreview::on_mediaStatusChanged);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this, &XVideoPreview::on_thumbCapture);
    connect(_thumbPlayer, &QMediaPlayer::errorOccurred, this, &XVideoPreview::on_thumbError);
    connect(_thumbPlayer, &QMediaPlayer::durationChanged, this, &XVideoPreview::on_durationChanged);
    connect(&m_debouncer, &QTimer::timeout, this, [this]() {
        LogHandler::Debug("Extract debounce");
        extract();
    });
}
XVideoPreview::~XVideoPreview() {
    LogHandler::Debug("~XVideoPreview");
    tearDownPlayer();
}
void XVideoPreview::setUpThumbPlayer()
{
    LogHandler::Debug("setUpThumbPlayer");
}

void XVideoPreview::setUpInfoPlayer() {
    LogHandler::Debug("setUpInfoPlayer");
}

void XVideoPreview::tearDownPlayer()
{
    LogHandler::Debug("tearDownPlayer");
//    if(_thumbPlayer)
//    {
//        disconnect(_thumbPlayer, nullptr, nullptr, nullptr);
//        disconnect(_thumbNailVideoSurface, nullptr, nullptr,nullptr);
//        disconnect(_thumbPlayer, &QMediaPlayer::stateChanged, this, &XVideoPreview::on_mediaStateChange);
//        disconnect(_thumbPlayer, &QMediaPlayer::mediaStatusChanged, this, &XVideoPreview::on_mediaStatusChanged);
//        disconnect(_thumbNailVideoSurface, &XVideoSurface::frameCapture, this, &XVideoPreview::on_thumbCapture);
//        disconnect(_thumbNailVideoSurface, &XVideoSurface::frameCaptureError, this, &XVideoPreview::on_thumbError);
//        disconnect(_thumbPlayer, &QMediaPlayer::durationChanged, this, &XVideoPreview::on_durationChanged);
//        if(_thumbNailVideoSurface->isActive())
//            _thumbNailVideoSurface->stop();

    //    if(_thumbPlayer->playbackState() == QMediaPlayer::PlayingState)
    _thumbPlayer->stop();
    // _thumbPlayer->setMedia(QMediaContent());
//        else {
//            delete _thumbPlayer;
//            _thumbPlayer = 0;
//            _thumbNailVideoSurface = 0;
//        }
//    }
}

void XVideoPreview::extract(QString file, qint64 time)
{
    setUpThumbPlayer();
    m_fileChanged = _file != file;
    _file = file;
    if(_file.isNull()) {
        LogHandler::Error("In valid file path.");
        emit frameExtractionError("In valid file path.");
        return;
    }
    if(!QFile::exists(_file)) {
        LogHandler::Error("File: "+file+" does not exist.");
        emit frameExtractionError("File: "+file+" does not exist.");
        return;
    }
    _time = time;
    m_debouncer.start(100);
}

void XVideoPreview::extract() {
    LogHandler::Debug("extract at: " + QString::number(_time) + " from: "+ _file);
    m_processed = false;
    if(m_fileChanged)
    {
        QUrl mediaUrl = QUrl::fromLocalFile(_file);
        _thumbPlayer->setSource(mediaUrl);
        m_fileChanged = false;
    }
    if(_time > -1)
    {
        _loadingInfo = false;
        _extracting = true;
        _thumbPlayer->setPosition(_time);
    }
    _thumbPlayer->play();
}

void XVideoPreview::load(QString file)
{
    if(_file == file && _lastDuration > 0) {
        emit durationChanged(_lastDuration);
        return;
    } else {
        _lastDuration = 0;
    }
    LogHandler::Debug("load: "+ file);
    _loadingInfo = true;
    extract(file);
}

void XVideoPreview::stop()
{
    tearDownPlayer();
}

QString XVideoPreview::getLastError()
{
    return _lastError;
}

QImage XVideoPreview::extractSync(QString file, qint64 time, qint64 timeout)
{
    LogHandler::Debug("[XVideoPreview::extractSync] at: " + QString::number(time) + " from: "+ file);
    auto currentTime = QTime::currentTime().msecsSinceStartOfDay();
    tearDownPlayer();
    setUpThumbPlayer();
    if(file.isNull()) {
        LogHandler::Error("[XVideoPreview::extractSync] In valid file path.");
//        emit frameExtractionError("In valid file path.");
        _lastError = "Invalid file path.";
        return QImage();
    }
    if(!QFile::exists(file)) {
        LogHandler::Error("[XVideoPreview::extractSync] File: "+file+" does not exist.");
        _lastError = "File: "+file+" does not exist.";
//        emit frameExtractionError("File: "+file+" does not exist.");
        return QImage();
    }
    m_fileChanged = file != _file;
    _file = file;

    m_processed = false;
    if(m_fileChanged)
    {
        QUrl mediaUrl = QUrl::fromLocalFile(_file);
        _thumbPlayer->setSource(mediaUrl);
        _lastDuration = -1;
    }
    if(time == -1)
    {
        while(_lastDuration == -1)
        {
            auto duration = _thumbPlayer->metaData().value(QMediaMetaData::Duration).toLongLong(0);
//            auto duration = _thumbPlayer->duration();
            _lastDuration = duration == 0 ? duration : -1;
            if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
            {
                _lastError = "Duration load timeout";
                tearDownPlayer();
                return QImage();
            }
            QThread::msleep(100);
        }
    }
    qint64 position = time > 0 ? time : XMath::random((qint64)1, _lastDuration);
    LogHandler::Debug("[XVideoPreview::extractSync] extract at: " + QString::number(position));
//    _time = time;
    _thumbPlayer->setPosition(position);
    _thumbPlayer->play();
//    m_debouncer.start(100);
    _extracting = true;
    currentTime = QTime::currentTime().msecsSinceStartOfDay();
    while(_lastImage.isNull())
    {
        if(QTime::currentTime().msecsSinceStartOfDay() - currentTime >= timeout)
        {
            _lastError = "Image extraction timeout";
            tearDownPlayer();
            return QImage();
        }
        QThread::msleep(100);
    }
    _lastError = "";
    return _lastImage;
}

// Private
void XVideoPreview::on_thumbCapture(const QVideoFrame &frame)
{
    // if(!frame.isValid()) {
    //     emit frameExtractionError("Invalid!");
    // }
    if(_extracting) {
        LogHandler::Debug("on_thumbCapture: "+ _file);
        _lastImage = frame.toImage();
        // frame = QImage();
        //tearDownPlayer();
        process();
    }
}

void XVideoPreview::on_thumbError(QMediaPlayer::Error error, const QString &errorString)
{
    LogHandler::Debug("on_thumbError: "+ _file);
    m_processed = true;
    emit frameExtractionError(errorString);
    // if(_extracting) {
    //     LogHandler::Debug("on_thumbError: "+ _file);
    //     _lastError = errorString;
    //     //tearDownPlayer();
    //     process();
    // }
}

void XVideoPreview::on_mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if(status == QMediaPlayer::MediaStatus::LoadedMedia) {
//        if(_thumbNailVideoSurface && !_thumbNailVideoSurface->isActive() && !_loadingInfo)
//            _thumbNailVideoSurface->start(m_format);
    } else if(status == QMediaPlayer::MediaStatus::InvalidMedia) {
        emit frameExtractionError("Invalid media");
    }
}
void XVideoPreview::on_mediaStateChange(QMediaPlayer::PlaybackState state)
{
    if(state == QMediaPlayer::PlayingState)
    {
        //_thumbPlayer->pause();
    }
    else if(state == QMediaPlayer::StoppedState)
    {
        process();

    }
}

void XVideoPreview::process() {
    if(!m_processed)
    {
        if(_extracting)
        {
            _extracting = false;
            if(!_lastError.isNull())
            {
                m_processed = true;
                emit frameExtractionError(_lastError);
                _lastError.clear();
            }
            else if(!_lastImage.isNull())
            {
                m_processed = true;
                emit frameExtracted(_lastImage.copy());
                _lastImage = QImage();
            }
            tearDownPlayer();
        }
        else if(_loadingInfo && _lastDuration > 0)
        {
            _loadingInfo = false;
            m_processed = true;
            emit durationChanged(_lastDuration);
        }
    }
}

void XVideoPreview::on_durationChanged(qint64 duration)
{
    if(_loadingInfo && duration > 0)
    {
        LogHandler::Debug("on_durationChanged: "+ _file);
        LogHandler::Debug("on_durationChanged: "+ QString::number(duration));
        _lastDuration = duration;
        //tearDownPlayer();
        process();
//        _loadingInfo = false;
//        emit durationChanged(_lastDuration);
    }
}
