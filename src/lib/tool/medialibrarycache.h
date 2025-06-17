
#ifndef MEDIALIBRARYCACHE_H
#define MEDIALIBRARYCACHE_H

#include <QReadWriteLock>

#include "../struct/LibraryListItem27.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT MediaLibraryCache
{
public:
    MediaLibraryCache();
    void lockForRead();
    void lockForWrite();
    void unlock();

    // const QList<LibraryListItem27>* getItems();
    // QList<LibraryListItem27> copyItems();
    int length() const;
    int indexOf(const LibraryListItem27& item) const;
    const LibraryListItem27& at(int i) const;
    const QList<LibraryListItem27>* getItems() const;
    LibraryListItem27* value(int i);
    LibraryListItem27& valueRef(int i);
    void append(const LibraryListItem27 &value);
    void remove(const LibraryListItem27 &value);
    void update(const LibraryListItem27 &value);
    void prepend(const LibraryListItem27 &value);
    void clear();
    float getProgress(const LibraryListItem27 &value);
    LibraryListItem27* findItemByID(const QString &value);
    LibraryListItem27* findItemByNameNoExtension(const QString &value);
    LibraryListItem27* findItemByName(const QString &value);
    LibraryListItem27* findItemByMediaPath(const QString &value);
    LibraryListItem27* findItemByPartialMediaPath(const QString &value);
    LibraryListItem27* findItemByThumbPath(const QString &value);
    LibraryListItem27* findItemByPartialThumbPath(const QString &value);
    LibraryListItem27* findItemBySubtitle(const QString &value);
    LibraryListItem27* findItemByPartialSubtitle(const QString &value);
    LibraryListItem27* findItemByAltScript(const QString &value);
    LibraryListItem27* findItemByPartialAltScript(const QString &value);
    LibraryListItem27* findItemByMetadataKey(const QString &value);
    LibraryListItem27* findItemByReference(const LibraryListItem27* value);
    int findItemIndexByID(const QString &value);

private:
    QList<LibraryListItem27> m_items;
    // QMutex m_mutex;
    QReadWriteLock m_mutex;
};

#endif // MEDIALIBRARYCACHE_H
