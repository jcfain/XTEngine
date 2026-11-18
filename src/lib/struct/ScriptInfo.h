
#ifndef SCRIPTINFO_H
#define SCRIPTINFO_H

#include <QString>
#include <QMetaType>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDataStream>
#include "Funscript.h"

enum class ScriptType {
    MAIN,
    ALTERNATE
};

enum class ScriptContainerType {
    BASE,
    MFS,
    ZIP,
    SFMA
};

struct ScriptInfo {
    ///
    /// \brief name User friendly name
    ///
    QString name;
    ///
    /// \brief filename Base file name without the path no extension
    ///
    QString filename;
    ///
    /// \brief path Full path to file
    ///
    QString path;
    ///
    /// \brief track Mfs track is exists. Empty otherwise
    ///
    QString track;
    ScriptType type = ScriptType::MAIN;
    ScriptContainerType containerType = ScriptContainerType::BASE;
    QString containerTypeName;
    ///
    /// \brief version Funscript version from the script file. Empty if not present
    ///
    QString version;
    ///
    /// \brief inverted True if the script is marked as inverted
    ///
    bool inverted = false;
    ///
    /// \brief hasInverted True if the inverted property exists in the script file
    ///
    bool hasInverted = false;
    ///
    /// \brief metadata Funscript metadata from the script file
    ///
    FunscriptMetadata metadata;
    ///
    /// \brief hasMetadata True if the metadata object exists in the script file
    ///
    bool hasMetadata = false;

    friend bool operator==(const ScriptInfo &object1 , const ScriptInfo &object2 )
    {
        return object1.containerType == object2.containerType
            && object1.type == object2.type
            && object1.name == object2.name
            && object1.path == object2.path
            && object1.track == object2.track
            && object1.version == object2.version
            && object1.inverted == object2.inverted
            && object1.hasInverted == object2.hasInverted
            && object1.metadata == object2.metadata
            && object1.hasMetadata == object2.hasMetadata;
    }
    friend bool operator!=(const ScriptInfo &object1 , const ScriptInfo &object2 )
    {
        return object1.containerType != object2.containerType
               || object1.type != object2.type
               || object1.name != object2.name
               || object1.path != object2.path
               || object1.track != object2.track
               || object1.version != object2.version
               || object1.inverted != object2.inverted
               || object1.hasInverted != object2.hasInverted
               || object1.metadata != object2.metadata
               || object1.hasMetadata != object2.hasMetadata;
    }

    friend QDataStream & operator<<(QDataStream &dataStream, const ScriptInfo &object )
    {
        dataStream << object.name;
        dataStream << object.filename;
        dataStream << object.path;
        dataStream << object.track;
        dataStream << (qint8&)object.type;
        dataStream << (qint8&)object.containerType;
        dataStream << object.containerTypeName;
        dataStream << object.version;
        dataStream << object.inverted;
        dataStream << object.hasInverted;
        dataStream << QJsonDocument(object.metadata.toJson()).toJson(QJsonDocument::Compact);
        dataStream << object.hasMetadata;
        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, ScriptInfo &object)
    {
        dataStream >> object.name;
        dataStream >> object.filename;
        dataStream >> object.path;
        dataStream >> object.track;
        // Read the enums through a qint8 and assign: the stream carries one
        // byte, and writing it straight into the (4 byte) enum would leave
        // the remaining bytes uninitialized.
        qint8 type = 0;
        dataStream >> type;
        object.type = (ScriptType)type;
        qint8 containerType = 0;
        dataStream >> containerType;
        object.containerType = (ScriptContainerType)containerType;
        dataStream >> object.containerTypeName;
        dataStream >> object.version;
        dataStream >> object.inverted;
        dataStream >> object.hasInverted;
        QByteArray metadataJson;
        dataStream >> metadataJson;
        object.metadata = FunscriptMetadata::fromJson(QJsonDocument::fromJson(metadataJson).object());
        dataStream >> object.hasMetadata;
        return dataStream;
    }

    static QJsonObject toJson(ScriptInfo item)
    {
        QJsonObject obj;
        obj["name"] = item.name;
        obj["filename"] = item.filename;
        obj["path"] = item.path;
        obj["track"] = item.track;
        obj["type"] = (qint8)item.type;
        obj["containerType"] = (qint8)item.containerType;
        obj["containerTypeName"] = item.containerTypeName;
        obj["version"] = item.version;
        obj["inverted"] = item.inverted;
        obj["hasInverted"] = item.hasInverted;
        obj["metadata"] = item.metadata.toJson();
        obj["hasMetadata"] = item.hasMetadata;
        return obj;
    }

    static ScriptInfo fromJson(QJsonObject obj)
    {
        ScriptInfo newItem;
        newItem.name = obj["name"].toString();
        newItem.filename = obj["filename"].toString();
        newItem.path = obj["path"].toString();
        newItem.track = obj["track"].toString();
        newItem.type = (ScriptType)obj["type"].toInt();

        newItem.containerType = (ScriptContainerType)obj["containerType"].toInt();
        QString typeName = "";
        if(newItem.containerType == ScriptContainerType::MFS)
        {
            typeName = "("+newItem.track+")";
        }
        else if(newItem.containerType == ScriptContainerType::ZIP)
        {
            typeName = "(ZIP)";
        }
        newItem.containerTypeName = typeName;
        newItem.version = obj["version"].toString();
        newItem.inverted = obj["inverted"].toBool();
        newItem.hasInverted = obj["hasInverted"].toBool();
        newItem.metadata = FunscriptMetadata::fromJson(obj["metadata"].toObject());
        newItem.hasMetadata = obj["hasMetadata"].toBool();
        return newItem;
    }
};

Q_DECLARE_METATYPE(ScriptInfo)
#endif // SCRIPTINFO_H
