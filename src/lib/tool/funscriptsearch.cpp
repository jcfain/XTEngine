#include "funscriptsearch.h"
#include <QUrl>

#include "../handler/settingshandler.h"
#include "../handler/loghandler.h"
#include "file-util.h"

FunscriptSearch::FunscriptSearch(QObject *parent) : QObject(parent) {}

void FunscriptSearch::searchForFunscript(const QString& mediaPath, const qint64& mediaDuration, const QStringList& extensions)
{
    QStringList pathsToSearch = SettingsHandler::mediaLibrarySettings.get(LibraryType::MAIN);
    QStringList vrLibraryPaths = SettingsHandler::mediaLibrarySettings.get(LibraryType::VR);
    QStringList funscriptLibraryPaths = SettingsHandler::mediaLibrarySettings.get(LibraryType::FUNSCRIPT);
    foreach(QString libraryPath, vrLibraryPaths)
    {
        pathsToSearch << libraryPath;
    }
    foreach(QString libraryPath, funscriptLibraryPaths)
    {
        pathsToSearch << libraryPath;
    }
    searchForFunscript(mediaPath, pathsToSearch, mediaDuration, extensions);
}

void FunscriptSearch::searchForFunscript(const QString& mediaPath, const QStringList& pathsToSearch, const qint64& mediaDuration, const QStringList& extensions)
{
    if(_funscriptSearchFuture.isRunning())
    {
        m_stopResursiveFunscriptSearch = true;
        _funscriptSearchFuture.cancel();
        _funscriptSearchFuture.waitForFinished();
    }

    _funscriptSearchFuture = QtConcurrent::run([this](const QString& mediaPath, const QStringList& pathsToSearch, const qint64& mediaDuration, const QStringList& extensions) {
        LogHandler::Debug("searchForFunscript thread start for media: "+mediaPath);
        QString funscriptPath = searchForFunscript(mediaPath, pathsToSearch, extensions);

        if(_funscriptSearchFuture.isRunning() && _funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript canceled: "+mediaPath);
            emit searchStop();
            return;
        }

        if(funscriptPath.isEmpty())
        {
            funscriptPath = SettingsHandler::getDeoDnlaFunscript(mediaPath);
            if(!funscriptPath.isEmpty())
            {
                if(!QFile::exists(funscriptPath))
                {
                    SettingsHandler::removeLinkedVRFunscript(mediaPath);
                    funscriptPath = nullptr;
                }
            }
        }

        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptMFS(mediaPath, pathsToSearch);

        // Deep search last
        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptDeep(mediaPath, pathsToSearch, extensions);
        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptMFSDeep(mediaPath, pathsToSearch);

        emit searchFinish(mediaPath, funscriptPath, mediaDuration);

    }, mediaPath, pathsToSearch, mediaDuration, extensions);
}

QString FunscriptSearch::searchForFunscript(const QString& mediaPath, const QStringList& pathsToSearch, const QStringList& extensions)
{
    QString funscriptPath;
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isRunning() && _funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript paths canceled: "+mediaPath);
            return funscriptPath;
        }
        LogHandler::Debug("searchForFunscript in library: "+libraryPath);
        funscriptPath = searchForFunscriptHttp(mediaPath, libraryPath, extensions);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
        funscriptPath = searchForFunscript(mediaPath, libraryPath, extensions);
        if(!funscriptPath.isEmpty())
            return funscriptPath;

    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscriptDeep(const QString& mediaPath, const QStringList& pathsToSearch, const QStringList& extensions)
{
    QString funscriptPath;
    LogHandler::Debug("searchForFunscript deep "+ mediaPath);
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isRunning() && _funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscriptDeep canceled: "+mediaPath);
            return funscriptPath;
        }
        funscriptPath = XFileUtil::searchForFileRecursive(mediaPath, extensions, libraryPath, m_stopResursiveFunscriptSearch);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscript(const QString& mediaPath, const QString& pathToSearch, const QStringList& extensions)
{
    //Check the input device media directory for funscript.
    LogHandler::Debug("searchForFunscript local: "+ mediaPath);
    QString funscriptPath;
    QStringList libraryScriptPaths = XFileUtil::getFunscriptPaths(mediaPath, extensions, pathToSearch);
    foreach (QString libraryScriptPath, libraryScriptPaths)
    {
        if(_funscriptSearchFuture.isRunning() && _funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript path canceled: "+mediaPath);
            return funscriptPath;
        }
        LogHandler::Debug("searchForFunscript Searching path: "+libraryScriptPath);
        if(QFile::exists(libraryScriptPath))
        {
            LogHandler::Debug("searchForFunscript script found in path of media");
            funscriptPath = libraryScriptPath;
        }
    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscriptHttp(const QString& mediaPath, const QString& pathToSearch, const QStringList& extensions)
{
    QString funscriptPath;
    if(mediaPath.contains("http") ||
        mediaPath.startsWith("/media") ||
        mediaPath.startsWith("media") ||
        mediaPath.startsWith("/storage/emulated/0/Interactive/"))
    {
        LogHandler::Debug("searchForFunscript from http in: "+ pathToSearch);
        QUrl funscriptUrl = QUrl(mediaPath);
        QString path = funscriptUrl.path();
        QString localpath = path;
        if(path.startsWith("/media"))
            localpath = path.remove("/media/");
        else if(path.startsWith("media/"))
            localpath = path.remove("media/");
        else if(path.startsWith("/storage/emulated/0/Interactive/"))
            localpath = path.remove("/storage/emulated/0/Interactive/");

        QFileInfo fileInfo(localpath);
        QString localpathTemp = localpath;
        QString mediaPath = pathToSearch + XFileUtil::getSeperator(pathToSearch) + localpathTemp.remove(fileInfo.fileName());
        LogHandler::Debug("searchForFunscript http in library path: "+ mediaPath);
        funscriptPath = searchForFunscript(localpath, mediaPath, extensions);
        if(!funscriptPath.isEmpty()) {
            LogHandler::Debug("searchForFunscript from http found: "+ funscriptPath);
            return funscriptPath;
        } else {
            LogHandler::Debug("searchForFunscript from http not found");
        }
    } else {
        LogHandler::Debug("searchForFunscript not http");
    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscriptMFS(const QString& mediaPath, const QStringList& pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscript(mediaPath, extensions, pathsToSearch);
}

QString FunscriptSearch::searchForFunscriptMFSDeep(const QString& mediaPath, const QStringList& pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs deep: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscriptDeep(mediaPath, extensions, pathsToSearch);
}
