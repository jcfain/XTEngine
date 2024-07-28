#include "tcodechannellookup.h"
#include "../tool/xmath.h"

const QMap<TCodeVersion, QString> TCodeChannelLookup::SupportedTCodeVersions = {
    {TCodeVersion::v2, "TCode v0.2"},
    {TCodeVersion::v3, "TCode v0.3"}
};

TCodeChannelLookup::~TCodeChannelLookup() {
    delete m_defaultChannel;
}

void TCodeChannelLookup::profileChanged() {
    m_debounceTimer.start(100);
}

void TCodeChannelLookup::load(QSettings* settingsToLoadFrom, bool firstLoad) {
    if (firstLoad) {
        setSelectedTCodeVersion(TCodeVersion::v3);
        setAllProfileDefaults();
    } else {
        auto selectedTCodeVersion = (TCodeVersion)(settingsToLoadFrom->value("selectedTCodeVersion").toInt());
        setSelectedTCodeVersion(selectedTCodeVersion);
        QString selectedChannelProfile = settingsToLoadFrom->value("selectedChannelProfile").toString();
        if(selectedChannelProfile.isEmpty())
            setSelectedChannelProfile("Default");
        else
            setSelectedChannelProfile(selectedChannelProfile);
    }
    m_debounceTimer.setSingleShot(true);
    connect(&m_debounceTimer, &QTimer::timeout, [] () {
        if(m_availableChanels.contains(m_selectedChannelProfile)) {
            resetLiveXRange();
            setValidMFSExtensions();
            syncChannelMapToCurrentProfile();
            emit TCodeChannelLookup::instance()->channelProfileChanged(&m_availableChanels[m_selectedChannelProfile]);
        }
    });
}

void TCodeChannelLookup::setSelectedTCodeVersion(TCodeVersion version) {
    m_selectedTCodeVersion = version;
    m_selectedTCodeVersionMap = TCodeVersionMap.value(version);
}

void TCodeChannelLookup::changeSelectedTCodeVersion(TCodeVersion version) {
    if(m_selectedTCodeVersion != version) {
        m_selectedTCodeVersion = version;
        m_selectedTCodeVersionMap = TCodeVersionMap.value(version);
        emit instance()->tcodeVersionChanged();
    }
}

QString TCodeChannelLookup::getSelectedChannelProfile() {
    QMutexLocker locker(&m_mutex);
    return m_selectedChannelProfile;
}

void TCodeChannelLookup::setSelectedChannelProfile(QString value) {
    m_selectedChannelProfile = value;
    profileChanged();
}

TCodeVersion TCodeChannelLookup::getSelectedTCodeVersion() {
    return m_selectedTCodeVersion;
}

int TCodeChannelLookup::getTCodeMaxValue() {
    return getSelectedTCodeVersion() >= TCodeVersion::v3 ? 9999 : 999;
}

QString TCodeChannelLookup::getSelectedTCodeVersionName()
{
    return SupportedTCodeVersions.value(m_selectedTCodeVersion);
}

QString TCodeChannelLookup::getTCodeVersionName(TCodeVersion version) {
    return SupportedTCodeVersions.value(version);
}

QMap<ChannelName,  QString> TCodeChannelLookup::GetSelectedVersionMap()
{
    return m_selectedTCodeVersionMap;
}

