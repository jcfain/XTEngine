#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include <QPixmap>
#include "XTEngine_global.h"

class XTENGINE_EXPORT ImageFactory
{
public:
    ImageFactory();
public:
    static QPixmap* resize(QString filepath, QSize thumbSize);
private:
    static QSize calculateMaxSize(QSize size);
};

#endif // IMAGEFACTORY_H
