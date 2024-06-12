#include "xmediastatehandler.h"
#include "settingshandler.h"

//XMediaStateHandler::XMediaStateHandler(QObject *parent)
//    : QObject{parent}
//{

//}

//XMediaStateHandler::~XMediaStateHandler()
//{

//}

bool XMediaStateHandler::isPlaying()
{
    return !m_playingItem.ID.isEmpty();
}

bool XMediaStateHandler::isExternal()
{
    return isPlaying() && m_isInternal;
}

void XMediaStateHandler::setPlaying(LibraryListItem27 playingItem, bool internal)
{
    m_playingItem = LibraryListItem27(playingItem);
    m_isInternal = internal;
}

void XMediaStateHandler::setPlaying(LibraryListItem27* playingItem, bool internal)
{
    if(playingItem) {
        m_playingItem = LibraryListItem27(playingItem);
        m_isInternal = internal;
        processMetaData();
    } else {
        stop();
    }
}

LibraryListItem27 XMediaStateHandler::getPlaying()
{
    return m_playingItem;
}

void XMediaStateHandler::stop()
{
    m_playingItem = LibraryListItem27();
}

void XMediaStateHandler::updateDuration(qint64 currentPos, qint64 duration)
{
    const qint64 timeLeft = duration - currentPos;
    const qint64 viewedThreshold = duration * SettingsHandler::viewedThreshold();
    if(duration > 0 && currentPos > -1 && timeLeft < viewedThreshold)
    {
        auto metadata = SettingsHandler::getLibraryListItemMetaData(m_playingItem);
        if(!metadata.tags.contains(SettingsHandler::getXTags().VIEWED))
        {
            metadata.tags.removeAll(SettingsHandler::getXTags().UNVIEWED);
            metadata.tags.append(SettingsHandler::getXTags().VIEWED);
            SettingsHandler::updateLibraryListItemMetaData(metadata);
        }
    }
}

void XMediaStateHandler::processMetaData()
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(m_playingItem);
    SettingsHandler::setLiveOffset(libraryListItemMetaData.offset);
}

LibraryListItem27 XMediaStateHandler::m_playingItem;
bool XMediaStateHandler::m_isInternal;
