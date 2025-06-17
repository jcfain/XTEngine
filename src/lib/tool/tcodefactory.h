#ifndef TCODEFACTORY_H
#define TCODEFACTORY_H
#include <QString>
#include <QHash>
#include "../struct/ChannelModel33.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT TCodeFactory : public QObject
{
    Q_OBJECT
public:
    TCodeFactory(double inputStart, double inputEnd, QObject* parent = nullptr);
    void init();
    void calculate(QString axisName, double value, QVector<ChannelValueModel> &axisValues);
    QString formatTCode(QVector<ChannelValueModel>* values);

private:
    int calculateTcodeRange(double value, ChannelModel33* channel);
    int calculateGamepadSpeed(double gpIn);

    QHash<QString, double>* _addedAxis = new QHash<QString, double>();
    double _input_start = 0.0;
    double _input_end = 1.0;
};

#endif // TCODEFACTORY_H
