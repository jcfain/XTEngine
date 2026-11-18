#include "t_scriptinfo.h"

#include <QByteArray>
#include <QDataStream>
#include <QJsonObject>
#include <QTest>

ScriptInfo T_ScriptInfo::makeFullScriptInfo()
{
    ScriptInfo script;
    script.name = "name";
    script.filename = "filename";
    script.path = "/path/file.funscript";
    script.track = "twist";
    script.type = ScriptType::ALTERNATE;
    script.containerType = ScriptContainerType::MFS;
    script.containerTypeName = "(twist)";
    script.version = "1.1";
    script.inverted = true;
    script.hasInverted = true;
    script.hasMetadata = true;
    script.metadata.creator = "creator";
    script.metadata.original_name = "original name";
    script.metadata.url = "https://example.com/script";
    script.metadata.url_video = "https://example.com/video";
    script.metadata.tags = {"tag one", "tag two"};
    script.metadata.performers = {"performer one"};
    script.metadata.paid = true;
    script.metadata.hasPaid = true;
    script.metadata.comment = "a comment";
    script.metadata.original_total_duration_ms = 123456;
    script.metadata.bookmarks = {{"bookmark one", 90500}};
    script.metadata.chapters = {{"chapter one", 0, 120000}};
    return script;
}

void T_ScriptInfo::equalityIdentical()
{
    ScriptInfo a = makeFullScriptInfo();
    ScriptInfo b = makeFullScriptInfo();

    QVERIFY(a == b);
    QVERIFY(!(a != b));
}

void T_ScriptInfo::equalityDiffersOnMetadataField_data()
{
    QTest::addColumn<QString>("field");
    QTest::newRow("version") << "version";
    QTest::newRow("inverted") << "inverted";
    QTest::newRow("hasInverted") << "hasInverted";
    QTest::newRow("hasMetadata") << "hasMetadata";
    QTest::newRow("metadata.creator") << "metadata.creator";
    QTest::newRow("metadata.paid") << "metadata.paid";
    QTest::newRow("metadata.hasPaid") << "metadata.hasPaid";
    QTest::newRow("metadata.bookmarks") << "metadata.bookmarks";
}

void T_ScriptInfo::equalityDiffersOnMetadataField()
{
    QFETCH(QString, field);

    ScriptInfo a = makeFullScriptInfo();
    ScriptInfo b = a;
    if (field == "version")
        b.version = "9.9";
    else if (field == "inverted")
        b.inverted = !b.inverted;
    else if (field == "hasInverted")
        b.hasInverted = !b.hasInverted;
    else if (field == "hasMetadata")
        b.hasMetadata = !b.hasMetadata;
    else if (field == "metadata.creator")
        b.metadata.creator = "other creator";
    else if (field == "metadata.paid")
        b.metadata.paid = !b.metadata.paid;
    else if (field == "metadata.hasPaid")
        b.metadata.hasPaid = !b.metadata.hasPaid;
    else if (field == "metadata.bookmarks")
        b.metadata.bookmarks.append({"bookmark two", 180000});

    // Each metadata field participates in the comparison: a change in any
    // single one must be detected (this is what marks an item as
    // metadataChanged during the metadata process).
    QVERIFY2(a != b, qPrintable(QString("field %1 not detected by !=").arg(field)));
    QVERIFY2(!(a == b), qPrintable(QString("field %1 not detected by ==").arg(field)));
}

void T_ScriptInfo::jsonRoundTrip()
{
    ScriptInfo original = makeFullScriptInfo();
    ScriptInfo restored = ScriptInfo::fromJson(ScriptInfo::toJson(original));
    QCOMPARE(restored, original);

    // A default-constructed ScriptInfo round-trips as well.
    ScriptInfo defaultItem;
    QCOMPARE(ScriptInfo::fromJson(ScriptInfo::toJson(defaultItem)), defaultItem);
}

void T_ScriptInfo::fromJsonToleratesMissingKeys()
{
    // Old saved metadata (pre-0.6.11) carries no version/inverted/metadata
    // keys: it must load with defaults instead of failing.
    QJsonObject obj;
    obj["name"] = "name";
    obj["filename"] = "filename";
    obj["path"] = "/path/file.funscript";
    obj["track"] = "";
    obj["type"] = (qint8)ScriptType::MAIN;
    obj["containerType"] = (qint8)ScriptContainerType::BASE;
    obj["containerTypeName"] = "";

    ScriptInfo item = ScriptInfo::fromJson(obj);
    QCOMPARE(item.name, QString("name"));
    QCOMPARE(item.path, QString("/path/file.funscript"));
    QCOMPARE(item.type, ScriptType::MAIN);
    QCOMPARE(item.containerType, ScriptContainerType::BASE);
    QCOMPARE(item.version, QString());
    QCOMPARE(item.inverted, false);
    QVERIFY(!item.hasInverted);
    QVERIFY(!item.hasMetadata);
    QCOMPARE(item.metadata, FunscriptMetadata());
}

void T_ScriptInfo::dataStreamRoundTrip()
{
    ScriptInfo original = makeFullScriptInfo();

    QByteArray buffer;
    {
        QDataStream out(&buffer, QIODevice::WriteOnly);
        out << original;
    }
    ScriptInfo restored;
    {
        QDataStream in(&buffer, QIODevice::ReadOnly);
        in >> restored;
    }
    QCOMPARE(restored, original);
}

void T_ScriptInfo::metadataJsonRoundTrip()
{
    FunscriptMetadata original;
    original.creator = "creator";
    original.original_name = "original name";
    original.url = "https://example.com/script";
    original.url_video = "https://example.com/video";
    original.tags = {"tag one", "tag two"};
    original.performers = {"performer one"};
    original.paid = true;
    original.hasPaid = true;
    original.comment = "a comment";
    original.original_total_duration_ms = 123456;
    original.bookmarks = {{"bookmark one", 90500}};
    original.chapters = {{"chapter one", 0, 120000}};

    QCOMPARE(FunscriptMetadata::fromJson(original.toJson()), original);

    // Defaults: the -1 duration sentinel and the hasPaid flag survive.
    FunscriptMetadata defaults;
    FunscriptMetadata restoredDefaults = FunscriptMetadata::fromJson(defaults.toJson());
    QCOMPARE(restoredDefaults, defaults);
    QCOMPARE(restoredDefaults.original_total_duration_ms, qint64(-1));
    QVERIFY(!restoredDefaults.hasPaid);
}