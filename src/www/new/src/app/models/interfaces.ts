import { ConnectionInterface, 
        NetworkProtocol, 
        ConnectionStatus, 
        ChannelDimension, 
        ChannelType, 
        ChannelTimeType, 
        MediaType, 
        ScriptType, 
        ScriptContainerType,
        ThumbState, 
        FormControlType, 
        SettingProfile, 
        Track } from './enums';

export interface MediaItem {
    id: string;
    type: MediaType;
    name: string;
    displayName: string;
    path: string;
    relativePath: string;
    subtitle: string;
    subtitleRelative: string;
    script: string;
    scriptNoExtensionRelativePath: string;
    thumb: string;
    relativeThumb: string;
    thumbSize: number;
    duration?: number;
    modifiedDate: Date;
    isMFS: boolean;
    tooltip: string;
    hasScript: boolean;
    thumbState: ThumbState;
    thumbFileLoading: string;
    thumbFileLoadingCurrent: string;
    thumbFileError: string;
    thumbFileUnknown: string;
    thumbFileExists: boolean;
    thumbVersion?: number;
    loaded: boolean;
    playing: boolean;
    managedThumb: boolean;
    metaData: MediaItemMetaData;
}

export interface MediaItemMetaData {
    id: string;
    key: string;
    libraryItemPath: string;
    watched: boolean;
    lastPlayPosition: string; // long
    lastLoopEnabled: boolean;
    lastLoopStart: string; // long
    lastLoopEnd: string; // long
    offset: number;
    moneyShotMillis: string; // long
    moneyShotSecs: number;
    funscriptModifier: number;
    rating: number;
    toolTip: string;
    subtitle: string;
    isMFS: boolean;
    isSFMA: boolean;
    hasAlternate: boolean;
    dateAdded: Date;
    thumbExtractError: string;
    bookmarks: Bookmark[];
    tags: string[];
    mfsScripts: string[];// TODO: do these convert from MFS to mfs camel case?
    mfsTracks: string[];
    scripts: ScriptInfo[];
}

export interface ChannelInfo {
    channel: string;
    friendlyName: string;
}

export interface AvailableChannels {
    [name: string]: ChannelModel;
}

export interface ChannelModel {
    // track: Track ;
    // channelName: string;
    // channel: string;
    // min: number;
    // mid: number;
    // max: number;
    // userMin: number;
    // userMid: number;
    // userMax: number;
    // dimension: ChannelDimension;
    // type: ChannelType;
    // trackName: string;
    // multiplierEnabled: boolean;
    // speedEnabled: boolean;
    // speedRandom: boolean;
    // speedValue: number;
    // funscriptInverted: boolean;
    // gamepadInverted: boolean;
    // linkToRelatedMFS: boolean;
    // relatedChannel: string;
    // offset: number;
    // gradient: boolean;
    // linkedInverted: boolean;

    axisName: string;
    channel: string;
    dimension: number;
    friendlyName: string;
    funscriptInverted: boolean;
    gamepadInverted: boolean;
    gradient: boolean;
    linkToRelatedMFS: boolean;
    linkedInverted: boolean;
    max: number;
    mid: number;
    min: number;
    multiplierEnabled: boolean;
    offset: number;
    relatedChannel: string;
    speedEnabled: boolean;
    speedRandom: boolean;
    speedValue: number;
    track: number;
    trackName: string;
    type: number;
    userMax: number;
    userMid: number;
    userMin: number;
}

export interface RemoteUserSettings {
    os: string;
    xteVersion: string;
    webSocketServerPort: number;
    availableChannels: AvailableChannels;
    multiplierEnabled: boolean;
    selectedChannelProfile: string;
    allChannelProfileNames: string[];
    connection: ConnectionInfo;
    hasLaunchPass: boolean;
    allTags: string[];
    userTags: string[];
    smartTags: string[];
    settings: Setting[];
    args: string[];
    availableChannelsArray?: ChannelModel[];
    // Pending tag changes, applied on save
    tagsToAdd?: string[];
    tagsToRemove?: string[];
    smartTagsToAdd?: string[];
    smartTagsToRemove?: string[];
}

export interface TCodeCommand {
    name: string;
    command: string;
    hidden: boolean;
}

export interface ActiveSession {
    id: string;
    lastAccessed: string;
    expire: string;
    current: boolean;
}

export interface ConnectionInfo {
    input: ConnectionInputInfo;
    output: ConnectionOutputInfo;
}

export interface ConnectionInputInfo {
    selectedDevice: ConnectionInterface;
    gamePadEnabled: boolean;
    deoAddress: string;
    deoPort: string;
}

export interface ConnectionOutputInfo {
    selectedDevice: ConnectionInterface;
    networkAddress: string;
    networkPort: string;
    networkProtocol: NetworkProtocol;
    serialPort: string;
}

export interface Setting {
    key: string;
    label: string;
    description: string;
    type: FormControlType;
    group: string;
    value: any;
    profile: SettingProfile;
    internal?: boolean;
    requiresRestart?: boolean;
}

export interface AllSettings {
    settings: Setting[];
}

export interface DeviceState {
    type: ConnectionInterface;
    status: ConnectionStatus;
    address?: string;
}

export interface WebSocketMessage {
    command: string;
    message?: any;
}

export interface ChannelStatus {
    channel: string;
    value: number;
    time: number;
    timeType: ChannelTimeType;
    type: ChannelType;
}

export interface ProfileInfo {
    name: string;
    channels: ChannelInfo[];
}

export interface BrowserInfo {
    userAgent: string;
    isDeo: boolean;
    isHereSphere: boolean;
    isMobile: boolean;
}

export interface Bookmark {
    name: string;
    time: string;// long
    channelProfile: string;
}

export interface ScriptInfo {
    ///
    /// \brief name User friendly name
    ///
    name: string;
    ///
    /// \brief filename Base file name without the path no extension
    ///
    filename: string;
    ///
    /// \brief path Full path to file
    ///
    path: string;
    ///
    /// \brief track Mfs track is exists. Empty otherwise
    ///
    track: string;
    type: ScriptType;
    containerType: ScriptContainerType;
    containerTypeName: string;
}

export interface XTPWebMediaState {
    path: string | undefined;
    playing: boolean;
    currentTime: number;
    duration: number;
    playbackSpeed: number;
}