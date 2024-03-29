#ifndef TCODEHANDLER_H
#define TCODEHANDLER_H
#include "../struct/Funscript.h"
#include "../struct/ChannelModel33.h"
#include "../struct/GamepadState.h"
#include "../lookup/tcodechannellookup.h"
#include "../lookup/AxisNames.h"
#include "../tool/xmath.h"
#include "settingshandler.h"
#include "funscripthandler.h"
#include "loghandler.h"
#include <QMap>
#include <QPair>
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
};

#endif // TCODEHANDLER_H
