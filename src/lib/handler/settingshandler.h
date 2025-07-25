#ifndef SETTINGSHANDLER_H
#define SETTINGSHANDLER_H
#include "XTEngine_global.h"

#include <QCoreApplication>
#include <QSettings>
#include <QObject>
#include <QMutex>
#include <QHash>
#include <QSize>
#include <QProcess>
#include <QTranslator>
#include <QStandardPaths>
#include <QDirIterator>
#include <QtConcurrent/QtConcurrent>
#include "loghandler.h"
#include "../lookup/enum.h"
#include "../lookup/AxisNames.h"
#include "../lookup/tcodechannellookup.h"
#include "../lookup/GamepadAxisNames.h"
#include "../lookup/MediaActions.h"
#include "../lookup/xvideorenderer.h"
#include "../lookup/SettingMap.h"
// #include "../tool/qsettings_json.hpp"
#include "../tool/xmath.h"
#include "../struct/ChannelModel.h"
#include "../struct/ChannelModel33.h"
#include "../struct/DecoderModel.h"
#include "../struct/LibraryListItem.h"
#include "../struct/LibraryListItem27.h"
#include "../struct/LibraryListItemMetaData.h"
#include "../struct/LibraryListItemMetaData258.h"
#include "lib/lookup/TCodeCommand.h"
#include "../lookup/xtags.h"
#include "../struct/NetworkDeviceInfo.h"

#define ORGANIZATION_NAME "cUrbSide prOd"
#define APPLICATION_NAME "XTEngine"

class XTENGINE_EXPORT SettingsHandler: public QObject
{
    Q_OBJECT
signals:
    void settingChange(QString settingName, QVariant value);
    void settingsChanged(bool dirty);
    void messageSend(QString message, XLogLevel loglevel);
    void messageSendWait(QString message, XLogLevel loglevel, QFunctionPointer callback);
    void restartRequired(bool enabled);
    void tagsChanged();

public slots:
    static void changeSetting(QString settingName, QVariant value);
    static void changeSetting(QString settingName, QVariant value, bool restart);
    void setMoneyShot(LibraryListItem27& selectedLibraryListItem27, qint64 currentPosition, bool userSet = true);
    void addBookmark(LibraryListItem27& LibraryListItem27, QString name, qint64 currentPosition);

public:
    static SettingsHandler* instance(){
        if(!m_instance)
            m_instance = new SettingsHandler();
        return m_instance;
    }
    static QSettings* getSettings();
    static void copy(const QSettings* from, QSettings* into);
    static QVariant getSetting(const QString& settingName);
    static void getSetting(const QString& settingName, QJsonObject& json);
    static QString getSettingPath(const SettingMap &setting);
    static QString getSettingPath(const QString &settingName);
    static const QString XTEVersion;
    static const QString XTEVersionTimeStamp;
    static const float XTEVersionNum;
    static bool getSettingsChanged();
    static bool getHideWelcomeScreen();
    static void setHideWelcomeScreen(bool value);
    static int getTCodePadding();

    static void changeSelectedTCodeVersion(TCodeVersion key);
    //static void migrateTCodeVersion();
    static QString getDeoDnlaFunscript(QString key);
    static QHash<QString, QVariant> getDeoDnlaFunscripts();
    static QStringList getSelectedLibrary();
    static bool hasAnyLibrary(const QString& value, QStringList& messages);
    static bool isLibraryParentChildOrEqual(const QString& value, QStringList& messages);
    static bool isLibraryChildOrEqual(const QString& value, QStringList& messages);
    static QString getLastSelectedLibrary();
    static bool addSelectedLibrary(QString value, QStringList &messages);
    static void removeSelectedLibrary(QString value);
    static QStringList getVRLibrary();
    static bool isVRLibraryParentChildOrEqual(const QString& value, QStringList& messages);
    static bool isVRLibraryChildOrEqual(const QString& value, QStringList& messages);
    static QString getLastSelectedVRLibrary();
    static bool addSelectedVRLibrary(QString value, QStringList &messages);
    static void removeSelectedVRLibrary(QString value);
    static void removeAllVRLibraries();
    static QStringList getLibraryExclusions();
    static bool isLibraryExclusionChildOrEqual(const QString& value, QStringList& messages);
    static bool addToLibraryExclusions(QString values, QStringList& errors);
    static void removeFromLibraryExclusions(QList<int> indexes);
    static QString getSelectedThumbsDir();
    static void setSelectedThumbsDir(QString thumbDir);
    static void setSelectedThumbsDirDefault();
    static void setUseMediaDirForThumbs(bool value);
    static bool getUseMediaDirForThumbs();
    static QString getSelectedFunscriptLibrary();
    static DeviceName getSelectedOutputDevice();
    static void setSelectedOutputDevice(DeviceName deviceName);
    static DeviceName getSelectedInputDevice();
    static void setSelectedInputDevice(DeviceName deviceName);
    static void setSelectedNetworkDevice(NetworkProtocol value);
    static NetworkProtocol getSelectedNetworkDevice();
    static QStringList getCustomTCodeCommands();
    static void addCustomTCodeCommand(QString command);
    static void removeCustomTCodeCommand(QString command);
    static void editCustomTCodeCommand(QString command, QString newCommand);

