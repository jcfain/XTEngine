#include "xmediastatehandler.h"
#include "settingshandler.h"

// XMediaStateHandler::XMediaStateHandler(MediaLibraryHandler* libraryHandler, QObject *parent):
//     m_libraryHandler(libraryHandler) {}

//XMediaStateHandler::~XMediaStateHandler()
//{

//}

void XMediaStateHandler::setMediaLibraryHandler(MediaLibraryHandler *libraryHandler)
{
    m_libraryHandler = libraryHandler;
}

bool XMediaStateHandler::isPlaying()
{
    return !m_playingItem.ID.isEmpty();
}

bool XMediaStateHandler::isExternal()
{
    return isPlaying() && !m_isInternal;
}

// void XMediaStateHandler::setPlaying(LibraryListItem27& playingItem, bool internal)
// {
//     m_playingItem = LibraryListItem27(playingItem);
//     m_playingItemRef = &playingItem;
//     m_isInternal = internal;
// }

void XMediaStateHandler::setPlaying(const LibraryListItem27* playingItem, bool internal)
{
    if(!playingItem->ID.isEmpty()) {
        m_playingItem = *playingItem;
        m_isInternal = internal;
        processMetaData();
    } else {
        stop();
    }
}

LibraryListItem27* XMediaStateHandler::getPlaying()
{
    if(!m_libraryHandler)
        return 0;
    return m_playingItem.ID.isEmpty() ? 0 : m_libraryHandler->findItemByID(m_playingItem.ID);
}

QString XMediaStateHandler::getPlayingID()
{
    return m_playingItem.ID;
}

void XMediaStateHandler::stop()
{
    m_playingItem.ID = "";
}

void XMediaStateHandler::updateDuration(qint64 currentPos, qint64 duration)
{
    auto mediaItem = getPlaying();
    if(!mediaItem)
        return;
    const qint64 timeLeft = duration - currentPos;
    const qint64 viewedThreshold = duration * SettingsHandler::getViewedThreshold();
    if(duration > 0 && currentPos > -1 && timeLeft < viewedThreshold)
    {
        if(!mediaItem->metadata.tags.contains(SettingsHandler::getXTags().VIEWED))
        {
            mediaItem->metadata.tags.removeAll(SettingsHandler::getXTags().UNVIEWED);
            mediaItem->metadata.tags.append(SettingsHandler::getXTags().VIEWED);
            SettingsHandler::updateLibraryListItemMetaData(m_playingItem);
        }
    }
}

void XMediaStateHandler::processMetaData()
{
    auto mediaItem = getPlaying();
    if(!mediaItem)
        return;
    SettingsHandler::setLiveOffset(mediaItem->metadata.offset);
}

LibraryListItem27 XMediaStateHandler::m_playingItem;
MediaLibraryHandler* XMediaStateHandler::m_libraryHandler = 0;
bool XMediaStateHandler::m_isInternal;
