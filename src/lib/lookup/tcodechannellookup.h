#ifndef TCODECHANNELLOOKUP_H
#define TCODECHANNELLOOKUP_H
#include <QString>
#include <QSettings>
#include <QMap>
#include <QMutex>
#include <QTimer>
#include "AxisNames.h"
#include "TCodeVersion.h"
#include <QHash>
#include "../struct/ChannelModel.h"
#include "MediaActions.h"
#include "XTEngine_global.h"

class XTENGINE_EXPORT TCodeChannelLookup: public QObject
{
    Q_OBJECT
signals:
    void tcodeVersionChanged();
    void channelProfileChanged(QMap<QString, ChannelModel33>* channelProfile);

public:
    static TCodeChannelLookup& instance(){
        return m_instance;
    }
    static const QMap<TCodeVersion, QString> SupportedTCodeVersions;
    static void load(QSettings* settingsToLoadFrom, bool firstLoad = false);
    static QString PositiveModifier;
    static QString NegativeModifier;
    static QHash<TCodeVersion,  QMap<AxisName,  QString>> TCodeVersionMap;
    static void setSelectedTCodeVersion(TCodeVersion version);
    static void changeSelectedTCodeVersion(TCodeVersion version);
    static QString getSelectedChannelProfile();
    static void setSelectedChannelProfile(QString value);
    static TCodeVersion getSelectedTCodeVersion();
    static QString getSelectedTCodeVersionName();
    static QString getTCodeVersionName(TCodeVersion version);
    static QMap<AxisName, QString> GetSelectedVersionMap();
    static void AddUserAxis(QString channel);
    static bool ChannelExists(QString channel);
    static QStringList getValidMFSExtensions();
    static void addChannelsProfile(QString name, QMap<QString, ChannelModel33> channels = QMap<QString, ChannelModel33>());
    static void copyChannelsProfile(QString oldName, QString newName = nullptr);
    static void deleteChannelsProfile(QString name);

    static QList<QString> getChannelProfiles();
    static QList<QString> getChannels(QString profile = nullptr);


    static ChannelModel33* getChannel(QString name, QString profile = nullptr);
    static void setChannel(QString name, ChannelModel33 channel, QString profile = nullptr);
    static void addChannel(QString name, ChannelModel33 channel, QString profile = nullptr);
    static void deleteChannel(QString axis, QString profile = nullptr);
    static void clearChannels(QString profile = nullptr);
    static bool hasChannel(QString name, QString profile = nullptr);
    static bool hasProfile(QString profile);
    static void clearChannelProfiles();
    static void setAllProfileDefaults();
    static void setProfileDefaults(QString profile = nullptr);
    static QMap<QString, ChannelModel33> getDefaultChannelProfile();
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
    static TCodeChannelLookup m_instance;
    static QMutex m_mutex;
    static int m_channelCount;
    static TCodeVersion m_selectedTCodeVersion;
    static QMap<AxisName,  QString> m_selectedTCodeVersionMap;
    static QMap<QString, QMap<QString, ChannelModel33>> m_availableChanels;
    static QString m_selectedChannelProfile;
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


    static void setAvailableChannelsProfiles(QMap<QString, QMap<QString, ChannelModel33>> channels);
    static QMap<QString, QMap<QString, ChannelModel33>>* getAvailableChannelProfiles();
    static QMap<QString, ChannelModel33>* getAvailableChannels();
    static void profileChanged();
    static QTimer m_debounceTimer;
};

#endif // TCODECHANNELLOOKUP_H
