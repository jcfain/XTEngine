#ifndef ENUM_H
#define ENUM_H
enum DeviceType
{
    Input,
    Output
};

enum DeviceName
{
    Serial,
    Network,
    Deo,
    Whirligig,
    Gamepad,
    XTPWeb,
    None
};

enum class NetworkDeviceType {
    UDP,
    WEBSOCKET
};

enum ConnectionStatus
{
    Connected,
    Disconnected,
    Connecting,
    Error
};

enum LibraryView
{
    Thumb,
    List
};

enum LibrarySortMode {
    NAME_ASC,
    NAME_DESC,
    RANDOM,
    CREATED_ASC,
    CREATED_DESC,
    TYPE_ASC,
    TYPE_DESC,
    NONE
};

enum PasswordResponse {
    CORRECT,
    INCORRECT,
    CANCEL
};
#endif // ENUM_H