QString TCodeChannelLookup::ToString(ChannelName axisName)
{
    return m_selectedTCodeVersionMap.value(axisName);
}
void TCodeChannelLookup::addUserChannelMap(QString channel) {
    if(!channel.isEmpty()) {
        m_selectedTCodeVersionMap.insert((ChannelName)m_channelCount, channel);
        m_channelCount++;
    }
}
void TCodeChannelLookup::deleteUserChannelMap(QString channel) {
    auto key = m_selectedTCodeVersionMap.keys().value(m_selectedTCodeVersionMap.values().indexOf(channel));
    if(key != ChannelName::None) {
        m_selectedTCodeVersionMap.remove(key);
        m_channelCount--;
    }
}
bool TCodeChannelLookup::ChannelExists(QString channel)
{
    return m_selectedTCodeVersionMap.values().contains(channel);
}
QString TCodeChannelLookup::None()
{
    return m_selectedTCodeVersionMap.value(ChannelName::None);
}
QString TCodeChannelLookup::Stroke()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Stroke);
}
QString TCodeChannelLookup::StrokeUp()
{
    return m_selectedTCodeVersionMap.value(ChannelName::StrokeUp);
}
QString TCodeChannelLookup::StrokeDown()
{
    return m_selectedTCodeVersionMap.value(ChannelName::StrokeDown);
}
QString TCodeChannelLookup::Sway()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Sway);
}
QString TCodeChannelLookup::SwayRight()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SwayRight);
}
QString TCodeChannelLookup::SwayLeft()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SwayLeft);
}
QString TCodeChannelLookup::Surge()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Surge);
}
QString TCodeChannelLookup::SurgeForward()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SurgeForward);
}
QString TCodeChannelLookup::SurgeBack()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SurgeBack);
}
QString TCodeChannelLookup::Twist()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Twist);
}
QString TCodeChannelLookup::TwistCounterClockwise()
{
    return m_selectedTCodeVersionMap.value(ChannelName::TwistCounterClockwise);
}
QString TCodeChannelLookup::TwistClockwise()
{
    return m_selectedTCodeVersionMap.value(ChannelName::TwistClockwise);
}
QString TCodeChannelLookup::Roll()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Roll);
}
QString TCodeChannelLookup::RollRight()
{
    return m_selectedTCodeVersionMap.value(ChannelName::RollRight);
}
QString TCodeChannelLookup::RollLeft()
{
    return m_selectedTCodeVersionMap.value(ChannelName::RollLeft);
}
QString TCodeChannelLookup::Pitch()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Pitch);
}
QString TCodeChannelLookup::PitchForward()
{
    return m_selectedTCodeVersionMap.value(ChannelName::PitchForward);
}
QString TCodeChannelLookup::PitchBack()
{
    return m_selectedTCodeVersionMap.value(ChannelName::PitchBack);
}
QString TCodeChannelLookup::Vib()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Vib);
}
QString TCodeChannelLookup::Lube()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Lube);
}
QString TCodeChannelLookup::Suck()
{
    return m_selectedTCodeVersionMap.value(ChannelName::Suck);
}
QString TCodeChannelLookup::SuckMore()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SuckMore);
}
QString TCodeChannelLookup::SuckLess()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SuckLess);
}
QString TCodeChannelLookup::SuckPosition()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SuckPosition);
}
QString TCodeChannelLookup::SuckMorePosition()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SuckMorePosition);
}
QString TCodeChannelLookup::SuckLessPosition()
{
    return m_selectedTCodeVersionMap.value(ChannelName::SuckLessPosition);
}

bool TCodeChannelLookup::isDefaultChannel(QString channelName) {
    return getDefaultChannelProfile().contains(channelName);
}

QList<QString> TCodeChannelLookup::getChannelProfiles() {
    return m_availableChanels.keys();
}
QList<QString> TCodeChannelLookup::getChannels(QString profile) {
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    if(m_availableChanels.contains(profile))
        return m_availableChanels[profile].keys();
    return QStringList();
}
ChannelModel33* TCodeChannelLookup::getChannel(QString name, QString profile) {
    QMutexLocker locker(&m_mutex);
    if(name.isEmpty())
        name = None();
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    if(hasChannel(name, profile))
        return &m_availableChanels[profile][name];
    return 0;
}

bool TCodeChannelLookup::hasChannel(QString name, QString profile) {
    if(name.isEmpty())
        name = None();
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    if(m_availableChanels.contains(profile))
        return m_availableChanels[profile].contains(name);
    return false;
}

