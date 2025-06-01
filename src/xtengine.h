#ifndef XTENGINE_H
#define XTENGINE_H

#include "XTEngine_global.h"
#include <QObject>
#include <QCoreApplication>

#include "lib/handler/loghandler.h"
#include "lib/handler/settingshandler.h"

#include "lib/handler/settingsactionhandler.h"
#if BUILD_QT5
#include "lib/handler/httphandler.h"
#endif
#include "lib/handler/synchandler.h"
#include "lib/handler/medialibraryhandler.h"
#include "lib/handler/connectionhandler.h"
#include "lib/handler/scheduler.h"
#include "lib/tool/tcodefactory.h"
#include "lib/handler/tcodehandler.h"
#include "lib/tool/heatmap.h"

class XTENGINE_EXPORT XTEngine: public QObject
{
    Q_OBJECT
signals:
    void stopAllMedia();

private slots:
    void onFunscriptSearchResult(QString mediaPath, QString funscriptPath, qint64 mediaDuration);

public:
    XTEngine(QString appName = nullptr, QObject* parent = nullptr);
    ~XTEngine();

    void init();

    SyncHandler* syncHandler() {
        return _syncHandler;
    }
    MediaLibraryHandler* mediaLibraryHandler() {
        return _mediaLibraryHandler;
    }
    TCodeHandler* tcodeHandler() {
        return _tcodeHandler;
    }
#if BUILD_QT5
    HttpHandler* httpHandler() {

        if(!SettingsHandler::getEnableHttpServer())
            LogHandler::Warn("Http server is not enabled");
        return _httpHandler;
        return 0
    }
#endif
    ConnectionHandler* connectionHandler() {
        return _connectionHandler;
    }
    SettingsActionHandler* settingsActionHandler() {
        return _settingsActionHandler;
    }
    TCodeFactory* tcodeFactory() {
        return _tcodeFactory;
    }
    Scheduler* scheduler() {
        return m_scheduler;
    }
    void scheduleLibraryLoadEnableChange(bool enabled);
//    XMediaStateHandler* xMediaStateHandler() {
//        return m_xMediaStateHandler;
//    }


private:
    SyncHandler* _syncHandler = 0;
    MediaLibraryHandler* _mediaLibraryHandler = 0;
    TCodeHandler* _tcodeHandler = 0;
#if BUILD_QT5
    HttpHandler* _httpHandler = 0;
#endif
    ConnectionHandler* _connectionHandler = 0;
    SettingsActionHandler* _settingsActionHandler = 0;
    TCodeFactory *_tcodeFactory = 0;
    HeatMap* m_heatmap;
    Scheduler* m_scheduler;

//    XMediaStateHandler* m_xMediaStateHandler;


    //TODO maybe move this to a more sutible place somehow?...
    MediaActions actions;
    void skipToNextAction();
    void skipToMoneyShot();
};

#endif // XTENGINE_H
