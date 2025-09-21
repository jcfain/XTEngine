#ifndef INPUTCONNECTIONPACKET_H
#define INPUTCONNECTIONPACKET_H
#include <QString>
#include <QMetaType>

struct InputConnectionPacket
{
    QString path;
    qint64 duration;
    qint64 currentTime;
    double playbackSpeed;
    bool playing;
    bool stopped;
};

Q_DECLARE_METATYPE(InputConnectionPacket)
#endif // INPUTCONNECTIONPACKET_H