bool TCodeChannelLookup::hasProfile(QString profile) {
    return m_availableChanels.contains(profile);
}
void TCodeChannelLookup::setChannel(QString name, ChannelModel33 channel, QString profile)
{
    if(name.isEmpty())
        return;
    QMutexLocker locker(&m_mutex);
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    m_availableChanels[profile][name] = channel;
    profileChanged();
}
void TCodeChannelLookup::addChannel(QString name, ChannelModel33 channel, QString profile)
{
    if(name.isEmpty())
        return;
    QMutexLocker locker(&m_mutex);
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    m_availableChanels[profile].insert(name, channel);

    //        if(availableChannelProfiles.isEmpty())
    //            availableChannelProfiles.insert("Default", { { axis, ChannelModel33::fromVariant(availableChannelJson.value(axis)) } });
    //        else
    //            availableChannelProfiles["Default"].insert({{ axis, ChannelModel33::fromVariant(availableChannelJson.value(axis)) }});
    if(!ChannelExists(name))
        addUserChannelMap(name);
    profileChanged();
}
void TCodeChannelLookup::deleteChannel(QString name, QString profile)
{
    QMutexLocker locker(&m_mutex);
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    if(m_availableChanels.contains(profile)) {
        m_availableChanels[profile].remove(name);
        deleteUserChannelMap(name);
        profileChanged();
    }
}

void TCodeChannelLookup::clearChannels(QString profile) {
    QMutexLocker locker(&m_mutex);
    if(profile.isEmpty())
        profile = m_selectedChannelProfile;
    if(m_availableChanels.contains(profile))
        m_availableChanels[profile].clear();
}

//QMap<QString, ChannelModel33>* TCodeChannelLookup::getAvailableChannels() {
//    QMutexLocker locker(&m_mutex);
//    return &m_availableChanels[m_selectedChannelProfile];
//}

//QMap<QString, QMap<QString, ChannelModel33>>* TCodeChannelLookup::getAvailableChannelProfiles() {
//    QMutexLocker locker(&m_mutex);
//    return &m_availableChanels;
//}
//void TCodeChannelLookup::setAvailableChannelsProfiles(QMap<QString, QMap<QString, ChannelModel33>> channels) {
//    QMutexLocker locker(&m_mutex);
//    m_availableChanels = QMap<QString, QMap<QString, ChannelModel33>>(channels);
//    profileChanged();
//}

void TCodeChannelLookup::setupChannelsProfile(QString name, QMap<QString, ChannelModel33> channels) {
    QMutexLocker locker(&m_mutex);
    MediaActions::AddOtherAction(name, "Channel profile: " + name, ActionType::CHANNEL_PROFILE);
    if(channels.empty())
        m_availableChanels.insert(name, getDefaultChannelProfile());
    else
        m_availableChanels.insert(name, channels);
    profileChanged();
}
void TCodeChannelLookup::addChannelsProfile(QString name, QMap<QString, ChannelModel33> channels) {
    setupChannelsProfile(name, channels);
    setSelectedChannelProfile(name);
}

void TCodeChannelLookup::copyChannelsProfile(QString newName, QString oldName) {
    auto profileToCopy = m_selectedChannelProfile;
    if(!oldName.isEmpty())
        profileToCopy = oldName;
    addChannelsProfile(newName, m_availableChanels.value(profileToCopy));
}

void TCodeChannelLookup::deleteChannelsProfile(QString name) {
    QMutexLocker locker(&m_mutex);
    m_availableChanels.remove(name);
    setSelectedChannelProfile(m_availableChanels.keys().last());
    profileChanged();
}

void TCodeChannelLookup::setAllProfileDefaults(bool keepCurrent) {
    if(!keepCurrent) {
        m_availableChanels.clear();
        m_availableChanels.insert("Default", getDefaultChannelProfile());
        m_selectedChannelProfile = "Default";
        emit instance()->allProfilesDeleted();
    } else {
        foreach(auto profile, m_availableChanels.keys()) {
            if(m_availableChanels.contains(profile))
                m_availableChanels[profile] = getDefaultChannelProfile();
        }
    }
    profileChanged();
}

void TCodeChannelLookup::setProfileDefaults(QString profile) {
    auto selectedProfile = m_selectedChannelProfile;
    if(!profile.isEmpty())
        selectedProfile = profile;
    if(m_availableChanels.contains(selectedProfile)) {
        m_availableChanels[selectedProfile] = getDefaultChannelProfile();
        profileChanged();
    }
}

void TCodeChannelLookup::clearChannelProfiles() {
    m_availableChanels.clear();
}

