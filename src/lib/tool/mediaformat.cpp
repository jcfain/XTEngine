#include "mediaformat.h"

#include <QMediaMetaData>
#include <QFileInfo>
#include <QStringList>

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <dlfcn.h>
#endif

// FFmpeg codec IDs (libavcodec/codec_id.h). These enum values are stable
// across FFmpeg versions.
static const struct { const char* name; int id; } kProbeCodecs[] = {
    {"H264", 27},     // AV_CODEC_ID_H264
    {"VP8", 139},     // AV_CODEC_ID_VP8
    {"VP9", 166},     // AV_CODEC_ID_VP9
    {"HEVC", 172},    // AV_CODEC_ID_HEVC
    {"AV1", 222},     // AV_CODEC_ID_AV1
    {"MP3", 86017},   // AV_CODEC_ID_MP3
    {"AAC", 86018},   // AV_CODEC_ID_AAC
    {"FLAC", 86028},  // AV_CODEC_ID_FLAC
    {"OPUS", 86076},  // AV_CODEC_ID_OPUS
};

CodecProbeResult MediaFormat::probeFFmpegCodecs()
{
    CodecProbeResult result;

    // Candidate library names, newest SONAME first. The Qt 6.12 FFmpeg
    // plugin is linked against FFmpeg 9.0.x (libavcodec.so.63 /
    // avcodec-63.dll / avcodec.63.dylib).
    QStringList candidates;
#ifdef Q_OS_WIN
    candidates << "avcodec-63.dll" << "avcodec-62.dll" << "avcodec-61.dll" << "avcodec-60.dll";
#elif defined(Q_OS_MACOS)
    candidates << "avcodec.63.dylib" << "avcodec.62.dylib" << "avcodec.61.dylib" << "avcodec.60.dylib";
#else
    candidates << "libavcodec.so.63" << "libavcodec.so.62" << "libavcodec.so.61" << "libavcodec.so.60";
#endif

    void* handle = nullptr;
    // Resolution must mirror the FFmpeg plugin's own search order:
    // LD_LIBRARY_PATH first, then the plugin's RUNPATH (the Qt lib dir),
    // and only then the system paths. So for each candidate (newest
    // SONAME first) we try the plain name, then the Qt lib dir.
    QStringList qtLibCandidates;
#ifndef Q_OS_WIN
    {
        Dl_info info;
        if (dladdr(reinterpret_cast<void*>(&qVersion), &info) && info.dli_fname)
        {
            const QString qtLibDir = QFileInfo(QString::fromLocal8Bit(info.dli_fname)).absolutePath();
            for (const QString& name : candidates)
                qtLibCandidates << qtLibDir + "/" + name;
        }
    }
#endif

    for (int i = 0; i < candidates.size() && !handle; ++i)
    {
#ifdef Q_OS_WIN
        handle = LoadLibraryExW(candidates[i].toStdWString().c_str(), nullptr, 0);
        if (handle)
            result.libraryPath = candidates[i];
#else
        // 1) LD_LIBRARY_PATH first - this is what the launcher sets for the
        //    codec pack, and it takes precedence in the plugin's own search
        //    order. (A plain dlopen would miss it if this library carries an
        //    old-style DT_RPATH, which is searched before LD_LIBRARY_PATH.)
        const QStringList ldDirs = QString::fromLocal8Bit(qgetenv("LD_LIBRARY_PATH")).split(':', Qt::SkipEmptyParts);
        for (const QString& dir : ldDirs)
        {
            const QString full = dir + "/" + candidates[i];
            handle = dlopen(full.toUtf8().constData(), RTLD_LAZY);
            if (handle)
            {
                result.libraryPath = full;
                break;
            }
        }
        if (!handle)
        {
            handle = dlopen(candidates[i].toUtf8().constData(), RTLD_LAZY);
            if (handle)
                result.libraryPath = candidates[i];
            else if (i < qtLibCandidates.size())
            {
                handle = dlopen(qtLibCandidates[i].toUtf8().constData(), RTLD_LAZY);
                if (handle)
                    result.libraryPath = qtLibCandidates[i];
            }
        }
#endif
    }

    if (!handle)
        return result; // no FFmpeg library reachable (OS backend in use?)

    result.ffmpegAvailable = true;

    // int avcodec_version(void);
    typedef int (*avcodec_version_fn)();
    // const AVCodec* avcodec_find_decoder(enum AVCodecID id);
    typedef void* (*avcodec_find_decoder_fn)(int);

    avcodec_version_fn version = nullptr;
    avcodec_find_decoder_fn findDecoder = nullptr;
#ifdef Q_OS_WIN
    version = reinterpret_cast<avcodec_version_fn>(GetProcAddress(static_cast<HMODULE>(handle), "avcodec_version"));
    findDecoder = reinterpret_cast<avcodec_find_decoder_fn>(GetProcAddress(static_cast<HMODULE>(handle), "avcodec_find_decoder"));
#else
    version = reinterpret_cast<avcodec_version_fn>(dlsym(handle, "avcodec_version"));
    findDecoder = reinterpret_cast<avcodec_find_decoder_fn>(dlsym(handle, "avcodec_find_decoder"));
#endif

    if (version)
    {
        const int v = version();
        result.ffmpegVersion = QString("%1.%2.%3")
            .arg(v >> 16).arg((v >> 8) & 0xFF).arg(v & 0xFF);
    }

    if (findDecoder)
    {
        for (const auto& codec : kProbeCodecs)
            result.codecs[codec.name] = (findDecoder(codec.id) != nullptr);
    }

    // AV1 special case: FFmpeg's built-in native AV1 decoder exists in
    // every build, but it only works with hardware acceleration. A working
    // software decoder requires dav1d or libaom to be linked in, so check
    // for those directly (dlsym searches the library's dependency tree).
#ifdef Q_OS_WIN
    result.codecs["AV1"] = (LoadLibraryExW(L"dav1d.dll", nullptr, 0) != nullptr)
                        || (LoadLibraryExW(L"aom.dll", nullptr, 0) != nullptr);
#else
    result.codecs["AV1"] = (dlsym(handle, "dav1d_close") != nullptr)
                        || (dlsym(handle, "aom_codec_init") != nullptr);
#endif

    // --- Hardware acceleration backends ---
    // Report which HW device types this FFmpeg build is compiled with. This
    // reflects the system's actual capability (not the user's preference), so
    // the UI/log can show the true state. We use av_hwdevice_iterate_types
    // (cheap, no device init) rather than creating device contexts (which can
    // be slow, e.g. CUDA).
    //
    // Note: per-codec HW support (H.264/HEVC/AV1) is not queryable here -
    // av_hwdevice_get_type_support does not exist in this FFmpeg build - so the
    // per-codec flags are left at their default (false). The backend is still
    // listed; the user can force it and the GPU decides at decode time.
    typedef int (*hw_iterate_fn)(int);
    typedef const char* (*hw_name_fn)(int);
    hw_iterate_fn hwIterate = nullptr;
    hw_name_fn hwName = nullptr;
#ifdef Q_OS_WIN
    hwIterate = reinterpret_cast<hw_iterate_fn>(GetProcAddress(static_cast<HMODULE>(handle), "av_hwdevice_iterate_types"));
    hwName = reinterpret_cast<hw_name_fn>(GetProcAddress(static_cast<HMODULE>(handle), "av_hwdevice_get_type_name"));
#else
    hwIterate = reinterpret_cast<hw_iterate_fn>(dlsym(handle, "av_hwdevice_iterate_types"));
    hwName = reinterpret_cast<hw_name_fn>(dlsym(handle, "av_hwdevice_get_type_name"));
#endif
    if (hwIterate && hwName)
    {
        int type = -1; // AV_HWDEVICE_TYPE_NONE
        int count = 0;
        // Safety: av_hwdevice_iterate_types takes the current type BY VALUE and
        // returns the next one (matching the ffmpeg CLI's show_hwaccels()). In
        // this FFmpeg build the iterator does not terminate cleanly (it loops),
        // so we stop on a repeated backend name AND cap the iteration count to
        // guarantee startup can never hang.
        while ((type = hwIterate(type)) >= 0 && count < 32)
        {
            const char* n = hwName(type);
            if (n && *n)
            {
                const QString name = QString::fromUtf8(n);
                bool seen = false;
                for (const HwAccelBackend& b : result.hwAccel)
                    if (b.name == name) { seen = true; break; }
                if (seen)
                    break; // iterator is looping - stop
                HwAccelBackend backend;
                backend.name = name;
                result.hwAccel.append(backend);
            }
            count++;
        }
    }

    // Keep the library loaded: the FFmpeg plugin uses the same copy.
    return result;
}

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
    if(!supportedAudioCodecs.isEmpty())
    {
        // Backend implements QMediaFormat queries (e.g. GStreamer)
        return std::any_of(supportedAudioCodecs.constBegin(), supportedAudioCodecs.constEnd(), [&](QMediaFormat::AudioCodec systemCodec) {
            return fileCodec == systemCodec;
        });
    }

    // FFmpeg backend: QMediaFormat reports nothing, so probe the FFmpeg
    // library the plugin uses directly.
    const CodecProbeResult probe = probeFFmpegCodecs();
    if(!probe.ffmpegAvailable)
        return true; // no FFmpeg backend in use - don't report unsupported
    QString name;
    switch (fileCodec) {
    case QMediaFormat::AudioCodec::MP3:  name = "MP3"; break;
    case QMediaFormat::AudioCodec::AAC:  name = "AAC"; break;
    case QMediaFormat::AudioCodec::FLAC: name = "FLAC"; break;
    case QMediaFormat::AudioCodec::Opus: name = "OPUS"; break;
    default: return true; // unknown codec - let the backend decide
    }
    return probe.codecs.value(name, true);
}

