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

// Front end agnostic: the web front end mirrors this enum (ThumbObjectFit in
// wwwsrc/src/app/models/enums.ts) and translates the value to the CSS
// object-fit property itself. Keep the values in sync with the TS enum.
// Smart is resolved per image by the front end: images narrower than the
// frame are shown whole (contain), wider ones fill the frame (cover).
enum class MediaThumbObjectFit
{
    Cover = 0,
    ScaleDown = 1,
    Contain = 2,
    Smart = 3
};

#endif // ENUM_H
