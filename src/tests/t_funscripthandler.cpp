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

QString T_FunscriptHandler::writeScriptFile(const QJsonObject &root) const
{
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

// ---------------------------------------------------------------------------
// Script metadata parsing (parseScriptInfo / readScriptJson)
// ---------------------------------------------------------------------------

QJsonObject T_FunscriptHandler::fullMetadataJson() const
{
    QJsonObject metadata;
    metadata["creator"] = "creator";
    metadata["original_name"] = "original name";
    metadata["url"] = "https://example.com/script";
    metadata["url_video"] = "https://example.com/video";
    QJsonArray tags;
    tags.append("tag one");
    tags.append("tag two");
    metadata["tags"] = tags;
    QJsonArray performers;
    performers.append("performer one");
    metadata["performers"] = performers;
    metadata["paid"] = true;
    metadata["comment"] = "a comment";
    metadata["original_total_duration_ms"] = "123456";
    QJsonArray bookmarks;
    QJsonObject bookmark;
    bookmark["name"] = "bookmark one";
    bookmark["time"] = "00:01:30.500";
    bookmarks.append(bookmark);
    metadata["bookmarks"] = bookmarks;
    QJsonArray chapters;
    QJsonObject chapter;
    chapter["name"] = "chapter one";
    chapter["startTime"] = "00:00:00.000";
    chapter["endTime"] = "00:02:00.000";
    chapters.append(chapter);
    metadata["chapters"] = chapters;

    QJsonObject root;
    root["version"] = "1.1";
    root["inverted"] = true;
    root["metadata"] = metadata;
    return root;
}

void T_FunscriptHandler::fullMetadataParsed()
{
    ScriptInfo script;
    FunscriptHandler::parseScriptInfo(fullMetadataJson(), script);

    QCOMPARE(script.version, QString("1.1"));
    QCOMPARE(script.inverted, true);
    QVERIFY(script.hasInverted);
    QVERIFY(script.hasMetadata);
    QCOMPARE(script.metadata.creator, QString("creator"));
    QCOMPARE(script.metadata.original_name, QString("original name"));
    QCOMPARE(script.metadata.url, QString("https://example.com/script"));
    QCOMPARE(script.metadata.url_video, QString("https://example.com/video"));
    QCOMPARE(script.metadata.tags, QList<QString>({"tag one", "tag two"}));
    QCOMPARE(script.metadata.performers, QList<QString>({"performer one"}));
    QCOMPARE(script.metadata.paid, true);
    QVERIFY(script.metadata.hasPaid);
    QCOMPARE(script.metadata.comment, QString("a comment"));
    QCOMPARE(script.metadata.original_total_duration_ms, qint64(123456));
    QCOMPARE(script.metadata.bookmarks, QList<FunscriptBookmark>({{"bookmark one", 90500}}));
    QCOMPARE(script.metadata.chapters, QList<FunscriptChapter>({{"chapter one", 0, 120000}}));
}

void T_FunscriptHandler::absentPropertiesKeepDefaults()
{
    ScriptInfo script;
    FunscriptHandler::parseScriptInfo(QJsonObject(), script);

    QCOMPARE(script.version, QString());
    QCOMPARE(script.inverted, false);
    QVERIFY(!script.hasInverted);
    QVERIFY(!script.hasMetadata);
    QCOMPARE(script.metadata, FunscriptMetadata());
}

void T_FunscriptHandler::invertedFalseIsPresent()
{
    QJsonObject root;
    root["inverted"] = false;

    ScriptInfo script;
    FunscriptHandler::parseScriptInfo(root, script);

    // The property exists with a false value: the flag must be set so the
    // UI can show "false" instead of "Unknown".
    QCOMPARE(script.inverted, false);
    QVERIFY(script.hasInverted);
}

void T_FunscriptHandler::paidFalseIsPresent()
{
    QJsonObject metadata;
    metadata["paid"] = false;
    QJsonObject root;
    root["metadata"] = metadata;

    ScriptInfo script;
    FunscriptHandler::parseScriptInfo(root, script);

    QVERIFY(script.hasMetadata);
    QCOMPARE(script.metadata.paid, false);
    QVERIFY(script.metadata.hasPaid);
}

void T_FunscriptHandler::wrongTypesIgnored()
{
    QJsonObject root;
    root["version"] = 1.1;          // number, not string
    root["inverted"] = "true";      // string, not bool
    root["metadata"] = QJsonArray(); // array, not object

    ScriptInfo script;
    FunscriptHandler::parseScriptInfo(root, script);

    QCOMPARE(script.version, QString());
    QVERIFY(!script.hasInverted);
    QVERIFY(!script.hasMetadata);
    QCOMPARE(script.metadata, FunscriptMetadata());
}

void T_FunscriptHandler::durationParsedFromString()
{
    // Per the funscript spec original_total_duration_ms is a string; it is
    // read through toString(), so a string value must parse.
    QJsonObject metadataString;
    metadataString["original_total_duration_ms"] = "654321";
    QJsonObject rootString;
    rootString["metadata"] = metadataString;

    ScriptInfo scriptString;
    FunscriptHandler::parseScriptInfo(rootString, scriptString);
    QCOMPARE(scriptString.metadata.original_total_duration_ms, qint64(654321));
}

void T_FunscriptHandler::parsesRegularScriptFile()
{
    const QString path = writeScriptFile(fullMetadataJson());
    QVERIFY(!path.isEmpty());

    ScriptInfo script;
    script.path = path;
    FunscriptHandler::parseScriptInfo(script);

    QCOMPARE(script.version, QString("1.1"));
    QVERIFY(script.hasInverted);
    QVERIFY(script.hasMetadata);
    QCOMPARE(script.metadata.creator, QString("creator"));
}

void T_FunscriptHandler::sfmaUsesTrackProperties()
{
    QJsonObject stroke;
    stroke["version"] = "1.0";

    QJsonObject twistMetadata;
    twistMetadata["creator"] = "twist creator";
    QJsonObject twist;
    twist["version"] = "1.1";
    twist["inverted"] = true;
    twist["metadata"] = twistMetadata;

    QJsonObject channels;
    channels["stroke"] = stroke;
    channels["twist"] = twist;
    QJsonObject root;
    root["channels"] = channels;

    const QString path = writeScriptFile(root);
    QVERIFY(!path.isEmpty());

    ScriptInfo script;
    script.path = path;
    script.track = "twist";
    FunscriptHandler::parseScriptInfo(script);

    // The twist track's properties, not the stroke track's.
    QCOMPARE(script.version, QString("1.1"));
    QVERIFY(script.hasInverted);
    QVERIFY(script.hasMetadata);
    QCOMPARE(script.metadata.creator, QString("twist creator"));
}

void T_FunscriptHandler::sfmaMissingTrackFallsBackToRoot()
{
    QJsonObject stroke;
    stroke["version"] = "1.0";
    QJsonObject channels;
    channels["stroke"] = stroke;
    QJsonObject root;
    root["channels"] = channels;

    const QString path = writeScriptFile(root);
    QVERIFY(!path.isEmpty());

    ScriptInfo script;
    script.path = path;
    script.track = "twist"; // not present in the channels object
    FunscriptHandler::parseScriptInfo(script);

    // Falls back to the root object, which has no properties of its own.
    QCOMPARE(script.version, QString());
    QVERIFY(!script.hasInverted);
    QVERIFY(!script.hasMetadata);
}

void T_FunscriptHandler::missingFileLeavesScriptUnchanged()
{
    ScriptInfo script;
    script.path = m_tempDir.filePath("does-not-exist.funscript");
    FunscriptHandler::parseScriptInfo(script);

    QCOMPARE(script.version, QString());
    QVERIFY(!script.hasInverted);
    QVERIFY(!script.hasMetadata);
    QCOMPARE(script.metadata, FunscriptMetadata());
}

void T_FunscriptHandler::invalidJsonLeavesScriptUnchanged()
{
    const QString path = m_tempDir.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + ".funscript");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not json {");
    file.close();

    ScriptInfo script;
    script.path = path;
    FunscriptHandler::parseScriptInfo(script);

    QCOMPARE(script.version, QString());
    QVERIFY(!script.hasInverted);
    QVERIFY(!script.hasMetadata);
    QCOMPARE(script.metadata, FunscriptMetadata());
}

void T_FunscriptHandler::readScriptJsonValidFile()
{
    const QString path = writeScriptFile(fullMetadataJson());
    QVERIFY(!path.isEmpty());

    QJsonObject json = FunscriptHandler::readScriptJson(path);
    QCOMPARE(json["version"].toString(), QString("1.1"));
    QVERIFY(json["metadata"].isObject());
}

void T_FunscriptHandler::readScriptJsonMissingFile()
{
    QJsonObject json = FunscriptHandler::readScriptJson(m_tempDir.filePath("does-not-exist.funscript"));
    QVERIFY(json.isEmpty());
}

void T_FunscriptHandler::readScriptJsonInvalidFile()
{
    const QString path = m_tempDir.filePath(QUuid::createUuid().toString(QUuid::WithoutBraces) + ".funscript");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("this is not json {");
    file.close();

    QJsonObject json = FunscriptHandler::readScriptJson(path);
    QVERIFY(json.isEmpty());
}