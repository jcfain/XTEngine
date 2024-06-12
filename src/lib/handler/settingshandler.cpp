#include "settingshandler.h"

const QString SettingsHandler::XTEVersion = "0.454b";
const float SettingsHandler::XTEVersionNum = 0.454f;
const QString SettingsHandler::XTEVersionTimeStamp = QString(XTEVersion +" %1T%2").arg(__DATE__).arg(__TIME__);

SettingsHandler::SettingsHandler(){
    m_settingsChangedNotificationDebounce.setSingleShot(true);
}
SettingsHandler::~SettingsHandler()
{
    delete settings;
    if(m_instance)
        delete m_instance;
}

void SettingsHandler::setSaveOnExit(bool enabled)
{
    _saveOnExit = enabled;
}

bool SettingsHandler::getFirstLoad()
{
    return m_firstLoad;
}

void SettingsHandler::setMoneyShot(LibraryListItem27 libraryListItem, qint64 currentPosition, bool userSet)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem);
    if(!userSet && libraryListItemMetaData.moneyShotMillis > 0)
        return;
    libraryListItemMetaData.moneyShotMillis = currentPosition;
    SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
}
void SettingsHandler::addBookmark(LibraryListItem27 libraryListItem, QString name, qint64 currentPosition)
{
    auto libraryListItemMetaData = SettingsHandler::getLibraryListItemMetaData(libraryListItem);
    libraryListItemMetaData.bookmarks.append({name, currentPosition});
    SettingsHandler::updateLibraryListItemMetaData(libraryListItemMetaData);
}

QSettings* SettingsHandler::getSettings() {
    return settings;
}
void SettingsHandler::Load(QSettings* settingsToLoadFrom)
{
    QMutexLocker locker(&mutex);
    _applicationDirPath = QCoreApplication::applicationDirPath();
    _appdataLocation = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if(_appdataLocation.isEmpty())
        _appdataLocation = _applicationDirPath;
    QDir dir(_appdataLocation);
    if (!dir.exists())
        dir.mkpath(_appdataLocation);
    if(!settingsToLoadFrom)
    {
        QFile settingsini(_applicationDirPath + "/settings.ini");
        if(settingsini.exists())
        {
            settings = new QSettings("settings.ini", QSettings::Format::IniFormat);
        }
        else
        {
            settings = new QSettings("cUrbSide prOd", "XTEngine");
        }
        settingsToLoadFrom = settings;
    }

    float currentVersion = settingsToLoadFrom->value("version").toFloat();
    m_firstLoad = currentVersion == 0;

    TCodeChannelLookup::load(settingsToLoadFrom, m_firstLoad);

    if (m_firstLoad)
    {
        locker.unlock();
        SetMapDefaults();
    }

    locker.relock();

    QJsonObject availableChannelJson = settingsToLoadFrom->value("availableChannels").toJsonObject();
    _funscriptLoaded.clear();
    foreach(auto profile, availableChannelJson.keys())
    {
        TCodeChannelLookup::setupChannelsProfile(profile, QMap<QString, ChannelModel33>());
        foreach(auto axis, availableChannelJson.value(profile).toObject().keys())
        {
            TCodeChannelLookup::addChannel(axis, ChannelModel33::fromVariant(availableChannelJson.value(profile).toObject().value(axis)), profile);
            _funscriptLoaded.insert(axis, false);
        }
    }

    selectedLibrary = settingsToLoadFrom->value("selectedLibrary").toStringList();
    _selectedThumbsDir = settingsToLoadFrom->value("selectedThumbsDir").toString();
    _useMediaDirForThumbs = settingsToLoadFrom->value("useMediaDirForThumbs").toBool();
    _hideWelcomeScreen = settingsToLoadFrom->value("hideWelcomeScreen").toBool();
    _selectedOutputDevice = settingsToLoadFrom->value("selectedDevice").toInt();
    _selectedNetworkDeviceType = (NetworkDeviceType)settingsToLoadFrom->value("selectedNetworkDeviceType").toInt();
    playerVolume = settingsToLoadFrom->value("playerVolume").toInt();
    offSet = settingsToLoadFrom->value("offSet").toInt();
    _disableTCodeValidation = settingsToLoadFrom->value("disableSerialTCodeValidation").toBool();
    selectedFunscriptLibrary = settingsToLoadFrom->value("selectedFunscriptLibrary").toString();
    serialPort = settingsToLoadFrom->value("serialPort").toString();
    serverAddress = settingsToLoadFrom->value("serverAddress").toString();
    serverAddress = serverAddress.isEmpty() ? "tcode.local" : serverAddress;
    serverPort = settingsToLoadFrom->value("serverPort").toString();
    serverPort = serverPort.isEmpty() ? "8000" : serverPort;
    deoAddress = settingsToLoadFrom->value("deoAddress").toString();
    deoAddress = deoAddress.isEmpty() ? "127.0.0.1" : deoAddress;
    deoPort = settingsToLoadFrom->value("deoPort").toString();
    deoPort = deoPort.isEmpty() ? "23554" : deoPort;
    deoEnabled = settingsToLoadFrom->value("deoEnabled").toBool();

    whirligigAddress = settingsToLoadFrom->value("whirligigAddress").toString();
    whirligigAddress = whirligigAddress.isEmpty() ? "127.0.0.1" : whirligigAddress;
    whirligigPort = settingsToLoadFrom->value("whirligigPort").toString();
    whirligigPort = whirligigPort.isEmpty() ? "2000" : whirligigPort;
    whirligigEnabled = settingsToLoadFrom->value("whirligigEnabled").toBool();

    _xtpWebSyncEnabled = settingsToLoadFrom->value("xtpWebSyncEnabled").toBool();

    libraryView = settingsToLoadFrom->value("libraryView").toInt();
    _librarySortMode = settingsToLoadFrom->value("selectedLibrarySortMode").toInt();
    thumbSize = settingsToLoadFrom->value("thumbSize").toInt();
    thumbSize = thumbSize == 0 ? 150 : thumbSize;
    thumbSizeList = settingsToLoadFrom->value("thumbSizeList").toInt();
    thumbSizeList = thumbSizeList == 0 ? 50 : thumbSizeList;
    videoIncrement = settingsToLoadFrom->value("videoIncrement").toInt();
    videoIncrement = videoIncrement == 0 ? 10 : videoIncrement;
    deoDnlaFunscriptLookup = settingsToLoadFrom->value("deoDnlaFunscriptLookup").toHash();

    _gamePadEnabled = settingsToLoadFrom->value("gamePadEnabled").toBool();
    _multiplierEnabled = settingsToLoadFrom->value("multiplierEnabled").toBool();
    _liveMultiplierEnabled = _multiplierEnabled;

    QVariantMap gamepadButtonMap = settingsToLoadFrom->value("gamepadButtonMap").toMap();
    _gamepadButtonMap.clear();
    foreach(auto button, gamepadButtonMap.keys())
    {
        _gamepadButtonMap.insert(button, gamepadButtonMap[button].toStringList());
    }
    QVariantMap keyboardKeyMap = settingsToLoadFrom->value("keyboardKeyMap").toMap();
    _keyboardKeyMap.clear();
    foreach(auto key, keyboardKeyMap.keys())
    {
        _keyboardKeyMap.insert(key, keyboardKeyMap[key].toStringList());
    }
    QVariantMap tcodeCommandMap = settingsToLoadFrom->value("tcodeCommandMap").toMap();
    m_tcodeCommandMap.clear();
    foreach(auto key, tcodeCommandMap.keys())
    {
        m_tcodeCommandMap.insert(key, tcodeCommandMap[key].toStringList());
    }

    // QVariantList tcodeCommands = settingsToLoadFrom->value("tcodeCommands").toList();
    // m_tcodeCommands.clear();
    // foreach(auto value, tcodeCommands)
    // {
    //     auto command = TCodeCommand::fromVariant(value);
    //     m_tcodeCommands.insert(command.id, command);
    // }

    _gamepadSpeed = settingsToLoadFrom->value("gamepadSpeed").toInt();
    _gamepadSpeed = _gamepadSpeed == 0 ? 1000 : _gamepadSpeed;
    _gamepadSpeedStep = settingsToLoadFrom->value("gamepadSpeedStep").toInt();
    _gamepadSpeedStep = _gamepadSpeedStep == 0 ? 500 : _gamepadSpeedStep;
    _liveGamepadSpeed = _gamepadSpeed;
    _xRangeStep = settingsToLoadFrom->value("xRangeStep").toInt();
    _xRangeStep = _xRangeStep == 0 ? 50 : _xRangeStep;
    disableSpeechToText = settingsToLoadFrom->value("disableSpeechToText").toBool();
    _disableVRScriptSelect = settingsToLoadFrom->value("disableVRScriptSelect").toBool();
    _disableNoScriptFound = settingsToLoadFrom->value("disableNoScriptFound").toBool();

    _skipToMoneyShotPlaysFunscript = settingsToLoadFrom->value("skipToMoneyShotPlaysFunscript").toBool();
    _skipToMoneyShotFunscript = settingsToLoadFrom->value("skipToMoneyShotFunscript").toString();
    _skipToMoneyShotSkipsVideo = settingsToLoadFrom->value("skipToMoneyShotSkipsVideo").toBool();
    _skipToMoneyShotStandAloneLoop = settingsToLoadFrom->value("skipToMoneyShotStandAloneLoop").toBool();

    _hideStandAloneFunscriptsInLibrary = settingsToLoadFrom->value("hideStandAloneFunscriptsInLibrary").toBool();
    _showVRInLibraryView = settingsToLoadFrom->value("showVRInLibraryView").toBool();
    _skipPlayingSTandAloneFunscriptsInLibrary = settingsToLoadFrom->value("skipPlayingSTandAloneFunscriptsInLibrary").toBool();

    _enableHttpServer = settingsToLoadFrom->value("enableHttpServer").toBool();
    _httpServerRoot = settingsToLoadFrom->value("httpServerRoot").toString();
    if(_httpServerRoot.isEmpty() || !QDir(_httpServerRoot).exists())
    {
#if defined(Q_OS_WIN)
        _httpServerRoot = "www";
#elif defined(Q_OS_MAC)
        _httpServerRoot = _applicationDirPath + "/www";
#elif defined(Q_OS_LINUX)
        _httpServerRoot = _applicationDirPath + "/www";
#endif
    }
    _vrLibrary = settingsToLoadFrom->value("vrLibrary").toStringList();
    _httpChunkSize = settingsToLoadFrom->value("httpChunkSize").toLongLong();
    if(!_httpChunkSize)
        _httpChunkSize = 26214400;
    _httpPort = settingsToLoadFrom->value("httpPort").toInt();
    if(!_httpPort)
        _httpPort = 80;
    _webSocketPort = settingsToLoadFrom->value("webSocketPort").toInt();
    _httpThumbQuality = settingsToLoadFrom->value("httpThumbQuality").toInt();
    if(m_firstLoad)
        _httpThumbQuality = -1;

    _funscriptOffsetStep = settingsToLoadFrom->value("funscriptOffsetStep").toInt();
    if(!_funscriptOffsetStep)
        _funscriptOffsetStep = 100;
    _funscriptModifierStep = settingsToLoadFrom->value("funscriptModifierStep").toInt();
    if(!_funscriptModifierStep)
        _funscriptModifierStep = 5;

    _channelPulseAmount = settingsToLoadFrom->value("channelPulseAmount").toInt();
    _channelPulseEnabled = settingsToLoadFrom->value("channelPulseEnabled").toBool();
    _channelPulseFrequency = settingsToLoadFrom->value("channelPulseFrequency").toInt();

    QList<QVariant> decoderPriorityvarient = settingsToLoadFrom->value("decoderPriority").toList();
    decoderPriority.clear();
    foreach(auto varient, decoderPriorityvarient)
    {
        decoderPriority.append(DecoderModel::fromVariant(varient));
    }

    _selectedVideoRenderer = (XVideoRenderer)settingsToLoadFrom->value("selectedVideoRenderer").toInt();

    _libraryExclusions = settingsToLoadFrom->value("libraryExclusions").toStringList();

    QVariantMap playlists = settingsToLoadFrom->value("playlists").toMap();
    _playlists.clear();
    foreach(auto playlist, playlists.keys())
    {
        QVariant variant = playlists.value(playlist);
        QSequentialIterable playlistArray = variant.value<QSequentialIterable>();

        QList<LibraryListItem27> items;
        int idTracker = 1;
        foreach(QVariant item, playlistArray)
        {
            auto itemTyped = LibraryListItem27::fromVariant(item);
            itemTyped.ID = idTracker;
            items.append(itemTyped);
            idTracker++;
        }
        _playlists.insert(playlist, items);
    }

    _hashedPass = settingsToLoadFrom->value("userData").toString();
    _hashedWebPass = settingsToLoadFrom->value("userWebData").toString();

    m_customTCodeCommands = settingsToLoadFrom->value("customTCodeCommands").toStringList();
    foreach(auto command, m_customTCodeCommands) {
        MediaActions::AddOtherAction(command, "TCode command: " + command, ActionType::TCODE);
    }


    QStringList tags = settingsToLoadFrom->value("tags").toStringList();
    foreach (auto tag, tags) {
        m_xTags.addTag(tag);
    }
    QStringList smartTags = settingsToLoadFrom->value("smartTags").toStringList();
    foreach (auto tag, smartTags) {
        m_xTags.addSmartTag(tag);
    }


    m_viewedThreshold = settingsToLoadFrom->value("viewedThreshold").toFloat();
    if(!m_viewedThreshold) {
        m_viewedThreshold = 0.9f;
    }


    if(!m_firstLoad && currentVersion < 0.258f)
    {
        locker.unlock();
        MigrateLibraryMetaDataTo258();
    }
    else
    {
        _libraryListItemMetaDatas.clear();
        QVariantHash libraryListItemMetaDatas = settingsToLoadFrom->value("libraryListItemMetaDatas").toHash();
        foreach(auto key, libraryListItemMetaDatas.keys())
        {
            _libraryListItemMetaDatas.insert(key, LibraryListItemMetaData258::fromVariant(libraryListItemMetaDatas[key]));
        }
    }

    if(!m_firstLoad)
    {
        if(currentVersion < 0.2f)
        {
            setupGamepadButtonMap();
        }
        if(currentVersion < 0.23f)
        {
            locker.unlock();
            MigrateTo23();
        }
        if(currentVersion < 0.25f)
        {
            locker.unlock();
            MigrateTo25();
        }
        if(currentVersion < 0.252f)
        {
            locker.unlock();
            MigrateTo252();
        }
        if(currentVersion < 0.2581f)
        {
            locker.unlock();
            TCodeChannelLookup::setAllProfileDefaults();
        }
        if(currentVersion < 0.2615f)
        {
            locker.unlock();
            MigratrTo2615();
        }
        if(currentVersion < 0.263f)
        {
            locker.unlock();
            MigrateTo263();
        }
        if(currentVersion < 0.27f)
        {
            locker.unlock();
            settings->setValue("version", 0.27f);
            Save();
            Load();
        }
        if(currentVersion < 0.272f)
        {
            locker.unlock();
            MigrateToQVariant(settingsToLoadFrom);
            Save();
            Load();
        }
        if(currentVersion < 0.284f)
        {
            locker.unlock();
            MigrateToQVariant2(settingsToLoadFrom);
            Save();
            Load();
        }
        if(currentVersion < 0.286f) {
            locker.unlock();
            _httpThumbQuality = -1;
            TCodeChannelLookup::setAllProfileDefaults();
            Save();
            Load();
        }
        if(currentVersion < 0.32f) {
            locker.unlock();
            MigrateTo32a(settingsToLoadFrom);
            Save();
            Load();
        }
        if(currentVersion < 0.324f) {
            locker.unlock();
            MigrateToQVariantChannelModel(settingsToLoadFrom);
            Save();
            Load();
        }
        if(currentVersion < 0.333f) {
            locker.unlock();
            setupKeyboardKeyMap();
            auto channel = TCodeChannelLookup::getChannel(TCodeChannelLookup::Stroke());
            if(TCodeChannelLookup::getChannels().isEmpty() || !channel || channel->AxisName.isEmpty()) {
                TCodeChannelLookup::setAllProfileDefaults();
                SaveChannelMap();
            }
            auto libraryExclusions = settingsToLoadFrom->value("libraryExclusions").value<QList<QString>>();
            _libraryExclusions = QStringList(libraryExclusions);
            Save();
            Load();
        }
        if(currentVersion < 0.41f) {
            locker.unlock();
            auto library = settingsToLoadFrom->value("selectedLibrary").toString();
            if(!selectedLibrary.contains(library))
                selectedLibrary.append(library);
            Save();
            Load();
        }
        if(currentVersion < 0.414f) {
            locker.unlock();
            MigrateTo42(settingsToLoadFrom);
            Save();
            Load();
        }
        if(currentVersion < 0.426f) {
            locker.unlock();
            _hashedPass = nullptr;
            Save();
            Load();
        }
        if(currentVersion < 0.451f) {
            locker.unlock();
            SetTCodeCommandMapDefaults();
            Save();
            Load();
        }
        if(currentVersion < 0.454f) {
            locker.unlock();
            SetSystemTagDefaults();
            Save();
            Load();
        }

    }
    settingsChangedEvent(false);
}

