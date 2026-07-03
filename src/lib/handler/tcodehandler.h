#ifndef TCODEHANDLER_H
#define TCODEHANDLER_H
#include <QMap>
#include <QPair>
#include <QMutex>

#include "connectionhandler.h"
#include "../struct/Funscript.h"
#include "../struct/ChannelModel33.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT TCodeHandler : public QObject
{
    Q_OBJECT
signals:
    //void sendTCode(QString tcode);
    void delayTCode(QString tcode, int delayMS);
public:
    TCodeHandler(QObject* parent = nullptr);
    ~TCodeHandler();
    QString funscriptToTCode(QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime);
    int calculateRange(const char* channel, int rawValue);
    QString getRunningHome();
    QString getAllHome();
    QString getSwitchedHome();
    QString getChannelHome(QString channel);
    QString getDelayedActions(qint64 referenceTime);
    void clearDelayedActions();

private:
    struct XDelayedFunscriptAction
    {
        qint64 at;
        QString tcode;
        bool operator==(const XDelayedFunscriptAction& in)
        {
            return at == in.at && tcode == in.tcode;
        }
    };
    QMutex mutex;
    qint64 m_lastTimeTracker = 0;
    QString handleMotionModifier(std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime);
    QString getMotionModifierTCode(ChannelModel33* channel, std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions, qint64 referenceTime);
    void getChannelHome(ChannelModel33* channel, QString &tcode);
    QMap<QString, int> channelValueTracker;
    QHash<Track, bool> multiplierEnabledTracker;
    int getDistance(int current, int last);
    int calculateGradient(int previous, int current, int next, qint64 previousAt, qint64 nextAt);
    QList<XDelayedFunscriptAction> m_delayedFunscriptActions;
};

#endif // TCODEHANDLER_H
