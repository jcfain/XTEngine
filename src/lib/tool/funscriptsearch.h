#ifndef FUNSCRIPTSEARCH_H
#define FUNSCRIPTSEARCH_H

#include "QObject"
#include "QString"
#include "QFuture"

#include "../struct/InputConnectionPacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT FunscriptSearch : public QObject
{
    Q_OBJECT
signals:
    void searchStart();
    void searchStop();
    void searchFinish(QString mediaPath, QString funscriptPath, qint64 mediaDuration);
public:
    FunscriptSearch(QObject *parent = nullptr);
    void searchForFunscript(const QString& mediaPath, const qint64& mediaDuration = -1, const QStringList& extensions = QStringList() << ".funscript");
    void searchForFunscript(const QString& mediaPath, const QStringList& pathsToSearch, const qint64& mediaDuration = -1, const QStringList& extensions = QStringList() << ".funscript");
    QString searchForFunscriptDeep(const QString& mediaPath, const QStringList& pathsToSearch, const QStringList& extensions = QStringList() << ".funscript");
private:
    QFuture<void> _funscriptSearchFuture;
    bool m_stopResursiveFunscriptSearch = false;

    QString searchForFunscript(const QString& mediaPath, const QStringList& pathsToSearch, const QStringList& extensions = QStringList() << ".funscript");
    QString searchForFunscript(const QString& mediaPath, const QString& pathToSearch, const QStringList& extensions = QStringList() << ".funscript");
    QString searchForFunscriptHttp(const QString& mediaPath, const QString& pathToSearch, const QStringList& extensions = QStringList() << ".funscript");
    QString searchForFunscriptMFS(const QString& mediaPath, const QStringList& pathsToSearch);
    QString searchForFunscriptMFSDeep(const QString& mediaPath, const QStringList& pathsToSearch);
};

#endif // FUNSCRIPTSEARCH_H
