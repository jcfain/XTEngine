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
#include "../struct/InputDevicePacket.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT FunscriptHandler : public QObject
{
    Q_OBJECT
public:
    FunscriptHandler(QString name , QObject* parent = nullptr);
    ~FunscriptHandler();
    bool load(QString funscript);
    bool load(QByteArray funscript);
    bool isLoaded();
    void setLoaded(bool value);
    bool exists(QString path);
    Funscript* currentFunscript();
    static bool getInverted();
    static void setInverted(bool value);
    qint64 getMin() const;
    qint64 getMax() const;
    qint64 getNext() const;
    std::shared_ptr<FunscriptAction> getPosition(qint64 at);
    QString channel() const;

    void play(QString funscript);
    void stop();

    static void setModifier(double percentage);
    static void resetModifier();
    static double getModifier();


private:
    static QMutex mutexStat;
    static bool _inverted;
    QMutex mutex;
    QString _channel;
    bool _loaded = false;
    bool _firstActionExecuted;
    void JSonToFunscript(QJsonObject jsonDoc);
    qint64 findClosest(qint64 value, QList<qint64> a);
    qint64 lastActionIndex;
    qint64 nextActionIndex;
    int lastActionPos;
    int lastActionInterval;
    QList<qint64> atList;
    Funscript* funscript = 0;
    int n;
    qint64 _funscriptMin = 0;
    qint64 _funscriptMax = -1;
    static double _modifier;
};

#endif // FUNSCRIPTHANDLER_H
