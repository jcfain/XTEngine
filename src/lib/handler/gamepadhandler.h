#ifndef GAMEPADHANDLER_H
#define GAMEPADHANDLER_H

#include <QGamepad>
#include "../tool/tcodefactory.h"
#include "../lookup/MediaActions.h"
#include "../tool/xtimer.h"
#include "../struct/ConnectionChangedSignal.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT GamepadHandler : public QThread
{
    Q_OBJECT
signals:
    void connectionChange(ConnectionChangedSignal status);
    void emitTCode(QString tcode);
    void emitAction(QString action);
public:
    GamepadHandler(QObject *parent = nullptr);
    ~GamepadHandler();
    QHash<QString, QVariant>* getState();
    void init();
    void dispose();
    bool isConnected();


private:
    void run() override;
    void gamePadConnectionChanged(bool connected);
    void connectedGamepadsChanged();
    void disposeInternal();
    double calculateDeadZone(double gpIn);
    void executeAction(QString axisName, double gamepadValue, QVector<ChannelValueModel> &axisValues, MediaActions &mediaActions, XTimer &actionTimer);
    QStringList subtract(QList<QString> isIn, QList<QString> contains);


    TCodeFactory* _tcodeFactory;
    QList<int> _gamepads;
    QGamepad* _gamepad = nullptr;
    QHash<QString, QVariant>* _gamepadState = nullptr;
    QMutex _mutex;
    QWaitCondition _cond;
    int _waitTimeout = 0;
    bool _stop = false;
    bool _isConnected = false;
    double _deadzone = 0.2;
};

#endif // GAMEPADHANDLER_H
