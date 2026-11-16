#include "t_tcodehandler.h"

#include <QMap>
#include <QTest>

#include "lib/handler/tcodehandler.h"
#include "lib/lookup/tcodechannellookup.h"

void T_TCodeHandler::initTestCase()
{
    // Default profile with v4 (4-digit) channels; the Stroke channel (L0)
    // then has UserMin 0 / UserMax 9999 and Gradient enabled.
    TCodeChannelLookup::instance()->load(nullptr, true);
    TCodeChannelLookup::resetLiveXRange();
}

std::shared_ptr<FunscriptAction> T_TCodeHandler::makeAction(int pos, int interval,
                                                            int lastPos, qint64 lastAt,
                                                            int curPos, qint64 curAt,
                                                            int nextPos, qint64 nextAt)
{
    auto action = std::make_shared<FunscriptAction>();
    action->channel = TCodeChannelLookup::Stroke();
    action->pos = pos;
    action->interval = interval;
    action->currentSequence.lastDestinationPos = lastPos;
    action->currentSequence.lastDestinationAt = lastAt;
    action->currentSequence.currentDestinationPos = curPos;
    action->currentSequence.currentDestinationAt = curAt;
    action->currentSequence.nextDestinationPos = nextPos;
    action->currentSequence.nextDestinationAt = nextAt;
    return action;
}

QString T_TCodeHandler::toTCode(std::shared_ptr<FunscriptAction> action)
{
    TCodeHandler handler;
    QMap<QString, std::shared_ptr<FunscriptAction>> actions;
    actions.insert(TCodeChannelLookup::Stroke(), action);
    return handler.funscriptToTCode(actions, 0).trimmed();
}

// ---------------------------------------------------------------------------
// Command format
// ---------------------------------------------------------------------------

void T_TCodeHandler::basicCommandFormat()
{
    // L0 + 5000 (50% of 0-9999) + I500 + G1000
    // (g1 = 5000/0.5s = 10000, g2 = 4999/0.5s = 9998, avg 9999 -> 999.9 -> 1000).
    auto action = makeAction(50, 500, 0, 0, 50, 500, 100, 1000);
    QCOMPARE(toTCode(action), QString("L05000I500G1000"));
}

// ---------------------------------------------------------------------------
// calculateGradient() via funscriptToTCode()
// ---------------------------------------------------------------------------

void T_TCodeHandler::gradientReversalIsZero()
{
    // 0 -> 50 -> 20: the two segments point in opposite directions -> G0.
    auto action = makeAction(50, 500, 0, 0, 50, 500, 20, 1000);
    QCOMPARE(toTCode(action), QString("L05000I500G0"));
}

void T_TCodeHandler::gradientIntermediateIsAverageOfSegments()
{
    // 0 -> 50 (0.5s) -> 100 (1.0s): g1 = 10000, g2 = 4999,
    // avg = 7499.5 -> 749.95 -> 750. (Not the chord slope, which would be 5000.)
    auto action = makeAction(50, 500, 0, 0, 50, 500, 100, 1500);
    QCOMPARE(toTCode(action), QString("L05000I500G750"));
}

void T_TCodeHandler::gradientStartSentinelIsZero()
{
    // lastDestinationAt == -1 marks the first sequence point -> G0.
    auto action = makeAction(50, 500, -1, -1, 50, 500, 100, 1000);
    QCOMPARE(toTCode(action), QString("L05000I500G0"));
}

void T_TCodeHandler::gradientEndSentinelIsZero()
{
    // nextDestinationAt == -1 marks the last sequence point -> G0.
    auto action = makeAction(50, 500, 0, 0, 50, 500, -1, -1);
    QCOMPARE(toTCode(action), QString("L05000I500G0"));
}

void T_TCodeHandler::gradientZeroTimeSegmentIgnored()
{
    // 0 and 50 at the same time: the first segment has zero duration and
    // contributes 0; g2 = 4999/0.5s = 9998, avg = 4999 -> 499.9 -> 500.
    auto action = makeAction(50, 500, 0, 500, 50, 500, 100, 1000);
    QCOMPARE(toTCode(action), QString("L05000I500G500"));
}

void T_TCodeHandler::gradientInvertedChannelConsistent()
{
    // Raw: 0 -> 20 -> 40. Inverted: 100 -> 80 -> 60, i.e. 9999 -> 7999 -> 5999.
    // Both segments: -2000/0.5s = -4000, avg -4000 -> -400.
    // (Without consistent inversion the gradient would see 0 -> 80 -> 40,
    // a reversal, and produce G0.)
    ChannelModel33* channel = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke());
    QVERIFY(channel);
    const bool originalInverted = channel->FunscriptInverted;
    channel->FunscriptInverted = true;

    auto action = makeAction(20, 500, 0, 0, 20, 500, 40, 1000);
    QCOMPARE(toTCode(action), QString("L07999I500G-400"));

    channel->FunscriptInverted = originalInverted;
}

void T_TCodeHandler::gradientV2PaddingUnits()
{
    // v2 (3-digit, deprecated): L0 range 0-999, divisor 1 (not 10).
    // 50 -> 500; g1 = 500/0.5s = 1000, g2 = 499/0.5s = 998, avg 999 -> 999.
    ChannelModel33* channel = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke());
    QVERIFY(channel);
    const int originalUserMin = channel->UserMin;
    const int originalUserMax = channel->UserMax;
    TCodeChannelLookup::changeSelectedTCodeVersion(TCodeVersion::v2);
    channel->UserMin = 0;
    channel->UserMax = 999;
    TCodeChannelLookup::setLiveXRangeMin(0);
    TCodeChannelLookup::setLiveXRangeMax(999);

    auto action = makeAction(50, 500, 0, 0, 50, 500, 100, 1000);
    QCOMPARE(toTCode(action), QString("L0500I500G999"));

    TCodeChannelLookup::changeSelectedTCodeVersion(TCodeVersion::v4);
    channel->UserMin = originalUserMin;
    channel->UserMax = originalUserMax;
    TCodeChannelLookup::resetLiveXRange();
}