QStringList TCodeChannelLookup::getValidMFSExtensions() {
    QMutexLocker locker(&m_mutex);
    return m_validMFSExtensions;
}
void TCodeChannelLookup::setValidMFSExtensions() {
    if(m_availableChanels.contains(m_selectedChannelProfile)) {
        m_validMFSExtensions.clear();
        foreach(auto channelName, m_availableChanels[m_selectedChannelProfile].keys())
        {
            auto track = m_availableChanels[m_selectedChannelProfile].value(channelName);
            if(channelName.isEmpty() || channelName == TCodeChannelLookup::Stroke() || track.Type == ChannelType::HalfOscillate || track.TrackName.isEmpty())
                continue;

            m_validMFSExtensions << "." + track.TrackName + ".funscript";
        }
    }
}

QMap<QString, ChannelModel33> TCodeChannelLookup::getDefaultChannelProfile() {
    QMap<QString, ChannelModel33> defaultChannelProfile = {
        {TCodeChannelLookup::None(),  setupAvailableChannel(TCodeChannelLookup::None(), TCodeChannelLookup::None(), TCodeChannelLookup::None(), ChannelDimension::None, ChannelType::None, "", "") },

        {TCodeChannelLookup::Stroke(), setupAvailableChannel("Stroke", TCodeChannelLookup::Stroke(), TCodeChannelLookup::Stroke(), ChannelDimension::Heave, ChannelType::Oscillate, "", TCodeChannelLookup::Twist())  },
        {TCodeChannelLookup::StrokeUp(), setupAvailableChannel("Stroke Up", TCodeChannelLookup::StrokeUp(), TCodeChannelLookup::Stroke(), ChannelDimension::Heave, ChannelType::HalfOscillate, "", TCodeChannelLookup::TwistClockwise()) },
        {TCodeChannelLookup::StrokeDown(), setupAvailableChannel("Stroke Down", TCodeChannelLookup::StrokeDown(), TCodeChannelLookup::Stroke(), ChannelDimension::Heave, ChannelType::HalfOscillate, "", TCodeChannelLookup::TwistCounterClockwise()) },

        {TCodeChannelLookup::Sway(), setupAvailableChannel("Sway", TCodeChannelLookup::Sway(), TCodeChannelLookup::Sway(), ChannelDimension::Sway, ChannelType::Oscillate, "sway", TCodeChannelLookup::Roll()) },
        {TCodeChannelLookup::SwayLeft(), setupAvailableChannel("Sway Left", TCodeChannelLookup::SwayLeft(), TCodeChannelLookup::Sway(), ChannelDimension::Sway, ChannelType::HalfOscillate, "", TCodeChannelLookup::RollLeft()) },
        {TCodeChannelLookup::SwayRight(), setupAvailableChannel("Sway Right", TCodeChannelLookup::SwayRight(), TCodeChannelLookup::Sway(), ChannelDimension::Sway, ChannelType::HalfOscillate, "", TCodeChannelLookup::RollRight()) },

        {TCodeChannelLookup::Surge(), setupAvailableChannel("Surge", TCodeChannelLookup::Surge(), TCodeChannelLookup::Surge(), ChannelDimension::Surge, ChannelType::Oscillate, "surge", TCodeChannelLookup::Pitch()) },
        {TCodeChannelLookup::SurgeBack(), setupAvailableChannel("Surge Back", TCodeChannelLookup::SurgeBack(), TCodeChannelLookup::Surge(), ChannelDimension::Surge, ChannelType::HalfOscillate, "", TCodeChannelLookup::PitchBack()) },
        {TCodeChannelLookup::SurgeForward(), setupAvailableChannel("Surge Forward", TCodeChannelLookup::SurgeForward(), TCodeChannelLookup::Surge(), ChannelDimension::Surge, ChannelType::HalfOscillate, "", TCodeChannelLookup::PitchForward()) },

        {TCodeChannelLookup::Pitch(), setupAvailableChannel("Pitch", TCodeChannelLookup::Pitch(), TCodeChannelLookup::Pitch(), ChannelDimension::Pitch, ChannelType::Oscillate, "pitch", TCodeChannelLookup::Surge()) },
        {TCodeChannelLookup::PitchBack(), setupAvailableChannel("Pitch Back", TCodeChannelLookup::PitchBack(), TCodeChannelLookup::Pitch(), ChannelDimension::Pitch, ChannelType::HalfOscillate, "", TCodeChannelLookup::SurgeBack()) },
        {TCodeChannelLookup::PitchForward(), setupAvailableChannel("Pitch Forward", TCodeChannelLookup::PitchForward(), TCodeChannelLookup::Pitch(), ChannelDimension::Pitch, ChannelType::HalfOscillate, "", TCodeChannelLookup::SurgeForward()) },

        {TCodeChannelLookup::Roll(), setupAvailableChannel("Roll", TCodeChannelLookup::Roll(), TCodeChannelLookup::Roll(), ChannelDimension::Roll, ChannelType::Oscillate, "roll", TCodeChannelLookup::Sway()) },
        {TCodeChannelLookup::RollLeft(), setupAvailableChannel("Roll Left", TCodeChannelLookup::RollLeft(), TCodeChannelLookup::Roll(), ChannelDimension::Roll, ChannelType::HalfOscillate, "", TCodeChannelLookup::SwayLeft()) },
        {TCodeChannelLookup::RollRight(), setupAvailableChannel("Roll Right", TCodeChannelLookup::RollRight(), TCodeChannelLookup::Roll(), ChannelDimension::Roll, ChannelType::HalfOscillate, "", TCodeChannelLookup::SwayRight()) },

        {TCodeChannelLookup::Twist(), setupAvailableChannel("Twist", TCodeChannelLookup::Twist(), TCodeChannelLookup::Twist(), ChannelDimension::Yaw, ChannelType::Oscillate, "twist", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::TwistClockwise(), setupAvailableChannel("Twist (CW)", TCodeChannelLookup::TwistClockwise(), TCodeChannelLookup::Twist(), ChannelDimension::Yaw, ChannelType::HalfOscillate, "", TCodeChannelLookup::StrokeUp()) },
        {TCodeChannelLookup::TwistCounterClockwise(), setupAvailableChannel("Twist (CCW)", TCodeChannelLookup::TwistCounterClockwise(), TCodeChannelLookup::Twist(), ChannelDimension::Yaw, ChannelType::HalfOscillate, "", TCodeChannelLookup::StrokeDown()) },

        {TCodeChannelLookup::Suck(), setupAvailableChannel("Suck", TCodeChannelLookup::Suck(), TCodeChannelLookup::Suck(), ChannelDimension::None, ChannelType::Oscillate, "suck", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::SuckMore(), setupAvailableChannel("Suck more", TCodeChannelLookup::SuckMore(), TCodeChannelLookup::Suck(), ChannelDimension::None, ChannelType::HalfOscillate, "suck", TCodeChannelLookup::StrokeUp()) },
        {TCodeChannelLookup::SuckLess(), setupAvailableChannel("Suck less", TCodeChannelLookup::SuckLess(), TCodeChannelLookup::Suck(), ChannelDimension::None, ChannelType::HalfOscillate, "suck", TCodeChannelLookup::StrokeDown()) },

        {TCodeChannelLookup::Vib(), setupAvailableChannel("Vib", TCodeChannelLookup::Vib(), TCodeChannelLookup::Vib(), ChannelDimension::None, ChannelType::Ramp, "vib", TCodeChannelLookup::Stroke()) },
        {TCodeChannelLookup::Lube(), setupAvailableChannel("Lube", TCodeChannelLookup::Lube(), TCodeChannelLookup::Lube(), ChannelDimension::None, ChannelType::Ramp, "lube", TCodeChannelLookup::Stroke()) }
    };
    if(m_selectedTCodeVersion == TCodeVersion::v3)
    {
       defaultChannelProfile.insert(TCodeChannelLookup::SuckPosition(), setupAvailableChannel("Suck manual", TCodeChannelLookup::SuckPosition(), TCodeChannelLookup::SuckPosition(), ChannelDimension::None, ChannelType::Oscillate, "suckManual", TCodeChannelLookup::Stroke()));
       defaultChannelProfile.insert(TCodeChannelLookup::SuckMorePosition(), setupAvailableChannel("Suck manual more", TCodeChannelLookup::SuckMorePosition(), TCodeChannelLookup::SuckPosition(), ChannelDimension::None, ChannelType::HalfOscillate, "suckManual", TCodeChannelLookup::StrokeUp()));
       defaultChannelProfile.insert(TCodeChannelLookup::SuckLessPosition(), setupAvailableChannel("Suck manual less", TCodeChannelLookup::SuckLessPosition(), TCodeChannelLookup::SuckPosition(), ChannelDimension::None, ChannelType::HalfOscillate, "suckManual", TCodeChannelLookup::StrokeDown()));
    }
    else
    {
        auto v3ChannelMap = TCodeChannelLookup::TCodeVersionMap.value(TCodeVersion::v3);
        auto suckPositionV3Channel = v3ChannelMap.value(ChannelName::SuckPosition);
        defaultChannelProfile.remove(suckPositionV3Channel);

        auto suckMorePositionV3Channel = v3ChannelMap.value(ChannelName::SuckMorePosition);
        defaultChannelProfile.remove(suckMorePositionV3Channel);

        auto suckLessPositionV3Channel = v3ChannelMap.value(ChannelName::SuckLessPosition);
        defaultChannelProfile.remove(suckLessPositionV3Channel);
    }
    return defaultChannelProfile;
}

ChannelModel33 TCodeChannelLookup::setupAvailableChannel(QString friendlyName, QString axisName, QString channel, ChannelDimension dimension, ChannelType type, QString mfsTrackName, QString relatedChannel) {
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
             false, //DamperEnabled
             false,//bool DamperRandom;
             1.0, //DamperValue
             false, //FunscriptInverted
             false, //GamepadInverted
             false, //LinkToRelatedMFS
             relatedChannel
          };
}

void TCodeChannelLookup::setLiveXRangeMin(int value)
{
    QMutexLocker locker(&m_mutex);
    _liveXRangeMin = value;
    _liveXRangeMid = XMath::middle(_liveXRangeMin, _liveXRangeMax);
}
int TCodeChannelLookup::getLiveXRangeMin()
{
    QMutexLocker locker(&m_mutex);
    return _liveXRangeMin;
}

void TCodeChannelLookup::setLiveXRangeMax(int value)
{
    QMutexLocker locker(&m_mutex);
    _liveXRangeMax = value;
    _liveXRangeMid = XMath::middle(_liveXRangeMin, _liveXRangeMax);
}

int TCodeChannelLookup::getLiveXRangeMax()
{
    QMutexLocker locker(&m_mutex);
    return _liveXRangeMax;
}

void TCodeChannelLookup::setLiveXRangeMid(int value)
{
    QMutexLocker locker(&m_mutex);
    _liveXRangeMid = value;
}

int TCodeChannelLookup::getLiveXRangeMid()
{
    QMutexLocker locker(&m_mutex);
    return _liveXRangeMid;
}
void TCodeChannelLookup::resetLiveXRange()
{
    _liveXRangeMax = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->UserMax;
    _liveXRangeMin = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->UserMin;
    _liveXRangeMid = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke())->UserMid;
}

