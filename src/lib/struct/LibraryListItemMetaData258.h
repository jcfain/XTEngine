#ifndef LIBRARYLISTITEMMETADATA258_H
#define LIBRARYLISTITEMMETADATA258_H
#include <QString>
#include <QMetaType>
#include <QDate>
#include <QVariant>
#include <QFileInfo>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include "Bookmark.h"
#include "ScriptInfo.h"
#include "XTEngine_global.h"

struct XTENGINE_EXPORT LibraryListItemMetaData258
{
    QString ID;
    QString key;
    QString libraryItemPath;
    bool watched;
    qint64 lastPlayPosition;
    bool lastLoopEnabled;
    qint64 lastLoopStart;
    qint64 lastLoopEnd;
    int offset;
    qint64 moneyShotMillis;
    double funscriptModifier;
    //Live values
    QString toolTip;
    QString subtitle;
    bool isMFS;
    bool isSFMA;
    bool hasAlternate;
    QDateTime dateAdded;
    QString thumbExtractError;

    QList<Bookmark> bookmarks;
    QList<QString> funscripts;
    QList<QString> tags;
    QStringList MFSScripts;
    QStringList MFSTracks;
    QList<ScriptInfo> scripts;

    friend QDataStream & operator<<(QDataStream &dataStream, const LibraryListItemMetaData258 &object )
    {
        dataStream << object.ID;
        dataStream << object.key;
        dataStream << object.libraryItemPath;
        dataStream << object.watched;
        dataStream << object.lastPlayPosition;
        dataStream << object.lastLoopEnabled;
        dataStream << object.lastLoopStart;
        dataStream << object.lastLoopEnd;
        dataStream << object.offset;
        dataStream << object.moneyShotMillis;
        dataStream << object.funscriptModifier;
        dataStream << object.toolTip;
        dataStream << object.subtitle;
        dataStream << object.isMFS;
        dataStream << object.isSFMA;
        dataStream << object.hasAlternate;
        dataStream << object.dateAdded;
        dataStream << object.thumbExtractError;
        foreach(auto bookmark, object.bookmarks )
            dataStream << bookmark;
        foreach(auto funscript, object.funscripts )
            dataStream << funscript;
        foreach(auto tag, object.tags )
            dataStream << tag;
        foreach(auto script, object.MFSScripts )
            dataStream << script;
        foreach(auto track, object.MFSTracks )
            dataStream << track;
        foreach(auto script, object.scripts )
            dataStream << script;

        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, LibraryListItemMetaData258 &object)
    {
        dataStream >> object.ID;
        dataStream >> object.key;
        dataStream >> object.libraryItemPath;
        dataStream >> object.watched;
        dataStream >> object.lastPlayPosition;
        dataStream >> object.lastLoopEnabled;
        dataStream >> object.lastLoopStart;
        dataStream >> object.lastLoopEnd;
        dataStream >> object.offset;
        dataStream >> object.moneyShotMillis;
        dataStream >> object.funscriptModifier;
        dataStream >> object.toolTip;
        dataStream >> object.subtitle;
        dataStream >> object.isMFS;
        dataStream >> object.isSFMA;
        dataStream >> object.hasAlternate;
        dataStream >> object.dateAdded;
        dataStream >> object.thumbExtractError;
        foreach(auto bookmark, object.bookmarks )
            dataStream >> bookmark;
        foreach(auto funscript, object.funscripts )
            dataStream >> funscript;
        foreach(auto tag, object.tags )
            dataStream >> tag;
        foreach(auto script, object.MFSScripts )
            dataStream >> script;
        foreach(auto track, object.MFSTracks )
            dataStream >> track;
        foreach(auto script, object.scripts )
            dataStream >> script;
        return dataStream;
    }
    friend bool operator==(const LibraryListItemMetaData258 &p1, const LibraryListItemMetaData258 &p2)
    {
        return p1.libraryItemPath == p2.libraryItemPath;
    }

