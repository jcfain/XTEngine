#ifndef SETTINGSBASE_H
#define SETTINGSBASE_H

#include <QObject>
#include <QSettings>

#include "XTEngine_global.h"

class XTENGINE_EXPORT SettingsBase : public QObject
{
    Q_OBJECT
public:
    explicit SettingsBase(QObject *parent = nullptr);

    virtual void Load(QSettings* settings) = 0;
    virtual void Save(QSettings* settings) = 0;
signals:
    void settingsChangedEvent(bool dirty);
};

#endif // SETTINGSBASE_H
