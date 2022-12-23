#ifndef FUNSCRIPTSTRUCT_H
#define FUNSCRIPTSTRUCT_H

#include <QString>
#include <QHash>

struct FunscriptAction {
    QString channel;
    qint64 at;
    int pos;
    int speed;
    int lastPos;
    int lastSpeed;
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

struct Funscript {
    QString version;
    bool inverted;
    int range;
    QHash<qint64, int> actions;
    FunscriptMetadata metadata;
};


#endif // FUNSCRIPTSTRUCT_H
