#ifndef MEDIAFORMAT_H
#define MEDIAFORMAT_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaFormat>

class MediaFormat : public QObject
{
    Q_OBJECT
signals:
    void unsupported(QString message);
public:
    explicit MediaFormat(QMediaPlayer* mediaPlayer, QObject *parent = nullptr);
    bool isCodecSupported(QString& error);

private:
    QMediaPlayer* m_mediaPlayer;
    bool isCodecSupported(QMediaFormat::AudioCodec fileCodec) const;
    bool isCodecSupported(QMediaFormat::VideoCodec fileCodec) const;
};

#endif // MEDIAFORMAT_H
