#ifndef SETTINGSACTIONHANDLER_H
#define SETTINGSACTIONHANDLER_H

#include <QObject>
#include <QVariant>
#include "lib/lookup/MediaActions.h"

#include "XTEngine_global.h"
/**
 * @brief The SettingsActionHandler class handles the internal settings state of XTEngine.
 * The action is then emitted through actionExecuted signal for further extension.
 */
class XTENGINE_EXPORT SettingsActionHandler : public QObject
{
    Q_OBJECT

signals:
    void actionExecuted(QString action, QString actionExecuted, QVariant value);
    void tcode_action(QString tcode);

public slots:
    void media_action(QString action);

public:
    explicit SettingsActionHandler(QObject *parent = nullptr);

private:
    MediaActions actions;

};

#endif // SETTINGSACTIONHANDLER_H
