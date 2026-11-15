#ifndef T_IMAGEFACTORY_H
#define T_IMAGEFACTORY_H

#include <QObject>

// Unit tests for ImageFactory's animated-image detection and static
// first-frame generation (the hover-to-animate thumb feature).
//
// Fixtures live in data/ (committed to the repo) so the tests have no
// runtime dependency on ffmpeg or any other image tooling.
class T_ImageFactory : public QObject
{
    Q_OBJECT

private slots:
    // isAnimatedImageFile()
    void animatedGifDetected();
    void staticGifDetected();
    void animatedWebpDetected();
    void staticWebpNotDetected();
    void apngDetected();
    void staticPngNotDetected();
    void animatedAvifDetected();
    void staticAvifNotDetected();
    void missingFileNotAnimated();
    void resourcePathNotAnimated();
    void emptyPathNotAnimated();

    // staticFirstFrameJpeg()
    void firstFrameFromAnimatedWebp();
    void firstFrameFromGif();
    void firstFrameFromPng();
    void firstFrameMissingFileIsEmpty();
    void firstFrameResourcePathIsEmpty();
    void firstFrameEmptyPathIsEmpty();
    void firstFrameAvifIsEmpty();
    void firstFrameCacheHit();
    void firstFrameCacheInvalidatedOnFileChange();

private:
    QString dataFile(const QString &name) const;
    static bool isJpeg(const QByteArray &bytes);
};

#endif // T_IMAGEFACTORY_H