void SettingsHandler::Save(QSettings* settingsToSaveTo)
{
    QMutexLocker locker(&mutex);
    if (_saveOnExit)
    {
        LogHandler::Info("Saving settings");
        if(!settingsToSaveTo)
            settingsToSaveTo = settings;

        float currentVersion = settingsToSaveTo->value("version").toFloat();

        if(XTEVersionNum > currentVersion)
            settingsToSaveTo->setValue("version", XTEVersionNum);

        //TODO: move to TCodeChannelLookup
        settingsToSaveTo->setValue("selectedTCodeVersion", ((int)TCodeChannelLookup::getSelectedTCodeVersion()));
        settingsToSaveTo->setValue("selectedChannelProfile", TCodeChannelLookup::getSelectedChannelProfile());

        settingsToSaveTo->setValue("playerVolume", playerVolume);

        settingsToSaveTo->setValue("hideWelcomeScreen", ((int)_hideWelcomeScreen));
        settingsToSaveTo->setValue("selectedLibrary", selectedLibrary);
        settingsToSaveTo->setValue("selectedThumbsDir", _selectedThumbsDir);
        settingsToSaveTo->setValue("useMediaDirForThumbs", _useMediaDirForThumbs);
        settingsToSaveTo->setValue("selectedDevice", _selectedOutputDevice);
        settingsToSaveTo->setValue("selectedNetworkDeviceType", (int)_selectedNetworkDeviceType);
        settingsToSaveTo->setValue("offSet", offSet);
        settingsToSaveTo->setValue("disableSerialTCodeValidation", _disableTCodeValidation);
        settingsToSaveTo->setValue("selectedFunscriptLibrary", selectedFunscriptLibrary);
        settingsToSaveTo->setValue("serialPort", serialPort);
        settingsToSaveTo->setValue("serverAddress", serverAddress);
        settingsToSaveTo->setValue("serverPort", serverPort);
        settingsToSaveTo->setValue("deoAddress", deoAddress);
        settingsToSaveTo->setValue("deoPort", deoPort);
        settingsToSaveTo->setValue("deoEnabled", deoEnabled);
        settingsToSaveTo->setValue("whirligigAddress", whirligigAddress);
        settingsToSaveTo->setValue("whirligigPort", whirligigPort);
        settingsToSaveTo->setValue("whirligigEnabled", whirligigEnabled);
        settingsToSaveTo->setValue("xtpWebSyncEnabled", _xtpWebSyncEnabled);


        settingsToSaveTo->setValue("libraryView", libraryView);
        settingsToSaveTo->setValue("selectedLibrarySortMode", _librarySortMode);

        settingsToSaveTo->setValue("thumbSize", thumbSize);
        settingsToSaveTo->setValue("thumbSizeList", thumbSizeList);
        settingsToSaveTo->setValue("videoIncrement", videoIncrement);

        settingsToSaveTo->setValue("deoDnlaFunscriptLookup", deoDnlaFunscriptLookup);

        settingsToSaveTo->setValue("gamePadEnabled", _gamePadEnabled);
        settingsToSaveTo->setValue("multiplierEnabled", _multiplierEnabled);

        QList<QVariant> decoderVarient;
        foreach(auto decoder, decoderPriority)
        {
            decoderVarient.append(DecoderModel::toVariant(decoder));
        }
        settingsToSaveTo->setValue("decoderPriority", decoderVarient);

        settingsToSaveTo->setValue("selectedVideoRenderer", (int)_selectedVideoRenderer);

        SaveChannelMap(settingsToSaveTo);

        QVariantMap gamepadMap;
        foreach(auto button, _gamepadButtonMap.keys())
        {
            gamepadMap.insert(button, QVariant::fromValue(_gamepadButtonMap[button]));
        }
        settingsToSaveTo->setValue("gamepadButtonMap", gamepadMap);

        QVariantMap keyboardKeyMap;
        foreach(auto key, _keyboardKeyMap.keys())
        {
            keyboardKeyMap.insert(key, QVariant::fromValue(_keyboardKeyMap[key]));
        }
        settingsToSaveTo->setValue("keyboardKeyMap", keyboardKeyMap);

        // QVariantList tcodeCommands;
        // foreach (auto value, m_tcodeCommands) {
        //     tcodeCommands.append(TCodeCommand::toVariant(value));
        // }
        settings->remove("tcodeCommands");

        SaveTCodeCommandMap(settingsToSaveTo);
        //SaveTCodeCommands(settingsToSaveTo);

        settingsToSaveTo->setValue("gamepadSpeed", _gamepadSpeed);
        settingsToSaveTo->setValue("gamepadSpeedStep", _gamepadSpeedStep);
        settingsToSaveTo->setValue("xRangeStep", _xRangeStep);
        ;

        settingsToSaveTo->setValue("disableSpeechToText", disableSpeechToText);
        settingsToSaveTo->setValue("disableVRScriptSelect", _disableVRScriptSelect);
        settingsToSaveTo->setValue("disableNoScriptFound", _disableNoScriptFound);

        settingsToSaveTo->setValue("libraryExclusions", QVariant::fromValue(_libraryExclusions));

        QVariantMap playlists;
        foreach(auto playlist, _playlists.keys())
        {
            QList<LibraryListItem27> playlistItems = _playlists[playlist];
            QVariantList variantList;
            foreach(auto playlistItem, playlistItems)
            {
                variantList.append(LibraryListItem27::toVariant(playlistItem));
            }
            playlists.insert(playlist, variantList);
        }
        settingsToSaveTo->setValue("playlists", playlists);
        settingsToSaveTo->setValue("userData", _hashedPass);
        settingsToSaveTo->setValue("userWebData", _hashedWebPass);

        storeMediaMetaDatas(settingsToSaveTo);

        settingsToSaveTo->setValue("skipToMoneyShotPlaysFunscript", _skipToMoneyShotPlaysFunscript);
        settingsToSaveTo->setValue("skipToMoneyShotFunscript", _skipToMoneyShotFunscript);
        settingsToSaveTo->setValue("skipToMoneyShotSkipsVideo", _skipToMoneyShotSkipsVideo);
        settingsToSaveTo->setValue("skipToMoneyShotStandAloneLoop", _skipToMoneyShotStandAloneLoop);

        settingsToSaveTo->setValue("hideStandAloneFunscriptsInLibrary", _hideStandAloneFunscriptsInLibrary);
        settingsToSaveTo->setValue("showVRInLibraryView", _showVRInLibraryView);
        settingsToSaveTo->setValue("skipPlayingSTandAloneFunscriptsInLibrary", _skipPlayingSTandAloneFunscriptsInLibrary);

        settingsToSaveTo->setValue("enableHttpServer", _enableHttpServer);
        settingsToSaveTo->setValue("httpServerRoot", _httpServerRoot);
        settingsToSaveTo->setValue("vrLibrary", _vrLibrary);
        settingsToSaveTo->setValue("httpChunkSize", _httpChunkSize);
        settingsToSaveTo->setValue("httpPort", _httpPort);
        settingsToSaveTo->setValue("webSocketPort", _webSocketPort);
        settingsToSaveTo->setValue("httpThumbQuality", _httpThumbQuality);


        settingsToSaveTo->setValue("funscriptModifierStep", _funscriptModifierStep);
        settingsToSaveTo->setValue("funscriptOffsetStep", _funscriptOffsetStep);


        settingsToSaveTo->setValue("channelPulseAmount", _channelPulseAmount);
        settingsToSaveTo->setValue("channelPulseEnabled", _channelPulseEnabled);
        settingsToSaveTo->setValue("channelPulseFrequency", _channelPulseFrequency);

        settingsToSaveTo->setValue("customTCodeCommands", m_customTCodeCommands);

        settingsToSaveTo->setValue("viewedThreshold", m_viewedThreshold);


        QVariantList tagsList;
        foreach(auto tag, m_xTags.getUserTags())
        {
            tagsList.append(tag);
        }
        settingsToSaveTo->setValue("tags", tagsList);

        QVariantList smartTagsList;
        foreach(auto tag, m_xTags.getUserSmartags())
        {
            smartTagsList.append(tag);
        }
        settingsToSaveTo->setValue("smartTags", smartTagsList);

        settingsToSaveTo->sync();

        settingsChangedEvent(false);
        LogHandler::Info("Save complete");
    }

}

