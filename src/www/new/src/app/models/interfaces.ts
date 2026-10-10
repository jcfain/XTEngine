import { DeviceType, 
        NetworkProtocol, 
        ConnectionStatus, 
        ConnectionInterface, 
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
    id: number;
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
    modifiedDate: string;
    isMFS: boolean;
    tooltip: string;
    hasScript: boolean;
    thumbState: ThumbState;
    thumbFileLoading: string;
    thumbFileLoadingCurrent: string;
    thumbFileError: string;
    thumbFileUnknown: string;
    thumbFileExists: boolean;
    loaded: boolean;
    playing: boolean;
    managedThumb: boolean;
    metaData: MediaItemMetaData;

    // TODO: what are these?
    thumbnail?: string;
    viewed?: number;
    tags?: string[];
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

export interface ChannelModel {
    track: Track ;
    channelName: string;
    channel: string;
    min: number;
    mid: number;
    max: number;
    userMin: number;
    userMid: number;
    userMax: number;
    dimension: ChannelDimension;
    type: ChannelType;
    trackName: string;
    multiplierEnabled: boolean;
    speedEnabled: boolean;
    speedRandom: boolean;
    speedValue: number;
    funscriptInverted: boolean;
    gamepadInverted: boolean;
    linkToRelatedMFS: boolean;
    relatedChannel: string;
    offset: number;
    gradient: boolean;
    linkedInverted: boolean;
}

export interface RemoteUserSettings {
    os: string;
    xteVersion: string;
    webSocketServerPort: number;
    availableChannels: ChannelModel[];
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

    // availableChannelsArray: ChannelInfo[];

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
    type: DeviceType;
    status: ConnectionStatus;
    address?: string;
}

export interface WebSocketMessage {
    command: string;
    [key: string]: any;
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