    static LibraryListItemMetaData258 fromVariant(QVariant item)
    {
        QJsonObject obj = item.toJsonObject();
        return fromJson(obj);
//        newItem.libraryItemPath = obj["libraryItemPath"].toString();
//        newItem.lastPlayPosition = obj["lastPlayPosition"].toString().toLongLong();
//        newItem.lastLoopEnabled = obj["lastLoopEnabled"].toBool();
//        newItem.lastLoopStart = obj["lastLoopStart"].toString().toLongLong();
//        newItem.lastLoopEnd = obj["lastLoopEnd"].toString().toLongLong();
//        newItem.offset = obj["offset"].toInt();
//        newItem.moneyShotMillis = obj["moneyShotMillis"].toString().toLongLong();
//        foreach(auto bookmark, obj["bookmarks"].toArray())
//        {
//            Bookmark bookMark;
//            bookMark.Name = bookmark["Name"].toString();
//            bookMark.Time = bookmark["Time"].toString().toLongLong();
//            newItem.bookmarks.append(bookMark);
//        }
//        foreach(auto funscript, obj["funscripts"].toArray())
//        {
//            newItem.funscripts.append(funscript.toString());
//        }
//        return newItem;
    }

    static QVariant toVariant(LibraryListItemMetaData258 item)
    {
        return QVariant::fromValue(toJson(item));
    }

    static QJsonObject toJson(LibraryListItemMetaData258 item)
    {
        QJsonObject obj;
        obj["ID"] = item.ID;
        obj["key"] = item.key;
        obj["libraryItemPath"] = item.libraryItemPath;
        obj["watched"] = item.watched;
        obj["lastPlayPosition"] = QString::number(item.lastPlayPosition);
        obj["lastLoopEnabled"] = item.lastLoopEnabled;
        obj["lastLoopStart"] = QString::number(item.lastLoopStart);
        obj["lastLoopEnd"] = QString::number(item.lastLoopEnd);
        obj["offset"] = item.offset;
        obj["moneyShotMillis"] = QString::number(item.moneyShotMillis);
        obj["moneyShotSecs"] = item.moneyShotMillis / 1000;
        obj["funscriptModifier"] = item.funscriptModifier;
        obj["toolTip"] = item.toolTip;
        obj["subtitle"] = item.subtitle;
        obj["isMFS"] = item.isMFS;
        obj["isSFMA"] = item.isSFMA;
        obj["hasAlternate"] = item.hasAlternate;
        obj["dateAdded"] = item.dateAdded.toString(Qt::DateFormat::ISODateWithMs);
        obj["thumbExtractError"] = item.thumbExtractError;

        QJsonArray bookmarks;
        foreach(Bookmark bookmark, item.bookmarks) {
            QJsonObject bookmarkObj;
            bookmarkObj["Name"] = bookmark.Name;
            bookmarkObj["Time"] = QString::number(bookmark.Time);
            bookmarks.append(bookmarkObj);
        }
        obj["bookmarks"] = bookmarks;
        QJsonArray funscripts;
        foreach(QString funscript, item.funscripts) {
            funscripts.append(funscript);
        }
        //obj["funscripts"] = funscripts;
        QJsonArray tags;
        foreach(QString tag, item.tags) {
            tags.append(tag);
        }
        obj["tags"] = tags;
        QJsonArray mfsScripts;
        foreach (auto script, item.MFSScripts) {
            mfsScripts.append(script);
        }
        obj["MFSScripts"] = mfsScripts;
        QJsonArray mfsTracks;
        foreach (auto script, item.MFSTracks) {
            mfsScripts.append(script);
        }
        obj["MFSTracks"] = mfsTracks;
        QJsonArray scripts;
        foreach (auto script, item.scripts) {
            scripts.append(ScriptInfo::toJson(script));
        }
        obj["scripts"] = scripts;
        return obj;
    }

