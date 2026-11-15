#include "t_imagefactory.h"

#include <QDir>
#include <QFile>
#include <QImage>
#include <QTemporaryDir>
#include <QTest>

#include "lib/tool/imagefactory.h"

QString T_ImageFactory::dataFile(const QString &name) const
{
    return QStringLiteral(TEST_DATA_DIR) + QDir::separator() + name;
}

bool T_ImageFactory::isJpeg(const QByteArray &bytes)
{
    // JPEG SOI (FFD8) ... EOI (FFD9) markers.
    return bytes.size() > 4
           && bytes.left(2) == QByteArray::fromHex("FFD8")
           && bytes.right(2) == QByteArray::fromHex("FFD9");
}

// ---------------------------------------------------------------------------
// isAnimatedImageFile()
// ---------------------------------------------------------------------------

void T_ImageFactory::animatedGifDetected()
{
    QVERIFY(ImageFactory::isAnimatedImageFile(dataFile("animated.gif")));
}

void T_ImageFactory::staticGifDetected()
{
    // Every GIF counts as animated (browsers animate any GIF).
    QVERIFY(ImageFactory::isAnimatedImageFile(dataFile("static.gif")));
}

void T_ImageFactory::animatedWebpDetected()
{
    QVERIFY(ImageFactory::isAnimatedImageFile(dataFile("animated.webp")));
}

void T_ImageFactory::staticWebpNotDetected()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(dataFile("static.webp")));
}

void T_ImageFactory::apngDetected()
{
    QVERIFY(ImageFactory::isAnimatedImageFile(dataFile("animated.png")));
}

void T_ImageFactory::staticPngNotDetected()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(dataFile("static.png")));
}

void T_ImageFactory::animatedAvifDetected()
{
    QVERIFY(ImageFactory::isAnimatedImageFile(dataFile("animated.avif")));
}

void T_ImageFactory::staticAvifNotDetected()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(dataFile("static.avif")));
}

void T_ImageFactory::missingFileNotAnimated()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(dataFile("does-not-exist.webp")));
}

void T_ImageFactory::resourcePathNotAnimated()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(":/images/animated.webp"));
}

void T_ImageFactory::emptyPathNotAnimated()
{
    QVERIFY(!ImageFactory::isAnimatedImageFile(""));
}

// ---------------------------------------------------------------------------
// staticFirstFrameJpeg()
// ---------------------------------------------------------------------------

void T_ImageFactory::firstFrameFromAnimatedWebp()
{
    const QByteArray jpeg = ImageFactory::staticFirstFrameJpeg(dataFile("animated.webp"));
    QVERIFY2(!jpeg.isEmpty(), "expected a JPEG-encoded first frame");
    QVERIFY(isJpeg(jpeg));
    QImage image;
    QVERIFY2(image.loadFromData(jpeg), "encoded bytes must decode as an image");
    QVERIFY(!image.isNull());
}

void T_ImageFactory::firstFrameFromGif()
{
    const QByteArray jpeg = ImageFactory::staticFirstFrameJpeg(dataFile("animated.gif"));
    QVERIFY2(!jpeg.isEmpty(), "expected a JPEG-encoded first frame");
    QVERIFY(isJpeg(jpeg));
    QImage image;
    QVERIFY2(image.loadFromData(jpeg), "encoded bytes must decode as an image");
    QVERIFY(!image.isNull());
}

void T_ImageFactory::firstFrameFromPng()
{
    const QByteArray jpeg = ImageFactory::staticFirstFrameJpeg(dataFile("static.png"));
    QVERIFY2(!jpeg.isEmpty(), "expected a JPEG-encoded first frame");
    QVERIFY(isJpeg(jpeg));
    QImage image;
    QVERIFY2(image.loadFromData(jpeg), "encoded bytes must decode as an image");
    QVERIFY(!image.isNull());
}

void T_ImageFactory::firstFrameMissingFileIsEmpty()
{
    QVERIFY(ImageFactory::staticFirstFrameJpeg(dataFile("does-not-exist.webp")).isEmpty());
}

void T_ImageFactory::firstFrameResourcePathIsEmpty()
{
    QVERIFY(ImageFactory::staticFirstFrameJpeg(":/images/animated.webp").isEmpty());
}

void T_ImageFactory::firstFrameEmptyPathIsEmpty()
{
    QVERIFY(ImageFactory::staticFirstFrameJpeg("").isEmpty());
}

void T_ImageFactory::firstFrameAvifIsEmpty()
{
    // Qt has no AVIF decoder, so encoding must fail gracefully (empty result)
    // - the HTTP handler falls back to serving the original file in that case.
    QVERIFY(ImageFactory::staticFirstFrameJpeg(dataFile("animated.avif")).isEmpty());
}

void T_ImageFactory::firstFrameCacheHit()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString tempFile = tempDir.filePath("cached.webp");
    QVERIFY(QFile::copy(dataFile("animated.webp"), tempFile));

    const QByteArray first = ImageFactory::staticFirstFrameJpeg(tempFile);
    QVERIFY(!first.isEmpty());
    // Second call for the unchanged file must return the cached bytes.
    QCOMPARE(ImageFactory::staticFirstFrameJpeg(tempFile), first);
}

void T_ImageFactory::firstFrameCacheInvalidatedOnFileChange()
{
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const QString tempFile = tempDir.filePath("changed.webp");
    QVERIFY(QFile::copy(dataFile("animated.webp"), tempFile));

    const QByteArray before = ImageFactory::staticFirstFrameJpeg(tempFile);
    QVERIFY(!before.isEmpty());

    // Replace the file with an image whose first frame is visibly different:
    // size and mtime change, so the cache entry must be invalidated and the
    // new file re-encoded.
    QVERIFY(QFile::remove(tempFile));
    QVERIFY(QFile::copy(dataFile("red.webp"), tempFile));

    const QByteArray after = ImageFactory::staticFirstFrameJpeg(tempFile);
    QVERIFY(!after.isEmpty());
    QVERIFY2(before != after, "cache must be invalidated when the file changes");
}
