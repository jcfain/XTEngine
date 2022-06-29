#include "imagefactory.h"
#include <QFileInfo>
#include <QIcon>
#include <QSize>
#include "../tool/xmath.h"
#include "../lookup/Constants.h"

ImageFactory::ImageFactory()
{

}

QPixmap ImageFactory::resize(QString filepath, QSize thumbSize)
{
    if(!QFileInfo::exists(filepath))
        filepath = LibraryThumbNail::ERROR_IMAGE;
    QPixmap bgPixmap(filepath);
    if(bgPixmap.width() == thumbSize.width())
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
    return bgPixmap.scaledToHeight(thumbSize.height(), Qt::TransformationMode::SmoothTransformation);;
}


QSize ImageFactory::calculateMaxSize(QSize size)
{
    return {size.width(), (int)round(size.height()-size.height()/3.5)};
}
