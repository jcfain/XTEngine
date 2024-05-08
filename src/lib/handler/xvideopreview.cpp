#include "xvideopreview.h"
#include <QProcess>
#include <QCoreApplication>
#include <QFileInfo>

XVideoPreview::XVideoPreview(QObject* parent) : QObject(parent), _thumbNailVideoSurface(0), _thumbPlayer(0)
{
    LogHandler::Debug("XVideoPreview");
    _thumbPlayer = new QMediaPlayer(this, QMediaPlayer::VideoSurface);
    _thumbNailVideoSurface = new XVideoSurface(_thumbPlayer);
    _thumbPlayer->setVideoOutput(_thumbNailVideoSurface);
    _thumbPlayer->setMuted(true);
    m_debouncer.setSingleShot(true);
    connect(_thumbPlayer, &QMediaPlayer::stateChanged, this, &XVideoPreview::on_mediaStateChange);
    connect(_thumbPlayer, &QMediaPlayer::mediaStatusChanged, this, &XVideoPreview::on_mediaStatusChanged);
    connect(_thumbNailVideoSurface, &XVideoSurface::frameCapture, this, &XVideoPreview::on_thumbCapture);
    connect(_thumbNailVideoSurface, &XVideoSurface::frameCaptureError, this, &XVideoPreview::on_thumbError);
    connect(_thumbPlayer, &QMediaPlayer::durationChanged, this, &XVideoPreview::on_durationChanged);
    connect(&m_debouncer, &QTimer::timeout, this, [this]() {
        LogHandler::Debug("Extract debounce");
        extract();
    });

//    m_ffmpeg = new QProcess(this);
//    m_ffmprobe = new QProcess(this);
//    connect(m_ffmpeg, &QProcess::readyReadStandardOutput, this, &XVideoPreview::onFFMpegThumbSave);
//    connect(m_ffmprobe, &QProcess::readyReadStandardOutput, this, &XVideoPreview::onFFMpegDuration);
//    connect(m_ffmprobe, &QProcess::readyReadStandardError, this, &XVideoPreview::onFFMpegDuration);
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
//        if(_thumbNailVideoSurface->isActive())
//            _thumbNailVideoSurface->stop();

    stop();

        //_thumbPlayer->setMedia(QMediaContent());
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
    QUrl mediaUrl = QUrl::fromLocalFile(_file);
    QMediaContent mc(mediaUrl);
    _thumbPlayer->setMedia(mc);
//    bool isFFmpeg = checkForFFMpeg();
    if(_time > -1)
    {
        _loadingInfo = false;
        _extracting = true;
//        if(!isFFmpeg)
            _thumbPlayer->setPosition(_time);
    }
//    if(!isFFmpeg)
        _thumbPlayer->play();
//    else
//        getDurationFromFFmpeg();
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
    if(_thumbPlayer->state() == QMediaPlayer::PlayingState)
        _thumbPlayer->stop();
    _thumbPlayer->setMedia(QMediaContent());
}

// Private
void XVideoPreview::on_thumbCapture(QImage frame)
{
    if(_extracting) {
        LogHandler::Debug("on_thumbCapture: "+ _file);
        _lastImage = frame;
        frame = QImage();
        //tearDownPlayer();
        process();
    }
}

void XVideoPreview::on_thumbError(QString error)
{
    if(_extracting) {
        LogHandler::Debug("on_thumbError: "+ _file);
        _lastError = error;
        //tearDownPlayer();
        process();
    }
}

void XVideoPreview::on_mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if(status == QMediaPlayer::MediaStatus::LoadedMedia) {
//        if(_thumbNailVideoSurface && !_thumbNailVideoSurface->isActive() && !_loadingInfo)
//            _thumbNailVideoSurface->start(m_format);
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
        process();

    }
}

void XVideoPreview::process() {
    if(!m_processed)
    {
        if(_extracting) {
            _extracting = false;
            if(!_lastError.isNull()) {
                m_processed = true;
                emit frameExtractionError(_lastError);
                _lastError.clear();
            }
            else if(!_lastImage.isNull()) {
                m_processed = true;
                emit frameExtracted(_lastImage);
                //_lastImage = QImage();
            }
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

//bool XVideoPreview::checkForFFMpeg()
//{
//    if(QFileInfo::exists(QCoreApplication::applicationDirPath() + "/ffmpeg")) {
//        return true;
//    }
//    QProcess program;
//    QStringList environment = program.systemEnvironment();
//    program.start("ffmpeg", QStringList() << "-h");
//    bool started = program.waitForStarted();
//    if (!program.waitForFinished(10000)) // 10 Second timeout
//        program.kill();

//    int exitCode = program.exitCode();
//    QString stdOutput = program.readAllStandardOutput();
//    if(!stdOutput.isEmpty())
//        LogHandler::Debug("ffmpeg detect: "+ stdOutput);
//    QString stdError = program.readAllStandardError();
//    if(!stdError.isEmpty())
//        LogHandler::Error("ffmpeg detect: "+ stdError);
//    if(exitCode > 0)
//        return false;
//    return started;
//}

//void XVideoPreview::getDurationFromFFmpeg()
//{
//    m_ffmprobe->start("ffprobe", QStringList() << _file);
////    bool started = program.waitForStarted();
////    if (!program.waitForFinished(10000)) // 10 Second timeout
////        program.kill();

//}

//void XVideoPreview::onFFMpegDuration()
//{
//    int exitCode = m_ffmprobe->exitCode();
//    QString stdOutput = m_ffmprobe->readAllStandardOutput();
//    QString stdError = m_ffmprobe->readAllStandardError();
//    if(exitCode == 0) {
//        if(!stdOutput.isEmpty() || !stdError.isEmpty()) {
//            QString message = !stdOutput.isEmpty() ? stdOutput : stdError;
//            LogHandler::Debug("FFMprobe output: " + message);
//            QRegExp durationRegex("Duration: (?:[01]\\d|2[0-3]):(?:[0-5]\\d):(?:[0-5]\\d).(?:[0-5]\\d)");
//            durationRegex.indexIn(message);
//            QStringList list = durationRegex.capturedTexts();
//            if(list.isEmpty()) {
//                emit frameExtractionError("Unknown error");
//                return;
//            }
//            QString durationFormat = list.first().remove("Duration: ");
//            QTime durationTime = QTime::fromString(durationFormat, "hh:mm:ss.zz");
//            qint64 duration = durationTime.msec();
//            emit durationChanged(duration);
//        } else {
//            emit frameExtractionError("Unknown error");
//        }
//    } else {
//        if(!stdError.isEmpty()) {
//            LogHandler::Error(stdError);
//            emit frameExtractionError(stdError);
//        } else {
//            emit frameExtractionError("Unknown error");
//        }
//    }
//}

//void XVideoPreview::getThumbFromFFmpeg()
//{

//}

//void XVideoPreview::onFFMpegThumbSave()
//{

//}