    static void setSerialPort(QString value);
    static void setServerAddress(QString value);
    static void setServerPort(QString value);
    static QString getSerialPort();
    static QString getServerAddress();
    static QString getServerPort();

    static int getPlayerVolume();
    static int getoffSet();
    static bool getDisableTCodeValidation();
    static void setDisableTCodeValidation(bool value);

    static int getChannelUserMin(QString channel);
    static int getChannelUserMax(QString channel);
    static void setChannelUserMin(QString channel, int value);
    static void setChannelUserMax(QString channel, int value);
    static void setChannelUserMid(QString channel, int value);

    static LibraryView getLibraryView();
    static int getVideoIncrement();

    static bool getGamepadEnabled();
    static int getGamepadSpeed();
    static int getLiveGamepadSpeed();
    static int getGamepadSpeedIncrement();

    static bool getDisableSpeechToText();
    static void setDisableSpeechToText(bool value);
    static bool getDisableNoScriptFound();
    static void setDisableNoScriptFound(bool value);
    static bool getDisableVRScriptSelect();
    static void setDisableVRScriptSelect(bool value);

    static void setLinkedVRFunscript(QString key, QString value);
    static void removeLinkedVRFunscript(QString key);
    static void setSelectedFunscriptLibrary(QString value);

    static QString getDeoAddress();
    static QString getDeoPort();
    static void setDeoAddress(QString value);
    static void setDeoPort(QString value);

    static QString getWhirligigAddress();
    static QString getWhirligigPort();
    static void setWhirligigAddress(QString value);
    static void setWhirligigPort(QString value);

    static void setPlayerVolume(int value);
    static void setoffSet(int value);

    static bool getMultiplierChecked(QString channel);
    static void setMultiplierChecked(QString channel, bool value);

    static bool getChannelFunscriptInverseChecked(QString channel);
    static void setChannelFunscriptInverseChecked(QString channel, bool value);
    static bool getChannelGamepadInverse(QString channel);
    static void setChannelGamepadInverse(QString channel, bool value);

    static float getDamperValue(QString channel);
    static void setDamperValue(QString channel, float value);
    static bool getDamperChecked(QString channel);
    static void setDamperChecked(QString channel, bool value);
    static bool getLinkToRelatedAxisChecked(QString channel);
    static void setLinkToRelatedAxisChecked(QString channel, bool value);
    static void setLinkToRelatedAxis(QString channel, QString linkedChannel);

    static void setLibraryView(int value);
    static void setThumbSize(int value);
    static int getThumbSize();
    static void setVideoIncrement(int value);

    static void setLibrarySortMode(int value);
    static LibrarySortMode getLibrarySortMode();

    static void setShowVRInLibraryView(bool value);
    static bool getShowVRInLibraryView();

    static void setGamepadEnabled(bool value);
    ///Returns assigned actions per gamepad button
    static QMap<QString, QStringList> getGamePadMap();
    ///Returns assigned gamepad buttons per action
    static QMap<QString, QStringList> getGamePadMapInverse();
    static QStringList getGamePadMapButton(QString gamepadButton);
    static void setGamePadMapButton(QString gamePadButton, QString action);
    static void removeGamePadMapButton(QString gamePadButton, QString action);
    static void clearGamePadMapButton(QString gamePadButton);

    static QMap<QString, QStringList> getKeyboardMap();
    static QMap<QString, QStringList> getKeyboardMapInverse();
    static QStringList getKeyboardKeyActionList(int key, int modifiers);
    static void setKeyboardMapKey(QString key, QString action);
    static void removeKeyboardMapKey(QString key, QString action);
    static void clearKeyboardMapKey(QString key);
    static QString getKeyboardKey(int key, int modifiers = 0);

