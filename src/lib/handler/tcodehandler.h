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
    QString funscriptToTCode(QMap<QString, std::shared_ptr<FunscriptAction>> actions);
    int calculateRange(const char* channel, int rawValue);
    QString getRunningHome();
    QString getAllHome();
    QString getSwitchedHome();
    QString getChannelHome(QString channel);

private:
    QMutex mutex;
    QString handleMotionModifier(std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions);
    QString getMotionModifierTCode(ChannelModel33* channel, std::shared_ptr<FunscriptAction> mainAction, QMap<QString, std::shared_ptr<FunscriptAction>> actions);
    void getChannelHome(ChannelModel33* channel, QString &tcode);
    QMap<QString, int> channelValueTracker;
    QHash<Track, bool> multiplierEnabledTracker;
    int getDistance(int current, int last);
};

#endif // TCODEHANDLER_H