void SettingsHandler::SaveLinkedFunscripts(QSettings* settingsToSaveTo)
{
    if(!settingsToSaveTo)
        settingsToSaveTo = settings;
    settingsToSaveTo->setValue("deoDnlaFunscriptLookup", deoDnlaFunscriptLookup);
    settingsToSaveTo->sync();
}

void SettingsHandler::Clear()
{
    QMutexLocker locker(&mutex);
    _saveOnExit = false;
    settings->clear();
    settingsChangedEvent(true);
}

void SettingsHandler::Quit(bool restart)
{
    QStringList arguments = qApp->arguments().mid(1);
    QCoreApplication::quit();
    if(restart)
        QProcess::startDetached(QCoreApplication::applicationFilePath(), arguments);
}

void SettingsHandler::Restart()
{
    QStringList arguments = qApp->arguments().mid(1);
    QCoreApplication::quit();
    QProcess::startDetached(QCoreApplication::applicationFilePath(), arguments);
}

bool SettingsHandler::Import(QString file)
{
    if(file.isEmpty()) {
        LogHandler::Error("Invalid file: empty file");
        return false;
    }
    if(!QFileInfo::exists(file)) {
        LogHandler::Error("Invalid file: does not exist.");
        return false;
    }
    QSettings* settingsImport = new QSettings(file, QSettings::Format::IniFormat);
    Load(settingsImport);
    Save();
    setSaveOnExit(false);
    delete settingsImport;
    return true;
}

void SettingsHandler::settingsChangedEvent(bool dirty)
{
    _settingsChanged = dirty;
    emit instance()->settingsChanged(dirty);
}
bool SettingsHandler::getSettingsChanged()
{
    return _settingsChanged;
}

void SettingsHandler::PersistSelectSettings()
{
    QVariantMap playlists;
    foreach(auto playlist, _playlists.keys())
    {
        playlists.insert(playlist, QVariant::fromValue(_playlists[playlist]));
    }
    settings->setValue("playlists", playlists);

    if(deoDnlaFunscriptLookup.count() > 0)
        settings->setValue("deoDnlaFunscriptLookup", deoDnlaFunscriptLookup);

    settings->sync();
}
void SettingsHandler::Default()
{
    Clear();
    settingsChangedEvent(true);
}

void SettingsHandler::SetMapDefaults()
{
    TCodeChannelLookup::setAllProfileDefaults();
    SaveChannelMap();
    SetGamepadMapDefaults();
    SetKeyboardKeyDefaults();
    SetTCodeCommandMapDefaults();
}
void SettingsHandler::SaveChannelMap(QSettings* settingsToSaveTo)
{
    if(!settingsToSaveTo)
        settingsToSaveTo = settings;
    QVariantMap availableChannelVariant;
    QList<QString> availableChannelProfiles = TCodeChannelLookup::getChannelProfiles();
    foreach(auto channelProfileName, availableChannelProfiles) {
        QVariantMap availableChannelProfileVarient;
        auto channels = TCodeChannelLookup::getChannels(channelProfileName);
        foreach(auto channel, channels) {
            auto variant = ChannelModel33::toVariant(*TCodeChannelLookup::getChannel(channel, channelProfileName));
            availableChannelProfileVarient.insert(channel, variant);
        }
        if(!availableChannelVariant.contains(channelProfileName))
            availableChannelVariant.insert(channelProfileName, availableChannelProfileVarient);
    }
    settingsToSaveTo->setValue("availableChannels", availableChannelVariant);
}

void SettingsHandler::SaveTCodeCommandMap(QSettings *settingsToSaveTo)
{
    QVariantMap tcodeCommandMap;
    for(auto it = m_tcodeCommandMap.begin(); it != m_tcodeCommandMap.end(); it++)
    {
        // if(std::find_if(m_tcodeCommands.begin(), m_tcodeCommands.end(), [it](const TCodeCommand& command) {
        //         return command.command == it.key();
        // }) != m_tcodeCommands.end())
        // {
            tcodeCommandMap.insert(it.key(), QVariant::fromValue(it.value()));
        // }
    }
    settingsToSaveTo->setValue("tcodeCommandMap", tcodeCommandMap);
}

// void SettingsHandler::SaveTCodeCommands(QSettings *settingsToSaveTo)
// {
//     QVariantList tcodeCommands;
//     for( auto it = m_tcodeCommands.begin(); it != m_tcodeCommands.end(); it++)
//     {
//         tcodeCommands.append(TCodeCommand::toVariant(it.value()));
//     }
//     settingsToSaveTo->setValue("tcodeCommands", tcodeCommands);
// }

void SettingsHandler::storeMediaMetaDatas(QSettings* settingsToSaveTo)
{
    if(!settingsToSaveTo)
        settingsToSaveTo = settings;
    QVariantHash libraryListItemMetaDatas;
    foreach(auto libraryListItemMetaData, _libraryListItemMetaDatas.keys())
    {
        libraryListItemMetaDatas.insert(libraryListItemMetaData, LibraryListItemMetaData258::toVariant(_libraryListItemMetaDatas[libraryListItemMetaData]));
    }
    settingsToSaveTo->setValue("libraryListItemMetaDatas", libraryListItemMetaDatas);
}

float SettingsHandler::viewedThreshold()
{
    return m_viewedThreshold;
}

void SettingsHandler::setViewedThreshold(float newViewedThreshold)
{
    m_viewedThreshold = newViewedThreshold;
}

XTags SettingsHandler::getXTags()
{
    return m_xTags;
}

QStringList SettingsHandler::getTags()
{
    return m_xTags.getTags();
}

QStringList SettingsHandler::getUserTags()
{
    return m_xTags.getUserTags();
}
void SettingsHandler::removeUserTag(QString tag)
{
    if(tag.isEmpty())
        return;
    m_xTags.removeTag(tag);
    if(!m_settingsChangedNotificationDebounce.isActive()) {
        m_settingsChangedNotificationDebounce.callOnTimeout(
            [] () {emit instance()->tagsChanged();}
            );
    }
    m_settingsChangedNotificationDebounce.start(500);
}

void SettingsHandler::addUserTag(QString tag)
{
    if(tag.isEmpty())
        return;
    m_xTags.addTag(tag);
    if(!m_settingsChangedNotificationDebounce.isActive()) {
        m_settingsChangedNotificationDebounce.callOnTimeout(
            [] () {emit instance()->tagsChanged();}
            );
    }
    m_settingsChangedNotificationDebounce.start(500);
}

bool SettingsHandler::hasTag(QString tag)
{
    return m_xTags.hasTag(tag);
}

void SettingsHandler::removeUserSmartTag(QString tag)
{
    m_xTags.removeSmartTag(tag);
    if(!m_settingsChangedNotificationDebounce.isActive()) {
        m_settingsChangedNotificationDebounce.callOnTimeout(
            [] () {emit instance()->tagsChanged();}
            );
    }
    m_settingsChangedNotificationDebounce.start(500);
}

void SettingsHandler::addUserSmartTag(QString tag)
{
    m_xTags.addSmartTag(tag);
    if(!m_settingsChangedNotificationDebounce.isActive()) {
        m_settingsChangedNotificationDebounce.callOnTimeout(
            [] () {emit instance()->tagsChanged();}
            );
    }
    m_settingsChangedNotificationDebounce.start(500);
}

bool SettingsHandler::hasSmartTag(QString tag)
{
    return m_xTags.hasSmartTag(tag);
}

QStringList SettingsHandler::getUserSmartTags()
{
    return m_xTags.getUserSmartags();
}

void SettingsHandler::SetGamepadMapDefaults()
{
    setupGamepadButtonMap();
    QVariantMap gamepadMap;
    foreach(auto button, _gamepadButtonMap.keys())
    {
        gamepadMap.insert(button, QVariant::fromValue(_gamepadButtonMap[button]));
    }
    settings->setValue("gamepadButtonMap", gamepadMap);
    settingsChangedEvent(true);
}

void SettingsHandler::SetKeyboardKeyDefaults() {
    setupKeyboardKeyMap();
    QVariantMap keyboardKeyMap;
    foreach(auto key, _keyboardKeyMap.keys())
    {
        keyboardKeyMap.insert(key, QVariant::fromValue(_keyboardKeyMap[key]));
    }
    settings->setValue("keyboardKeyMap", keyboardKeyMap);
    settingsChangedEvent(true);
}

// void SettingsHandler::SetTCodeCommandDefaults()
// {
//     setupTCodeCommands();
//     QVariantList tcodeCommands;
//     foreach (auto value, m_tcodeCommands) {
//         tcodeCommands.append(TCodeCommand::toVariant(value));
//     }
//     settings->setValue("tcodeCommands", tcodeCommands);
//     SetTCodeCommandMapDefaults();
// }

void SettingsHandler::SetTCodeCommandMapDefaults() {
    setupTCodeCommandMap();
    QVariantMap tcodeCommandMap;
    foreach(auto key, m_tcodeCommandMap.keys())
    {
        tcodeCommandMap.insert(key, QVariant::fromValue(m_tcodeCommandMap[key]));
    }
    settings->setValue("tcodeCommandMap", tcodeCommandMap);
    settingsChangedEvent(true);
}

void SettingsHandler::SetSmartTagDefaults()
{
    m_xTags.clearUserSmartTags();
    foreach (auto tag, m_xTags.getBuiltInSmartTags()) {
        m_xTags.addSmartTag(tag);
    }
    emit instance()->tagsChanged();
}

void SettingsHandler::SetUserTagDefaults()
{
    m_xTags.clearUserTags();
    foreach (auto tag, m_xTags.getBuiltInTags()) {
        m_xTags.addTag(tag);
    }
    emit instance()->tagsChanged();
}

void SettingsHandler::SetSystemTagDefaults()
{
    SetSmartTagDefaults();
    SetUserTagDefaults();
}


void SettingsHandler::MigrateTo23()
{
    settings->setValue("version", 0.23f);
    TCodeChannelLookup::setProfileDefaults();
    Save();
    Load();
    emit instance()->messageSend("Due to a standards update your RANGE settings\nhave been set to default for a new data structure.", XLogLevel::Information);
}

void SettingsHandler::MigrateTo25()
{
    settings->setValue("version", 0.25f);
    Save();
    Load();
}

