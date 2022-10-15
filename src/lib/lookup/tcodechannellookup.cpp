#include "tcodechannellookup.h"


const QMap<TCodeVersion, QString> TCodeChannelLookup::SupportedTCodeVersions = {
    {TCodeVersion::v2, "TCode v0.2"},
    {TCodeVersion::v3, "TCode v0.3"}
};

void TCodeChannelLookup::load(QSettings* settingsToLoadFrom, bool firstLoad) {
    if (firstLoad) {
        setSelectedChannelProfile("Default");
        setSelectedTCodeVersion(TCodeVersion::v3);
        setAvailableChannelDefaults();
    } else {
        QString selectedChannelProfile = settingsToLoadFrom->value("selectedChannelProfile").toString();
        if(selectedChannelProfile.isEmpty())
            setSelectedChannelProfile("Default");
        else
            setSelectedChannelProfile(selectedChannelProfile);
        auto selectedTCodeVersion = (TCodeVersion)(settingsToLoadFrom->value("selectedTCodeVersion").toInt());
        setSelectedTCodeVersion(selectedTCodeVersion);
    }

}

void TCodeChannelLookup::setSelectedTCodeVersion(TCodeVersion version) {
    m_selectedTCodeVersion = version;
    m_selectedTCodeVersionMap = TCodeVersionMap.value(version);
}

void TCodeChannelLookup::changeSelectedTCodeVersion(TCodeVersion version) {
    if(m_selectedTCodeVersion != version) {
        setSelectedTCodeVersion(version);
        setAvailableChannelDefaults();
    }
}

QString TCodeChannelLookup::getSelectedChannelProfile() {
    QMutexLocker locker(&m_mutex);
    return m_selectedChannelProfile;
}

void TCodeChannelLookup::setSelectedChannelProfile(QString value) {
    QMutexLocker locker(&m_mutex);
    m_selectedChannelProfile = value;
}

TCodeVersion TCodeChannelLookup::getSelectedTCodeVersion() {
    return m_selectedTCodeVersion;
}
QString TCodeChannelLookup::getSelectedTCodeVersionName()
{
    return SupportedTCodeVersions.value(m_selectedTCodeVersion);
}

QString TCodeChannelLookup::getTCodeVersionName(TCodeVersion version) {
    return SupportedTCodeVersions.value(version);
}

QMap<AxisName,  QString> TCodeChannelLookup::GetSelectedVersionMap()
{
    return m_selectedTCodeVersionMap;
}

