#ifndef INPUTDEVICEPACKET_H
#define INPUTDEVICEPACKET_H
#include <QString>
#include <QMetaType>

struct InputDevicePacket
{
    QString path;
    qint64 duration;
    qint64 currentTime;
    float playbackSpeed;
    bool playing;
    bool stopped;
};

Q_DECLARE_METATYPE(InputDevicePacket);
#endif // INPUTDEVICEPACKET_H