void TCodeChannelLookup::setChannelRange(QString channelName, int min, int max) {
    if(hasChannel(channelName)) {
        ChannelModel33* channel = getChannel(channelName);
        channel->UserMax = max;
        channel->UserMin = min;
        channel->UserMid = XMath::middle(min, max);
        if(channelName == Stroke())
            setChannelRangeLive(channelName, min, max);
    }
}

/**
 * @brief Only for X (stroke) for now. Channel is ignored
 * @param channel
 * @param min
 * @param max
 */
void TCodeChannelLookup::setChannelRangeLive(QString channel, int min, int max) {
    setLiveXRangeMax(max);
    setLiveXRangeMin(min);
    setLiveXRangeMid(XMath::middle(min, max));
}

void TCodeChannelLookup::syncChannelMapToCurrentProfile() {
    m_selectedTCodeVersionMap = TCodeVersionMap.value(m_selectedTCodeVersion);
    foreach (auto channel, m_availableChanels[m_selectedChannelProfile]) {
        if(!ChannelExists(channel.Channel))
            addUserChannelMap(channel.Channel);
    }
    foreach(auto channelMap, m_selectedTCodeVersionMap) {
        if(m_availableChanels.contains(channelMap))
            deleteUserChannelMap(channelMap);
    }
//    for(auto __begin = m_selectedTCodeVersionMap.begin(), __end = m_selectedTCodeVersionMap.end();  __begin != __end; ++__begin) {
//        if(m_availableChanels.contains(__begin.value()))
//            deleteUserChannelMap(__begin.value());
//    }
}

