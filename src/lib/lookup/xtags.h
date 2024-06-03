#ifndef XTAGS_H
#define XTAGS_H

#include <QString>
#include <QStringList>

class XTags {
public:
    const QString MFS = "mfs";
    const QString VR = "vr";
    const QString VIDEO_2D = "2d";
    const QString AUDIO = "audio";
    const QString FUNSCRIPT = "funscript";
    const QString PLAYLIST_INTERNAL = "playlistInternal";
    const QString SUBTITLE = "subtitle";
    const QString MISSING_SCRIPT = "missingScript";
    const QStringList builtin = {
        MFS,
        VR,
        VIDEO_2D,
        AUDIO,
        FUNSCRIPT,
        PLAYLIST_INTERNAL,
        SUBTITLE,
        MISSING_SCRIPT
    };
    QStringList userTags;
};

#endif // XTAGS_H
