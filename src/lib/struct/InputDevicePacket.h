#ifndef VRPACKET_H
#define VRPACKET_H
#include <QString>
#include <QMetaType>

struct InputDevicePacket
{
    QString path;
    qint64 duration;
    qint64 currentTime;
    float playbackSpeed;
    bool playing;
};

Q_DECLARE_METATYPE(InputDevicePacket);
#endif // DEOPACKET_H
