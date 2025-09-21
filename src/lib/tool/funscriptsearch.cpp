#include "funscriptsearch.h"
#include <QUrl>

#include "../handler/settingshandler.h"
#include "../handler/loghandler.h"
#include "file-util.h"

FunscriptSearch::FunscriptSearch(QObject *parent) : QObject(parent) {}

void FunscriptSearch::searchForFunscript(QString videoPath, qint64 mediaDuration, QStringList extensions)
{
    QStringList libraryPaths = SettingsHandler::mediaLibrarySettings.get(LibraryType::MAIN);
    QStringList vrLibraryPaths = SettingsHandler::mediaLibrarySettings.get(LibraryType::VR);
    QStringList funscriptLibraryPaths = SettingsHandler::mediaLibrarySettings.get(LibraryType::FUNSCRIPT);
    foreach(QString libraryPath, vrLibraryPaths)
    {
        libraryPaths << libraryPath;
    }
    foreach(QString libraryPath, funscriptLibraryPaths)
    {
        libraryPaths << libraryPath;
    }
    searchForFunscript(videoPath, libraryPaths, mediaDuration, extensions);
}


void FunscriptSearch::searchForFunscript(QString videoPath, QStringList libraryPaths, qint64 mediaDuration, QStringList extensions)
{
    if(_funscriptSearchFuture.isRunning())
    {
        m_stopResursiveFunscriptSearch = true;
        _funscriptSearchFuture.cancel();
        _funscriptSearchFuture.waitForFinished();
    }

    _funscriptSearchFuture = QtConcurrent::run([this](QString videoPath, QStringList libraryPaths, qint64 mediaDuration, QStringList extensions) {
        LogHandler::Debug("searchForFunscript thread start for media: "+videoPath);
        QString funscriptPath = searchForFunscript(videoPath, libraryPaths, extensions);

        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript canceled: "+videoPath);
            emit searchStop();
            return;
        }

        if(funscriptPath.isEmpty())
        {
            funscriptPath = SettingsHandler::getDeoDnlaFunscript(videoPath);
            if(!funscriptPath.isEmpty())
            {
                if(!QFile::exists(funscriptPath))
                {
                    SettingsHandler::removeLinkedVRFunscript(videoPath);
                    funscriptPath = nullptr;
                }
            }
        }

        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptMFS(videoPath, libraryPaths);

        // Deep search last
        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptDeep(videoPath, libraryPaths, extensions);
        if(funscriptPath.isEmpty())
            funscriptPath = searchForFunscriptMFSDeep(videoPath, libraryPaths);

        emit searchFinish(videoPath, funscriptPath, mediaDuration);

    }, videoPath, libraryPaths, mediaDuration, extensions);
}

QString FunscriptSearch::searchForFunscript(QString videoPath, QStringList pathsToSearch, QStringList extensions)
{
    QString funscriptPath;
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript paths canceled: "+videoPath);
            return funscriptPath;
        }
        LogHandler::Debug("searchForFunscript in library: "+libraryPath);
        funscriptPath = searchForFunscriptHttp(videoPath, libraryPath);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
        funscriptPath = searchForFunscript(videoPath, libraryPath);
        if(!funscriptPath.isEmpty())
            return funscriptPath;

    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscriptDeep(QString videoPath, QStringList pathsToSearch, QStringList extensions)
{
    QString funscriptPath;
    LogHandler::Debug("searchForFunscript deep "+ videoPath);
    foreach(QString libraryPath, pathsToSearch)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscriptDeep canceled: "+videoPath);
            return funscriptPath;
        }
        funscriptPath = XFileUtil::searchForFileRecursive(videoPath, extensions, libraryPath, m_stopResursiveFunscriptSearch);
        if(!funscriptPath.isEmpty())
            return funscriptPath;
    }
    return funscriptPath;
}

QString FunscriptSearch::searchForFunscript(QString videoPath, QString pathToSearch, QStringList extensions)
{
    //Check the input device media directory for funscript.
    LogHandler::Debug("searchForFunscript local: "+ videoPath);
    QString funscriptPath;
    QStringList libraryScriptPaths = XFileUtil::getFunscriptPaths(videoPath, extensions, pathToSearch);
    foreach (QString libraryScriptPath, libraryScriptPaths)
    {
        if(_funscriptSearchFuture.isCanceled()) {
            LogHandler::Debug("searchForFunscript path canceled: "+videoPath);
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

QString FunscriptSearch::searchForFunscriptHttp(QString videoPath, QString pathToSearch, QStringList extensions)
{
    QString funscriptPath;
    if(videoPath.contains("http") ||
        videoPath.startsWith("/media") ||
        videoPath.startsWith("media") ||
        videoPath.startsWith("/storage/emulated/0/Interactive/"))
    {
        LogHandler::Debug("searchForFunscript from http in: "+ pathToSearch);
        QUrl funscriptUrl = QUrl(videoPath);
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

QString FunscriptSearch::searchForFunscriptMFS(QString mediaPath, QStringList pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscript(mediaPath, extensions, pathsToSearch);
}

QString FunscriptSearch::searchForFunscriptMFSDeep(QString mediaPath, QStringList pathsToSearch)
{
    LogHandler::Debug("searchForFunscript mfs deep: "+ mediaPath);
    QStringList extensions = TCodeChannelLookup::getValidMFSExtensions();
    return searchForFunscriptDeep(mediaPath, extensions, pathsToSearch);
}
