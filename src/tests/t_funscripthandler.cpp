#include "t_funscripthandler.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QTest>
#include <QUuid>

#include "lib/handler/funscripthandler.h"
#include "lib/lookup/tcodechannellookup.h"

void T_FunscriptHandler::initTestCase()
{
    // Give TCodeChannelLookup a Default profile (v4) so the handlers under
    // test see the same channel model the application starts with.
    TCodeChannelLookup::instance()->load(nullptr, true);
    TCodeChannelLookup::resetLiveXRange();
}

QString T_FunscriptHandler::writeScript(const QList<QPair<qint64, int>> &actions) const
{
    QJsonArray actionArray;
    for(const auto &action : actions) {
        QJsonObject object;
        object["at"] = action.first;
        object["pos"] = action.second;
        actionArray.append(object);
    }
    QJsonObject root;
    root["version"] = "1.0";
    root["actions"] = actionArray;

    const QString path = m_tempDir.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + ".funscript");
    QFile file(path);
    if(!file.open(QIODevice::WriteOnly))
        return QString();
    file.write(QJsonDocument(root).toJson());
    file.close();
    return path;
}

QList<int> T_FunscriptHandler::playScript(FunscriptHandler &handler, qint64 duration, qint64 step)
{
    QList<int> positions;
    for(qint64 time = 0; time <= duration; time += step) {
        auto action = handler.getPosition(Track::Stroke, time, 0);
        if(action)
            positions.append(action->pos);
    }
    return positions;
}

// ---------------------------------------------------------------------------
// Amplitude modifier
// ---------------------------------------------------------------------------

void T_FunscriptHandler::identityAt100Percent()
{
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 40}, {3000, 60}, {4000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 100.0);
    const QList<int> positions = playScript(handler, 6000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({40, 60, 40, 60, 40}));
}

void T_FunscriptHandler::finalPointEmitted()
{
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 40}, {3000, 60}, {4000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    const QList<int> positions = playScript(handler, 6000);

    QCOMPARE(positions.size(), 5);
    QCOMPARE(positions.last(), 40); // the raw final point must be emitted
}

void T_FunscriptHandler::amplitude150ExpandsBand()
{
    // Band 40-60, center 50: 40 -> 35, 60 -> 65.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 40}, {3000, 60}, {4000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 150.0);
    const QList<int> positions = playScript(handler, 6000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({35, 65, 35, 65, 35}));
}

void T_FunscriptHandler::amplitude50ContractsBand()
{
    // Band 40-60, center 50: 40 -> 45, 60 -> 55.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 40}, {3000, 60}, {4000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 50.0);
    const QList<int> positions = playScript(handler, 6000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({45, 55, 45, 55, 45}));
}

void T_FunscriptHandler::amplitude0IsFlat()
{
    // 0% rests at the band center: 40-60 oscillation becomes a flat 50.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 40}, {3000, 60}, {4000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 0.0);
    const QList<int> positions = playScript(handler, 6000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({50, 50, 50, 50, 50}));
}

void T_FunscriptHandler::clampedToSpecRange()
{
    // Full-range 0-100 script expanded by 150%: 0 -> -75 and 100 -> 125
    // must clamp to the 0-100 spec range.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 0}, {1000, 100}, {2000, 0}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 150.0);
    const QList<int> positions = playScript(handler, 4000);
    FunscriptHandler::resetModifier(Track::Stroke);

    for(int position : positions)
        QVERIFY2(position >= 0 && position <= 100,
                 qPrintable(QString("position %1 out of spec range").arg(position)));
    QCOMPARE(positions, QList<int>({0, 100, 0}));
}

void T_FunscriptHandler::roamingBandsScaleIndependently()
{
    // The user's roaming example: bands 0-20, 60-70 and 70-100.
    // Turning points: 0(0) 1(20) 2(0) 5(70) 6(60) 7(100) 8(70) 9(100)
    // Centers:        10  10  10  35  35  35  65  80  85  85
    // At 150%:        0   25   0   13  73  88  58  100 63  100
    FunscriptHandler handler;
    const QString path = writeScript({
        {0, 0}, {1000, 20}, {2000, 0}, {3000, 20}, {4000, 60},
        {5000, 70}, {6000, 60}, {7000, 100}, {8000, 70}, {9000, 100}
    });
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 150.0);
    const QList<int> positions = playScript(handler, 11000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({0, 25, 0, 13, 73, 88, 58, 100, 63, 100}));
}

void T_FunscriptHandler::flatSectionStaysFlat()
{
    // Only turning points are the two 40s (edges), so the whole script
    // scales around center 40: the flat 60-run must move together to 70,
    // not oscillate around spurious per-point centers.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 40}, {1000, 60}, {2000, 60}, {3000, 60}, {4000, 40}, {5000, 40}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 150.0);
    const QList<int> positions = playScript(handler, 7000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({40, 70, 70, 70, 40, 40}));
}

void T_FunscriptHandler::singleActionScript()
{
    // A single point is its own center: 60 stays 60 at 150%.
    FunscriptHandler handler;
    const QString path = writeScript({{0, 60}});
    QVERIFY(!path.isEmpty());
    QVERIFY(handler.load(Track::Stroke, path));

    FunscriptHandler::setModifier(Track::Stroke, 150.0);
    const QList<int> positions = playScript(handler, 2000);
    FunscriptHandler::resetModifier(Track::Stroke);

    QCOMPARE(positions, QList<int>({60}));
}