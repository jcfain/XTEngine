#ifndef XNETWORK_H
#define XNETWORK_H
#include <cmath>
#include <QtGlobal>
#include <QHostAddress>
#include <QProcess>
#include <QElapsedTimer>
#include "XTEngine_global.h"

class XTENGINE_EXPORT XNetwork {
public:
    static bool ping(QHostAddress address, int &ms) {
#if defined(WIN32)
        QString parameter = "-n";
#else
        QString parameter = "-c";
#endif
        QElapsedTimer timer;
        timer.start();
        int exitCode = QProcess::execute("ping", QStringList() << parameter << "1" << address.toString().remove("::ffff:"));
        ms = (round(timer.nsecsElapsed() / 1000000));
        return exitCode == 0;
    }
};

#endif // XNETWORK_H