    static QMap<QString, QStringList> getTCodeCommandMap();
    static QMap<QString, QStringList> getTCodeCommandMapInverse();
    static QStringList getTCodeCommandMapCommands(QString command);
    static void setTCodeCommandMapKey(QString key, QString action);
    static void removeTCodeCommandMapKey(QString key, QString action);
    static void clearTCodeCommandMapKey(QString key);

    static QMap<QString, QString> getAllActions();

    // static QMap<QString, TCodeCommand> getTCodeCommands();
    // static void setTCodeCommands(QMap<QString, TCodeCommand> commands);
    // static TCodeCommand* getTCodeCommand(QString command);
    // static void addTCodeCommand(TCodeCommand command);
    // static void removeTCodeCommand(QString key);

    static void setAxis(QString axis, ChannelModel33 channel);
    static void addAxis(ChannelModel33 channel);
    static void deleteAxis(QString axis);
    static void setGamepadSpeed(int value);
    static void setGamepadSpeedStep(int value);
    static void setLiveGamepadSpeed(int value);

    static void setXRangeStep(int value);
    static int getXRangeStep();

    static bool getMultiplierEnabled();
    static void setMultiplierEnabled(bool value);
    static void setLiveMultiplierEnabled(bool value);
    static bool getLiveGamepadConnected();
    static void setLiveGamepadConnected(bool value);
    static int getLiveOffSet();
    static void setLiveOffset(int value);
    static bool isSmartOffSet();
    static int getSmartOffSet();
    static void setSmartOffset(int value);

    static void setDecoderPriority(QList<DecoderModel> value);
    static QList<DecoderModel> getDecoderPriority();
    static void setSelectedVideoRenderer(XVideoRenderer value);
    static XVideoRenderer getSelectedVideoRenderer();

    static QMap<QString, QList<LibraryListItem27>> getPlaylists();
    static void setPlaylists(QMap<QString, QList<LibraryListItem27>> value);
    static void addToPlaylist(QString name, LibraryListItem27 value);
    static void updatePlaylist(QString name, QList<LibraryListItem27> value);
    static void addNewPlaylist(QString name);
    static void deletePlaylist(QString name);

    static void setFunscriptLoaded(QString key, bool loaded);
    static bool getFunscriptLoaded(QString key);

    static QHash<QString, LibraryListItemMetaData258> getLibraryListItemMetaData();
    static void getLibraryListItemMetaData(LibraryListItem27& item);
    static bool hasLibraryListItemMetaData(const LibraryListItem27& item);
    static void removeLibraryListItemMetaData(LibraryListItem27& item);
    static void removeLibraryListItemMetaData(const QString key);
    static void updateLibraryListItemMetaData(const LibraryListItem27& item, bool sync = true);
    static bool getForceMetaDataFullProcess();
    static void setForceMetaDataFullProcess(bool enable);
    static void setForceMetaDataFullProcessComplete();

    static XTags getAvailableTags();

    static QString GetHashedPass();
    static void SetHashedPass(QString value);

    static QSize getMaxThumbnailSize();

    static bool getSkipToMoneyShotPlaysFunscript();
    static void setSkipToMoneyShotPlaysFunscript(bool value);
    static QString getSkipToMoneyShotFunscript();
    static void setSkipToMoneyShotFunscript(QString value);
    static bool getSkipToMoneyShotSkipsVideo();
    static void setSkipToMoneyShotSkipsVideo(bool value);
    static bool getSkipToMoneyShotStandAloneLoop();
    static void setSkipToMoneyShotStandAloneLoop(bool value);


    static void setHideStandAloneFunscriptsInLibrary(bool value);
    static bool getHideStandAloneFunscriptsInLibrary();
    static void setSkipPlayingStandAloneFunscriptsInLibrary(bool value);
    static bool getSkipPlayingStandAloneFunscriptsInLibrary();

    static bool getEnableHttpServer();
    static void setEnableHttpServer(bool enable);
    static QString getHttpServerRoot();
    static void setHttpServerRoot(QString value);
    static QString setHttpServerRootDefault();
    static qint64 getHTTPChunkSize();
    static void setHTTPChunkSize(qint64 value);
    static double getHTTPChunkSizeMB();
    static void setHTTPChunkSizeMB(double value);
    static int getHTTPPort();
    static void setHTTPPort(int value);
    static int getWebSocketPort();
    static void setWebSocketPort(int value);
    static int getHttpThumbQuality();
    static void setHttpThumbQuality(int value);

    static void setFunscriptModifierStep(int value);
    static double getFunscriptModifierStep();
    static void setFunscriptOffsetStep(int value);
    static int getFunscriptOffsetStep();
    static void setPlaybackRateStep(double value);
    static double getPlaybackRateStep();
    static void setDisableAutoThumbGeneration(bool value);
    static bool getDisableAutoThumbGeneration();

