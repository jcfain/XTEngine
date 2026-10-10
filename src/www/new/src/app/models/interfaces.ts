import { DeviceType, ConnectionStatus, ChannelType, ChannelTimeType, MediaType, ScriptType, ThumbState, FormControlType, SettingProfile } from './enums';

export interface MediaItem {
    id: number;
    name: string;
    type: MediaType;
    thumbnail?: string;
    duration?: number;
    loaded?: boolean;
    playing?: boolean;
    viewed?: number;
    tags?: string[];
    [key: string]: any;
}

export interface ChannelInfo {
    channel: string;
    friendlyName: string;
}

export interface RemoteUserSettings {
    availableChannelsArray: ChannelInfo[];
    [key: string]: any;
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