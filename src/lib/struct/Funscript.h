#ifndef FUNSCRIPTSTRUCT_H
#define FUNSCRIPTSTRUCT_H

#include <QString>
#include <QHash>
#include "../lookup/Track.h"

struct FunscriptActionSequence {
    int lastDestinationPos = -1;
    int lastDestinationAt = -1;
    int lastDestinationInterval = -1;
    int currentDestinationPos = -1;
    int currentDestinationAt = -1;
    int currentDestinationInterval = -1;
    int nextDestinationPos = -1;
    int nextDestinationAt = -1;
    int nextDestinationInterval = -1;
};

struct FunscriptAction {
    QString channel;
    qint64 at;
    int pos;
    int interval;
    // int lastPos;
    // qint64 lastAt;
    // int lastSpeed;
    // int nextPos;
    // qint64 nextAt;
    // int nextSpeed;
    FunscriptActionSequence currentSequence;
    FunscriptActionSequence nextSequence;
    qint64 index;
};

struct FunscriptBookmark {
    QString name;
    qint64 time;
};

struct FunscriptChapter {
    QString name;
    qint64 startTime;
    qint64 endTime;
};

struct FunscriptMetadata {
    QString creator;
    QString  original_name;
    QString url;
    QString url_video;
    QList<QString> tags;
    QList<QString> performers;
    bool paid;
    QString comment;
    qint64 original_total_duration_ms;
    QList<FunscriptBookmark> bookmarks;
    QList<FunscriptChapter> chapters;
};

struct XFunscript {
    Track channel;
    QString trackName;
    double modifier = 1;
    qint64 lastActionIndex = -1;
    qint64 nextActionIndex = 0;
    int lastActionPos = 0;
    qint64 lastActionAt = 0;
    int lastActionPosModified = 0;
    int lastActionInterval = 0;
    int nextActionPos = 0;
    qint64 nextActionAt = 0;
    int nextActionPosModified = 0;
    int nextActionInterval = 0;
    qint64 min = -1;
    qint64 max = -1;
    QList<qint64> atList;
};

struct Funscript {
    QString version;
    bool inverted = false;
    // int range = 1;
    QHash<qint64, int> actions;
    FunscriptMetadata metadata;
    XFunscript settings;
};


#endif // FUNSCRIPTSTRUCT_H