    static void setLubePulseAmount(int value);
    static int getLubePulseAmount();
    static void setLubePulseEnabled(bool value);
    static bool getLubePulseEnabled();
    static void setLubePulseFrequency(int value);
    static int getLubePulseFrequency();

    static void SetGamepadMapDefaults();
    static void SetKeyboardKeyDefaults();
    // static void SetTCodeCommandDefaults();
    static void SetTCodeCommandMapDefaults();
    static void SetSmartTagDefaults();
    static void SetUserTagDefaults();
    static void SetSystemTagDefaults();
    static void setDisableHeartBeat(bool value);
    static bool getDisableHeartBeat();
    static void setUseDTRAndRTS(bool value);
    static bool getUseDTRAndRTS();

    static void setSaveOnExit(bool enabled);
    static bool getFirstLoad();
    static void Load(QSettings* settingsToLoadFrom = 0);
    static void Save(QSettings* settingsToSaveTo = 0);
    static void Sync();
    static void SaveLinkedFunscripts(QSettings* settingsToSaveTo = nullptr);
    static void PersistSelectSettings();
    static void Default();
    static void Clear();
    static void Quit(bool restart);
    static void Restart();
    static bool Import(QString file, QSettings::Format format);
    static bool Export(QString file, QSettings::Format format);


    static const QStringList getVideoExtensions()
    {
        return QStringList()
                << "mp4"
                << "avi"
                << "mpg"
                << "wmv"
                << "mkv"
                << "webm"
                << "mp2"
                << "mpeg"
                << "mpv"
                << "ogg"
                << "m4p"
                << "m4v"
                << "mov"
                << "qt"
                << "flv"
                << "swf"
                << "avchd";
    }
    static const QStringList getAudioExtensions()
    {
        return QStringList()
                << "m4a"
                << "mp3"
                << "aac"
                << "flac"
                << "wav"
                << "wma";
    }
    static const QStringList getSubtitleExtensions()
    {
        return QStringList()
               << "vtt"
               << "srt";
    }
    static const QStringList getImageExtensions()
    {
        return QStringList() << "jpg" << "jpeg" << "png" << "jfif" << "webp" << "gif";
    }

    static const QString getThumbFormatExtension() {
        return "jpg";
    }

    static const QString &hashedWebPass();
    static void setHashedWebPass(const QString &newHashedWebPass);

    static XTags getXTags();
    static QStringList getTags();
    static QStringList getUserTags();
    static QStringList getUserSmartTags();
    static void removeUserTag(QString tag);
    static void addUserTag(QString tag);
    static bool hasTag(QString tag);
    static void removeUserSmartTag(QString tag);
    static void addUserSmartTag(QString tag);
    static bool hasSmartTag(QString tag);

    static float getViewedThreshold();
    static void setViewedThreshold(float value);

    static bool scheduleLibraryLoadEnabled();
    static void setScheduleLibraryLoadEnabled(bool value);

    static QTime scheduleLibraryLoadTime();
    static void setScheduleLibraryLoadTime(QTime value);

    static bool scheduleLibraryLoadFullProcess();
    static void setScheduleLibraryLoadFullProcess(bool value);

    static bool processMetadataOnStart();
    static void setProcessMetadataOnStart(bool value);

    static bool scheduleSettingsSync();
    static void setScheduleSettingsSync(bool value);


private:
    SettingsHandler();
    ~SettingsHandler();
    static bool m_isPortable;
    static bool m_isAppImage;
    static QString m_appimageMountDir;
    static QMap<QString, QVariant> m_changedSettings;
    static QString _applicationDirPath;
    static SettingsHandler* m_instance;
    static QFuture<void> m_syncFuture;
    static bool _settingsChanged;
    static void settingsChangedEvent(bool dirty);
    static void SetMapDefaults();
    static void setupGamepadButtonMap();
    static void setupKeyboardKeyMap();
    // static void setupTCodeCommands();
    static void setupTCodeCommandMap();
    static void MigrateTo23();
    static void MigrateTo25();
    static void MigrateTo252();
    static void MigrateLibraryMetaDataTo258();
    static void MigratrTo2615();
    static void MigrateTo263();
    static void MigrateToQVariant(QSettings* settingsToLoadFrom);
    static void MigrateToQVariant2(QSettings* settingsToLoadFrom);
    static void MigrateToQVariantChannelModel(QSettings* settingsToLoadFrom);
    static void MigrateTo281();
    static void DeMigrateLibraryMetaDataTo258();
    static void MigrateTo32a(QSettings* settingsToLoadFrom);
    static void MigrateTo42(QSettings* settingsToLoadFrom);
    static void MigrateTo46(QSettings* settingsToLoadFrom);

