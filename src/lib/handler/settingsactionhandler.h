#ifndef SETTINGSACTIONHANDLER_H
#define SETTINGSACTIONHANDLER_H

#include <QObject>
#include "lib/lookup/MediaActions.h"
#include "settingshandler.h"
#include "funscripthandler.h"

#include "XTEngine_global.h"
/**
 * @brief The SettingsActionHandler class handles the internal settings state of XTEngine.
 * The action is then emitted through actionExecuted signal for further extension.
 */
class XTENGINE_EXPORT SettingsActionHandler : public QObject
{
    Q_OBJECT

signals:
    void actionExecuted(QString action, QString actionExecuted);

public slots:
    void media_action(QString action);

public:
    explicit SettingsActionHandler(QObject *parent = nullptr);

private:

};

#endif // SETTINGSACTIONHANDLER_H
