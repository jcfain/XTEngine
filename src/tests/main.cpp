#include <QCoreApplication>
#include <QTest>

#include "t_imagefactory.h"
#include "t_funscripthandler.h"
#include "t_scriptinfo.h"
#include "t_tcodehandler.h"

int main(int argc, char *argv[])
{
    // QCoreApplication is sufficient: the functions under test use QImage/
    // QFile only (no QPixmap/GUI), so no platform plugin is required.
    QCoreApplication app(argc, argv);

    int exitCode = 0;
    T_ImageFactory imageFactoryTests;
    exitCode |= QTest::qExec(&imageFactoryTests, argc, argv);
    T_FunscriptHandler funscriptTests;
    exitCode |= QTest::qExec(&funscriptTests, argc, argv);
    T_ScriptInfo scriptInfoTests;
    exitCode |= QTest::qExec(&scriptInfoTests, argc, argv);
    T_TCodeHandler tcodeTests;
    exitCode |= QTest::qExec(&tcodeTests, argc, argv);
    return exitCode;
}
