#ifndef XMEDIASTATEHANDLER_H
#define XMEDIASTATEHANDLER_H

#include "XTEngine_global.h"

#include <QObject>
#include "../struct/LibraryListItem27.h"

class XTENGINE_EXPORT XMediaStateHandler// : public QObject
{
    //Q_OBJECT
public:
//    explicit XMediaStateHandler(QObject *parent = nullptr);
//    ~XMediaStateHandler();

    static bool isPlaying();
    static bool isExternal();
    static void setPlaying(LibraryListItem27 playingItem, bool internal = true);
    static void setPlaying(LibraryListItem27* playingItem, bool internal = true);
    static LibraryListItem27 getPlaying();
    static void stop();

//signals:

private:
    static LibraryListItem27 m_playingItem;
    static bool m_isInternal;

    static void processMetaData();
};

#endif // XMEDIASTATEHANDLER_H
