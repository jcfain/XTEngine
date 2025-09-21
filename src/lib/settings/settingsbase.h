#ifndef SETTINGSBASE_H
#define SETTINGSBASE_H

#include <QObject>
#include <QSettings>

class SettingsBase : public QObject
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