void SettingsHandler::MigrateTo252()
{
    settings->setValue("version", 0.252f);
    TCodeChannelLookup::setProfileDefaults();
    Save();
    Load();
    emit instance()->messageSend("Due to a standards update your CHANNELS\nhave been set to default for a new data structure.\nPlease reset your Multiplier/Range settings before using.", XLogLevel::Information);
}
void SettingsHandler::MigrateLibraryMetaDataTo258()
{
    settings->setValue("version", 0.258f);
    QVariantHash libraryListItemMetaDatas = settings->value("libraryListItemMetaDatas").toHash();
    foreach(auto key, libraryListItemMetaDatas.keys())
    {
        auto libraryListItemMetaData = libraryListItemMetaDatas[key].value<LibraryListItemMetaData258>();
        QFile file(libraryListItemMetaData.libraryItemPath);
        if(file.exists())
        {
            _libraryListItemMetaDatas.insert(key, {
                                                     libraryListItemMetaData.libraryItemPath, // libraryItemPath
                                                    libraryListItemMetaData.watched, // libraryItemPath
                                                     libraryListItemMetaData.lastPlayPosition, // lastPlayPosition
                                                     libraryListItemMetaData.lastLoopEnabled, // lastLoopEnabled
                                                     libraryListItemMetaData.lastLoopStart, // lastLoopStart
                                                     libraryListItemMetaData.lastLoopEnd, // lastLoopEnd
                                                     0, // offset
                                                     libraryListItemMetaData.moneyShotMillis, // moneyShotMillis
                                                     libraryListItemMetaData.bookmarks, // bookmarks
                                                     libraryListItemMetaData.funscripts,
                                                     libraryListItemMetaData.tags
                                              });
            foreach(auto bookmark, libraryListItemMetaDatas[key].value<LibraryListItemMetaData258>().bookmarks)
                _libraryListItemMetaDatas[key].bookmarks.append(bookmark);
            foreach(auto funscript, libraryListItemMetaDatas[key].value<LibraryListItemMetaData258>().funscripts)
                _libraryListItemMetaDatas[key].funscripts.append(funscript);
        }
    }
    Save();
    auto fromDir = _applicationDirPath + "/thumbs/";
    QDir oldThumbPath(fromDir);
    if(oldThumbPath.exists())
    {
        auto toDir = _appdataLocation + "/thumbs/";
        QDirIterator it(fromDir, QDirIterator::Subdirectories);
        QDir dir(fromDir);
        const int absSourcePathLength = dir.absoluteFilePath(fromDir).length();

        while (it.hasNext()){
            it.next();
            const auto fileInfo = it.fileInfo();
            if(!fileInfo.isHidden()) { //filters dot and dotdot
                const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
                const QString constructedAbsolutePath = toDir + subPathStructure;

                if(fileInfo.isDir()){
                    //Create directory in target folder
                    dir.mkpath(constructedAbsolutePath);
                } else if(fileInfo.isFile()) {
                    //Copy File to target directory

                    //Remove file at target location, if it exists, or QFile::copy will fail
                    QFile::remove(constructedAbsolutePath);
                    QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
                }
            }
        }
    }
    Load();
}
void SettingsHandler::MigratrTo2615()
{
    settings->setValue("version", 0.2615f);
    TCodeChannelLookup::setProfileDefaults();
    Save();
    Load();
    emit instance()->messageSend("Due to a standards update your CHANNEL SETTINGS\nhave been set to default for a new data structure.\nPlease reset your RANGES and MULTIPLIERS settings before using.", XLogLevel::Information);
}

void SettingsHandler::MigrateTo263() {

    settings->setValue("version", 0.263f);
    auto currentChannels = TCodeChannelLookup::getChannels();
    foreach(auto axis, currentChannels)
    {
        int max = TCodeChannelLookup::getChannel(axis)->UserMax;
        int min = TCodeChannelLookup::getChannel(axis)->UserMin;
        setChannelUserMid(axis, XMath::middle(min, max));
    }
    Save();
    Load();
}

void SettingsHandler::MigrateToQVariant(QSettings* settingsToLoadFrom)
{
    _playlists.clear();
    QVariantMap playlists = settingsToLoadFrom->value("playlists").toMap();
    foreach(auto playlist, playlists.keys())
    {
        QList<LibraryListItem> list = playlists[playlist].value<QList<LibraryListItem>>();
        QList<LibraryListItem27> list27;
        foreach(auto item, list)
            list27.append(item.toLibraryListItem27());
        _playlists.insert(playlist, list27);
    }

    _libraryListItemMetaDatas.clear();
    QVariantHash libraryListItemMetaDatas = settingsToLoadFrom->value("libraryListItemMetaDatas").toHash();
    foreach(auto key, libraryListItemMetaDatas.keys())
    {
        _libraryListItemMetaDatas.insert(key, libraryListItemMetaDatas[key].value<LibraryListItemMetaData258>());
        foreach(auto bookmark, libraryListItemMetaDatas[key].value<LibraryListItemMetaData>().bookmarks)
            _libraryListItemMetaDatas[key].bookmarks.append(bookmark);
        foreach(auto funscript, libraryListItemMetaDatas[key].value<LibraryListItemMetaData>().funscripts)
            _libraryListItemMetaDatas[key].funscripts.append(funscript);
    }
}

void SettingsHandler::MigrateToQVariant2(QSettings* settingsToLoadFrom)
{
    // CANT GET TO CONVERT PROBABLY CAUSE THE ENUM TYPES!!!!!!
//    QVariantMap availableAxis = settingsToLoadFrom->value("availableAxis").toMap();
//    _availableAxis.clear();
//    _funscriptLoaded.clear();
//    foreach(auto axis, availableAxis.keys())
//    {
//        _availableAxis.insert(axis, availableAxis[axis].value<ChannelModel>());
//        _funscriptLoaded.insert(axis, false);
//        if(!TCodeChannelLookup::ChannelExists(axis))
//            TCodeChannelLookup::AddUserAxis(axis);
//    }
    QList<QVariant> decoderPriorityvarient = settingsToLoadFrom->value("decoderPriority").toList();
    decoderPriority.clear();
    foreach(auto varient, decoderPriorityvarient)
    {
        decoderPriority.append(varient.value<DecoderModel>());
    }
}

void SettingsHandler::MigrateToQVariantChannelModel(QSettings* settingsToLoadFrom)
{
    QVariantMap availableAxis = settingsToLoadFrom->value("availableAxis").toMap();
    auto availableChannels = TCodeChannelLookup::getChannels();
    //TCodeChannelLookup::clearChannels();
    _funscriptLoaded.clear();
    QMap<QString, ChannelModel> availableChannelsTemp;
    foreach(auto axis, availableAxis.keys())
    {
        availableChannelsTemp.insert(axis, availableAxis[axis].value<ChannelModel>());
        _funscriptLoaded.insert(axis, false);
    }
    foreach(auto axis, availableChannelsTemp.keys())
    {
        TCodeChannelLookup::addChannel(axis, availableChannelsTemp[axis].toChannelModel33());
        _funscriptLoaded.insert(axis, false);
    }
}

void SettingsHandler::MigrateTo281()
{

}

void SettingsHandler::MigrateTo32a(QSettings* settingsToLoadFrom)
{
    QVariantMap gamepadButtonMap = settingsToLoadFrom->value("gamepadButtonMap").toMap();
    _gamepadButtonMap.clear();
    foreach(auto button, gamepadButtonMap.keys())
    {
        _gamepadButtonMap.insert(button, QStringList(gamepadButtonMap[button].toString()));
    }
}

void  SettingsHandler::MigrateTo42(QSettings* settingsToLoadFrom) {
    QJsonObject availableChannelJson = settingsToLoadFrom->value("availableChannels").toJsonObject();
    TCodeChannelLookup::clearChannelProfiles();
    foreach(auto axis, availableChannelJson.keys())
    {
        TCodeChannelLookup::addChannel(axis, ChannelModel33::fromVariant(availableChannelJson.value(axis)), "Default");
    }
}

void SettingsHandler::changeSelectedTCodeVersion(TCodeVersion key)
{
    if(TCodeChannelLookup::getSelectedTCodeVersion() != key)
    {
        TCodeChannelLookup::changeSelectedTCodeVersion(key);
        settingsChangedEvent(true);
    }
}

//void SettingsHandler::migrateTCodeVersion()
//{
//    foreach(auto axis, _availableAxis.keys())
//    {
//        if(_selectedTCodeVersion == TCodeVersion::v3)
//        {
//            _availableAxis[axis].Max = 9999;
//            _availableAxis[axis].Mid = _availableAxis[axis].Type == AxisType::Switch ? 0 : 5000;
//            _availableAxis[axis].UserMax = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMax, 0, 999, 0, 9999), 0 ,9999);
//            _availableAxis[axis].UserMin = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMin, 0, 999, 0, 9999), 0 ,9999);
//            _availableAxis[axis].UserMid = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMid, 0, 999, 0, 9999), 0 ,9999);
//        }
//        else
//        {
//            _availableAxis[axis].Max = 999;
//            _availableAxis[axis].Mid = _availableAxis[axis].Type == AxisType::Switch ? 0 : 500;
//            _availableAxis[axis].UserMax = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMax, 0, 9999, 0, 999), 0 ,999);
//            _availableAxis[axis].UserMin = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMin, 0, 9999, 0, 999), 0 ,999);
//            _availableAxis[axis].UserMid = XMath::constrain(XMath::mapRange(_availableAxis[axis].UserMid, 0, 9999, 0, 999), 0 ,999);
//        }
//    }
//    _liveXRangeMax = _availableAxis.value(TCodeChannelLookup::Stroke()).UserMax;
//    _liveXRangeMin = _availableAxis.value(TCodeChannelLookup::Stroke()).UserMin;
//    _liveXRangeMid = _availableAxis.value(TCodeChannelLookup::Stroke()).UserMid;

////    ChannelModel suckMoreModel = { "Suck more", TCodeChannelLookup::SuckMore(), TCodeChannelLookup::Suck(), 0, 500, 999, 0, 500, 999, AxisDimension::None, AxisType::HalfRange, "suck", false, 2.50f, false, 1.0f, false, false, TCodeChannelLookup::StrokeUp() };
////    ChannelModel suckLessModel = { "Suck less", TCodeChannelLookup::SuckLess(), TCodeChannelLookup::Suck(), 0, 500, 999, 0, 500, 999, AxisDimension::None, AxisType::HalfRange, "suck", false, 2.50f, false, 1.0f, false, false, TCodeChannelLookup::StrokeDown() };
//    if(_selectedTCodeVersion == TCodeVersion::v3)
//    {
//        auto v2ChannelMap = TCodeChannelLookup::TCodeVersionMap.value(TCodeVersion::v2);

//        auto lubeV2Channel = v2ChannelMap.value(AxisNames::Lube);
//        if(_availableAxis.contains(lubeV2Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::Lube(), _availableAxis.value(lubeV2Channel));
//            _availableAxis[TCodeChannelLookup::Lube()].AxisName = TCodeChannelLookup::Lube();
//            _availableAxis[TCodeChannelLookup::Lube()].Channel = TCodeChannelLookup::Lube();
//            _availableAxis.remove(lubeV2Channel);
//        }

//        auto suckV2Channel = v2ChannelMap.value(AxisNames::Suck);
//        if(_availableAxis.contains(suckV2Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::Suck(), _availableAxis.value(suckV2Channel));
//            _availableAxis[TCodeChannelLookup::Suck()].AxisName = TCodeChannelLookup::Suck();
//            _availableAxis[TCodeChannelLookup::Suck()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckV2Channel);
//        }

//        auto suckMoreV2Channel = v2ChannelMap.value(AxisNames::SuckMore);
//        if(_availableAxis.contains(suckMoreV2Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::SuckMore(), _availableAxis.value(suckMoreV2Channel));
//            _availableAxis[TCodeChannelLookup::SuckMore()].AxisName = TCodeChannelLookup::SuckMore();
//            _availableAxis[TCodeChannelLookup::SuckMore()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckMoreV2Channel);
//        }

//        auto suckLessV2Channel = v2ChannelMap.value(AxisNames::SuckLess);
//        if(_availableAxis.contains(suckLessV2Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::SuckLess(), _availableAxis.value(suckLessV2Channel));
//            _availableAxis[TCodeChannelLookup::SuckLess()].AxisName = TCodeChannelLookup::SuckLess();
//            _availableAxis[TCodeChannelLookup::SuckLess()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckLessV2Channel);
//        }

//        ChannelModel suctionPositionModel = { "Suck manual", TCodeChannelLookup::SuckPosition(), TCodeChannelLookup::SuckPosition(), 0, 5000, 9999, 0, 5000, 9999, AxisDimension::None, AxisType::Range, "suckManual", false, 2.50f, false, 1.0f, false, false, TCodeChannelLookup::Stroke() };
//        ChannelModel suctionMorePositionModel = { "Suck manual more ", TCodeChannelLookup::SuckMorePosition(), TCodeChannelLookup::SuckPosition(), 0, 5000, 9999, 0, 5000, 9999, AxisDimension::None, AxisType::HalfRange, "suckManual", false, 2.50f, false, 1.0f, false, false, TCodeChannelLookup::StrokeUp() };
//        ChannelModel suctionLessPositionModel = { "Suck manual less ", TCodeChannelLookup::SuckLessPosition(), TCodeChannelLookup::SuckPosition(), 0, 5000, 9999, 0, 5000, 9999, AxisDimension::None, AxisType::HalfRange, "suckManual", false, 2.50f, false, 1.0f, false, false, TCodeChannelLookup::StrokeDown() };
//       _availableAxis.insert(TCodeChannelLookup::SuckPosition(), suctionPositionModel);
//       _availableAxis.insert(TCodeChannelLookup::SuckMorePosition(), suctionMorePositionModel);
//       _availableAxis.insert(TCodeChannelLookup::SuckLessPosition(), suctionLessPositionModel);
//    }
//    else
//    {
//        auto v3ChannelMap = TCodeChannelLookup::TCodeVersionMap.value(TCodeVersion::v3);

