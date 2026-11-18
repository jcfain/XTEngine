#ifndef T_FUNSCRIPTHANDLER_H
#define T_FUNSCRIPTHANDLER_H

#include <QJsonObject>
#include <QObject>
#include <QList>
#include <QPair>
#include <QTemporaryDir>

// Unit tests for FunscriptHandler: the amplitude modifier (local-center
// scaling), the getPosition() action stream, and the script metadata parsing
// (parseScriptInfo / readScriptJson).
//
// Scripts are written to temporary files and played back by stepping
// getPosition() forward in small increments, mimicking the player's timer
// loop. Expected values are computed by hand from the scaling formula:
//     modified = clamp(center + (pos - center) * modifier, 0, 100)
// where center is the midpoint of the two turning points bracketing the
// point (or the point itself at the script edges).
class T_FunscriptHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // 100% modifier is the identity: every raw position is emitted unchanged.
    void identityAt100Percent();

    // Regression: the last point of a script must be emitted (it used to be
    // dropped because nextIndex was -1 for it).
    void finalPointEmitted();

    // 150% expands each oscillation band around its own center.
    void amplitude150ExpandsBand();

    // 50% contracts each band toward its center.
    void amplitude50ContractsBand();

    // 0% rests at the band center: a 40-60 oscillation becomes a flat 50.
    void amplitude0IsFlat();

    // Expansion of a full-range (0-100) script must stay within 0-100.
    void clampedToSpecRange();

    // A roaming script (bands at different positions of the range) scales
    // each band independently without clipping (user's roaming example).
    void roamingBandsScaleIndependently();

    // Repeated positions (flat runs) must scale together, not oscillate.
    void flatSectionStaysFlat();

    // A script with a single action emits that (modified) position once.
    void singleActionScript();

    // ------------------------------------------------------------------
    // Script metadata parsing (parseScriptInfo / readScriptJson)
    // ------------------------------------------------------------------

    // version, inverted and a full metadata object are parsed into the
    // ScriptInfo and the has* flags are set.
    void fullMetadataParsed();

    // A script without the optional properties keeps the ScriptInfo defaults
    // and the has* flags stay false (the UI shows "Unknown").
    void absentPropertiesKeepDefaults();

    // An explicit inverted:false must be distinguished from an absent property.
    void invertedFalseIsPresent();

    // An explicit paid:false in the metadata must be distinguished from absent.
    void paidFalseIsPresent();

    // Properties with the wrong JSON type are ignored.
    void wrongTypesIgnored();

    // original_total_duration_ms is a string per the funscript spec and is
    // read through toString().
    void durationParsedFromString();

    // The file-based overload parses a regular .funscript file.
    void parsesRegularScriptFile();

    // For SFMA files the properties of the script's track (channels[track])
    // are used, not the ones of the other tracks.
    void sfmaUsesTrackProperties();

    // An SFMA file whose channels object lacks the script's track falls back
    // to the root object, which carries no properties: nothing is set.
    void sfmaMissingTrackFallsBackToRoot();

    // A missing file leaves the ScriptInfo unchanged.
    void missingFileLeavesScriptUnchanged();

    // An invalid JSON file leaves the ScriptInfo unchanged.
    void invalidJsonLeavesScriptUnchanged();

    // readScriptJson parses a valid file into a JSON object.
    void readScriptJsonValidFile();

    // readScriptJson returns an empty object for a missing file.
    void readScriptJsonMissingFile();

    // readScriptJson returns an empty object for an invalid file.
    void readScriptJsonInvalidFile();

private:
    // Writes the given (at, pos) pairs as a funscript JSON file and returns
    // its path.
    QString writeScript(const QList<QPair<qint64, int>> &actions) const;

    // Writes the given JSON object as a .funscript file and returns its path.
    QString writeScriptFile(const QJsonObject &root) const;

    // Builds a funscript root object carrying version, inverted and a full
    // metadata object (used by the metadata parsing tests).
    QJsonObject fullMetadataJson() const;

    // Steps getPosition() from 0 to duration in step-ms increments and
    // collects the positions of every emitted action.
    static QList<int> playScript(class FunscriptHandler &handler, qint64 duration, qint64 step = 10);

    QTemporaryDir m_tempDir;
};

#endif // T_FUNSCRIPTHANDLER_H