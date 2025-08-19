
#include "medialibrarycache.h"
#include "QThread"
#include "../handler/loghandler.h"
#include "../tool/xmath.h"

MediaLibraryCache::MediaLibraryCache()
{

}

void MediaLibraryCache::lockForWrite()
{
    // qDebug() << "[MediaLibraryCache::lockForWrite]: from thread: " << QThread::currentThreadId();
    m_mutex.lockForWrite();
}

void MediaLibraryCache::lockForRead()
{
    // qDebug() << "[MediaLibraryCache::lockForRead]: from thread: " << QThread::currentThreadId();
    m_mutex.lockForRead();
}

void MediaLibraryCache::unlock()
{
    // qDebug() << "[MediaLibraryCache::unlock]: from thread: " << QThread::currentThreadId();
    m_mutex.unlock();
}

int MediaLibraryCache::length() const
{
    return m_items.length();
}

int MediaLibraryCache::indexOf(const LibraryListItem27 &item) const
{
    return m_items.indexOf(item);
}

const LibraryListItem27& MediaLibraryCache::at(int i) const
{
    return m_items.at(i);
}

const QList<LibraryListItem27> *MediaLibraryCache::getItems() const
{
    return &m_items;
}

///
/// \brief MediaLibraryCache::value Call lock for read or write before calling this method.
/// \param i
/// \return
///
LibraryListItem27 *MediaLibraryCache::value(int i)
{
    if(1<0 || i>m_items.length())
        return 0;
    return &m_items[i];
}

LibraryListItem27 &MediaLibraryCache::valueRef(int i)
{
    return m_items[i];
}

// const QList<LibraryListItem27> *MediaLibraryCache::getItems()
// {
//     return &m_items;
// }

void MediaLibraryCache::append(const LibraryListItem27& item)
{
    const QWriteLocker locker(&m_mutex);
    m_items.append(item);
}

void MediaLibraryCache::remove(const LibraryListItem27& item)
{
    int index = findItemIndexByID(item.ID);
    const QWriteLocker locker(&m_mutex);
    if(index > -1)
        m_items.remove(index);
}

void MediaLibraryCache::update(const LibraryListItem27 &value)
{
    int i = findItemIndexByID(value.ID);
    const QWriteLocker locker(&m_mutex);
    if(i > -1) {
        m_items[i] = value;
    }
}

void MediaLibraryCache::prepend(const LibraryListItem27 &value)
{
    const QWriteLocker locker(&m_mutex);
    m_items.prepend(value);
}

void MediaLibraryCache::clear()
{
    const QWriteLocker locker(&m_mutex);
    m_items.clear();
}

float MediaLibraryCache::getProgress(const LibraryListItem27 &value)
{
    const QReadLocker locker(&m_mutex);
    return XMath::roundTwoDecimal(((float)indexOf(value)/length()) * 100.0);
}

LibraryListItem27* MediaLibraryCache::findItemByID(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.ID == value;
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
int MediaLibraryCache::findItemIndexByID(const QString &value)
{
    const QReadLocker locker(&m_mutex);

    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.ID == value;
    });
    if(itr != m_items.end())
        return itr - m_items.begin();
    return -1;
}
LibraryListItem27* MediaLibraryCache::findItemByNameNoExtension(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return !item.nameNoExtension.compare(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryCache::findItemByName(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return !item.name.compare(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}
LibraryListItem27* MediaLibraryCache::findItemByMediaPath(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return !item.path.compare(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryCache::findItemByPartialMediaPath(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.path.startsWith(value, Qt::CaseInsensitive) || item.path.endsWith(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryCache::findItemByThumbPath(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return !item.thumbFile.compare(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27* MediaLibraryCache::findItemByPartialThumbPath(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.thumbFile.startsWith(value, Qt::CaseInsensitive) || item.thumbFile.endsWith(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemBySubtitle(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return !item.metadata.subtitle.compare(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByPartialSubtitle(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.metadata.subtitle.startsWith(value, Qt::CaseInsensitive) || item.metadata.subtitle.endsWith(value, Qt::CaseInsensitive);
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByAltScript(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& script)
        {
            return script.type == ScriptType::ALTERNATE && !script.filename.compare(value, Qt::CaseInsensitive);
        });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByPartialAltScript(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        auto itr2 = std::find_if(item.metadata.scripts.begin(), item.metadata.scripts.end(), [value](const ScriptInfo& item)
        {
            return item.type == ScriptType::ALTERNATE && (item.filename.startsWith(value, Qt::CaseInsensitive) || item.filename.endsWith(value, Qt::CaseInsensitive));
        });
        return itr2 != item.metadata.scripts.end();
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByMetadataKey(const QString &value)
{
    const QReadLocker locker(&m_mutex);
    auto itr = std::find_if(m_items.begin(), m_items.end(), [value](const LibraryListItem27& item)
    {
        return item.metadata.key == value;
    });
    if(itr != m_items.end())
        return &m_items[itr - m_items.begin()];
    return 0;
}

LibraryListItem27 *MediaLibraryCache::findItemByReference(const LibraryListItem27 *value)
{
    auto item = findItemByID(value->ID);
    if(!item)
    {
        item = findItemByNameNoExtension(value->nameNoExtension);
    }
    if(!item)
    {
        item = findItemByMediaPath(value->path);
    }
    return item;
}
