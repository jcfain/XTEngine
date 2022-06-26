#include "xvideosurface.h"

XVideoSurface::XVideoSurface(QObject *parent)
    : QAbstractVideoSurface(parent)
    , imageFormat(QImage::Format_Invalid)
{
}

QList<QVideoFrame::PixelFormat> XVideoSurface::supportedPixelFormats(
        QAbstractVideoBuffer::HandleType handleType) const
{
    if (handleType == QAbstractVideoBuffer::NoHandle) {
        return QList<QVideoFrame::PixelFormat>()
             << QVideoFrame::Format_ARGB32
             << QVideoFrame::Format_ARGB32_Premultiplied
             << QVideoFrame::Format_RGB32
             << QVideoFrame::Format_RGB24
             << QVideoFrame::Format_RGB565
             << QVideoFrame::Format_RGB555
             << QVideoFrame::Format_ARGB8565_Premultiplied
             << QVideoFrame::Format_BGRA32
             << QVideoFrame::Format_BGRA32_Premultiplied
             << QVideoFrame::Format_BGR32
             << QVideoFrame::Format_BGR24
             << QVideoFrame::Format_BGR565
             << QVideoFrame::Format_BGR555
             << QVideoFrame::Format_BGRA5658_Premultiplied
             << QVideoFrame::Format_AYUV444
             << QVideoFrame::Format_AYUV444_Premultiplied
             << QVideoFrame::Format_YUV444
             << QVideoFrame::Format_YUV420P
             << QVideoFrame::Format_YV12
             << QVideoFrame::Format_UYVY
             << QVideoFrame::Format_YUYV
             << QVideoFrame::Format_NV12
             << QVideoFrame::Format_NV21
             << QVideoFrame::Format_IMC1
             << QVideoFrame::Format_IMC2
             << QVideoFrame::Format_IMC3
             << QVideoFrame::Format_IMC4
             << QVideoFrame::Format_Y8
             << QVideoFrame::Format_Y16
             << QVideoFrame::Format_Jpeg
             << QVideoFrame::Format_CameraRaw
             << QVideoFrame::Format_AdobeDng;
    } else {
        return QList<QVideoFrame::PixelFormat>();
    }
}

bool XVideoSurface::isFormatSupported(const QVideoSurfaceFormat &format) const
{
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();

    return imageFormat != QImage::Format_Invalid
            && !size.isEmpty()
            && format.handleType() == QAbstractVideoBuffer::NoHandle;
}

bool XVideoSurface::start(const QVideoSurfaceFormat &format)
{
    const QImage::Format imageFormat = QVideoFrame::imageFormatFromPixelFormat(format.pixelFormat());
    const QSize size = format.frameSize();

    if (imageFormat != QImage::Format_Invalid && !size.isEmpty()) {
        this->imageFormat = imageFormat;
        QAbstractVideoSurface::start(format);
        return true;
    } else {
        emit frameCaptureError("Format invalid");
        return false;
    }
}

void XVideoSurface::stop()
{
    QAbstractVideoSurface::stop();
    fnClearPixmap();
}

void XVideoSurface::fnClearPixmap()
{
    imageCaptured = QImage();
}
bool XVideoSurface::present(const QVideoFrame &frame)
{
    if (surfaceFormat().pixelFormat() != frame.pixelFormat()
            || surfaceFormat().frameSize() != frame.size()) {
        setError(IncorrectFormatError);
        stop();
        return false;
    }
    else if (frame.isValid())
    {
//        if(!imageCaptured.isNull()){
//            qDebug() << "image captured: "+ QString::number(frame.endTime());
//            emit fnSurfaceStopped(imageCaptured);
//        }
        QVideoFrame currentFrame = QVideoFrame(frame);

        if(!currentFrame.map(QAbstractVideoBuffer::ReadOnly))
        {
           setError(ResourceError);
           stop();
           return false;
        }
        if(imageCaptured.isNull())
        {
            imageCaptured = QImage(
                    currentFrame.bits(),
                    currentFrame.width(),
                    currentFrame.height(),
                    currentFrame.bytesPerLine(),
                    imageFormat);
            //qDebug() << "image captured: "+ QString::number(frame.endTime());
            emit frameCapture(imageCaptured);
        }
        currentFrame.unmap();
        return true;
    }
    return false;
}
