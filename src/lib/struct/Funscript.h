#ifndef FUNSCRIPTSTRUCT_H
#define FUNSCRIPTSTRUCT_H

#include <QString>
#include <QHash>
#include "../lookup/Track.h"

struct FunscriptAction {
    QString channel;
    qint64 at;
    int pos;
    int speed;
    int gradient;
    int lastPos;
    int lastSpeed;
    int nextPos;
    int nextSpeed;
    int lastGradient;
    int nextGradient;
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
    int lastActionPosModified = 0;
    int lastActionInterval = 0;
    int lastActionGradient = 0;
    int nextActionPos = 0;
    int nextActionPosModified = 0;
    int nextActionInterval = 0;
    int nextActionGradient = 0;
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

struct FunscriptActionSequence {
    int posA = -1;
    int atA = -1;
    int posB = -1;
    int atB = -1;
    int posC = -1;
    int atC = -1;
};


#endif // FUNSCRIPTSTRUCT_H
