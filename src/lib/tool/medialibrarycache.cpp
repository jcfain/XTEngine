
#include "medialibrarycache.h"

MediaLibraryCache::MediaLibraryCache()
{

}

const QList<LibraryListItem27> *MediaLibraryCache::getItems()
{
    return &m_items;
}

void MediaLibraryCache::addItem(LibraryListItem27 item)
{

}

void MediaLibraryCache::removeItem(LibraryListItem27 item)
{

}

LibraryListItem27* MediaLibraryCache::findItemByID(QString id) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.ID == id)
    //            return &m_items[m_items.indexOf(item)];
    //    }
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [id](const LibraryListItem27& item) {
        return item.ID == id;
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
int MediaLibraryCache::findItemIndexByID(QString id) {
    //    int foundItem = -1;
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.ID == id)
    //            return m_items.indexOf(item);
    //    }
    const QMutexLocker locker(&m_mutex);

    auto itr = std::find_if(m_items.begin(), m_items.end(), [id](const LibraryListItem27& item) {
        return item.ID == id;
    });
    if(itr != m_items.end())
        return itr - m_items.begin();
    return -1;
}
LibraryListItem27* MediaLibraryCache::findItemByNameNoExtension(QString nameNoExtension) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.nameNoExtension == nameNoExtension)
    //            return &m_items[m_items.indexOf(item)];
    //    }

    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [nameNoExtension](const LibraryListItem27& item) {
        return !item.nameNoExtension.compare(nameNoExtension, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryCache::findItemByName(QString name) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.name == name)
    //            return &m_items[m_items.indexOf(item)];
    //    }
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [name](const LibraryListItem27& item) {
        return !item.name.compare(name, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryCache::findItemByMediaPath(QString mediaPath) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.path == mediaPath)
    //            return &m_items[m_items.indexOf(item)];
    //    }
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [mediaPath](const LibraryListItem27& item) {
        return !item.path.compare(mediaPath, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryCache::findItemByPartialMediaPath(QString partialMediaPath) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.path.startsWith(partialMediaPath))
    //            return &m_items[m_items.indexOf(item)];
    //        else if(item.path.endsWith(partialMediaPath))
    //            return &m_items[m_items.indexOf(item)];
    //    }
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [partialMediaPath](const LibraryListItem27& item) {
        return item.path.startsWith(partialMediaPath, Qt::CaseInsensitive) || item.path.endsWith(partialMediaPath, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryCache::findItemByThumbPath(QString thumbPath) {

    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [thumbPath](const LibraryListItem27& item) {
        return !item.thumbFile.compare(thumbPath, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryCache::findItemByPartialThumbPath(QString partialThumbPath) {
    //    foreach (LibraryListItem27 item, m_items) {
    //        if(item.thumbFile.startsWith(partialThumbPath) || item.thumbFile.endsWith(partialThumbPath))
    //            return &m_items[m_items.indexOf(item)];
    //    }
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [partialThumbPath](const LibraryListItem27& item) {
        return item.thumbFile.startsWith(partialThumbPath, Qt::CaseInsensitive) || item.thumbFile.endsWith(partialThumbPath, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;

}

LibraryListItem27 *MediaLibraryCache::findItemBySubtitle(QString subtitle)
{
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [subtitle](const LibraryListItem27& item) {
        return !item.metadata.subtitle.compare(subtitle, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByPartialSubtitle(QString partialSubtitle)
{
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [partialSubtitle](const LibraryListItem27& item) {
        return item.metadata.subtitle.startsWith(partialSubtitle, Qt::CaseInsensitive) || item.metadata.subtitle.endsWith(partialSubtitle, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByAltScript(QString value)
{
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item) {
        auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& script) {
            return script.type == ScriptType::ALTERNATE && !script.filename.compare(value, Qt::CaseInsensitive);
        });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByPartialAltScript(QString value)
{
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item) {
        auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& item) {
            return item.type == ScriptType::ALTERNATE && (item.filename.startsWith(value, Qt::CaseInsensitive) || item.filename.endsWith(value, Qt::CaseInsensitive));
        });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByMetadataKey(QString value)
{
    const QMutexLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item) {
        return item.metadata.key == value;
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByReference(const LibraryListItem27 *playListItem)
{
    auto item = findItemByID(playListItem->ID);
    if(!item) {
        item = findItemByNameNoExtension(playListItem->nameNoExtension);
    }
    if(!item) {
        item = findItemByMediaPath(playListItem->path);
    }
    return item;
}
