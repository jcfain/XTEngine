#ifndef IMAGEFACTORY_H
#define IMAGEFACTORY_H

#include <QPixmap>
#include <QHash>
#include <QDateTime>
#include "XTEngine_global.h"

class XTENGINE_EXPORT ImageFactory
{
public:
    // Entry for the static first-frame JPEG cache (see staticFirstFrameJpeg).
    struct StaticThumbCacheEntry
    {
        qint64 fileSize = 0;
        QDateTime fileTime;
        QByteArray jpeg;
    };
    static QPixmap resize(QString filepath, QSize thumbSize);
    static QPixmap resizeCache(QString filepath, QString key, QSize size, bool bustCache = false);
    // JPEG-encoded first frame of an animated image (GIF/WebP/...) for the
    // web front-end's hover-to-animate feature. Returns an empty QByteArray
    // when the image can't be decoded (callers should fall back to serving
    // the original file). Results are cached per file and invalidated on
    // size/mtime change so repeated hover swaps don't re-encode.
    static QByteArray staticFirstFrameJpeg(const QString &filepath, int quality = 90);
    // True when the image file contains animation (GIF, animated WebP, APNG,
    // animated AVIF). Detected from the file header so it works for any
    // format the browser would animate. Non-existent files and :// resources
    // are not animated.
    static bool isAnimatedImageFile(const QString &filepath);
    static void removeCache(QString key);
    static void clearCache();
    static QSize lastSize();
private:
    static QSize calculateMaxSize(QSize size);
    static QHash<QString, QPixmap> _imageCache;
    static QHash<QString, StaticThumbCacheEntry> _staticThumbCache;
};

#endif // IMAGEFACTORY_H
