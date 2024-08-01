#ifndef LIBRARYLISTITEM27_H
#define LIBRARYLISTITEM27_H
#include <QString>
#include <QMetaType>
#include <QDate>
#include <QVariant>
#include <QDataStream>
#include <QJsonObject>
#include <QJsonArray>
#include "XTEngine_global.h"
#include "LibraryListItemMetaData258.h"


#define ERROR_IMAGE "://images/icons/error.png"
#define LOADING_IMAGE "://images/icons/loading.png"
#define LOADING_CURRENT_IMAGE "://images/icons/loading_current.png"

enum class LibraryListItemType {
    PlaylistInternal,
    Video,
    Audio,
    FunscriptType,
    VR,
    External
};

enum class ThumbState {
    Waiting,
    Loading,
    Error,
    Ready
};

struct  XTENGINE_EXPORT LibraryListItem27
{
public:
    // LibraryListItem27() {}
    // LibraryListItem27(const LibraryListItem27* item)
    // {
    //     // type = item->type;
    //     // path = item->path;
    //     // name = item->name;
    //     // nameNoExtension = item->nameNoExtension;
    //     // script = item->script;
    //     // pathNoExtension = item->pathNoExtension;
    //     // mediaExtension = item->mediaExtension;
    //     // thumbFile = item->thumbFile;
    //     // zipFile = item->zipFile;
    //     // modifiedDate = item->modifiedDate;
    //     // duration = item->duration;
    //     // ID = item->ID;
    //     // libraryPath = item->libraryPath;
    //     // hasScript = item->hasScript;
    //     // metadata = LibraryListItemMetaData258(item->metadata);
    //     // thumbState = item->thumbState;
    //     // thumbFileExists = item->thumbFileExists;
    //     // managedThumb = item->managedThumb;
    //     // forceProcessMetadata = item->forceProcessMetadata;
    //     // thumbNails = item->thumbNails;
    //     // thumbFileLoading = item->thumbFileLoading;
    //     // thumbFileLoadingCurrent = item->thumbFileLoadingCurrent;
    //     // thumbFileError = item->thumbFileError;
    //     copyProperties(item, *this);
    // }
    LibraryListItemType type;
    QString path;
    QString name;
    QString nameNoExtension;
    QString script;
    QString pathNoExtension;
    QString mediaExtension;
    QString thumbFile;
    QString zipFile;
    QDate modifiedDate;
    quint64 duration;
    QString md5;

    // Live members
    QString ID;
    QString libraryPath;
    bool hasScript;
    LibraryListItemMetaData258 metadata;
    ThumbState thumbState = ThumbState::Waiting;
    bool thumbFileExists = false;
    bool managedThumb = false;
    bool forceProcessMetadata = false;
    // const QString thumbFileLoading = LibraryThumbNail::LOADING_IMAGE;// "://images/icons/loading.png";
    // const QString thumbFileLoadingCurrent = LibraryThumbNail::LOADING_CURRENT_IMAGE;// "://images/icons/loading_current.png";
    // const QString thumbFileError = LibraryThumbNail::ERROR_IMAGE;;// "://images/icons/error.png";

    friend QDataStream & operator<<( QDataStream &dataStream, const LibraryListItem27 &object )
    {
        dataStream << object.ID;
        dataStream << (int)object.type;
        dataStream << object.path;
        dataStream << object.name;
        dataStream << object.nameNoExtension;
        dataStream << object.script;
        dataStream << object.pathNoExtension;
        dataStream << object.mediaExtension;
        dataStream << object.thumbFile;
        dataStream << object.zipFile;
        dataStream << object.modifiedDate;
        dataStream << object.duration;
        dataStream << object.md5;
        dataStream << object.libraryPath;
        dataStream << object.thumbFileExists;
        dataStream << (int)object.thumbState;
        // dataStream << object.thumbFileLoading;
        // dataStream << object.thumbFileLoadingCurrent;
        // dataStream << object.thumbFileError;
        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, LibraryListItem27 &object)
    {
        dataStream >> object.ID;
        dataStream >> object.type;
        dataStream >> object.path;
        dataStream >> object.name;
        dataStream >> object.nameNoExtension;
        dataStream >> object.script;
        dataStream >> object.pathNoExtension;
        dataStream >> object.mediaExtension;
        dataStream >> object.thumbFile;
        dataStream >> object.zipFile;
        dataStream >> object.modifiedDate;
        dataStream >> object.duration;
        dataStream >> object.md5;
        dataStream >> object.libraryPath;
        dataStream >> object.thumbFileExists;
        dataStream >> object.thumbState;
        return dataStream;
    }

