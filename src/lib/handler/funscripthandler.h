#ifndef FUNSCRIPTHANDLER_H
#define FUNSCRIPTHANDLER_H
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QIODevice>
#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFile>
#include "settingshandler.h"
#include "loghandler.h"
#include "../struct/Funscript.h"
#include "../struct/InputConnectionPacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT FunscriptHandler : public QObject
{
    Q_OBJECT
public slots:
    static void updateMetadata(LibraryListItemMetaData258 value);
public:
    FunscriptHandler(QObject* parent = nullptr);
    ~FunscriptHandler();
    bool load(const QString& funscript);
    bool load(const QByteArray& funscript);
    bool load(const Track& channelName, const QString& funscript);
    bool load(const Track& channelName, const QByteArray& funscript);
    void unload();
    bool isLoaded();
    bool isLoaded(const Track& channelName);
    // void setLoaded(const ChannelName& channelName, const bool& value);
    bool exists(const QString& path);
    const Funscript* getFunscript();
    const Funscript* getFunscript(const Track& channelName);
    qint64 getMin(const Track& channelName) const;
    qint64 getMax(const Track& channelName) const;
    qint64 getNext(const Track& channelName) const;
    std::shared_ptr<FunscriptAction> getPosition(const Track& channelName, const qint64& at);

    void play(QString funscript);
    void stop();

    static const QList<Track> getLoaded();
    static bool getInverted(const Track& channelName);
    static void setInverted(const bool& value);
    static void setInverted(const Track& channelName, const bool& value);
    static double getModifier(const Track& channelName);
    static void setModifier(double percentage);
    static void setModifier(const Track& channelName, double percentage);
    static void resetModifier();
    static void resetModifier(const Track& channelName);
    static int getOffSet();
    static void setOffset(int value);
    static void resetOffset(int value);

    static bool isSFMA(QString libraryItemMediaPath);
    static QList<ScriptInfo> getSFMATracks(QString libraryItemMediaPath);
    static bool isMFS(QString libraryItemMediaPath);
    static QList<ScriptInfo> getMFSTracks(QString libraryItemMediaPath);


private:
    static inline QMutex mutex;
    static inline QHash<Track, Funscript> m_funscripts;
    // static bool _inverted;
    // QString _channel;
    bool m_loaded = false;
    bool _firstActionExecuted;
    static inline int m_offset;
    static QByteArray readFile(QString file);
    static QJsonObject readJson(QByteArray data);
    void jsonToFunscript(QJsonObject json);
    void jsonToFunscript(const QJsonObject& json, Funscript& funscript);
    void jsonToFunscript(const QJsonObject& json, FunscriptMetadata& metadata);
    void jsonToFunscript(const QJsonObject& json, QHash<qint64, int>& actions);
    void setFunscriptSettings(const Track& channelName, Funscript& funscript);
    qint64 findClosest(const qint64& value, const QList<qint64>& a);
};

#endif // FUNSCRIPTHANDLER_H
