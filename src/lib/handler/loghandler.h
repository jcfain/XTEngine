#ifndef LOGHANDLER_H
#define LOGHANDLER_H
#include <QString>
#include <QDebug>
#include <QMutex>
#include <QFile>
#include <QDateTime>


enum XLogLevel
{
    Information,
    Debuging,
    Warning,
    Critical
};

class LogHandler
{
public:

    static void Debug(QString message);
    static void Info(QString message);
    static void Error(QString message);
    static void Warn(QString message);
    //static void Dialog(QWidget* parent, QString message, XLogLevel level);
    static QString getLevel(XLogLevel level);
    static void setUserDebug(bool on);
    static bool getUserDebug();
    static void ExportDebug();
//    static void Loading(QWidget* parent, QString message);
//    static void LoadingClose();

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
