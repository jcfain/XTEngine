#ifndef XVIDEOSURFACE_H
#define XVIDEOSURFACE_H

#include <QVideoSink>
#include <QImage>
#include <QPixmap>
#include <QRect>
#include <QVideoFrame>
#include <QAbstractVideoBuffer>
#include <qvideoframe.h>

class XVideoSurface : public QVideoSink
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
    QRect targetRect;
    QRect sourceRect;

    QList<QPixelFormat> supportedPixelFormats(QAbstractVideoBuffer:: handleType = QAbstractVideoBuffer::NoHandle) const;
    bool isFormatSupported(const QPixelFormat &format) const;



    bool present(const QVideoFrame &frame);

    QRect videoRect() const { return targetRect; }
};

#endif // XVIDEOSURFACE_H