    friend bool operator==(const LibraryListItem27 &p1, const LibraryListItem27 &p2)
    {
       return p1.ID == p2.ID;
    }
    void copyProperties(const LibraryListItem27& from) {
        path = from.path;
        duration = from.duration;
        md5 = from.md5;
        mediaExtension = from.mediaExtension;
        modifiedDate = from.modifiedDate;
        name = from.name;
        nameNoExtension = from.nameNoExtension;
        script = from.script;
        pathNoExtension = from.pathNoExtension;
        thumbFile = from.thumbFile;
        type = from.type;
        zipFile = from.zipFile;
        //Live
        ID = from.ID;
        libraryPath = from.libraryPath;
        hasScript = from.hasScript;
        thumbState = from.thumbState;
        thumbFileExists = from.thumbFileExists;
        managedThumb = from.managedThumb;
        forceProcessMetadata = from.forceProcessMetadata;
        metadata = LibraryListItemMetaData258(from.metadata);
    }

    static LibraryListItem27 fromVariant(QVariant item)
    {
        QJsonObject obj = item.toJsonObject();
        return fromJson(obj);
    }

    static QJsonObject toJson(const LibraryListItem27 item)
    {
        QJsonObject obj;
        obj["path"] = item.path;
        obj["duration"] = QString::number(item.duration);
        obj["md5"] = item.md5;
        obj["mediaExtension"] = item.mediaExtension;;
        obj["modifiedDate"] = item.modifiedDate.toString();
        obj["name"] = item.name;
        obj["nameNoExtension"] = item.nameNoExtension;
        obj["script"] = item.script;
        obj["scriptNoExtension"] = item.pathNoExtension;
        obj["thumbFile"] = item.thumbFile;
        obj["type"] = (int)item.type;
        obj["zipFile"] = item.zipFile;

        //Live
        obj["id"] = item.ID;
        obj["libraryPath"] = item.libraryPath;
        obj["hasScript"] = item.hasScript;
        obj["thumbState"] = (int)item.thumbState;
        obj["thumbFileExists"] = item.thumbFileExists;
        obj["managedThumb"] = item.managedThumb;
        obj["forceProcessMetadata"] = item.forceProcessMetadata;
        obj["metadata"] = LibraryListItemMetaData258::toJson(item.metadata);
        return obj;
    }

    static QVariant toVariant(const LibraryListItem27 item)
    {
        QJsonObject obj;
        //obj["id"] = item.ID;
        obj["path"] = item.path;
        obj["duration"] = QString::number(item.duration);
        obj["md5"] = item.md5;
        obj["mediaExtension"] = item.mediaExtension;;
        obj["modifiedDate"] = item.modifiedDate.toString();
        obj["name"] = item.name;
        obj["nameNoExtension"] = item.nameNoExtension;
        obj["script"] = item.script;
        obj["scriptNoExtension"] = item.pathNoExtension;
        obj["thumbFile"] = item.thumbFile;
        obj["type"] = (int)item.type;
        obj["zipFile"] = item.zipFile;
//        obj["isMFS"] = item.isMFS;
//        obj["tooltip"] = item.toolTip;
        return QVariant::fromValue(obj);
     }

    static LibraryListItem27 fromJson(const QJsonObject obj) {
        LibraryListItem27 newItem;
        //newItem.ID = obj["id"].toInt();
        newItem.path = obj["path"].toString();
        newItem.duration = obj["path"].toString().toLongLong();
        newItem.md5 = obj["md5"].toString();
        newItem.mediaExtension = obj["mediaExtension"].toString();
        newItem.modifiedDate = QDate::fromString(obj["modifiedDate"].toString());
        newItem.name = obj["name"].toString();
        newItem.nameNoExtension = obj["nameNoExtension"].toString();
        newItem.script = obj["script"].toString();
        newItem.pathNoExtension = obj["scriptNoExtension"].toString();
        newItem.thumbFile = obj["thumbFile"].toString();
        newItem.type = (LibraryListItemType)obj["type"].toInt();
        newItem.zipFile = obj["zipFile"].toString();
        return newItem;
    }

    static void copyProperties(const LibraryListItem27 from, LibraryListItem27 &to) {
        to.copyProperties(from);
        // to.path = from.path;
        // to.duration = from.duration;
        // to.md5 = from.md5;
        // to.mediaExtension = from.mediaExtension;
        // to.modifiedDate = from.modifiedDate;
        // to.name = from.name;
        // to.nameNoExtension = from.nameNoExtension;
        // to.script = from.script;
        // to.pathNoExtension = from.pathNoExtension;
        // to.thumbFile = from.thumbFile;
        // to.type = from.type;
        // to.zipFile = from.zipFile;
        // //Live
        // to.ID = from.ID;
        // to.libraryPath = from.libraryPath;
        // to.hasScript = from.hasScript;
        // to.thumbState = from.thumbState;
        // to.thumbFileExists = from.thumbFileExists;
        // to.managedThumb = from.managedThumb;
        // to.forceProcessMetadata = from.forceProcessMetadata;
        // to.metadata = LibraryListItemMetaData258(from.metadata);
    }
//    //waiting ? "://images/icons/loading.png" : "://images/icons/loading_current.png"
};
Q_DECLARE_METATYPE(LibraryListItem27);
#endif // LIBRARYLISTITEM27_H
