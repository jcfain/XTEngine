#include "mediaformat.h"

#include <QMediaMetaData>

MediaFormat::MediaFormat(QMediaPlayer* mediaPlayer, QObject *parent)
    : QObject{parent},
    m_mediaPlayer(mediaPlayer)
{
    // connect(m_mediaPlayer, &QMediaPlayer::mediaStatusChanged,
    //         this, [this](QMediaPlayer::MediaStatus status) {
    //             if(status == QMediaPlayer::LoadedMedia) {
    //                 checkCodec();
    //             }
    //         });
}

bool MediaFormat::isCodecSupported(QString& error)
{
    const QMediaMetaData fileMetadata = m_mediaPlayer->metaData();
    const QMediaFormat::AudioCodec fileAudioCodec = fileMetadata.value(QMediaMetaData::AudioCodec).value<QMediaFormat::AudioCodec>();
    if(!isCodecSupported(fileAudioCodec))
    {
        error += tr("Audio %1 (%2) is not supported by your system. ")
                    .arg(QMediaFormat::audioCodecName(fileAudioCodec),
                         QMediaFormat::audioCodecDescription(fileAudioCodec));
    }
    const QMediaFormat::VideoCodec fileVideoCodec = fileMetadata.value(QMediaMetaData::VideoCodec).value<QMediaFormat::VideoCodec>();
    if(!isCodecSupported(fileVideoCodec))
    {
        error += tr("Video %1 (%2) is not supported by your system. ")
                             .arg(QMediaFormat::videoCodecName(fileVideoCodec),
                                  QMediaFormat::videoCodecDescription(fileVideoCodec));
    }
    if(!error.isEmpty()) {
        emit unsupported(error);
        return false;
    }
    return true;
}

bool MediaFormat::isCodecSupported(QMediaFormat::AudioCodec fileCodec) const
{
    // get supported audio codecs list
    QMediaFormat mediaFormat;
    const QList<QMediaFormat::AudioCodec> supportedAudioCodecs =  mediaFormat.supportedAudioCodecs(QMediaFormat::Decode);

    // check if file codec is supported by the system
    return std::any_of(supportedAudioCodecs.constBegin(), supportedAudioCodecs.constEnd(), [&](QMediaFormat::AudioCodec systemCodec) {
        return fileCodec == systemCodec;
    });
}

bool MediaFormat::isCodecSupported(QMediaFormat::VideoCodec fileCodec) const
{
    QMediaFormat mediaFormat;
    const QList<QMediaFormat::VideoCodec> supportedVideoCodecs =  mediaFormat.supportedVideoCodecs(QMediaFormat::Decode);

    // check if file codec is supported by the system
    return std::any_of(supportedVideoCodecs.constBegin(), supportedVideoCodecs.constEnd(), [&](QMediaFormat::VideoCodec systemCodec) {
        return fileCodec == systemCodec;
    });
}
