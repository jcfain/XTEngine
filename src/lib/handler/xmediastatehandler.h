#ifndef XMEDIASTATEHANDLER_H
#define XMEDIASTATEHANDLER_H

#include "XTEngine_global.h"

#include <QObject>
#include "../struct/LibraryListItem27.h"
#include "medialibraryhandler.h"

class XTENGINE_EXPORT XMediaStateHandler// : public QObject
{
public:

    static void setMediaLibraryHandler(MediaLibraryHandler* libraryHandler);
    static bool isPlaying();
    static bool isExternal();
    //static void setPlaying(LibraryListItem27& playingItem, bool internal = true);
    static void setPlaying(const LibraryListItem27* playingItem, bool internal = true);
    static LibraryListItem27* getPlaying();
    static QString getPlayingID();
    static void stop();
    static void updateDuration(qint64 currentPos, qint64 duration);
    static void setPlaybackSpeed(qreal speed);
    static qreal getPlaybackSpeed();

//signals:

private:
    static MediaLibraryHandler* m_libraryHandler;
    static LibraryListItem27 m_playingItem;
    static bool m_isInternal;
    static qreal m_playbackSpeed;

    static void processMetaData();
};

#endif // XMEDIASTATEHANDLER_H
