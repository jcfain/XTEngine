#ifndef THUMBEXTRACTOR_H
#define THUMBEXTRACTOR_H

#include <QObject>
#include <QMediaPlayer>
#include <QVideoSink>
#include <QVideoFrame>
#include <QImage>
#include <QTimer>

#include "XTEngine_global.h"

class XTENGINE_EXPORT ThumbExtractor : public QObject
{
    Q_OBJECT
public:
    explicit ThumbExtractor(QObject *parent = nullptr);

    QImage extract(QString videoPath, qint64 time = -1, qint64 timeout = 5000);
    void extractDebounce(QString videoPath, qint64 time = -1, qint64 timeout = 5000);
    QString lastError();

signals:
    void startPlaying();
    void stopPlaying();
    void pausePlaying();
    void setPosition(qint64 value);
    void frameExtracted(QImage frame);
    void frameExtractionError(QString error);

private:

    bool m_extracting = false;
    bool m_fileChanged = false;
    QString m_file;
    QMediaPlayer* m_mediaPlayer;
    QVideoSink m_videoSink;
    QImage m_lastImage;
    QString m_lastError;
    qint64 m_lastTime;
    qint64 m_lastTimeout;
    qint64 m_lastDuration = -1;
    QTimer m_debouncer;
    void videoFrameChanged(const QVideoFrame &thumb);
    void on_mediaError(QMediaPlayer::Error error, const QString &errorString);
};

#endif // THUMBEXTRACTOR_H
