
#ifndef MEDIALIBRARYCACHE_H
#define MEDIALIBRARYCACHE_H

#include <QMutex>

#include "../struct/LibraryListItem27.h"



class MediaLibraryCache
{
public:
    MediaLibraryCache();

    const QList<LibraryListItem27>* getItems();
    void addItem(LibraryListItem27 item);
    void removeItem(LibraryListItem27 item);
    LibraryListItem27* getItem(int index);
    LibraryListItem27* findItemByID(QString id);
    LibraryListItem27* findItemByNameNoExtension(QString nameNoExtension);
    LibraryListItem27* findItemByName(QString name);
    LibraryListItem27* findItemByMediaPath(QString mediaPath);
    LibraryListItem27* findItemByPartialMediaPath(QString partialMediaPath);
    LibraryListItem27* findItemByThumbPath(QString thumbPath);
    LibraryListItem27* findItemByPartialThumbPath(QString partialThumbPath);
    LibraryListItem27* findItemBySubtitle(QString subtitle);
    LibraryListItem27* findItemByPartialSubtitle(QString partialSubtitle);
    LibraryListItem27* findItemByAltScript(QString value);
    LibraryListItem27* findItemByPartialAltScript(QString value);
    LibraryListItem27* findItemByMetadataKey(QString value);
    LibraryListItem27* findItemByReference(const LibraryListItem27* playListItem);
    int findItemIndexByID(QString id);

    void clear();
private:
    QList<LibraryListItem27> m_items;
    QMutex m_mutex;
};

#endif // MEDIALIBRARYCACHE_H
