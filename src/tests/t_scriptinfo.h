#ifndef T_SCRIPTINFO_H
#define T_SCRIPTINFO_H

#include <QObject>
#include "lib/struct/ScriptInfo.h"

// Unit tests for the ScriptInfo metadata struct: the ==/!= operators (which
// drive the metadataChanged detection in the library) and the JSON /
// QDataStream serialization of the funscript metadata fields (version,
// inverted, hasInverted, metadata, hasMetadata).
class T_ScriptInfo : public QObject
{
    Q_OBJECT

private slots:
    // Two identical ScriptInfo objects compare equal (and not unequal).
    void equalityIdentical();

    // Flipping any single metadata field must make the objects unequal.
    void equalityDiffersOnMetadataField();
    void equalityDiffersOnMetadataField_data();

    // toJson -> fromJson returns an equal object (full and default).
    void jsonRoundTrip();

    // Old saved metadata without the 0.6.11 keys loads with defaults.
    void fromJsonToleratesMissingKeys();

    // QDataStream << -> >> returns an equal object.
    void dataStreamRoundTrip();

    // FunscriptMetadata toJson -> fromJson returns an equal object.
    void metadataJsonRoundTrip();

private:
    // A ScriptInfo with every metadata field set (used as the round-trip /
    // equality baseline).
    static ScriptInfo makeFullScriptInfo();
};

#endif // T_SCRIPTINFO_H