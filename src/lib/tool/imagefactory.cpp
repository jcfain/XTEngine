#include "imagefactory.h"
#include <QFileInfo>
#include <QIcon>
#include <QSize>
#include <QBuffer>
#include <QDateTime>
#include "../tool/xmath.h"
#include "lib/handler/settingshandler.h"


QPixmap ImageFactory::resize(QString filepath, QSize size)
{
    if(!QFileInfo::exists(filepath)) {
        filepath = ERROR_IMAGE;
    }
    QPixmap bgPixmap(filepath);
    if(bgPixmap.width() == size.width())
        return bgPixmap;
    //QSize maxThumbSize = SettingsHandler::getMaxThumbnailSize();
    //int newHeight = round((float)bgPixmap.height() / bgPixmap.width() * 1080);
    //QSize newSize = calculateSize(thumbSize);
    //QPixmap scaled = bgPixmap.scaledToHeight(thumbSize.height(), Qt::TransformationMode::SmoothTransformation);
    //QSize maxHeight = calculateMaxSize(thumbSize);

//    if(scaled.height() > maxHeight.height())
//    {
//        scaled = bgPixmap.scaled(maxHeight, Qt::AspectRatioMode::KeepAspectRatio);
////        QRect rect(0,0,scaled.width(), newHeight);
////        scaled = scaled.copy(rect);
//    }
    //bgPixmap = QPixmap();
    return bgPixmap.scaledToHeight(size.height(), Qt::TransformationMode::SmoothTransformation);;
}

QPixmap ImageFactory::resizeCache(QString filepath, QString key, QSize size, bool bustCache) {
    QPixmap pixMap;
    auto pixMapExists = _imageCache.contains(filepath);
    if(bustCache || !pixMapExists) {
        pixMap = resize(filepath, size);
        _imageCache.insert(filepath, pixMap);
    }
    return _imageCache.value(filepath);
}
void ImageFactory::removeCache(QString fileName) {
    _imageCache.remove(fileName);
}
void ImageFactory::clearCache() {
    _imageCache.clear();
}

QSize ImageFactory::calculateMaxSize(QSize size)
{
    return {size.width(), (int)qRound(size.height()-size.height()/3.5)};
}

QHash<QString, QPixmap> ImageFactory::_imageCache;
QHash<QString, ImageFactory::StaticThumbCacheEntry> ImageFactory::_staticThumbCache;

QByteArray ImageFactory::staticFirstFrameJpeg(const QString &filepath, int quality)
{
    if(filepath.isEmpty() || filepath.startsWith(":"))
        return {};
    QFileInfo info(filepath);
    if(!info.exists())
        return {};
    auto it = _staticThumbCache.constFind(filepath);
    if(it != _staticThumbCache.constEnd()
       && it->fileSize == info.size()
       && it->fileTime == info.lastModified())
    {
        return it->jpeg;
    }
    // QImage (not QPixmap) so this works on non-GUI threads; Qt image readers
    // only load the first frame of animated images.
    QImage image(filepath);
    if(image.isNull())
        return {};
    if(image.height() > 500)
        image = image.scaledToHeight(500, Qt::SmoothTransformation);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    if(!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "JPG", quality))
        return {};
    buffer.close();
    _staticThumbCache.insert(filepath, {info.size(), info.lastModified(), bytes});
    return bytes;
}

bool ImageFactory::isAnimatedImageFile(const QString &filepath)
{
    if(filepath.isEmpty() || filepath.startsWith(":"))
        return false;
    QFileInfo fileInfo(filepath);
    if(!fileInfo.exists())
        return false;
    const QString ext = fileInfo.suffix().toLower();
    if(ext == "gif")
        return true;
    QFile file(filepath);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    if(ext == "webp")
    {
        // Animated WebP: RIFF/WEBP container with a VP8X chunk whose flags
        // have the animation bit (0x02) set (ANMF chunks present).
        const QByteArray header = file.read(24);
        if(header.size() >= 24
           && header.left(4) == "RIFF"
           && header.mid(8, 4) == "WEBP"
           && header.mid(12, 4) == "VP8X")
        {
            const quint32 flags = qFromLittleEndian<quint32>(header.constData() + 20);
            return (flags & 0x02) != 0;
        }
        return false;
    }
    if(ext == "png")
    {
        // APNG: an acTL chunk must be present before the first IDAT chunk.
        const QByteArray data = file.read(65536);
        if(data.size() < 8 || data.left(8) != "\x89PNG\r\n\x1a\n")
            return false;
        int offset = 8;
        while(offset + 8 <= data.size())
        {
            const quint32 length = qFromBigEndian<quint32>(data.constData() + offset);
            const QByteArray type = data.mid(offset + 4, 4);
            if(type == "acTL")
                return true;
            if(type == "IDAT" || length > data.size())
                break;
            offset += 12 + length;
        }
        return false;
    }
    if(ext == "avif")
    {
        // Animated AVIF: an 'anim' box inside the top-level 'meta' box.
        const QByteArray data = file.read(131072);
        int offset = 0;
        while(offset + 8 <= data.size())
        {
            const quint32 size = qFromBigEndian<quint32>(data.constData() + offset);
            const QByteArray type = data.mid(offset + 4, 4);
            if(size < 8)
                break;
            if(type == "meta")
            {
                // meta box: 4 bytes of version/flags, then child boxes
                int childOffset = offset + 12;
                const int metaEnd = qMin<qsizetype>(static_cast<qsizetype>(offset + size), data.size());
                while(childOffset + 8 <= metaEnd)
                {
                    const quint32 childSize = qFromBigEndian<quint32>(data.constData() + childOffset);
                    const QByteArray childType = data.mid(childOffset + 4, 4);
                    if(childType == "anim")
                        return true;
                    if(childSize < 8)
                        break;
                    childOffset += childSize;
                }
                break;
            }
            offset += size;
        }
        return false;
    }
    return false;
}