QString TCodeChannelLookup::ToString(AxisName axisName)
{
    return m_selectedTCodeVersionMap.value(axisName);
}
void TCodeChannelLookup::AddUserAxis(QString channel)
{
    m_selectedTCodeVersionMap.insert((AxisName)m_channelCount, channel);
    m_channelCount++;
}
bool TCodeChannelLookup::ChannelExists(QString channel)
{
    return m_selectedTCodeVersionMap.values().contains(channel);
}
QString TCodeChannelLookup::None()
{
    return m_selectedTCodeVersionMap.value(AxisName::None);
}
QString TCodeChannelLookup::Stroke()
{
    return m_selectedTCodeVersionMap.value(AxisName::Stroke);
}
QString TCodeChannelLookup::StrokeUp()
{
    return m_selectedTCodeVersionMap.value(AxisName::StrokeUp);
}
QString TCodeChannelLookup::StrokeDown()
{
    return m_selectedTCodeVersionMap.value(AxisName::StrokeDown);
}
QString TCodeChannelLookup::Sway()
{
    return m_selectedTCodeVersionMap.value(AxisName::Sway);
}
QString TCodeChannelLookup::SwayRight()
{
    return m_selectedTCodeVersionMap.value(AxisName::SwayRight);
}
QString TCodeChannelLookup::SwayLeft()
{
    return m_selectedTCodeVersionMap.value(AxisName::SwayLeft);
}
QString TCodeChannelLookup::Surge()
{
    return m_selectedTCodeVersionMap.value(AxisName::Surge);
}
QString TCodeChannelLookup::SurgeForward()
{
    return m_selectedTCodeVersionMap.value(AxisName::SurgeForward);
}
QString TCodeChannelLookup::SurgeBack()
{
    return m_selectedTCodeVersionMap.value(AxisName::SurgeBack);
}
QString TCodeChannelLookup::Twist()
{
    return m_selectedTCodeVersionMap.value(AxisName::Twist);
}
QString TCodeChannelLookup::TwistCounterClockwise()
{
    return m_selectedTCodeVersionMap.value(AxisName::TwistCounterClockwise);
}
QString TCodeChannelLookup::TwistClockwise()
{
    return m_selectedTCodeVersionMap.value(AxisName::TwistClockwise);
}
QString TCodeChannelLookup::Roll()
{
    return m_selectedTCodeVersionMap.value(AxisName::Roll);
}
QString TCodeChannelLookup::RollRight()
{
    return m_selectedTCodeVersionMap.value(AxisName::RollRight);
}
QString TCodeChannelLookup::RollLeft()
{
    return m_selectedTCodeVersionMap.value(AxisName::RollLeft);
}
QString TCodeChannelLookup::Pitch()
{
    return m_selectedTCodeVersionMap.value(AxisName::Pitch);
}
QString TCodeChannelLookup::PitchForward()
{
    return m_selectedTCodeVersionMap.value(AxisName::PitchForward);
}
QString TCodeChannelLookup::PitchBack()
{
    return m_selectedTCodeVersionMap.value(AxisName::PitchBack);
}
QString TCodeChannelLookup::Vib()
{
    return m_selectedTCodeVersionMap.value(AxisName::Vib);
}
QString TCodeChannelLookup::Lube()
{
    return m_selectedTCodeVersionMap.value(AxisName::Lube);
}
QString TCodeChannelLookup::Suck()
{
    return m_selectedTCodeVersionMap.value(AxisName::Suck);
}
QString TCodeChannelLookup::SuckMore()
{
    return m_selectedTCodeVersionMap.value(AxisName::SuckMore);
}
QString TCodeChannelLookup::SuckLess()
{
    return m_selectedTCodeVersionMap.value(AxisName::SuckLess);
}
QString TCodeChannelLookup::SuckPosition()
{
    return m_selectedTCodeVersionMap.value(AxisName::SuckPosition);
}
QString TCodeChannelLookup::SuckMorePosition()
{
    return m_selectedTCodeVersionMap.value(AxisName::SuckMorePosition);
}
QString TCodeChannelLookup::SuckLessPosition()
{
    return m_selectedTCodeVersionMap.value(AxisName::SuckLessPosition);
}
QStringList TCodeChannelLookup::getValidMFSExtensions() {
    QMutexLocker locker(&m_mutex);
    return m_validMFSExtensions;
}
void TCodeChannelLookup::setValidMFSExtensions() {
    m_validMFSExtensions.clear();
    foreach(auto axisName, m_availableChanels.keys())
    {
        auto track = m_availableChanels.value(axisName);
        if(axisName == TCodeChannelLookup::Stroke() || track[m_selectedChannelProfile].Type == AxisType::HalfRange || track[m_selectedChannelProfile].TrackName.isEmpty())
            continue;

        m_validMFSExtensions << "." + track[m_selectedChannelProfile].TrackName + ".funscript";
    }
}

