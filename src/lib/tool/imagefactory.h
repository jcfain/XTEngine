#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include <QPixmap>
#include <QHash>
#include "XTEngine_global.h"

class XTENGINE_EXPORT ImageFactory
{
public:
    static QPixmap resize(QString filepath, QSize thumbSize);
    static QPixmap resizeCache(QString filepath, QString key, QSize size, bool bustCache = false);
    static void removeCache(QString key);
    static void clearCache();
    static QSize lastSize();
private:
    static QSize calculateMaxSize(QSize size);
    static QHash<QString, QPixmap> _imageCache;
};

#endif // IMAGEFACTORY_H
