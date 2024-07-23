#include "scheduler.h"

#include "settingshandler.h"

Scheduler::Scheduler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent)
    : QObject{parent},
    m_mediaLibraryHandler(mediaLibraryHandler)
{}

void Scheduler::startLibraryLoadSchedule()
{
    runTimeChange(SettingsHandler::scheduleLibraryLoadTime());
    connect(&m_timer, &QTimer::timeout, this, &Scheduler::checkLibraryLoadTime);
    m_timer.start(60000);
}

void Scheduler::stopLibraryLoadSchedule()
{
    disconnect(&m_timer, &QTimer::timeout, this, &Scheduler::checkLibraryLoadTime);
    m_timer.stop();
}

void Scheduler::runTimeChange(QTime time)
{
    // If the current time has already past the scheduled time today. Dont run until the next day
    int lastRunDayOffset = QTime::currentTime() >= time ? 0 : -1;
    m_lastRun = QDate::currentDate().addDays(lastRunDayOffset);
}

void Scheduler::checkLibraryLoadTime()
{
    if(m_lastRun < QDate::currentDate() && QTime::currentTime() >= SettingsHandler::scheduleLibraryLoadTime())
    {
        m_lastRun = QDate::currentDate();
        if(m_mediaLibraryHandler)
        {
            SettingsHandler::setForceMetaDataFullProcess(SettingsHandler::scheduleLibraryLoadFullProcess());
            m_mediaLibraryHandler->loadLibraryAsync();
        }
    }
}
