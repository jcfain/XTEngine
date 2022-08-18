#ifndef XTENGINE_H
#define XTENGINE_H

#include "XTEngine_global.h"
#include <QObject>
#include <QCoreApplication>

#include "lib/handler/loghandler.h"
#include "lib/handler/settingshandler.h"

#include "lib/handler/settingsactionhandler.h"
#include "lib/handler/httphandler.h"
#include "lib/handler/synchandler.h"
#include "lib/handler/medialibraryhandler.h"
#include "lib/handler/connectionhandler.h"
#include "lib/tool/tcodefactory.h"
#include "lib/handler/tcodehandler.h"

class XTENGINE_EXPORT XTEngine: public QObject
{
    Q_OBJECT
public:
    XTEngine(QObject* parent = nullptr);
    ~XTEngine();

    SyncHandler* syncHandler() {
        return _syncHandler;
    }
    MediaLibraryHandler* mediaLibraryHandler() {
        return _mediaLibraryHandler;
    }
    TCodeHandler* tcodeHandler() {
        return _tcodeHandler;
    }
    HttpHandler* httpHandler() {
        if(!SettingsHandler::getEnableHttpServer())
            LogHandler::Warn("Http server is not enabled");
        return _httpHandler;
    }
    ConnectionHandler* connectionHandler() {
        return _connectionHandler;
    }
    SettingsActionHandler* settingsActionHandler() {
        return _settingsActionHandler;
    }
    TCodeFactory* tcodeFactory() {
        return _tcodeFactory;
    }

private:
    SyncHandler* _syncHandler = 0;
    MediaLibraryHandler* _mediaLibraryHandler = 0;
    TCodeHandler* _tcodeHandler = 0;
    HttpHandler* _httpHandler = 0;
    ConnectionHandler* _connectionHandler = 0;
    SettingsActionHandler* _settingsActionHandler = 0;
    TCodeFactory *_tcodeFactory = 0;
};

#endif // XTENGINE_H
