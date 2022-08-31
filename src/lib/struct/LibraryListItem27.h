#ifndef LIBRARYLISTITEM27_H
#define LIBRARYLISTITEM27_H
#include <QString>
#include <QMetaType>
#include <QDate>
#include <QVariant>
#include <QDataStream>
#include <QJsonObject>
#include "../lookup/Constants.h"
#include "XTEngine_global.h"

enum class LibraryListItemType {
    PlaylistInternal,
    Video,
    Audio,
    FunscriptType,
    VR
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
    LibraryListItem27() {}
    LibraryListItem27(const LibraryListItem27* item)
    {
        type = item->type;
        path = item->path;
        name = item->name;
        nameNoExtension = item->nameNoExtension;
        script = item->script;
        scriptNoExtension = item->scriptNoExtension;
        mediaExtension = item->mediaExtension;
        thumbFile = item->thumbFile;
        zipFile = item->zipFile;
        modifiedDate = item->modifiedDate;
        duration = item->duration;
        ID = item->ID;
        libraryPath = item->libraryPath;
        isMFS = item->isMFS;
        toolTip = item->toolTip;
        thumbState = item->thumbState;
        thumbFileExists = item->thumbFileExists;
    }
    LibraryListItemType type;
    QString path;
    QString name;
    QString nameNoExtension;
    QString script;
    QString scriptNoExtension;
    QString mediaExtension;
    QString thumbFile;
    QString zipFile;
    QDate modifiedDate;
    quint64 duration;

    // Live members
    QString ID;
    QString libraryPath;
    bool isMFS;
    QString toolTip;
    ThumbState thumbState = ThumbState::Waiting;
    bool thumbFileExists = false;
    QString thumbFileLoading = LibraryThumbNail::LOADING_IMAGE;// "://images/icons/loading.png";
    QString thumbFileLoadingCurrent = LibraryThumbNail::LOADING_CURRENT_IMAGE;// "://images/icons/loading_current.png";
    QString thumbFileError = LibraryThumbNail::ERROR_IMAGE;// "://images/icons/error.png";

    friend QDataStream & operator<<( QDataStream &dataStream, const LibraryListItem27 &object )
    {
        dataStream << object.ID;
        dataStream << (int)object.type;
        dataStream << object.path;
        dataStream << object.name;
        dataStream << object.nameNoExtension;
        dataStream << object.script;
        dataStream << object.scriptNoExtension;
        dataStream << object.mediaExtension;
        dataStream << object.thumbFile;
        dataStream << object.zipFile;
        dataStream << object.modifiedDate;
        dataStream << object.duration;
        dataStream << object.isMFS;
        dataStream << object.libraryPath;
        dataStream << object.toolTip;
        dataStream << object.thumbFileExists;
        dataStream << (int)object.thumbState;
        dataStream << object.thumbFileLoading;
        dataStream << object.thumbFileLoadingCurrent;
        dataStream << object.thumbFileError;
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
        dataStream >> object.scriptNoExtension;
        dataStream >> object.mediaExtension;
        dataStream >> object.thumbFile;
        dataStream >> object.zipFile;
        dataStream >> object.modifiedDate;
        dataStream >> object.duration;
        dataStream >> object.isMFS;
        dataStream >> object.libraryPath;
        dataStream >> object.toolTip;
        dataStream >> object.thumbFileExists;
        dataStream >> object.thumbState;
        dataStream >> object.thumbFileLoading;
        dataStream >> object.thumbFileLoadingCurrent;
        dataStream >> object.thumbFileError;
        return dataStream;
    }

    friend bool operator==(const LibraryListItem27 &p1, const LibraryListItem27 &p2)
    {
       return p1.ID == p2.ID;
    }

    static LibraryListItem27 fromVariant(QVariant item)
    {
        QJsonObject obj = item.toJsonObject();
        return fromJson(obj);
    }

    static QVariant toVariant(LibraryListItem27 item)
    {
        QJsonObject obj;
        //obj["id"] = item.ID;
        obj["path"] = item.path;
        obj["duration"] = QString::number(item.duration);
        obj["mediaExtension"] = item.mediaExtension;;
        obj["modifiedDate"] = item.modifiedDate.toString();
        obj["name"] = item.name;
        obj["nameNoExtension"] = item.nameNoExtension;
        obj["script"] = item.script;
        obj["scriptNoExtension"] = item.scriptNoExtension;
        obj["thumbFile"] = item.thumbFile;
        obj["type"] = (int)item.type;
        obj["zipFile"] = item.zipFile;
//        obj["isMFS"] = item.isMFS;
//        obj["tooltip"] = item.toolTip;
        return QVariant::fromValue(obj);
     }

    static LibraryListItem27 fromJson(QJsonObject obj) {
        LibraryListItem27 newItem;
        //newItem.ID = obj["id"].toInt();
        newItem.path = obj["path"].toString();
        newItem.duration = obj["path"].toString().toLongLong();
        newItem.mediaExtension = obj["mediaExtension"].toString();
        newItem.modifiedDate = QDate::fromString(obj["modifiedDate"].toString());
        newItem.name = obj["name"].toString();
        newItem.nameNoExtension = obj["nameNoExtension"].toString();
        newItem.script = obj["script"].toString();
        newItem.scriptNoExtension = obj["scriptNoExtension"].toString();
        newItem.thumbFile = obj["thumbFile"].toString();
        newItem.type = (LibraryListItemType)obj["type"].toInt();
        newItem.zipFile = obj["zipFile"].toString();
        return newItem;
    }
//    //waiting ? "://images/icons/loading.png" : "://images/icons/loading_current.png"
};
Q_DECLARE_METATYPE(LibraryListItem27);
#endif // LIBRARYLISTITEM27_H
