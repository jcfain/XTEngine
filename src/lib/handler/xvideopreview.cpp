#include "xvideopreview.h"

XVideoPreview::XVideoPreview(QObject* parent) : QObject(parent), _thumbNailVideoSurface(0), _thumbPlayer(0)
{
    _thumbPlayer = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    _thumbNailVideoSurface = new XVideoSurface(_thumbPlayer);
    _thumbPlayer->setVideoOutput(_thumbNailVideoSurface);
    _thumbPlayer->setMuted(true);
    connect(_thumbPlayer, &QMediaPlayer::stateChanged, this, &XVideoPreview::on_mediaStateChange);
    connect(_thumbPlayer, &QMediaPlayer::mediaStatusChanged, this, &XVideoPreview::on_mediaStatusChanged);
    connect(_thumbNailVideoSurface, &XVideoSurface::frameCapture, this, &XVideoPreview::on_thumbCapture);
    connect(_thumbNailVideoSurface, &XVideoSurface::frameCaptureError, this, &XVideoPreview::on_thumbError);
    connect(_thumbPlayer, &QMediaPlayer::durationChanged, this, &XVideoPreview::on_durationChanged);
}
XVideoPreview::~XVideoPreview() {
    tearDownPlayer();
}
void XVideoPreview::setUpThumbPlayer()
{
    LogHandler::Debug("setUpThumbPlayer");
}

void XVideoPreview::setUpInfoPlayer() {
    LogHandler::Debug("setUpThumbPlayer");
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
        if(_thumbNailVideoSurface->isActive())
            _thumbNailVideoSurface->stop();
        if(_thumbPlayer->state() == QMediaPlayer::PlayingState)
            _thumbPlayer->stop();
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
    _file = file;
    if(_file.isNull()) {
        emit frameExtractionError("In valid file path.");
        return;
    }
    if(!QFile::exists(_file)) {
        emit frameExtractionError("File: "+file+" does not exist.");
        return;
    }
    _time = time;
    LogHandler::Debug("extract: "+ file);
    LogHandler::Debug("extract: "+ QString::number(time));
    QUrl mediaUrl = QUrl::fromLocalFile(file);
    QMediaContent mc(mediaUrl);
    _thumbPlayer->setMedia(mc);
    if(time > -1)
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

// Private
void XVideoPreview::on_thumbCapture(QImage frame)
{
    if(_extracting) {
        LogHandler::Debug("on_thumbCapture: "+ _file);
        _lastImage = frame;
        frame = QImage();
        tearDownPlayer();
    }
}

void XVideoPreview::on_thumbError(QString error)
{
    if(_extracting) {
        LogHandler::Debug("on_thumbError: "+ _file);
        _lastError = error;
        tearDownPlayer();
    }
}

void XVideoPreview::on_mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if(status == QMediaPlayer::MediaStatus::LoadedMedia) {
        if(_thumbNailVideoSurface && !_loadingInfo)
            _thumbNailVideoSurface->start(m_format);
    }
}
void XVideoPreview::on_mediaStateChange(QMediaPlayer::State state)
{
    if(state == QMediaPlayer::State::PlayingState)
    {
        //_thumbPlayer->pause();
    }
    else if(state == QMediaPlayer::State::StoppedState)
    {
        if(_extracting) {
            _extracting = false;
            if(!_lastError.isNull()) {
                emit frameExtractionError(_lastError);
                _lastError.clear();
            }
            else if(!_lastImage.isNull()) {
                emit frameExtracted(_lastImage);
                //_lastImage = QImage();
            }
        }
        else
        {
            _loadingInfo = false;
            emit durationChanged(_lastDuration);
            _lastDuration = 0;
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
        tearDownPlayer();
    }
}
