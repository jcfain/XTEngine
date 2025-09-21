#ifndef FUNSCRIPTSEARCH_H
#define FUNSCRIPTSEARCH_H

#include "QObject"
#include "QString"
#include "QFuture"

#include "../struct/InputConnectionPacket.h"

class FunscriptSearch : public QObject
{
    Q_OBJECT
signals:
    void searchStart();
    void searchStop();
    void searchFinish(QString mediaPath, QString funscriptPath, qint64 mediaDuration);
public:
    FunscriptSearch(QObject *parent = nullptr);
    void searchForFunscript(QString videoPath, qint64 mediaDuration = -1, QStringList extensions = QStringList() << ".funscript");
    void searchForFunscript(QString videoPath, QStringList libraryPaths, qint64 mediaDuration = -1, QStringList extensions = QStringList() << ".funscript");
private:
    QFuture<void> _funscriptSearchFuture;
    bool m_stopResursiveFunscriptSearch = false;

    QString searchForFunscript(QString videoPath, QStringList libraryPaths, QStringList extensions = QStringList() << ".funscript");
    QString searchForFunscriptDeep(QString videoPath, QStringList libraryPaths, QStringList extensions = QStringList() << ".funscript");
    QString searchForFunscript(QString videoPath, QString pathToSearch, QStringList extensions = QStringList() << ".funscript");
    QString searchForFunscriptHttp(QString videoPath, QString pathToSearch, QStringList extensions = QStringList() << ".funscript");
    QString searchForFunscriptMFS(QString mediaPath, QStringList libraryPaths);
    QString searchForFunscriptMFSDeep(QString mediaPath, QStringList libraryPaths);
};

#endif // FUNSCRIPTSEARCH_H
