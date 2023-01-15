#ifndef CRYPTHANDLER_H
#define CRYPTHANDLER_H

#include <QObject>
#include "XTEngine_global.h"

class XTENGINE_EXPORT CryptHandler : public QObject
{
    Q_OBJECT
public:
    explicit CryptHandler(QObject *parent = nullptr);

signals:

};

#endif // CRYPTHANDLER_H
