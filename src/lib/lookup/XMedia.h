#ifndef XMEDIA_H
#define XMEDIA_H
enum class XMediaStatus {
    LoadingMedia,
    LoadedMedia,
    NoMedia,
    BufferingMedia,
    BufferedMedia,
#if BUILD_QT5
    UnknownMediaStatus,
#endif
    StalledMedia,
    InvalidMedia,
    EndOfMedia

};
enum class XMediaState {
    Paused,
    Playing,
    Stopped,
};

#endif // XMEDIA_H
