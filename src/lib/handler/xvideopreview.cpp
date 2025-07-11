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
    connect(_thumbPlayer, &QMediaPlayer::durationChanged, this, &XVideoPreview::on_durationChanged);
    connect(&m_videoSink, &QVideoSink::videoFrameChanged, this, &XVideoPreview::on_thumbCapture);
    connect(_thumbPlayer, &QMediaPlayer::errorOccurred, this, &XVideoPreview::on_thumbError);
    connect(&m_debouncer, &QTimer::timeout, this, [this]() {
        LogHandler::Debug("Extract debounce");
        extract();
    });
}
XVideoPreview::~XVideoPreview()
{
    LogHandler::Debug("~XVideoPreview::~XVideoPreview");
    tearDownPlayer();
}
void XVideoPreview::setUpThumbPlayer()
{
    // LogHandler::Debug("XVideoPreview::setUpThumbPlayer");
}

void XVideoPreview::setUpInfoPlayer()
{
    LogHandler::Debug("XVideoPreview::setUpInfoPlayer");
}

void XVideoPreview::tearDownPlayer()
{
    LogHandler::Debug("XVideoPreview::tearDownPlayer");
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
    if(!m_fileChanged)
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
        _lastDuration = -1;
    }
    m_fileChanged = true;
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
            if(!_lastError.isEmpty())
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
