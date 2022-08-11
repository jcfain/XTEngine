#include "imagefactory.h"
#include <QFileInfo>
#include <QIcon>
#include <QSize>
#include "../tool/xmath.h"
#include "../lookup/Constants.h"
#include "lib/handler/settingshandler.h"


QPixmap ImageFactory::resize(QString filepath, QSize size)
{
    if(!QFileInfo::exists(filepath))
        filepath = LibraryThumbNail::ERROR_IMAGE;
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
    return {size.width(), (int)round(size.height()-size.height()/3.5)};
}

QHash<QString, QPixmap> ImageFactory::_imageCache;
