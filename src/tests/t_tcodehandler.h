#ifndef T_TCODEHANDLER_H
#define T_TCODEHANDLER_H

#include <QObject>
#include <memory>

struct FunscriptAction;

// Unit tests for TCodeHandler's TCode generation, in particular the
// calculateGradient() "Smooth Motion" spec: the gradient of a point is the
// average of the gradients of the two segments it belongs to (sentinel
// points and direction reversals produce 0).
//
// The gradient is exercised through the public funscriptToTCode() entry
// point with hand-built FunscriptAction sequences; expected values are
// computed by hand from the spec formula (v4: 4-digit positions, gradient
// in units of 10 per second).
class T_TCodeHandler : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    // L0 + 4-digit position + I<interval> + G<gradient>.
    void basicCommandFormat();

    // Direction reversal between the segments -> G0.
    void gradientReversalIsZero();

    // Interior point: average of the two segment gradients (spec example).
    void gradientIntermediateIsAverageOfSegments();

    // First sequence point (sentinel) -> G0.
    void gradientStartSentinelIsZero();

    // Last sequence point (sentinel) -> G0.
    void gradientEndSentinelIsZero();

    // A zero-time segment contributes 0; the other segment is used alone.
    void gradientZeroTimeSegmentIgnored();

    // Inverted channel: position and gradient must be inverted consistently
    // (the gradient must not be computed from raw positions).
    void gradientInvertedChannelConsistent();

    // v2 (3-digit) positions use the 1x unit divisor instead of 10x.
    void gradientV2PaddingUnits();

private:
    // Builds a stroke action with the given current position/interval and
    // last/current/next destination sequence.
    static std::shared_ptr<FunscriptAction> makeAction(int pos, int interval,
                                                       int lastPos, qint64 lastAt,
                                                       int curPos, qint64 curAt,
                                                       int nextPos, qint64 nextAt);

    // Runs funscriptToTCode() on a map containing only the given action and
    // returns the trimmed result.
    static QString toTCode(std::shared_ptr<FunscriptAction> action);
};

#endif // T_TCODEHANDLER_H