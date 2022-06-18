#ifndef LOGHANDLER_H
#define LOGHANDLER_H
#include <QString>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QDateTime>
#include "XTEngine_global.h"


enum XLogLevel
{
    Information,
    Debuging,
    Warning,
    Critical
};


class XTENGINE_EXPORT LogHandler
{
public:

    static void Debug(QString message);
    static void Info(QString message);
    static void Error(QString message);
    static void Warn(QString message);
    static QString getLevel(XLogLevel level);
    static void setUserDebug(bool on);
    static bool getUserDebug();
    static void ExportDebug();

private:
    LogHandler();
    static void DebugHandler(QtMsgType type, const QMessageLogContext &, const QString & msg);
    static QMutex mutex;
    static QString _debugFileName;
    static qint64 userDebugIndex;
    static bool _debugMode;
    static QHash<qint64, QString> _debugStore;

};

#endif // LOGHANDLER_H
