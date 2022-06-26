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
                << QVideoFrame::Format_RGB32
                << QVideoFrame::Format_ARGB32
                << QVideoFrame::Format_ARGB32_Premultiplied
                << QVideoFrame::Format_RGB565
                << QVideoFrame::Format_RGB555;
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

bool isStopped = false;
bool XVideoSurface::start(const QVideoSurfaceFormat &format)
{
    isStopped = false;
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
    isStopped = true;
    QAbstractVideoSurface::stop();
    fnClearPixmap();
}

void XVideoSurface::fnClearPixmap()
{
    imageCaptured = QImage();
}
bool XVideoSurface::present(const QVideoFrame &frame)
{
    if(!isStopped) {
    if (surfaceFormat().pixelFormat() != frame.pixelFormat()
            || surfaceFormat().frameSize() != frame.size()) {
        setError(IncorrectFormatError);
        return false;
    } else {
//        if(!imageCaptured.isNull()){
//            qDebug() << "image captured: "+ QString::number(frame.endTime());
//            emit fnSurfaceStopped(imageCaptured);
//        }
        currentFrame = QVideoFrame(frame);

        if(!currentFrame.map(QAbstractVideoBuffer::ReadOnly))
        {
           setError(ResourceError);
           return false;
        }
        if(imageCaptured.isNull()){
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

    }
    return false;
}