    static void SaveChannelMap(QSettings* settingsToSaveTo = 0);
    static void SaveTCodeCommandMap(QSettings* settingsToSaveTo = 0);
    // static void SaveTCodeCommands(QSettings* settingsToSaveTo = 0);

    static void storeMediaMetaDatas(QSettings* settingsToSaveTo = 0);

    static QString _appdataLocation;
    static GamepadAxisName gamepadAxisNames;
    static MediaActions mediaActions;
    static QSize _maxThumbnailSize;
    static bool _hideWelcomeScreen;

    static QHash<QString, QVariant> deoDnlaFunscriptLookup;
    static QStringList selectedLibrary;
    static QStringList _vrLibrary;
    static QString selectedFunscriptLibrary;
    static QString _selectedThumbsDir;
    static bool _useMediaDirForThumbs;
    static QString selectedFile;
    static int _selectedOutputDevice;
    static NetworkProtocol _selectedNetworkDeviceType;
    static QString serialPort;
    static QString serverAddress;
    static QString serverPort;
    static QString deoAddress;
    static QString deoPort;
    static bool deoEnabled;
    static QString whirligigAddress;
    static QString whirligigPort;
    static bool m_firstLoad;
    static bool whirligigEnabled;
    static bool _xtpWebSyncEnabled;
    static int playerVolume;
    static int offSet;
    static QStringList m_customTCodeCommands;

    static bool _gamePadEnabled;
    static QMap<QString, QStringList> _gamepadButtonMap;
    static QMap<QString, QStringList> _inverseGamePadMap;
    static QMap<QString, QStringList> _keyboardKeyMap;
    static QMap<QString, QStringList> _inverseKeyboardMap;
    static QMap<QString, QStringList> m_tcodeCommandMap;
    static QMap<QString, QStringList> m_inverseTcodeCommandMap;

    // static QMap<QString, TCodeCommand> m_tcodeCommands;

    static int _gamepadSpeed;
    static int _gamepadSpeedStep;
    static int _liveGamepadSpeed;
    static bool _liveGamepadConnected;
    static int _liveOffset;
    static bool m_smartOffsetEnabled;
    static int m_smartOffset;

    static int _xRangeStep;
    static bool _liveMultiplierEnabled;
    static bool _multiplierEnabled;

    static int libraryView;
    static int _librarySortMode;
    static int thumbSize;
    static int thumbSizeList;
    static int videoIncrement;

    static QList<DecoderModel> decoderPriority;
    static XVideoRenderer _selectedVideoRenderer;
    static QStringList _libraryExclusions;
    static QMap<QString, QList<LibraryListItem27>> _playlists;
    static QHash<QString, LibraryListItemMetaData258> _libraryListItemMetaDatas;

    static bool _disableVRScriptSelect;
    static bool _disableNoScriptFound;
    static bool disableSpeechToText;
    static bool _saveOnExit;
    static QString _hashedPass;
    static QString _hashedWebPass;

    static bool _skipToMoneyShotPlaysFunscript;
    static QString _skipToMoneyShotFunscript;
    static bool _skipToMoneyShotSkipsVideo;
    static bool _skipToMoneyShotStandAloneLoop;

    static bool _hideStandAloneFunscriptsInLibrary;
    static bool _skipPlayingSTandAloneFunscriptsInLibrary;
    static bool _enableHttpServer;
    static QString _httpServerRoot;
    //static qint64 _httpChunkSize;
    static int _httpPort;
    static int _httpThumbQuality;
    static int _webSocketPort;
    static bool _showVRInLibraryView;


    static double _funscriptModifierStep;
    static int _funscriptOffsetStep;

    static bool _channelPulseEnabled;
    static QString _channelPulseChannel;
    static qint64 _channelPulseFrequency;
    static int _channelPulseAmount;

    static float m_viewedThreshold;

    // static bool m_scheduleLibraryLoadEnabled;
    // static QTime m_scheduleLibraryLoadTime;
    // static bool m_scheduleLibraryLoadFullProcess;

    static XTags m_xTags;

    static QTimer m_settingsChangedNotificationDebounce;
    static QHash<QString, bool> _funscriptLoaded;
    static QSettings* settings;
    static QMutex mutex;
};

#endif // SETTINGSHANDLER_H
