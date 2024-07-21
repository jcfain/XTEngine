#include "scheduler.h"

#include "settingshandler.h"

Scheduler::Scheduler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent)
    : QObject{parent},
    m_mediaLibraryHandler(mediaLibraryHandler),
    m_lastRun(QDate::currentDate())
{}

void Scheduler::startLibraryLoadSchedule()
{
    connect(&m_timer, &QTimer::timeout, this, &Scheduler::checkLibraryLoadTime);
    m_timer.start(60000);
}

void Scheduler::stopLibraryLoadSchedule()
{
    disconnect(&m_timer, &QTimer::timeout, this, &Scheduler::checkLibraryLoadTime);
    m_timer.stop();
}

void Scheduler::checkLibraryLoadTime()
{
    if(m_lastRun < QDate::currentDate() &&  QTime::currentTime() >= SettingsHandler::scheduleLibraryLoadTime())
    {
        m_lastRun = QDate::currentDate();
        if(m_mediaLibraryHandler)
        {
            SettingsHandler::setForceMetaDataFullProcess(SettingsHandler::scheduleLibraryLoadFullProcess());
            m_mediaLibraryHandler->loadLibraryAsync();
        }
    }
}
