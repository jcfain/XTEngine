#ifndef ENUM_H
#define ENUM_H
enum ConnectionDirection
{
    Input,
    Output
};

enum ConnectionInterface
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

enum ConnectionStatus
{
    Connected,
    Disconnected,
    Connecting,
    Error
};

enum class NetworkProtocol {
    UDP,
    WEBSOCKET
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
    ADDED_ASC,
    ADDED_DESC,
    NONE
};

enum class LibraryType
{
    MAIN,
    FUNSCRIPT,
    VR,
    EXCLUSION
};

#endif // ENUM_H
