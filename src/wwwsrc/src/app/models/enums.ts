export enum XLogLevel {
    Information = 0,
    Debuging = 1,
    Warning = 2,
    Critical = 3
}

export enum ConnectionStatus {
    Connected = 0,
    Disconnected = 1,
    Connecting = 2,
    Error = 3
}

export enum ConnectionInterface
{
    Serial,
    Network,
    HereSphere,
    Whirligig,
    Gamepad,
    XTPWeb,
    BLE,
    None
};

export enum ChannelType {
    None = 0,
    Oscillate = 1,
    Ramp = 2,
    HalfOscillate = 3
}

export enum ChannelTimeType {
    None = 0,
    Interval = 1,
    Speed = 2
}

export enum ChannelDimension {
    None = 0,
    Heave = 1,
    Surge = 2,
    Sway = 3,
    Pitch = 4,
    Roll = 5,
    Yaw = 6
}

export enum MediaType {
    PlaylistInternal = 0,
    Video = 1,
    Audio = 2,
    FunscriptType = 3,
    VR = 4,
    ExternalType = 5
}

export enum ScriptType {
    MAIN = 0,
    ALTERNATE = 1
}

export enum ScriptContainerType {
    BASE = 0,
    MFS = 1,
    ZIP = 2
}

export enum Roles {
    DisplayRole = 0,
    DecorationRole = 1,
    EditRole = 2,
    ToolTipRole = 3,
    StatusTipRole = 4,
    WhatsThisRole = 5,
    FontRole = 6,
    TextAlignmentRole = 7,
    BackgroundRole = 8,
    ForegroundRole = 9
}

export enum ThumbState {
    Waiting = 0,
    Loading = 1,
    Error = 2,
    Ready = 3,
    Unknown = 4
}

export enum NetworkProtocol {
    UDP = 0,
    WEBSOCKET = 1
}

export enum SettingProfile {
    System = 0
}

export enum FormControlType {
    Double = 0,
    Int = 1,
    Text = 2,
    Radio = 3,
    Combo = 4,
    Checkbox = 5,
    DateTime = 6,
    Date = 7,
    Time = 8
}

export enum Track
{
    None,
    Stroke,
    StrokeUp,
    StrokeDown,
    Sway,
    SwayLeft,
    SwayRight,
    Surge,
    SurgeBack,
    SurgeForward,
    Twist,
    TwistClockwise,
    TwistCounterClockwise,
    Roll,
    RollLeft,
    RollRight,
    Pitch,
    PitchForward,
    PitchBack,
    Vib,
    Lube,
    Suck,
    SuckMore,
    SuckLess,
    SuckPosition,
    SuckMorePosition,
    SuckLessPosition,
    TRACKS_LENGTH
}

export enum SortMode {
    Name,
    NameDesc,
    Date,
    DateDesc,
    DateAdded,
    DateAddedDesc,
    Random
}
export enum ShowMode {
    All,
    TwoD,
    ThreeD
}

export enum AuthResult {
    Fail,
    Success
}

export enum SaveState {
    Idle,
    Saving,
    Success,
    Fail
}



export class MediaActions {
    static readonly TogglePause = "TogglePause";
    static readonly Next = "Next";
    static readonly Back = "Back";
    static readonly FullScreen = "FullScreen";
    static readonly VolumeUp = "VolumeUp";
    static readonly VolumeDown = "VolumeDown";
    static readonly Mute = "Mute";
    static readonly Stop = "Stop";
    static readonly Loop = "Loop";
    static readonly Rewind = "Rewind";
    static readonly FastForward = "Fast forward";
    static readonly TCodeSpeedUp = "TCodeSpeedUp";
    static readonly TCodeSpeedDown = "TCodeSpeedDown";
    static readonly TCodeHomeAll = "TCodeHomeAll";
    static readonly IncreaseXRange = "IncreaseXRange";
    static readonly DecreaseXRange = "DecreaseXRange";
    static readonly IncreaseXUpperRange = "IncreaseXUpperRange";
    static readonly DecreaseXUpperRange = "DecreaseXUpperRange";
    static readonly IncreaseXLowerRange = "IncreaseXLowerRange";
    static readonly DecreaseXLowerRange = "DecreaseXLowerRange";
    static readonly ResetLiveXRange = "ResetLiveXRange";
    static readonly ToggleAxisMultiplier = "ToggleAxisMiltiplier";
    static readonly ToggleChannelRollMultiplier = "ToggleChannelRollMultiplier";
    static readonly ToggleChannelPitchMultiplier = "ToggleChannelPitchMultiplier";
    static readonly ToggleChannelSurgeMultiplier = "ToggleChannelSurgeMultiplier";
    static readonly ToggleChannelSwayMultiplier = "ToggleChannelSwayMultiplier";
    static readonly ToggleChannelTwistMultiplier = "ToggleChannelTwistMultiplier";
    static readonly ToggleFunscriptInvert = "ToggleFunscriptInvert";
    static readonly TogglePauseAllDeviceActions = "TogglePauseAllDeviceActions";
    static readonly ChannelProfileNext = "ChannelProfileNext";
    static readonly ChannelProfilePrevious = "ChannelProfilePrevious";
    static readonly SkipToMoneyShot = "SkipToMoneyShot";
    static readonly ToggleSkipToMoneyShotPlaysFunscript = "ToggleSkipToMoneyShotPlaysFunscript";
    static readonly SkipToAction = "SkipToAction";
    static readonly IncreaseFunscriptModifier = "IncreaseFunscriptModifier";
    static readonly DecreaseFunscriptModifier = "DecreaseFunscriptModifier";
    static readonly ResetFunscriptModifier = "ResetFunscriptModifier";
    static readonly IncreaseOffset = "IncreaseDelay";
    static readonly DecreaseOffset = "DecreaseDelay";
    static readonly ResetOffset = "ResetOffset";
    static readonly IncreasePlaybackRate = "IncreasePlaybackRate";
    static readonly DecreasePlaybackRate = "DecreasePlaybackRate";
    static readonly ResetPlaybackRate = "ResetPlaybackRate";
    static readonly AltFunscriptNext = "AltFunscriptNext";
    static readonly AltFunscriptPrev = "AltFunscriptPrev";
}

;