//        auto lubeV3Channel = v3ChannelMap.value(AxisNames::Lube);
//        if(_availableAxis.contains(lubeV3Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::Lube(), _availableAxis.value(lubeV3Channel));
//            _availableAxis[TCodeChannelLookup::Lube()].AxisName = TCodeChannelLookup::Lube();
//            _availableAxis[TCodeChannelLookup::Lube()].Channel = TCodeChannelLookup::Lube();
//            _availableAxis.remove(lubeV3Channel);
//        }

//        auto suckV3Channel = v3ChannelMap.value(AxisNames::Suck);
//        if(_availableAxis.contains(suckV3Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::Suck(), _availableAxis.value(suckV3Channel));
//            _availableAxis[TCodeChannelLookup::Suck()].AxisName = TCodeChannelLookup::Suck();
//            _availableAxis[TCodeChannelLookup::Suck()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckV3Channel);
//        }

//        auto suckMoreV3Channel = v3ChannelMap.value(AxisNames::SuckMore);
//        if(_availableAxis.contains(suckMoreV3Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::SuckMore(), _availableAxis.value(suckMoreV3Channel));
//            _availableAxis[TCodeChannelLookup::SuckMore()].AxisName = TCodeChannelLookup::SuckMore();
//            _availableAxis[TCodeChannelLookup::SuckMore()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckMoreV3Channel);
//        }

//        auto suckLessV3Channel = v3ChannelMap.value(AxisNames::SuckLess);
//        if(_availableAxis.contains(suckLessV3Channel))
//        {
//            _availableAxis.insert(TCodeChannelLookup::SuckLess(), _availableAxis.value(suckLessV3Channel));
//            _availableAxis[TCodeChannelLookup::SuckLess()].AxisName = TCodeChannelLookup::SuckLess();
//            _availableAxis[TCodeChannelLookup::SuckLess()].Channel = TCodeChannelLookup::Suck();
//            _availableAxis.remove(suckLessV3Channel);
//        }

//        auto suckPositionV3Channel = v3ChannelMap.value(AxisNames::SuckPosition);
//        _availableAxis.remove(suckPositionV3Channel);

//        auto suckMorePositionV3Channel = v3ChannelMap.value(AxisNames::SuckMorePosition);
//        _availableAxis.remove(suckMorePositionV3Channel);

//        auto suckLessPositionV3Channel = v3ChannelMap.value(AxisNames::SuckLessPosition);
//        _availableAxis.remove(suckLessPositionV3Channel);

//    }
//}

bool SettingsHandler::getHideWelcomeScreen()
{
    return _hideWelcomeScreen;
}
void SettingsHandler::setHideWelcomeScreen(bool value)
{
    _hideWelcomeScreen = value;
    settingsChangedEvent(true);
}

int SettingsHandler::getTCodePadding()
{
    return TCodeChannelLookup::getSelectedTCodeVersion() == TCodeVersion::v3 ? 4 : 3;
}

QStringList SettingsHandler::getSelectedLibrary()
{
    QMutexLocker locker(&mutex);
    return selectedLibrary;
}

QString  SettingsHandler::getLastSelectedLibrary() {
    QString path;
    if(!selectedLibrary.isEmpty())
        path = selectedLibrary.last();
    if(path.isEmpty()) {
        selectedLibrary.removeAll("");
        path = QCoreApplication::applicationDirPath();
    }
    return path;
}
void SettingsHandler::addSelectedLibrary(QString value)
{
    QMutexLocker locker(&mutex);
    if(!value.isEmpty() && !selectedLibrary.contains(value))
        selectedLibrary << value;
    selectedLibrary.removeDuplicates();
    settingsChangedEvent(true);
}
void SettingsHandler::removeSelectedLibrary(QString value)
{
    QMutexLocker locker(&mutex);
    selectedLibrary.removeOne(value);
    settingsChangedEvent(true);
}

QStringList SettingsHandler::getVRLibrary()
{
    return _vrLibrary;
}
QString SettingsHandler::getLastSelectedVRLibrary() {
    QString path;
    if(!_vrLibrary.isEmpty())
        path = _vrLibrary.last();
    if(path.isEmpty()) {
        _vrLibrary.removeAll("");
    }
    return path;
}
void SettingsHandler::addSelectedVRLibrary(QString value)
{
    QMutexLocker locker(&mutex);
    _vrLibrary.clear();
    if(!value.isEmpty())
        _vrLibrary << value;
    settingsChangedEvent(true);
}
void SettingsHandler::removeSelectedVRLibrary(QString value)
{
    QMutexLocker locker(&mutex);
    _vrLibrary.removeOne(value);
    settingsChangedEvent(true);
}

QString SettingsHandler::getSelectedFunscriptLibrary()
{
     QMutexLocker locker(&mutex);
    return selectedFunscriptLibrary;
}
QString SettingsHandler::getSelectedThumbsDir()
{
    auto customThumbDirExists  = !_selectedThumbsDir.isEmpty() && QFileInfo::exists(_selectedThumbsDir);
    return (customThumbDirExists ? _selectedThumbsDir + "/" : _appdataLocation + "/thumbs/");
}
void SettingsHandler::setSelectedThumbsDir(QString thumbDir)
{
    QMutexLocker locker(&mutex);
   _selectedThumbsDir = thumbDir;
}
void SettingsHandler::setSelectedThumbsDirDefault()
{
    QMutexLocker locker(&mutex);
    _selectedThumbsDir = nullptr;
}
void SettingsHandler::setUseMediaDirForThumbs(bool value)
{
    QMutexLocker locker(&mutex);
    _useMediaDirForThumbs = value;
}
bool SettingsHandler::getUseMediaDirForThumbs()
{
    return _useMediaDirForThumbs;
}

DeviceName SettingsHandler::getSelectedOutputDevice()
{
    QMutexLocker locker(&mutex);
    return (DeviceName)_selectedOutputDevice;
}

void SettingsHandler::setSelectedOutputDevice(DeviceName deviceName)
{
    QMutexLocker locker(&mutex);
    _selectedOutputDevice = deviceName;
}

DeviceName SettingsHandler::getSelectedInputDevice()
{
    QMutexLocker locker(&mutex);
    if(deoEnabled)
        return DeviceName::HereSphere;
    else if(whirligigEnabled)
        return DeviceName::Whirligig;
    else if(_xtpWebSyncEnabled)
        return DeviceName::XTPWeb;
    return DeviceName::None;
}

void SettingsHandler::setSelectedInputDevice(DeviceName deviceName)
{
    deoEnabled = deviceName == DeviceName::HereSphere;
    whirligigEnabled = deviceName == DeviceName::Whirligig;
    _xtpWebSyncEnabled = deviceName == DeviceName::XTPWeb;
}

void SettingsHandler::setSelectedNetworkDevice(NetworkDeviceType value) {
    _selectedNetworkDeviceType = value;
}
NetworkDeviceType SettingsHandler::getSelectedNetworkDevice() {
    return _selectedNetworkDeviceType;
}

QStringList SettingsHandler::getCustomTCodeCommands()
{
    return m_customTCodeCommands;
}

void SettingsHandler::addCustomTCodeCommand(QString command)
{
    if(!m_customTCodeCommands.contains(command)) {
        m_customTCodeCommands.append(command);
        settingsChangedEvent(true);
    }
}

void SettingsHandler::removeCustomTCodeCommand(QString command)
{
    m_customTCodeCommands.removeAll(command);
    settingsChangedEvent(true);
}

void SettingsHandler::editCustomTCodeCommand(QString command, QString newCommand)
{
    if(m_customTCodeCommands.contains(command)) {
        m_customTCodeCommands.replace(m_customTCodeCommands.indexOf(command), newCommand);
        settingsChangedEvent(true);
    }
}

QString SettingsHandler::getSerialPort()
{
    QMutexLocker locker(&mutex);
    return serialPort;
}
QString SettingsHandler::getServerAddress()
{
    QMutexLocker locker(&mutex);
    return serverAddress;
}
QString SettingsHandler::getServerPort()
{
    QMutexLocker locker(&mutex);
    return serverPort;
}
int SettingsHandler::getPlayerVolume()
{
    QMutexLocker locker(&mutex);
    return playerVolume;
}

int SettingsHandler::getoffSet()
{
    QMutexLocker locker(&mutex);
    return offSet;
}
void SettingsHandler::setoffSet(int value)
{
    QMutexLocker locker(&mutex);
    offSet = value;
}

int SettingsHandler::getLiveOffSet()
{
    QMutexLocker locker(&mutex);
    return _liveOffset ? _liveOffset : offSet;
}
void SettingsHandler::setLiveOffset(int value)
{
    QMutexLocker locker(&mutex);
    _liveOffset = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::isSmartOffSet()
{
    return m_smartOffsetEnabled;
}

int SettingsHandler::getSmartOffSet()
{
    return m_smartOffsetEnabled ? m_smartOffset : 0;
}

void SettingsHandler::setSmartOffset(int value)
{
    m_smartOffset = value;
}

bool SettingsHandler::getDisableTCodeValidation()
{
    return _disableTCodeValidation;
}
void SettingsHandler::setDisableTCodeValidation(bool value)
{
    _disableTCodeValidation = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setAxis(QString axis, ChannelModel33 channel)
{
    QMutexLocker locker(&mutex);
    TCodeChannelLookup::setChannel(axis, channel);
    settingsChangedEvent(true);
}

void SettingsHandler::addAxis(ChannelModel33 channel)
{
    QMutexLocker locker(&mutex);
    TCodeChannelLookup::addChannel(channel.AxisName, channel);
    settingsChangedEvent(true);
}
void SettingsHandler::deleteAxis(QString axis)
{
    QMutexLocker locker(&mutex);
    TCodeChannelLookup::deleteChannel(axis);
    settingsChangedEvent(true);
}

int SettingsHandler::getChannelUserMin(QString channel)
{
    QMutexLocker locker(&mutex);
    return TCodeChannelLookup::getChannel(channel)->UserMin;
}
int SettingsHandler::getChannelUserMax(QString channel)
{
    QMutexLocker locker(&mutex);
    return TCodeChannelLookup::getChannel(channel)->UserMax;
}
void SettingsHandler::setChannelUserMin(QString channel, int value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->UserMin = value;
        if(channel == TCodeChannelLookup::Stroke())
            TCodeChannelLookup::setLiveXRangeMin(value);
        settingsChangedEvent(true);
    }
}
void SettingsHandler::setChannelUserMax(QString channel, int value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->UserMax = value;
        if(channel == TCodeChannelLookup::Stroke())
            TCodeChannelLookup::setLiveXRangeMax(value);
        settingsChangedEvent(true);
    }
}
void SettingsHandler::setChannelUserMid(QString channel, int value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->UserMid = value;
        if(channel == TCodeChannelLookup::Stroke())
            TCodeChannelLookup::setLiveXRangeMid(value);
        settingsChangedEvent(true);
    }
}

bool SettingsHandler::getMultiplierChecked(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->MultiplierEnabled;
    return false;
}
void SettingsHandler::setMultiplierChecked(QString channel, bool value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->MultiplierEnabled = value;
        settingsChangedEvent(true);
    }
}

bool SettingsHandler::getChannelFunscriptInverseChecked(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->FunscriptInverted;
    return false;
}
void SettingsHandler::setChannelFunscriptInverseChecked(QString channel, bool value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->FunscriptInverted = value;
        settingsChangedEvent(true);
    }
}

float SettingsHandler::getDamperValue(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->DamperValue;
    return 0.0;
}
void SettingsHandler::setDamperValue(QString channel, float value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->DamperValue = value;
        settingsChangedEvent(true);
    }
}

bool SettingsHandler::getDamperChecked(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->DamperEnabled;
    return false;
}
void SettingsHandler::setDamperChecked(QString channel, bool value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->DamperEnabled = value;
        settingsChangedEvent(true);
    }
}

bool SettingsHandler::getLinkToRelatedAxisChecked(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->LinkToRelatedMFS;
    return false;
}
void SettingsHandler::setLinkToRelatedAxisChecked(QString channel, bool value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->LinkToRelatedMFS = value;
        settingsChangedEvent(true);
    }
}

