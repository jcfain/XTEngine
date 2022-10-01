#ifndef TCODECHANNELLOOKUP_H
#define TCODECHANNELLOOKUP_H
#include <QString>
#include <QMap>
#include "AxisNames.h"
#include "TCodeVersion.h"
#include <QHash>
#include "../struct/ChannelModel.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT TCodeChannelLookup
{
public:
    static const QMap<TCodeVersion, QString> SupportedTCodeVersions;
    static QString PositiveModifier;
    static QString NegativeModifier;
    static QHash<TCodeVersion,  QMap<AxisName,  QString>> TCodeVersionMap;
    static void setSelectedTCodeVersion(TCodeVersion version);
    static void changeSelectedTCodeVersion(TCodeVersion version);
    static TCodeVersion getSelectedTCodeVersion();
    static QString getSelectedTCodeVersionName();
    static QString getTCodeVersionName(TCodeVersion version);
    static QMap<AxisName,  QString> GetSelectedVersionMap();
    static void AddUserAxis(QString channel);
    static bool ChannelExists(QString channel);
    static QStringList getValidMFSExtensions();
    static QMap<QString, ChannelModel33>* getAvailableAxis();
    static ChannelModel33* getChannel(QString name);
    static void setChannel(QString name, ChannelModel33 channel);
    static void addChannel(QString name, ChannelModel33 channel);
    static void deleteChannel(QString axis);
    static bool hasChannel(QString name);
    static void setupAvailableChannels();
    static QString ToString(AxisName channel);
    static QString None();
    static QString Stroke();
    static QString StrokeUp();
    static QString StrokeDown();
    static QString Sway();
    static QString SwayLeft();
    static QString SwayRight();
    static QString Surge();
    static QString SurgeBack();
    static QString SurgeForward();
    static QString Twist();
    static QString TwistClockwise();
    static QString TwistCounterClockwise();
    static QString Roll();
    static QString RollLeft();
    static QString RollRight();
    static QString Pitch();
    static QString PitchForward();
    static QString PitchBack();
    static QString Vib();
    static QString Lube();
    static QString Suck();
    static QString SuckMore();
    static QString SuckLess();
    static QString SuckPosition();
    static QString SuckMorePosition();
    static QString SuckLessPosition();
private:
    static int _channelCount;
    static TCodeVersion _selectedTCodeVersion;
    static QMap<AxisName,  QString> _selectedTCodeVersionMap;
    static QMap<QString, ChannelModel33> _availableAxis;
    static QString NA;
    static QString L0;
    static QString L2;
    static QString L1;
    static QString R0;
    static QString R1;
    static QString R2;
    static QString V0;
    static QString V1;
    static QString L3;
    static QString A0;
    static QString A1;
    static QString A2;
    static void setValidMFSExtensions();
    static QStringList m_validMFSExtensions;
    static ChannelModel33 setupAvailableChannel(QString friendlyName, QString axisName, QString channel, AxisDimension dimension, AxisType type, QString mfsTrackName, QString relatedChannel);
};

#endif // TCODECHANNELLOOKUP_H
