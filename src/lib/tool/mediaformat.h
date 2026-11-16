#ifndef MEDIAFORMAT_H
#define MEDIAFORMAT_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaFormat>
#include <QString>
#include <QMap>

// Result of probing the FFmpeg library used by the Qt FFmpeg media backend.
struct CodecProbeResult
{
    bool ffmpegAvailable = false; // libavcodec found and loaded
    QString ffmpegVersion;        // e.g. "9.0.1"
    QString libraryPath;          // the library that was loaded
    QMap<QString, bool> codecs;   // "AV1" -> true, "HEVC" -> false, ...
};

class MediaFormat : public QObject
{
    Q_OBJECT
signals:
    void unsupported(QString message);
public:
    explicit MediaFormat(QMediaPlayer* mediaPlayer, QObject *parent = nullptr);
    bool isCodecSupported(QString& error);
    // Probes the FFmpeg library used by the Qt FFmpeg media backend and
    // reports which decoders are available. The app itself never links
    // FFmpeg - only the Qt plugin does - so this loads the same library
    // the plugin resolves (dlopen / LoadLibrary) and asks it directly.
    // Returns ffmpegAvailable=false when no FFmpeg library can be found
    // (e.g. the OS media backend is in use).
    static CodecProbeResult probeFFmpegCodecs();

private:
    QMediaPlayer* m_mediaPlayer;
    bool isCodecSupported(QMediaFormat::AudioCodec fileCodec) const;
    bool isCodecSupported(QMediaFormat::VideoCodec fileCodec) const;
};

#endif // MEDIAFORMAT_H