int TCodeChannelLookup::m_channelCount = (int)AxisName::AXIS_NAMES_LENGTH;
QString TCodeChannelLookup::PositiveModifier = "+";
QString TCodeChannelLookup::NegativeModifier = "-";
QString TCodeChannelLookup::NA = "None";
QString TCodeChannelLookup::L0 = "L0";
QString TCodeChannelLookup::L2 = "L2";
QString TCodeChannelLookup::L1 = "L1";
QString TCodeChannelLookup::R0 = "R0";
QString TCodeChannelLookup::R1 = "R1";
QString TCodeChannelLookup::R2 = "R2";
QString TCodeChannelLookup::V0 = "V0";
QString TCodeChannelLookup::V1 = "V1";
QString TCodeChannelLookup::L3 = "L3";
QString TCodeChannelLookup::A0 = "A0";
QString TCodeChannelLookup::A1 = "A1";
QString TCodeChannelLookup::A2 = "A2";
QMap<AxisName, QString> TCodeChannelLookup::m_selectedTCodeVersionMap;
QHash<TCodeVersion, QMap<AxisName, QString>> TCodeChannelLookup::TCodeVersionMap =
{
    {
        TCodeVersion::v2,
        {
            {AxisName::None, NA},
            {AxisName::Stroke, L0},
            {AxisName::StrokeUp, L0 + PositiveModifier},
            {AxisName::StrokeDown, L0 + NegativeModifier},
            {AxisName::Roll, R1},
            {AxisName::RollRight, R1 + PositiveModifier},
            {AxisName::RollLeft, R1 + NegativeModifier},
            {AxisName::Pitch, R2},
            {AxisName::PitchForward, R2 + PositiveModifier},
            {AxisName::PitchBack, R2 + NegativeModifier},
            {AxisName::Twist, R0},
            {AxisName::TwistClockwise, R0 + PositiveModifier},
            {AxisName::TwistCounterClockwise, R0 + NegativeModifier},
            {AxisName::Surge, L1},
            {AxisName::SurgeForward, L1 + PositiveModifier},
            {AxisName::SurgeBack, L1 + NegativeModifier},
            {AxisName::Sway, L2},
            {AxisName::SwayLeft, L2 + PositiveModifier},
            {AxisName::SwayRight, L2 + NegativeModifier},
            {AxisName::Vib, V0},
            {AxisName::Lube, V1},
            {AxisName::Suck, L3},
            {AxisName::SuckMore, L3 + NegativeModifier},
            {AxisName::SuckLess, L3 + PositiveModifier}
        }
    },
    {
        TCodeVersion::v3,
        {
            {AxisName::None, NA},
            {AxisName::Stroke, L0},
            {AxisName::StrokeUp, L0 + PositiveModifier},
            {AxisName::StrokeDown, L0 + NegativeModifier},
            {AxisName::Roll, R1},
            {AxisName::RollRight, R1 + PositiveModifier},
            {AxisName::RollLeft, R1 + NegativeModifier},
            {AxisName::Pitch, R2},
            {AxisName::PitchForward, R2 + PositiveModifier},
            {AxisName::PitchBack, R2 + NegativeModifier},
            {AxisName::Twist, R0},
            {AxisName::TwistClockwise, R0 + PositiveModifier},
            {AxisName::TwistCounterClockwise, R0 + NegativeModifier},
            {AxisName::Surge, L1},
            {AxisName::SurgeForward, L1 + PositiveModifier},
            {AxisName::SurgeBack, L1 + NegativeModifier},
            {AxisName::Sway, L2},
            {AxisName::SwayLeft, L2 + PositiveModifier},
            {AxisName::SwayRight, L2 + NegativeModifier},
            {AxisName::Vib, V0},
            {AxisName::Suck, A1},
            {AxisName::SuckMore, A1 + NegativeModifier},
            {AxisName::SuckLess, A1 + PositiveModifier},
            {AxisName::SuckPosition, A0},
            {AxisName::SuckMorePosition, A0 + NegativeModifier},
            {AxisName::SuckLessPosition, A0 + PositiveModifier},
            {AxisName::Lube, A2}
        }
    }
};

ChannelModel33* TCodeChannelLookup::getChannel(QString name) {
    QMutexLocker locker(&m_mutex);
    return &m_availableChanels[m_selectedChannelProfile][name];
}

