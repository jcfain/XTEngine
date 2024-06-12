#ifndef TCODEHANDLER_H
#define TCODEHANDLER_H
#include <QMap>
#include <QPair>
#include <QMutex>

#include "../struct/Funscript.h"
#include "../struct/ChannelModel33.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT TCodeHandler : QObject
{
    Q_OBJECT
public:
    TCodeHandler(QObject* parent = nullptr);
    ~TCodeHandler();
    QString funscriptToTCode(std::shared_ptr<FunscriptAction> action, QMap<QString, std::shared_ptr<FunscriptAction>> otherActions);

    int calculateRange(const char* channel, int rawValue);
    QString getRunningHome();
    QString getAllHome();
    QString getSwitchedHome();
    QString getChannelHome(QString channel);

private:
    QMutex mutex;
    void getChannelHome(ChannelModel33* channel, QString &tcode);
    QMap<QString, int> channelValueTracker;
    int getDistance(int current, int last);
};

#endif // TCODEHANDLER_H
