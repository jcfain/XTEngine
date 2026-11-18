#ifndef FUNSCRIPTSTRUCT_H
#define FUNSCRIPTSTRUCT_H

#include <QString>
#include <QHash>
#include <QJsonObject>
#include <QJsonArray>
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

    friend bool operator==(const FunscriptBookmark &object1, const FunscriptBookmark &object2)
    {
        return object1.name == object2.name
            && object1.time == object2.time;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["time"] = QString::number(time);
        return obj;
    }

    static FunscriptBookmark fromJson(QJsonObject obj)
    {
        FunscriptBookmark bookmark;
        bookmark.name = obj["name"].toString();
        bookmark.time = obj["time"].toString().toLongLong();
        return bookmark;
    }
};

struct FunscriptChapter {
    QString name;
    qint64 startTime;
    qint64 endTime;

    friend bool operator==(const FunscriptChapter &object1, const FunscriptChapter &object2)
    {
        return object1.name == object2.name
            && object1.startTime == object2.startTime
            && object1.endTime == object2.endTime;
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["name"] = name;
        obj["startTime"] = QString::number(startTime);
        obj["endTime"] = QString::number(endTime);
        return obj;
    }

    static FunscriptChapter fromJson(QJsonObject obj)
    {
        FunscriptChapter chapter;
        chapter.name = obj["name"].toString();
        chapter.startTime = obj["startTime"].toString().toLongLong();
        chapter.endTime = obj["endTime"].toString().toLongLong();
        return chapter;
    }
};

struct FunscriptMetadata {
    QString creator;
    QString  original_name;
    QString url;
    QString url_video;
    QList<QString> tags;
    QList<QString> performers;
    bool paid = false;
    ///
    /// \brief hasPaid True if the paid property exists in the script file
    ///
    bool hasPaid = false;
    QString comment;
    ///
    /// \brief original_total_duration_ms -1 if not present in the script file
    ///
    qint64 original_total_duration_ms = -1;
    QList<FunscriptBookmark> bookmarks;
    QList<FunscriptChapter> chapters;

    friend bool operator==(const FunscriptMetadata &object1, const FunscriptMetadata &object2)
    {
        return object1.creator == object2.creator
            && object1.original_name == object2.original_name
            && object1.url == object2.url
            && object1.url_video == object2.url_video
            && object1.tags == object2.tags
            && object1.performers == object2.performers
            && object1.paid == object2.paid
            && object1.hasPaid == object2.hasPaid
            && object1.comment == object2.comment
            && object1.original_total_duration_ms == object2.original_total_duration_ms
            && object1.bookmarks == object2.bookmarks
            && object1.chapters == object2.chapters;
    }

    friend bool operator!=(const FunscriptMetadata &object1, const FunscriptMetadata &object2)
    {
        return !(object1 == object2);
    }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["creator"] = creator;
        obj["original_name"] = original_name;
        obj["url"] = url;
        obj["url_video"] = url_video;
        QJsonArray tagsArray;
        foreach(QString tag, tags)
            tagsArray.append(tag);
        obj["tags"] = tagsArray;
        QJsonArray performersArray;
        foreach(QString performer, performers)
            performersArray.append(performer);
        obj["performers"] = performersArray;
        obj["paid"] = paid;
        obj["hasPaid"] = hasPaid;
        obj["comment"] = comment;
        obj["original_total_duration_ms"] = QString::number(original_total_duration_ms);
        QJsonArray bookmarksArray;
        foreach(FunscriptBookmark bookmark, bookmarks)
            bookmarksArray.append(bookmark.toJson());
        obj["bookmarks"] = bookmarksArray;
        QJsonArray chaptersArray;
        foreach(FunscriptChapter chapter, chapters)
            chaptersArray.append(chapter.toJson());
        obj["chapters"] = chaptersArray;
        return obj;
    }

    static FunscriptMetadata fromJson(QJsonObject obj)
    {
        FunscriptMetadata metadata;
        metadata.creator = obj["creator"].toString();
        metadata.original_name = obj["original_name"].toString();
        metadata.url = obj["url"].toString();
        metadata.url_video = obj["url_video"].toString();
        foreach(auto tag, obj["tags"].toArray())
            metadata.tags.append(tag.toString());
        foreach(auto performer, obj["performers"].toArray())
            metadata.performers.append(performer.toString());
        metadata.paid = obj["paid"].toBool();
        metadata.hasPaid = obj["hasPaid"].toBool();
        metadata.comment = obj["comment"].toString();
        // Only overwrite when present: a missing key must keep the -1
        // "not present" sentinel (toString() on a missing value is "" -> 0).
        if (obj.contains("original_total_duration_ms"))
            metadata.original_total_duration_ms = obj["original_total_duration_ms"].toString().toLongLong();
        foreach(auto bookmark, obj["bookmarks"].toArray())
            metadata.bookmarks.append(FunscriptBookmark::fromJson(bookmark.toObject()));
        foreach(auto chapter, obj["chapters"].toArray())
            metadata.chapters.append(FunscriptChapter::fromJson(chapter.toObject()));
        return metadata;
    }
};

struct XFunscript {
    Track channel;
    QString trackName;
    double modifier = 1;
    qint64 lastActionIndex = -1;
    qint64 nextActionIndex = 0;
    qint64 lastActionAt = 0;
    int lastActionInterval = 0;
    int nextActionPos = 0;
    qint64 nextActionAt = 0;
    int nextActionInterval = 0;
    qint64 min = -1;
    qint64 max = -1;
    QList<qint64> atList;
    // Per-action scale center (parallel to atList): the midpoint of the turning points
    // (local min/max) bracketing that action in time. The amplitude modifier scales each
    // action around its own local center so each oscillation band expands/contracts
    // independently of the others.
    QList<double> centerList;
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
