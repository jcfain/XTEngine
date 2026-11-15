#include <QCoreApplication>
#include <QTest>

#include "t_imagefactory.h"

int main(int argc, char *argv[])
{
    // QCoreApplication is sufficient: the functions under test use QImage/
    // QFile only (no QPixmap/GUI), so no platform plugin is required.
    QCoreApplication app(argc, argv);
    T_ImageFactory tests;
    return QTest::qExec(&tests, argc, argv);
}
