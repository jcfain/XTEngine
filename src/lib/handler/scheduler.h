#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <QObject>
#include "medialibraryhandler.h"

class Scheduler : public QObject
{
    Q_OBJECT
public:
    explicit Scheduler(MediaLibraryHandler* mediaLibraryHandler, QObject *parent = nullptr);

    void startLibraryLoadSchedule();
    void stopLibraryLoadSchedule();
signals:
private:
    MediaLibraryHandler* m_mediaLibraryHandler;
    QTimer m_timer;
    QDate m_lastRun;

    void checkLibraryLoadTime();
};

#endif // SCHEDULER_H