bool TCodeChannelLookup::hasChannel(QString name) {
    return m_availableChanels.contains(name);
}
void TCodeChannelLookup::setChannel(QString axis, ChannelModel33 channel)
{
    QMutexLocker locker(&m_mutex);
    m_availableChanels[m_selectedChannelProfile][axis] = channel;
}
void TCodeChannelLookup::addChannel(QString name, ChannelModel33 channel)
{
    QMutexLocker locker(&m_mutex);
    m_availableChanels[m_selectedChannelProfile].insert(name, channel);
    if(!ChannelExists(channel.AxisName))
        AddUserAxis(channel.AxisName);
}
void TCodeChannelLookup::deleteChannel(QString axis)
{
    QMutexLocker locker(&m_mutex);
    m_availableChanels.remove(axis);
}

QMap<QString, ChannelModel33>* TCodeChannelLookup::getAvailableChannels()
{
    QMutexLocker locker(&m_mutex);
    return &m_availableChanels[m_selectedChannelProfile];
}

QMap<QString, QMap<QString, ChannelModel33>>* TCodeChannelLookup::getAvailableChannelProfiles() {
    QMutexLocker locker(&m_mutex);
    return &m_availableChanels;
}
void TCodeChannelLookup::setAvailableChannelsProfiles(QMap<QString, QMap<QString, ChannelModel33>> channels) {
    QMutexLocker locker(&m_mutex);
    m_availableChanels = QMap<QString, QMap<QString, ChannelModel33>>(channels);
    setValidMFSExtensions();
}

void TCodeChannelLookup::addChannelsProfile(QString name, QMap<QString, ChannelModel33> channels) {
    QMutexLocker locker(&m_mutex);

    m_availableChanels.insert(name, channels);
    m_selectedChannelProfile = name;
}

void TCodeChannelLookup::deleteChannelsProfile(QString name) {
    QMutexLocker locker(&m_mutex);
    m_availableChanels.remove(name);
    m_selectedChannelProfile = m_availableChanels.keys().last();
}

void TCodeChannelLookup::setAvailableChannelDefaults()
{
    m_availableChanels[m_selectedChannelProfile] = getDefaultChannelProfile();

    setValidMFSExtensions();
}

