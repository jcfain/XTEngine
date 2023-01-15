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

void XMediaStateHandler::processMetaData()
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(m_playingItem.path);
    SettingsHandler::setLiveOffset(libraryListItemMetaData.offset);
}

LibraryListItem27 XMediaStateHandler::m_playingItem;
bool XMediaStateHandler::m_isInternal;
