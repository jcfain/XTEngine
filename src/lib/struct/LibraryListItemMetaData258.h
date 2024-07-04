#ifndef LIBRARYLISTITEMMETADATA258_H
#define LIBRARYLISTITEMMETADATA258_H
#include <QString>
#include <QMetaType>
#include <QDate>
#include <QVariant>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include "Bookmark.h"
#include "XTEngine_global.h"

struct XTENGINE_EXPORT LibraryListItemMetaData258
{
    QString key;
    QString libraryItemPath;
    bool watched;
    qint64 lastPlayPosition;
    bool lastLoopEnabled;
    qint64 lastLoopStart;
    qint64 lastLoopEnd;
    int offset;
    qint64 moneyShotMillis;
    QString toolTip;
    QString subtitle;
    bool isMFS;
    QList<Bookmark> bookmarks;
    QList<QString> funscripts;
    QList<QString> tags;
    QStringList MFSScripts;

    friend QDataStream & operator<<(QDataStream &dataStream, const LibraryListItemMetaData258 &object )
    {
        dataStream << object.key;
        dataStream << object.libraryItemPath;
        dataStream << object.watched;
        dataStream << object.lastPlayPosition;
        dataStream << object.lastLoopEnabled;
        dataStream << object.lastLoopStart;
        dataStream << object.lastLoopEnd;
        dataStream << object.offset;
        dataStream << object.moneyShotMillis;
        dataStream << object.toolTip;
        dataStream << object.subtitle;
        dataStream << object.isMFS;
        foreach(auto bookmark, object.bookmarks )
            dataStream << bookmark;
        foreach(auto funscript, object.funscripts )
            dataStream << funscript;
        foreach(auto tag, object.tags )
            dataStream << tag;
        foreach(auto script, object.MFSScripts )
            dataStream << script;
        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, LibraryListItemMetaData258 &object)
    {
        dataStream >> object.key;
        dataStream >> object.libraryItemPath;
        dataStream >> object.watched;
        dataStream >> object.lastPlayPosition;
        dataStream >> object.lastLoopEnabled;
        dataStream >> object.lastLoopStart;
        dataStream >> object.lastLoopEnd;
        dataStream >> object.offset;
        dataStream >> object.moneyShotMillis;
        dataStream >> object.toolTip;
        dataStream >> object.subtitle;
        dataStream >> object.isMFS;
        foreach(auto bookmark, object.bookmarks )
            dataStream >> bookmark;
        foreach(auto funscript, object.funscripts )
            dataStream >> funscript;
        foreach(auto tag, object.tags )
            dataStream >> tag;
        foreach(auto script, object.MFSScripts )
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
        obj["toolTip"] = item.toolTip;
        obj["subtitle"] = item.subtitle;
        obj["isMFS"] = item.isMFS;
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
        return obj;
    }

    static LibraryListItemMetaData258 fromJson(QJsonObject obj) {
        LibraryListItemMetaData258 newItem;
        newItem.key = obj["key"].toString();
        newItem.libraryItemPath = obj["libraryItemPath"].toString();
        newItem.watched = obj["watched"].toBool();
        newItem.lastPlayPosition = obj["lastPlayPosition"].toString().toLongLong();
        newItem.lastLoopEnabled = obj["lastLoopEnabled"].toBool();
        newItem.lastLoopStart = obj["lastLoopStart"].toString().toLongLong();
        newItem.lastLoopEnd = obj["lastLoopEnd"].toString().toLongLong();
        newItem.offset = obj["offset"].toInt();
        newItem.moneyShotMillis = obj["moneyShotMillis"].toString().toLongLong();
        newItem.toolTip = obj["toolTip"].toString();
        newItem.subtitle = obj["subtitle"].toString();
        newItem.isMFS = obj["isMFS"].toBool();
        foreach(auto bookmark, obj["bookmarks"].toArray())
        {
            Bookmark bookMark;
            bookMark.Name = bookmark["Name"].toString();
            bookMark.Time = bookmark["Time"].toString().toLongLong();
            newItem.bookmarks.append(bookMark);
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
        return newItem;
    }
};

Q_DECLARE_METATYPE(LibraryListItemMetaData258);
#endif // LIBRARYLISTITEMMETADATA_H
