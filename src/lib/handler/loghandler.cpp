#include "loghandler.h"
#include <QLoggingCategory>


void LogHandler::Debug(QString message)
{
    QMutexLocker locker(&mutex);
    if (_debugMode)
    {
        qDebug() << "XTP DEBUG " + QDateTime::currentDateTime().toString("hh:mm:ss:zzz") +": " +  message;
    }
}

void LogHandler::Info(QString message)
{
    QMutexLocker locker(&mutex);
    qInfo() << "XTP INFO " + QDateTime::currentDateTime().toString("hh:mm:ss:zzz") +": " +  message;
}

void LogHandler::Warn(QString message)
{
    QMutexLocker locker(&mutex);
    qWarning() << "XTP WARN " + QDateTime::currentDateTime().toString("hh:mm:ss:zzz") + ": " + message;
}

void LogHandler::Error(QString message)
{
    QMutexLocker locker(&mutex);
    qCritical() << "XTP ERROR " + QDateTime::currentDateTime().toString("hh:mm:ss:zzz") +": " +  message;
}


QString LogHandler::getLevel(XLogLevel level)
{
    switch(level)
    {
        case XLogLevel::Debuging:
            return "Debug";
        case XLogLevel::Information:
            return "Information";
        case XLogLevel::Warning:
            return "Warning";
        case XLogLevel::Critical:
            return "Error";
    }

}

void LogHandler::DebugHandler(QtMsgType type, const QMessageLogContext &, const QString & msg)
{
    QString txt;
    switch (type) {
        case QtInfoMsg: {
            txt = QString("Info: %1").arg(msg);
            break;
        }
        case QtDebugMsg: {
            txt = QString("Debug: %1").arg(msg);
            break;
        }
        case QtWarningMsg: {
            txt = QString("Warning: %1").arg(msg);
            break;
        }
        case QtCriticalMsg: {
            txt = QString("Critical: %1").arg(msg);
            break;
        }
        case QtFatalMsg: {
            txt = QString("Fatal: %1").arg(msg);
            break;
        }
    }
    QMutexLocker locker(&mutex);
    userDebugIndex++;
    _debugStore.insert(userDebugIndex, QDateTime::currentDateTime().toString("MM-dd-yyyy_hh-mm-ss-zzz") + " " + txt);
}

void LogHandler::setUserDebug(bool on)
{
    QMutexLocker locker(&mutex);
    _debugMode = on;
//    if (on)
//    {
//        qInstallMessageHandler(DebugHandler);
//    }
//    else
//    {
//        qInstallMessageHandler(0);
//    }
}

void LogHandler::setQtDebuging(bool on)
{
    QMutexLocker locker(&mutex);
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\ndriver.usb.*=true"));
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\nqt.multimedia.*=true"));
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\nqt.serialport.*=true"));
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\nqt.network.*=true"));
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\nqt.gamepad.*=true"));
//    QLoggingCategory::setFilterRules(QString("*.debug=") + (on ? "true\n": "false\nqt.websockets.*=true"));
    //serialport network gamepad texttospeech compress websockets multimedia
    QLoggingCategory::setFilterRules(""
    "*.debug=true\n"
    "*.critical=true\n"
    "*.warning=true\n"
    "*.info=true\n"
    "");
}
bool LogHandler::getUserDebug()
{
    QMutexLocker locker(&mutex);
    return _debugMode;
}

//void LogHandler::ExportDebug()
//{
//    QMutexLocker locker(&mutex);
//    if(_debugStore.keys().length() > 0)
//    {
//        _debugFileName = "Debug_"+QDateTime::currentDateTime().toString("MM-dd-yyyy_hh-mm-ss")+".txt";
//        QFile outFile(QApplication::applicationDirPath() + "/" +_debugFileName);
//        outFile.open(QIODevice::WriteOnly | QIODevice::Append);
//        QTextStream ts(&outFile);
//        QMap<qint64, QString> debugStoreMap;
//        foreach (qint64 index, _debugStore.keys())
//        {
//            debugStoreMap.insert(index, _debugStore.value(index));
//        }
//        foreach (qint64 index, debugStoreMap.keys())
//        {
//            ts << debugStoreMap.value(index) << Qt::endl;
//        }
//        userDebugIndex = 0;
//        _debugStore.clear();
//    }
//}

QMutex LogHandler::mutex;
qint64 LogHandler::userDebugIndex = 0;
QHash<qint64, QString> LogHandler::_debugStore;
QString LogHandler::_debugFileName;
#ifdef QT_DEBUG
    bool LogHandler::_debugMode = true;
#else
    bool LogHandler::_debugMode = false;
#endif
