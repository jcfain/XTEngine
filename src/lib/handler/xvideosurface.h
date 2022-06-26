#ifndef XVIDEOSURFACE_H
#define XVIDEOSURFACE_H

#include <QAbstractVideoSurface>
#include <QImage>
#include <QPixmap>
#include <QRect>
#include <QVideoFrame>
#include <qabstractvideosurface.h>
#include <qvideosurfaceformat.h>

class XVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
signals:
    void frameCapture(QImage pix);
    void frameCaptureError(QString error);
public:
    XVideoSurface(QObject *parent = 0);
    void stop();
    bool start(const QVideoSurfaceFormat &format);
    void fnClearPixmap();
private:
    QImage::Format imageFormat;
    QImage imageCaptured;
    QRect targetRect;
    QRect sourceRect;
    QVideoFrame currentFrame;

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(
            QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const;
    bool isFormatSupported(const QVideoSurfaceFormat &format) const;



    bool present(const QVideoFrame &frame);

    QRect videoRect() const { return targetRect; }
};

#endif // XVIDEOSURFACE_H