QMap<QString, ChannelModel33> TCodeChannelLookup::getDefaultChannelProfile() {
    QMap<QString, ChannelModel33> defaultChannelProfile = {
        {TCodeChannelLookup::None(),  setupAvailableChannel(TCodeChannelLookup::None(), TCodeChannelLookup::None(), TCodeChannelLookup::None(), AxisDimension::None, AxisType::None, "", "") },

        {TCodeChannelLookup::Stroke(), setupAvailableChannel("Stroke", TCodeChannelLookup::Stroke(), TCodeChannelLookup::Stroke(), AxisDimension::Heave, AxisType::Range, "", TCodeChannelLookup::Twist())  },
        {TCodeChannelLookup::StrokeUp(), setupAvailableChannel("Stroke Up", TCodeChannelLookup::StrokeUp(), TCodeChannelLookup::Stroke(), AxisDimension::Heave, AxisType::HalfRange, "", TCodeChannelLookup::TwistClockwise()) },
        {TCodeChannelLookup::StrokeDown(), setupAvailableChannel("Stroke Down", TCodeChannelLookup::StrokeDown(), TCodeChannelLookup::Stroke(), AxisDimension::Heave, AxisType::HalfRange, "", TCodeChannelLookup::TwistCounterClockwise()) },

        {TCodeChannelLookup::Sway(), setupAvailableChannel("Sway", TCodeChannelLookup::Sway(), TCodeChannelLookup::Sway(), AxisDimension::Sway, AxisType::Range, "sway", TCodeChannelLookup::Roll()) },
        {TCodeChannelLookup::SwayLeft(), setupAvailableChannel("Sway Left", TCodeChannelLookup::SwayLeft(), TCodeChannelLookup::Sway(), AxisDimension::Sway, AxisType::HalfRange, "", TCodeChannelLookup::RollLeft()) },
        {TCodeChannelLookup::SwayRight(), setupAvailableChannel("Sway Right", TCodeChannelLookup::SwayRight(), TCodeChannelLookup::Sway(), AxisDimension::Sway, AxisType::HalfRange, "", TCodeChannelLookup::RollRight()) },

        {TCodeChannelLookup::Surge(), setupAvailableChannel("Surge", TCodeChannelLookup::Surge(), TCodeChannelLookup::Surge(), AxisDimension::Surge, AxisType::Range, "surge", TCodeChannelLookup::Pitch()) },
        {TCodeChannelLookup::SurgeBack(), setupAvailableChannel("Surge Back", TCodeChannelLookup::SurgeBack(), TCodeChannelLookup::Surge(), AxisDimension::Surge, AxisType::HalfRange, "", TCodeChannelLookup::PitchBack()) },
        {TCodeChannelLookup::SurgeForward(), setupAvailableChannel("Surge Forward", TCodeChannelLookup::SurgeForward(), TCodeChannelLookup::Surge(), AxisDimension::Surge, AxisType::HalfRange, "", TCodeChannelLookup::PitchForward()) },

        {TCodeChannelLookup::Pitch(), setupAvailableChannel("Pitch", TCodeChannelLookup::Pitch(), TCodeChannelLookup::Pitch(), AxisDimension::Pitch, AxisType::Range, "pitch", TCodeChannelLookup::Surge()) },
        {TCodeChannelLookup::PitchBack(), setupAvailableChannel("Pitch Back", TCodeChannelLookup::PitchBack(), TCodeChannelLookup::Pitch(), AxisDimension::Pitch, AxisType::HalfRange, "", TCodeChannelLookup::SurgeBack()) },
        {TCodeChannelLookup::PitchForward(), setupAvailableChannel("Pitch Forward", TCodeChannelLookup::PitchForward(), TCodeChannelLookup::Pitch(), AxisDimension::Pitch, AxisType::HalfRange, "", TCodeChannelLookup::SurgeForward()) },

        {TCodeChannelLookup::Roll(), setupAvailableChannel("Roll", TCodeChannelLookup::Roll(), TCodeChannelLookup::Roll(), AxisDimension::Roll, AxisType::Range, "roll", TCodeChannelLookup::Sway()) },
        {TCodeChannelLookup::RollLeft(), setupAvailableChannel("Roll Left", TCodeChannelLookup::RollLeft(), TCodeChannelLookup::Roll(), AxisDimension::Roll, AxisType::HalfRange, "", TCodeChannelLookup::SwayLeft()) },
        {TCodeChannelLookup::RollRight(), setupAvailableChannel("Roll Right", TCodeChannelLookup::RollRight(), TCodeChannelLookup::Roll(), AxisDimension::Roll, AxisType::HalfRange, "", TCodeChannelLookup::SwayRight()) },

        {TCodeChannelLookup::Twist(), setupAvailableChannel("Twist", TCodeChannelLookup::Twist(), TCodeChannelLookup::Twist(), AxisDimension::Yaw, AxisType::Range, "twist", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::TwistClockwise(), setupAvailableChannel("Twist (CW)", TCodeChannelLookup::TwistClockwise(), TCodeChannelLookup::Twist(), AxisDimension::Yaw, AxisType::HalfRange, "", TCodeChannelLookup::StrokeUp()) },
        {TCodeChannelLookup::TwistCounterClockwise(), setupAvailableChannel("Twist (CCW)", TCodeChannelLookup::TwistCounterClockwise(), TCodeChannelLookup::Twist(), AxisDimension::Yaw, AxisType::HalfRange, "", TCodeChannelLookup::StrokeDown()) },

        {TCodeChannelLookup::Suck(), setupAvailableChannel("Suck", TCodeChannelLookup::Suck(), TCodeChannelLookup::Suck(), AxisDimension::None, AxisType::Range, "suck", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::SuckMore(), setupAvailableChannel("Suck more", TCodeChannelLookup::SuckMore(), TCodeChannelLookup::Suck(), AxisDimension::None, AxisType::HalfRange, "suck", TCodeChannelLookup::StrokeUp()) },
        {TCodeChannelLookup::SuckLess(), setupAvailableChannel("Suck less", TCodeChannelLookup::SuckLess(), TCodeChannelLookup::Suck(), AxisDimension::None, AxisType::HalfRange, "suck", TCodeChannelLookup::StrokeDown()) },

        {TCodeChannelLookup::Vib(), setupAvailableChannel("Vib", TCodeChannelLookup::Vib(), TCodeChannelLookup::Vib(), AxisDimension::None, AxisType::Switch, "vib", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::Lube(), setupAvailableChannel("Lube", TCodeChannelLookup::Lube(), TCodeChannelLookup::Lube(), AxisDimension::None, AxisType::Switch, "lube", TCodeChannelLookup::Stroke()) }
    };
    if(m_selectedTCodeVersion == TCodeVersion::v3)
    {
       defaultChannelProfile.insert(TCodeChannelLookup::SuckPosition(), setupAvailableChannel("Suck manual", TCodeChannelLookup::SuckPosition(), TCodeChannelLookup::SuckPosition(), AxisDimension::None, AxisType::Range, "suckManual", TCodeChannelLookup::Stroke()));
       defaultChannelProfile.insert(TCodeChannelLookup::SuckMorePosition(), setupAvailableChannel("Suck manual more", TCodeChannelLookup::SuckMorePosition(), TCodeChannelLookup::SuckPosition(), AxisDimension::None, AxisType::HalfRange, "suckManual", TCodeChannelLookup::StrokeUp()));
       defaultChannelProfile.insert(TCodeChannelLookup::SuckLessPosition(), setupAvailableChannel("Suck manual less", TCodeChannelLookup::SuckLessPosition(), TCodeChannelLookup::SuckPosition(), AxisDimension::None, AxisType::HalfRange, "suckManual", TCodeChannelLookup::StrokeDown()));
    }
    else
    {
        auto v3ChannelMap = TCodeChannelLookup::TCodeVersionMap.value(TCodeVersion::v3);
        auto suckPositionV3Channel = v3ChannelMap.value(AxisName::SuckPosition);
        defaultChannelProfile.remove(suckPositionV3Channel);

        auto suckMorePositionV3Channel = v3ChannelMap.value(AxisName::SuckMorePosition);
        defaultChannelProfile.remove(suckMorePositionV3Channel);

        auto suckLessPositionV3Channel = v3ChannelMap.value(AxisName::SuckLessPosition);
        defaultChannelProfile.remove(suckLessPositionV3Channel);
    }
    return defaultChannelProfile;
}

ChannelModel33 TCodeChannelLookup::setupAvailableChannel(QString friendlyName, QString axisName, QString channel, AxisDimension dimension, AxisType type, QString mfsTrackName, QString relatedChannel) {
    QMutexLocker locker(&m_mutex);
    TCodeChannelLookup::setSelectedTCodeVersion(m_selectedTCodeVersion);
    int max = m_selectedTCodeVersion == TCodeVersion::v2 ? 999 : 9999;
    int mid = m_selectedTCodeVersion == TCodeVersion::v2 ? 500 : 5000;
    return {
             friendlyName,
             axisName,
             channel,
             0, //Min
             mid,
             max,
             0, //UserMin
             mid,
             max,
             dimension,
             type,
             mfsTrackName,
             false, //MultiplierEnabled
             2.50, //MultiplierValue
             false, //DamperEnabled
             1.0, //DamperValue
             false, //FunscriptInverted
             false, //GamepadInverted
             false, //LinkToRelatedMFS
             relatedChannel
          };
}

QMutex TCodeChannelLookup::m_mutex;
TCodeVersion TCodeChannelLookup::m_selectedTCodeVersion;
QMap<QString, QMap<QString, ChannelModel33>> TCodeChannelLookup::m_availableChanels;
QStringList TCodeChannelLookup::m_validMFSExtensions;
QString TCodeChannelLookup::m_selectedChannelProfile = "Default";