void SettingsHandler::setLinkToRelatedAxis(QString channel, QString linkedChannel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->RelatedChannel = linkedChannel;
        settingsChangedEvent(true);
    }
}

bool SettingsHandler::getChannelGamepadInverse(QString channel)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel))
        return TCodeChannelLookup::getChannel(channel)->GamepadInverted;
    return false;
}
void SettingsHandler::setChannelGamepadInverse(QString channel, bool value)
{
    QMutexLocker locker(&mutex);
    if(TCodeChannelLookup::hasChannel(channel)) {
        TCodeChannelLookup::getChannel(channel)->GamepadInverted = value;
        settingsChangedEvent(true);
    }
}


QString SettingsHandler::getDeoDnlaFunscript(QString key)
{
    QMutexLocker locker(&mutex);
    if (deoDnlaFunscriptLookup.contains(key))
    {
        return deoDnlaFunscriptLookup[key].toString();
    }
    return nullptr;
}
QHash<QString, QVariant> SettingsHandler::getDeoDnlaFunscripts()
{
    QMutexLocker locker(&mutex);
    return deoDnlaFunscriptLookup;
}

bool SettingsHandler::getGamepadEnabled()
{
    QMutexLocker locker(&mutex);
    return _gamePadEnabled;
}

int SettingsHandler::getGamepadSpeed()
{
    QMutexLocker locker(&mutex);
    return _gamepadSpeed;
}

void SettingsHandler::setGamepadSpeed(int value)
{
    QMutexLocker locker(&mutex);
    _gamepadSpeed = value;
    settingsChangedEvent(true);
}

int SettingsHandler::getGamepadSpeedIncrement()
{
    QMutexLocker locker(&mutex);
    return _gamepadSpeedStep;
}

void SettingsHandler::setGamepadSpeedStep(int value)
{
    QMutexLocker locker(&mutex);
    _gamepadSpeedStep = value;
    settingsChangedEvent(true);
}

int SettingsHandler::getLiveGamepadSpeed()
{
    QMutexLocker locker(&mutex);
    return _liveGamepadSpeed;
}

void SettingsHandler::setLiveGamepadSpeed(int value)
{
    QMutexLocker locker(&mutex);
    _liveGamepadSpeed = value;
}
void SettingsHandler::setLiveGamepadConnected(bool value)
{
    _liveGamepadConnected = value;
}
bool SettingsHandler::getLiveGamepadConnected()
{
    return _liveGamepadConnected;
}
void SettingsHandler::setLiveActionPaused(bool value)
{
    _liveActionPaused = value;
}
bool SettingsHandler::getLiveActionPaused()
{
    return _liveActionPaused;
}
void SettingsHandler::setXRangeStep(int value)
{
    QMutexLocker locker(&mutex);
    _xRangeStep = value;
    settingsChangedEvent(true);
}

int SettingsHandler::getXRangeStep()
{
    QMutexLocker locker(&mutex);
    return _xRangeStep;
}

void SettingsHandler::setLiveMultiplierEnabled(bool value)
{
    QMutexLocker locker(&mutex);
    _liveMultiplierEnabled = value;
}

bool SettingsHandler::getMultiplierEnabled()
{
    QMutexLocker locker(&mutex);
    return (_liveMultiplierEnabled && _multiplierEnabled) || _liveMultiplierEnabled;
}