TCodeChannelLookup* TCodeChannelLookup::m_instance = 0;
QMutex TCodeChannelLookup::m_mutex;
ChannelModel33* TCodeChannelLookup::m_defaultChannel = new ChannelModel33();
TCodeVersion TCodeChannelLookup::m_selectedTCodeVersion;
QMap<QString, QMap<QString, ChannelModel33>> TCodeChannelLookup::m_availableChanels;
QStringList TCodeChannelLookup::m_validMFSExtensions;
QString TCodeChannelLookup::m_selectedChannelProfile = "Default";
QTimer TCodeChannelLookup::m_debounceTimer;
int TCodeChannelLookup::_liveXRangeMax;
int TCodeChannelLookup::_liveXRangeMid;
int TCodeChannelLookup::_liveXRangeMin;

int TCodeChannelLookup::m_channelCount = (int)ChannelName::AXIS_NAMES_LENGTH;
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
QMap<ChannelName, QString> TCodeChannelLookup::m_selectedTCodeVersionMap;
QHash<TCodeVersion, QMap<ChannelName, QString>> TCodeChannelLookup::TCodeVersionMap =
{
    {
        TCodeVersion::v2,
        {
            {ChannelName::None, NA},
            {ChannelName::Stroke, L0},
            {ChannelName::StrokeUp, L0 + PositiveModifier},
            {ChannelName::StrokeDown, L0 + NegativeModifier},
            {ChannelName::Roll, R1},
            {ChannelName::RollRight, R1 + PositiveModifier},
            {ChannelName::RollLeft, R1 + NegativeModifier},
            {ChannelName::Pitch, R2},
            {ChannelName::PitchForward, R2 + PositiveModifier},
            {ChannelName::PitchBack, R2 + NegativeModifier},
            {ChannelName::Twist, R0},
            {ChannelName::TwistClockwise, R0 + PositiveModifier},
            {ChannelName::TwistCounterClockwise, R0 + NegativeModifier},
            {ChannelName::Surge, L1},
            {ChannelName::SurgeForward, L1 + PositiveModifier},
            {ChannelName::SurgeBack, L1 + NegativeModifier},
            {ChannelName::Sway, L2},
            {ChannelName::SwayLeft, L2 + PositiveModifier},
            {ChannelName::SwayRight, L2 + NegativeModifier},
            {ChannelName::Vib, V0},
            {ChannelName::Lube, V1},
            {ChannelName::Suck, L3},
            {ChannelName::SuckMore, L3 + NegativeModifier},
            {ChannelName::SuckLess, L3 + PositiveModifier}
        }
    },
    {
        TCodeVersion::v3,
        {
            {ChannelName::None, NA},
            {ChannelName::Stroke, L0},
            {ChannelName::StrokeUp, L0 + PositiveModifier},
            {ChannelName::StrokeDown, L0 + NegativeModifier},
            {ChannelName::Roll, R1},
            {ChannelName::RollRight, R1 + PositiveModifier},
            {ChannelName::RollLeft, R1 + NegativeModifier},
            {ChannelName::Pitch, R2},
            {ChannelName::PitchForward, R2 + PositiveModifier},
            {ChannelName::PitchBack, R2 + NegativeModifier},
            {ChannelName::Twist, R0},
            {ChannelName::TwistClockwise, R0 + PositiveModifier},
            {ChannelName::TwistCounterClockwise, R0 + NegativeModifier},
            {ChannelName::Surge, L1},
            {ChannelName::SurgeForward, L1 + PositiveModifier},
            {ChannelName::SurgeBack, L1 + NegativeModifier},
            {ChannelName::Sway, L2},
            {ChannelName::SwayLeft, L2 + PositiveModifier},
            {ChannelName::SwayRight, L2 + NegativeModifier},
            {ChannelName::Vib, V0},
            {ChannelName::Suck, A1},
            {ChannelName::SuckMore, A1 + NegativeModifier},
            {ChannelName::SuckLess, A1 + PositiveModifier},
            {ChannelName::SuckPosition, A0},
            {ChannelName::SuckMorePosition, A0 + NegativeModifier},
            {ChannelName::SuckLessPosition, A0 + PositiveModifier},
            {ChannelName::Lube, A2}
        }
    }
};