    static LibraryListItemMetaData258 fromJson(QJsonObject obj) {
        LibraryListItemMetaData258 newItem;
        newItem.ID = obj["ID"].toString();
        newItem.key = obj["key"].toString();
        newItem.libraryItemPath = obj["libraryItemPath"].toString();
        newItem.watched = obj["watched"].toBool();
        newItem.lastPlayPosition = obj["lastPlayPosition"].toString("-1").toLongLong();
        newItem.lastLoopEnabled = obj["lastLoopEnabled"].toBool();
        newItem.lastLoopStart = obj["lastLoopStart"].toString("-1").toLongLong();
        newItem.lastLoopEnd = obj["lastLoopEnd"].toString("-1").toLongLong();
        newItem.offset = obj["offset"].toInt(0);
        newItem.moneyShotMillis = obj["moneyShotMillis"].toString("-1").toLongLong();
        newItem.funscriptModifier = obj["funscriptModifier"].toDouble(100.0);
        newItem.toolTip = obj["toolTip"].toString();
        newItem.subtitle = obj["subtitle"].toString();
        newItem.isMFS = obj["isMFS"].toBool();
        newItem.isSFMA = obj["isSFMA"].toBool();
        newItem.hasAlternate = obj["hasAlternate"].toBool();
        newItem.dateAdded = QDateTime::fromString(obj["dateAdded"].toString(), Qt::DateFormat::ISODateWithMs);
        newItem.thumbExtractError = obj["thumbExtractError"].toString(nullptr);
        foreach(auto jasonValueConstObj, obj["bookmarks"].toArray())
        {
            QJsonObject obj = jasonValueConstObj.toObject();
            Bookmark bookmark;
            bookmark.Name = obj["Name"].toString();
            bookmark.Time = obj["Time"].toString().toLongLong();
            newItem.bookmarks.append(bookmark);
        }
        foreach(auto funscript, obj["funscripts"].toArray())
        {
            newItem.funscripts.append(funscript.toString());
        }
        foreach(auto tag, obj["tags"].toArray())
        {
            newItem.tags.append(tag.toString());
        }
        foreach(auto script, obj["MFSScripts"].toArray())
        {
            newItem.MFSScripts.append(script.toString());
        }
        foreach(auto track, obj["MFSTracks"].toArray())
        {
            newItem.MFSTracks.append(track.toString());
        }
        foreach(auto track, obj["scripts"].toArray())
        {
            auto script = track.toObject();
            auto path = script["path"].toString();
            if(QFileInfo::exists(path))
                newItem.scripts.append(ScriptInfo::fromJson(track.toObject()));
        }
        return newItem;
    }

    void defaultValues(const QString& IDIn, const QString& keyIn, const QString& libraryMediaPathIn)
    {
        QJsonObject obj;
        LibraryListItemMetaData258 newItem = fromJson(obj);
        ID = IDIn;
        key = keyIn;
        libraryItemPath = libraryMediaPathIn;
        watched = newItem.watched;
        lastPlayPosition = newItem.lastPlayPosition;
        lastLoopEnabled = newItem.lastLoopEnabled;
        lastLoopStart = newItem.lastLoopStart;
        lastLoopEnd = newItem.lastLoopEnd;
        offset = newItem.offset;
        moneyShotMillis = newItem.moneyShotMillis;
        funscriptModifier = newItem.funscriptModifier;
        toolTip = newItem.toolTip;
        subtitle = newItem.subtitle;
        isMFS = newItem.isMFS;
        isSFMA = newItem.isSFMA;
        hasAlternate = newItem.hasAlternate;
        bookmarks.clear();
        thumbExtractError = newItem.thumbExtractError;
        foreach(auto bookmark, newItem.bookmarks)
        {
            Bookmark bookMark;
            bookMark.Name = bookMark.Name;
            bookMark.Time = bookmark.Time;
            bookmarks.append(bookMark);
        }
        funscripts.clear();
        foreach(auto funscript, newItem.funscripts)
        {
            funscripts.append(funscript);
        }
        tags.clear();
        foreach(auto tag, newItem.tags)
        {
            tags.append(tag);
        }
        MFSScripts.clear();
        foreach(auto script, newItem.MFSScripts)
        {
            MFSScripts.append(script);
        }
        MFSTracks.clear();
        foreach(auto track, newItem.MFSTracks)
        {
            MFSTracks.append(track);
        }
        scripts.clear();
        foreach(auto track, newItem.scripts)
        {
            scripts.append(track);
        }
    }
};

Q_DECLARE_METATYPE(LibraryListItemMetaData258);
#endif // LIBRARYLISTITEMMETADATA_H