void SettingsHandler::setMultiplierEnabled(bool value)
{
    QMutexLocker locker(&mutex);
    _multiplierEnabled = value;
    _liveMultiplierEnabled = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getDisableSpeechToText()
{
    return disableSpeechToText;
}
void SettingsHandler::setDisableSpeechToText(bool value)
{
    QMutexLocker locker(&mutex);
    disableSpeechToText = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getDisableNoScriptFound()
{
    return _disableNoScriptFound;
}
void SettingsHandler::setDisableNoScriptFound(bool value)
{
    QMutexLocker locker(&mutex);
    _disableNoScriptFound = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getDisableVRScriptSelect()
{
    return _disableVRScriptSelect;
}
void SettingsHandler::setDisableVRScriptSelect(bool value)
{
    QMutexLocker locker(&mutex);
    _disableVRScriptSelect = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setLibrarySortMode(int value)
{
    _librarySortMode = value;
    settingsChangedEvent(true);
}
LibrarySortMode SettingsHandler::getLibrarySortMode()
{
    return (LibrarySortMode)_librarySortMode;
}

void SettingsHandler::setShowVRInLibraryView(bool value) {
    QMutexLocker locker(&mutex);
    _showVRInLibraryView = value;
    settingsChangedEvent(true);
}
bool SettingsHandler::getShowVRInLibraryView() {
    return _showVRInLibraryView;
}

QMap<QString, QStringList>  SettingsHandler::getGamePadMap()
{
    return _gamepadButtonMap;
}

QMap<QString, QStringList> SettingsHandler::getGamePadMapInverse()
{
    _inverseGamePadMap.clear();
    foreach (auto key, _gamepadButtonMap.keys())
    {
        QStringList actions = _gamepadButtonMap.value(key);
        foreach(auto action, actions) {
            QStringList existingKeys;
            if(_inverseGamePadMap.contains(action)) {
                existingKeys = _inverseGamePadMap.value(action);
            }
            existingKeys << key;
            _inverseGamePadMap.insert(action, existingKeys);
        }
    }
    return _inverseGamePadMap;
}
QStringList SettingsHandler::getGamePadMapButton(QString gamepadButton)
{
    if (_gamepadButtonMap.contains(gamepadButton))
        return _gamepadButtonMap[gamepadButton];
    return QStringList();
}

void SettingsHandler::setGamePadMapButton(QString gamePadButton, QString action)
{
    QMutexLocker locker(&mutex);
    if(!_gamepadButtonMap[gamePadButton].contains(action))
    {
        _gamepadButtonMap[gamePadButton] << action;
    }
    settingsChangedEvent(true);
}

void SettingsHandler::removeGamePadMapButton(QString gamePadButton, QString action) {
    QMutexLocker locker(&mutex);
    _gamepadButtonMap[gamePadButton].removeAll(action);
    settingsChangedEvent(true);
}

void SettingsHandler::clearGamePadMapButton(QString gamePadButton) {
    _gamepadButtonMap[gamePadButton].clear();
    settingsChangedEvent(true);
}

QMap<QString, QStringList> SettingsHandler::getKeyboardMapInverse() {
    _inverseKeyboardMap.clear();
    foreach (auto key, _keyboardKeyMap.keys())
    {
        QStringList actions = _keyboardKeyMap.value(key);
        foreach(auto action, actions) {
            QStringList existingKeys;
            if(_inverseKeyboardMap.contains(action)) {
                existingKeys = _inverseKeyboardMap.value(action);
            }
            existingKeys.append(key);
            _inverseKeyboardMap.insert(action, existingKeys);
        }
    }
    return _inverseKeyboardMap;
}

QMap<QString, QStringList> SettingsHandler::getKeyboardMap() {
    return _keyboardKeyMap;
}

void SettingsHandler::setKeyboardMapKey(QString key, QString action) {
    QMutexLocker locker(&mutex);
    if(!_keyboardKeyMap[key].contains(action))
    {
        _keyboardKeyMap[key].append(action);
    }
    settingsChangedEvent(true);
}

void SettingsHandler::removeKeyboardMapKey(QString key, QString action) {
    QMutexLocker locker(&mutex);
    _keyboardKeyMap[key].removeAll(action);
    settingsChangedEvent(true);
}

void SettingsHandler::clearKeyboardMapKey(QString key) {
    _keyboardKeyMap[key].clear();
    settingsChangedEvent(true);
}

QStringList SettingsHandler::getKeyboardKeyActionList(int key, int modifiers)
{
    auto keyCode = getKeyboardKey(key, modifiers);
    if(!keyCode.isEmpty() && _keyboardKeyMap.contains(keyCode)) {
        return _keyboardKeyMap.value(keyCode);
    }
    return QStringList();
}
#include <QKeySequence>
QString SettingsHandler::getKeyboardKey(int key, int keyModifiers) {
    if(key == Qt::Key_unknown ||
        key == Qt::Key_Control ||
        key == Qt::Key_Shift ||
        key == Qt::Key_Alt ||
        key == Qt::Key_Meta)
    {
        return nullptr;
    }
//    if(keyModifiers & Qt::ShiftModifier)
//        key += Qt::SHIFT;
//    if(keyModifiers & Qt::ControlModifier)
//        key += Qt::CTRL;
//    if(keyModifiers & Qt::AltModifier)
//        key += Qt::ALT;
//    if(keyModifiers & Qt::MetaModifier)
//        key += Qt::META;

    return QKeySequence(keyModifiers+key).toString(QKeySequence::NativeText);
}

QMap<QString, QStringList> SettingsHandler::getTCodeCommandMap()
{
    return m_tcodeCommandMap;
}

QMap<QString, QStringList> SettingsHandler::getTCodeCommandMapInverse()
{
    m_inverseTcodeCommandMap.clear();
    foreach (auto key, m_tcodeCommandMap.keys())
    {
        QStringList actions = m_tcodeCommandMap.value(key);
        foreach(auto action, actions) {
            QStringList existingKeys;
            if(m_inverseTcodeCommandMap.contains(action)) {
                existingKeys = m_inverseTcodeCommandMap.value(action);
            }
            existingKeys << key;
            m_inverseTcodeCommandMap.insert(action, existingKeys);
        }
    }
    return m_inverseTcodeCommandMap;
}

QStringList SettingsHandler::getTCodeCommandMapCommands(QString command)
{
    if (m_tcodeCommandMap.contains(command))
        return m_tcodeCommandMap[command];
    return QStringList();
}

void SettingsHandler::setTCodeCommandMapKey(QString key, QString action)
{
    QMutexLocker locker(&mutex);
    if(!m_tcodeCommandMap[key].contains(action))
    {
        m_tcodeCommandMap[key].append(action);
    }
    settingsChangedEvent(true);
}

void SettingsHandler::removeTCodeCommandMapKey(QString key, QString action)
{
    QMutexLocker locker(&mutex);
    m_tcodeCommandMap[key].removeAll(action);
    settingsChangedEvent(true);
}

void SettingsHandler::clearTCodeCommandMapKey(QString key)
{
    m_tcodeCommandMap[key].clear();
    settingsChangedEvent(true);
}

QMap<QString, QString> SettingsHandler::getAllActions()
{
    QMap<QString, QString> actions;
    // auto tcodeVersionMap = TCodeChannelLookup::GetSelectedVersionMap();
    // for(auto __begin = tcodeVersionMap.begin(), __end = tcodeVersionMap.end();  __begin != __end; ++__begin) {
    //     auto channel = TCodeChannelLookup::getChannel(TCodeChannelLookup::ToString(__begin.key()));
    //     if(channel)
    //         actions.insert(channel->AxisName, "Channel: " + channel->FriendlyName);
    // }

    MediaActions actionsMap;
    for(auto __begin = actionsMap.Values.begin(), __end = actionsMap.Values.end();  __begin != __end; ++__begin) {
        actions.insert(__begin.key(), __begin.value());
    }

    auto otherActions = MediaActions::GetOtherActions();
    for(auto __begin = otherActions.begin(), __end = otherActions.end();  __begin != __end; ++__begin) {
        actions.insert(__begin.key(), __begin.value());
    }
    return actions;
}

// QMap<QString, TCodeCommand> SettingsHandler::getTCodeCommands()
// {
//     return m_tcodeCommands;
// }

// void SettingsHandler::setTCodeCommands(QMap<QString, TCodeCommand> commands)
// {
//     m_tcodeCommands = commands;
// }

// TCodeCommand* SettingsHandler::getTCodeCommand(QString command)
// {
//     auto itr = std::find_if(m_tcodeCommands.begin(), m_tcodeCommands.end(), [command](const TCodeCommand&  item) {
//         return item.command == command;
//     });
//     if(itr == m_tcodeCommands.end())
//         return 0;

//     return &m_tcodeCommands[itr.key()];
// }

// void SettingsHandler::addTCodeCommand(TCodeCommand command)
// {
//     m_tcodeCommands.insert(command.command, command);
// }

// void SettingsHandler::removeTCodeCommand(QString key)
// {
//     m_tcodeCommands.remove(key);
// }

void SettingsHandler::setSelectedFunscriptLibrary(QString value)
{
    QMutexLocker locker(&mutex);
    selectedFunscriptLibrary = value;
    settingsChangedEvent(true);
}
void SettingsHandler::setSerialPort(QString value)
{
    QMutexLocker locker(&mutex);
    serialPort = value;
    settingsChangedEvent(true);
}
void SettingsHandler::setServerAddress(QString value)
{
    QMutexLocker locker(&mutex);
    serverAddress = value;
    settingsChangedEvent(true);
}
void SettingsHandler::setServerPort(QString value)
{
    QMutexLocker locker(&mutex);
    serverPort = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setDeoAddress(QString value)
{
    QMutexLocker locker(&mutex);
    deoAddress = value;
    settingsChangedEvent(true);
}
void SettingsHandler::setDeoPort(QString value)
{
    QMutexLocker locker(&mutex);
    deoPort = value;
    settingsChangedEvent(true);
}
QString SettingsHandler::getDeoAddress()
{
    QMutexLocker locker(&mutex);
    return deoAddress;
}
QString SettingsHandler::getDeoPort()
{
    QMutexLocker locker(&mutex);
    return deoPort;
}
void SettingsHandler::setWhirligigAddress(QString value)
{
    QMutexLocker locker(&mutex);
    whirligigAddress = value;
    settingsChangedEvent(true);
}
void SettingsHandler::setWhirligigPort(QString value)
{
    QMutexLocker locker(&mutex);
    whirligigPort = value;
    settingsChangedEvent(true);
}
QString SettingsHandler::getWhirligigAddress()
{
    QMutexLocker locker(&mutex);
    return whirligigAddress;
}
QString SettingsHandler::getWhirligigPort()
{
    QMutexLocker locker(&mutex);
    return whirligigPort;
}

void SettingsHandler::setPlayerVolume(int value)
{
    QMutexLocker locker(&mutex);
    playerVolume = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setLibraryView(int value)
{
    QMutexLocker locker(&mutex);
    libraryView = value;
    settingsChangedEvent(true);
}
LibraryView SettingsHandler::getLibraryView()
{
    QMutexLocker locker(&mutex);
    return (LibraryView)libraryView;
}

int SettingsHandler::getThumbSize()
{
    QMutexLocker locker(&mutex);
    if (libraryView == LibraryView::List)
    {
        return thumbSizeList;
    }
    return thumbSize;
}
void SettingsHandler::setThumbSize(int value)
{
    QMutexLocker locker(&mutex);
    if (libraryView == LibraryView::List)
    {
        thumbSizeList = value;
    }
    else
    {
        thumbSize = value;
    }
    settingsChangedEvent(true);
}

QSize SettingsHandler::getMaxThumbnailSize()
{
    QMutexLocker locker(&mutex);
    return _maxThumbnailSize;
}

int SettingsHandler::getVideoIncrement()
{
    QMutexLocker locker(&mutex);
    return videoIncrement;
}
void SettingsHandler::setVideoIncrement(int value)
{
    videoIncrement = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setLinkedVRFunscript(QString key, QString value)
{
    QMutexLocker locker(&mutex);
    deoDnlaFunscriptLookup[key] = value;// Should be saved on edit
}
void SettingsHandler::removeLinkedVRFunscript(QString key)
{
    QMutexLocker locker(&mutex);
    deoDnlaFunscriptLookup.remove(key);// Should be saved on edit
}

void SettingsHandler::setGamepadEnabled(bool value)
{
    QMutexLocker locker(&mutex);
    _gamePadEnabled = value;
    settingsChangedEvent(true);
}

void SettingsHandler::setDecoderPriority(QList<DecoderModel> value)
{
    decoderPriority = value;
    settingsChangedEvent(true);
}
QList<DecoderModel> SettingsHandler::getDecoderPriority()
{
    return decoderPriority;
}
void SettingsHandler::setSelectedVideoRenderer(XVideoRenderer value)
{
    _selectedVideoRenderer = value;
    settingsChangedEvent(true);
}
XVideoRenderer SettingsHandler::getSelectedVideoRenderer()
{
    return _selectedVideoRenderer;
}

void SettingsHandler::addToLibraryExclusions(QString values)
{
    _libraryExclusions.append(values);
    settingsChangedEvent(true);
}
void SettingsHandler::removeFromLibraryExclusions(QList<int> indexes)
{
    foreach(auto index, indexes)
        _libraryExclusions.removeAt(index);
    settingsChangedEvent(true);
}
QStringList SettingsHandler::getLibraryExclusions()
{
    return _libraryExclusions;
}

QMap<QString, QList<LibraryListItem27>> SettingsHandler::getPlaylists()
{
    return _playlists;
}
void SettingsHandler::setPlaylists(QMap<QString, QList<LibraryListItem27>> value)
{
    _playlists = value;
    settingsChangedEvent(true);
}
void SettingsHandler::updatePlaylist(QString name, QList<LibraryListItem27> value)
{
    _playlists.insert(name, value);
    settingsChangedEvent(true);
}
void SettingsHandler::addToPlaylist(QString name, LibraryListItem27 value)
{
    auto playlist = _playlists.value(name);
    playlist.append(value);
    _playlists.insert(name, playlist);
    settingsChangedEvent(true);
}
void SettingsHandler::addNewPlaylist(QString name)
{
    QList<LibraryListItem27> playlist;
    _playlists.insert(name, playlist);
    settingsChangedEvent(true);
}
void SettingsHandler::deletePlaylist(QString name)
{
    _playlists.remove(name);
    settingsChangedEvent(true);
}

QString SettingsHandler::GetHashedPass()
{
    return _hashedPass;
}
void SettingsHandler::SetHashedPass(QString value)
{
    _hashedPass = value;
    settingsChangedEvent(true);
}

const QString &SettingsHandler::hashedWebPass()
{
    return _hashedWebPass;
}
void SettingsHandler::setHashedWebPass(const QString &newHashedWebPass)
{
    _hashedWebPass = newHashedWebPass;
}

//private
void SettingsHandler::setupGamepadButtonMap()
{
    _gamepadButtonMap = {
        { gamepadAxisNames.LeftXAxis, QStringList(TCodeChannelLookup::Twist()) },
        { gamepadAxisNames.LeftYAxis, QStringList(TCodeChannelLookup::Stroke()) },
        { gamepadAxisNames.RightYAxis, QStringList(TCodeChannelLookup::Pitch())  },
        { gamepadAxisNames.RightXAxis, QStringList(TCodeChannelLookup::Roll())  },
        { gamepadAxisNames.RightTrigger, QStringList(mediaActions.IncreaseXRange) },
        { gamepadAxisNames.LeftTrigger, QStringList(mediaActions.DecreaseXRange) },
        { gamepadAxisNames.RightBumper, QStringList(mediaActions.FastForward) },
        { gamepadAxisNames.LeftBumper, QStringList(mediaActions.Rewind) },
        { gamepadAxisNames.Select, QStringList(mediaActions.FullScreen) },
        { gamepadAxisNames.Start, QStringList(mediaActions.TogglePause) },
        { gamepadAxisNames.X, QStringList(mediaActions.TogglePauseAllDeviceActions) },
        { gamepadAxisNames.Y, QStringList(mediaActions.Loop) },
        { gamepadAxisNames.B, QStringList(mediaActions.Stop) },
        { gamepadAxisNames.A, QStringList(mediaActions.Mute) },
        { gamepadAxisNames.DPadUp, QStringList(mediaActions.IncreaseXRange) },
        { gamepadAxisNames.DPadDown, QStringList(mediaActions.DecreaseXRange) },
        { gamepadAxisNames.DPadLeft, QStringList(mediaActions.TCodeSpeedDown) },
        { gamepadAxisNames.DPadRight, QStringList(mediaActions.TCodeSpeedUp) },
        { gamepadAxisNames.RightAxisButton, QStringList(mediaActions.ToggleAxisMultiplier) },
        { gamepadAxisNames.LeftAxisButton, QStringList() },
        { gamepadAxisNames.Center, QStringList() },
        { gamepadAxisNames.Guide, QStringList() }
    };
}

void SettingsHandler::setupKeyboardKeyMap() {
    _keyboardKeyMap = {
        { getKeyboardKey(Qt::Key::Key_Space), QStringList(mediaActions.TogglePause) },
        { getKeyboardKey(Qt::Key::Key_Enter), QStringList(mediaActions.TogglePause) },
        { getKeyboardKey(Qt::Key::Key_MediaPause), QStringList(mediaActions.TogglePause) },
        { getKeyboardKey(Qt::Key::Key_MediaTogglePlayPause), QStringList(mediaActions.TogglePause) },
        { getKeyboardKey(Qt::Key::Key_F11), QStringList(mediaActions.FullScreen) },
        { getKeyboardKey(Qt::Key::Key_M), QStringList(mediaActions.Mute) },
        { getKeyboardKey(Qt::Key::Key_Escape), QStringList(mediaActions.Stop) },
        { getKeyboardKey(Qt::Key::Key_MediaStop), QStringList(mediaActions.Stop) },
        { getKeyboardKey(Qt::Key::Key_E), QStringList(mediaActions.Next) },
        { getKeyboardKey(Qt::Key::Key_MediaNext), QStringList(mediaActions.Next) },
        { getKeyboardKey(Qt::Key::Key_Q), QStringList(mediaActions.Back) },
        { getKeyboardKey(Qt::Key::Key_MediaPrevious), QStringList(mediaActions.Back) },
        { getKeyboardKey(Qt::Key::Key_W), QStringList(mediaActions.VolumeUp) },
        { getKeyboardKey(Qt::Key::Key_VolumeUp), QStringList(mediaActions.VolumeUp) },
        { getKeyboardKey(Qt::Key::Key_S), QStringList(mediaActions.VolumeDown) },
        { getKeyboardKey(Qt::Key::Key_VolumeDown), QStringList(mediaActions.VolumeDown) },
        { getKeyboardKey(Qt::Key::Key_L), QStringList(mediaActions.Loop) },
        { getKeyboardKey(Qt::Key::Key_A), QStringList(mediaActions.Rewind) },
        { getKeyboardKey(Qt::Key::Key_D), QStringList(mediaActions.FastForward) },
        { getKeyboardKey(Qt::Key::Key_X), QStringList(mediaActions.IncreaseXRange) },
        { getKeyboardKey(Qt::Key::Key_Z), QStringList(mediaActions.DecreaseXRange) },
        { getKeyboardKey(Qt::Key::Key_F), QStringList(mediaActions.DecreaseXUpperRange) },
        { getKeyboardKey(Qt::Key::Key_R), QStringList(mediaActions.IncreaseXUpperRange) },
        { getKeyboardKey(Qt::Key::Key_G), QStringList(mediaActions.DecreaseXLowerRange) },
        { getKeyboardKey(Qt::Key::Key_T), QStringList(mediaActions.IncreaseXLowerRange) },
        { getKeyboardKey(Qt::Key::Key_Y), QStringList(mediaActions.ResetLiveXRange) },
        { getKeyboardKey(Qt::Key::Key_I), QStringList(mediaActions.ToggleFunscriptInvert) },
        { getKeyboardKey(Qt::Key::Key_V), QStringList(mediaActions.ToggleAxisMultiplier) },
        { getKeyboardKey(Qt::Key::Key_P), QStringList(mediaActions.TogglePauseAllDeviceActions) },
        { getKeyboardKey(Qt::Key::Key_J), QStringList(mediaActions.SkipToMoneyShot) },
        { getKeyboardKey(Qt::Key::Key_K), QStringList(mediaActions.SkipToAction) }
    };
}

// void SettingsHandler::setupTCodeCommands()
// {
//     m_tcodeCommands = {
//         { "#edge:1", { 0, TCodeCommandType::BUTTON, "#edge:1", 0 } },
//         { "#ok:1", { 1, TCodeCommandType::BUTTON, "#ok:1", 0 } },
//         { "#left:1", { 2, TCodeCommandType::BUTTON, "#left:1", 0 } },
//         { "#right:1", { 3, TCodeCommandType::BUTTON, "#right:1", 0 } },
//     };
// }

void SettingsHandler::setupTCodeCommandMap()
{
    m_tcodeCommandMap = {
        { "#edge:1", QStringList(mediaActions.TogglePauseAllDeviceActions) },
        { "#ok:1", QStringList(mediaActions.TogglePause) },
        { "#left:1", QStringList(mediaActions.AltFunscriptNext) },
        { "#right:1", QStringList(mediaActions.SkipToMoneyShot) },
    };
    // m_tcodeCommandMap.clear();
    // for ( auto it = defaultCommands.begin(); it != defaultCommands.end(); it++ ) {
    //     auto commandObject = std::find_if(m_tcodeCommands.begin(), m_tcodeCommands.end(),  [it] (const TCodeCommand& item) {
    //         return item.command == it.key();
    //     });
    //     if(commandObject != m_tcodeCommands.end()) {
    //         m_tcodeCommandMap.insert(it.key(), defaultCommands[it.key()]);
    //     }
    // }
}

void SettingsHandler::setFunscriptLoaded(QString key, bool loaded)
{
    if (_funscriptLoaded.contains(key))
        _funscriptLoaded[key] = loaded;
}
bool SettingsHandler::getFunscriptLoaded(QString key)
{
    if (_funscriptLoaded.contains(key))
        return _funscriptLoaded[key];
    return false;
}

bool SettingsHandler::getSkipToMoneyShotPlaysFunscript()
{
    return _skipToMoneyShotPlaysFunscript;
}
void SettingsHandler::setSkipToMoneyShotPlaysFunscript(bool value)
{
    _skipToMoneyShotPlaysFunscript = value;
    settingsChangedEvent(true);
}

QString SettingsHandler::getSkipToMoneyShotFunscript()
{
    return _skipToMoneyShotFunscript;
}
void SettingsHandler::setSkipToMoneyShotFunscript(QString value)
{
    _skipToMoneyShotFunscript = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getSkipToMoneyShotSkipsVideo()
{
    return _skipToMoneyShotSkipsVideo;
}
void SettingsHandler::setSkipToMoneyShotSkipsVideo(bool value)
{
    _skipToMoneyShotSkipsVideo = value;
    settingsChangedEvent(true);
}
bool SettingsHandler::getSkipToMoneyShotStandAloneLoop()
{
    return _skipToMoneyShotStandAloneLoop;
}
void SettingsHandler::setSkipToMoneyShotStandAloneLoop(bool value)
{
    _skipToMoneyShotStandAloneLoop = value;
    settingsChangedEvent(true);
}
bool SettingsHandler::getHideStandAloneFunscriptsInLibrary()
{
    return _hideStandAloneFunscriptsInLibrary;
}
void SettingsHandler::setHideStandAloneFunscriptsInLibrary(bool value)
{
    _hideStandAloneFunscriptsInLibrary = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getSkipPlayingStandAloneFunscriptsInLibrary()
{
    return _skipPlayingSTandAloneFunscriptsInLibrary;
}
void SettingsHandler::setSkipPlayingStandAloneFunscriptsInLibrary(bool value)
{
    _skipPlayingSTandAloneFunscriptsInLibrary = value;
    settingsChangedEvent(true);
}

bool SettingsHandler::getEnableHttpServer()
{
    return _enableHttpServer;
}

void SettingsHandler::setEnableHttpServer(bool enable)
{
    QMutexLocker locker(&mutex);
    _enableHttpServer = enable;
    settingsChangedEvent(true);
}

QString SettingsHandler::getHttpServerRoot()
{
    return _httpServerRoot;
}
void SettingsHandler::setHttpServerRoot(QString value)
{
    QMutexLocker locker(&mutex);
    _httpServerRoot = value;
    settingsChangedEvent(true);
}

qint64 SettingsHandler::getHTTPChunkSize()
{
    return _httpChunkSize;
}
void SettingsHandler::setHTTPChunkSize(qint64 value)
{
    QMutexLocker locker(&mutex);
    _httpChunkSize = value;
    settingsChangedEvent(true);
}

int SettingsHandler::getHTTPPort()
{
    return _httpPort;
}
void SettingsHandler::setHTTPPort(int value)
{
    QMutexLocker locker(&mutex);
    _httpPort = value;
    settingsChangedEvent(true);
}
int SettingsHandler::getWebSocketPort()
{
    return _webSocketPort;
}
void SettingsHandler::setWebSocketPort(int value)
{
    QMutexLocker locker(&mutex);
    _webSocketPort = value;
    settingsChangedEvent(true);
}
int SettingsHandler::getHttpThumbQuality()
{
    return _httpThumbQuality;
}
void SettingsHandler::setHttpThumbQuality(int value)
{
    QMutexLocker locker(&mutex);
    _httpThumbQuality = value;
    settingsChangedEvent(true);
}

void  SettingsHandler::setFunscriptModifierStep(int value)
{
    QMutexLocker locker(&mutex);
    _funscriptModifierStep = value;
}
int  SettingsHandler::getFunscriptModifierStep()
{
    QMutexLocker locker(&mutex);
    return _funscriptModifierStep;
}

void  SettingsHandler::setFunscriptOffsetStep(int value)
{
    QMutexLocker locker(&mutex);
    _funscriptOffsetStep = value;
}
int  SettingsHandler::getFunscriptOffsetStep()
{
    QMutexLocker locker(&mutex);
    return _funscriptOffsetStep;
}

void SettingsHandler::setLubePulseAmount(int value)
{
    QMutexLocker locker(&mutex);
    _channelPulseAmount = value;
}
int SettingsHandler::getLubePulseAmount()
{
    QMutexLocker locker(&mutex);
    return _channelPulseAmount;
}
void SettingsHandler::setLubePulseEnabled(bool value)
{
    QMutexLocker locker(&mutex);
    _channelPulseEnabled = value;
}
bool SettingsHandler::getLubePulseEnabled()
{
    QMutexLocker locker(&mutex);
    return _channelPulseEnabled;
}
void SettingsHandler::setLubePulseFrequency(int value)
{
    QMutexLocker locker(&mutex);
    _channelPulseFrequency = value;
}
int SettingsHandler::getLubePulseFrequency()
{
    QMutexLocker locker(&mutex);
    return _channelPulseFrequency;
}


QHash<QString, LibraryListItemMetaData258> SettingsHandler::getLibraryListItemMetaData()
{
    return _libraryListItemMetaDatas;
}

LibraryListItemMetaData258 SettingsHandler::getLibraryListItemMetaData(const LibraryListItem27 item)
{
    QMutexLocker locker(&mutex);
    if(_libraryListItemMetaDatas.contains(item.path))
    {
        return _libraryListItemMetaDatas.value(item.path);
    }
    //Default meta data
    QList<QString> funscripts;
    QList<Bookmark> bookmarks;
    QList<QString> tags;
    _libraryListItemMetaDatas.insert(item.path, {
                                         item.path, // libraryItemPath
                                         false,
                                         -1, // lastPlayPosition
                                         false, // lastLoopEnabled
                                         -1, // lastLoopStart
                                         -1, // lastLoopEnd
                                         0, // offset
                                         -1, // moneyShotMillis
                                         bookmarks, // bookmarks
                                         funscripts,
                                         tags
                                     });
    return _libraryListItemMetaDatas.value(item.path);
}

void SettingsHandler::removeLibraryListItemMetaData(const QString key)
{
    _libraryListItemMetaDatas.remove(key);
}

void SettingsHandler::updateLibraryListItemMetaData(LibraryListItemMetaData258 libraryListItemMetaData, bool sync)
{
    QMutexLocker locker(&mutex);
    _libraryListItemMetaDatas.insert(libraryListItemMetaData.libraryItemPath, libraryListItemMetaData);
    if(sync)
    {
        storeMediaMetaDatas();
        settings->sync();
        //settingsChangedEvent(true);
    }
}

QSettings* SettingsHandler::settings;
QString SettingsHandler::_applicationDirPath;
SettingsHandler* SettingsHandler::m_instance = 0;
bool SettingsHandler::m_firstLoad;
bool SettingsHandler::_settingsChanged;
bool SettingsHandler::_hideWelcomeScreen;
QMutex SettingsHandler::mutex;
QString SettingsHandler::_appdataLocation;
QHash<QString, bool> SettingsHandler::_funscriptLoaded;
QSize SettingsHandler::_maxThumbnailSize = {500, 500};
GamepadAxisName SettingsHandler::gamepadAxisNames;
MediaActions SettingsHandler::mediaActions;
QHash<QString, QVariant> SettingsHandler::deoDnlaFunscriptLookup;
QStringList SettingsHandler::selectedLibrary;
QString SettingsHandler::_selectedThumbsDir;
bool SettingsHandler::_useMediaDirForThumbs;
int SettingsHandler::_selectedOutputDevice;
NetworkDeviceType SettingsHandler::_selectedNetworkDeviceType;
int SettingsHandler::_librarySortMode;
int SettingsHandler::playerVolume;
int SettingsHandler::offSet;
bool SettingsHandler::_disableTCodeValidation;
QStringList SettingsHandler::m_customTCodeCommands;

int SettingsHandler::libraryView = LibraryView::Thumb;
int SettingsHandler::thumbSize = 175;
int SettingsHandler::thumbSizeList = 50;
int SettingsHandler::videoIncrement = 10;

bool SettingsHandler::_gamePadEnabled;
QMap<QString, QStringList> SettingsHandler::_gamepadButtonMap;
QMap<QString, QStringList> SettingsHandler::_inverseGamePadMap;
QMap<QString, QStringList> SettingsHandler::_keyboardKeyMap;
QMap<QString, QStringList> SettingsHandler::_inverseKeyboardMap;
QMap<QString, QStringList> SettingsHandler::m_tcodeCommandMap;
QMap<QString, QStringList> SettingsHandler::m_inverseTcodeCommandMap;

// QMap<QString, TCodeCommand> SettingsHandler::m_tcodeCommands;

int SettingsHandler::_gamepadSpeed;
int SettingsHandler::_gamepadSpeedStep;
int SettingsHandler::_liveGamepadSpeed;
bool SettingsHandler::_liveGamepadConnected;
bool SettingsHandler::_liveActionPaused;
int SettingsHandler::_liveOffset;
bool SettingsHandler::m_smartOffsetEnabled = false;
int SettingsHandler::m_smartOffset = 0;

int SettingsHandler::_xRangeStep;
bool SettingsHandler::_liveMultiplierEnabled = false;
bool SettingsHandler::_multiplierEnabled;

QString SettingsHandler::selectedFunscriptLibrary;
QString SettingsHandler::selectedFile;
QString SettingsHandler::serialPort;
QString SettingsHandler::serverAddress;
QString SettingsHandler::serverPort;
QString SettingsHandler::deoAddress;
QString SettingsHandler::deoPort;
bool SettingsHandler::deoEnabled;
QString SettingsHandler::whirligigAddress;
QString SettingsHandler::whirligigPort;
bool SettingsHandler::whirligigEnabled;
bool SettingsHandler::_xtpWebSyncEnabled;
bool SettingsHandler::_saveOnExit = true;
bool SettingsHandler::disableSpeechToText;
bool SettingsHandler::_disableVRScriptSelect;
bool SettingsHandler::_disableNoScriptFound;

bool SettingsHandler::_skipToMoneyShotPlaysFunscript;
QString SettingsHandler::_skipToMoneyShotFunscript;
bool SettingsHandler::_skipToMoneyShotSkipsVideo;
bool SettingsHandler::_skipToMoneyShotStandAloneLoop;

bool SettingsHandler::_hideStandAloneFunscriptsInLibrary;
bool SettingsHandler::_skipPlayingSTandAloneFunscriptsInLibrary;

bool SettingsHandler::_enableHttpServer;
QString SettingsHandler::_httpServerRoot;
qint64 SettingsHandler::_httpChunkSize;
int SettingsHandler::_httpPort;
int SettingsHandler::_webSocketPort;
int SettingsHandler::_httpThumbQuality;
QStringList SettingsHandler::_vrLibrary;
bool SettingsHandler::_showVRInLibraryView;

int SettingsHandler::_funscriptModifierStep;
int SettingsHandler::_funscriptOffsetStep;

bool SettingsHandler::_channelPulseEnabled;
qint64 SettingsHandler::_channelPulseFrequency;
int SettingsHandler::_channelPulseAmount;

QString SettingsHandler::_hashedPass;
QString SettingsHandler::_hashedWebPass;

QList<DecoderModel> SettingsHandler::decoderPriority;
XVideoRenderer SettingsHandler::_selectedVideoRenderer;

float SettingsHandler::m_viewedThreshold;

QStringList SettingsHandler::_libraryExclusions;
QMap<QString, QList<LibraryListItem27>> SettingsHandler::_playlists;
QHash<QString, LibraryListItemMetaData258> SettingsHandler::_libraryListItemMetaDatas;

QTimer SettingsHandler::m_settingsChangedNotificationDebounce;

XTags SettingsHandler::m_xTags;