bool MediaFormat::isCodecSupported(QMediaFormat::VideoCodec fileCodec) const
{
    QMediaFormat mediaFormat;
    const QList<QMediaFormat::VideoCodec> supportedVideoCodecs =  mediaFormat.supportedVideoCodecs(QMediaFormat::Decode);
    if(!supportedVideoCodecs.isEmpty())
    {
        // Backend implements QMediaFormat queries (e.g. GStreamer)
        return std::any_of(supportedVideoCodecs.constBegin(), supportedVideoCodecs.constEnd(), [&](QMediaFormat::VideoCodec systemCodec) {
            return fileCodec == systemCodec;
        });
    }

    // FFmpeg backend: QMediaFormat reports nothing, so probe the FFmpeg
    // library the plugin uses directly.
    const CodecProbeResult probe = probeFFmpegCodecs();
    if(!probe.ffmpegAvailable)
        return true; // no FFmpeg backend in use - don't report unsupported
    QString name;
    switch (fileCodec) {
    case QMediaFormat::VideoCodec::H264: name = "H264"; break;
    case QMediaFormat::VideoCodec::H265: name = "HEVC"; break;
    case QMediaFormat::VideoCodec::VP8:  name = "VP8"; break;
    case QMediaFormat::VideoCodec::VP9:  name = "VP9"; break;
    case QMediaFormat::VideoCodec::AV1:  name = "AV1"; break;
    default: return true; // unknown codec - let the backend decide
    }
    return probe.codecs.value(name, true);
}
