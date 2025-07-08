
#ifndef SCRIPTINFO_H
#define SCRIPTINFO_H

#include <QString>
#include <QMetaType>
#include <QJsonObject>
#include <QDataStream>

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
    ScriptType type;
    ScriptContainerType containerType;
    QString containerTypeName;

    friend bool operator==(const ScriptInfo &object1 , const ScriptInfo &object2 )
    {
        return object1.containerType == object2.containerType
            && object1.type == object2.type
            && object1.name == object2.name
            && object1.path == object2.path
            && object1.track == object2.track;
    }
    friend bool operator!=(const ScriptInfo &object1 , const ScriptInfo &object2 )
    {
        return object1.containerType != object2.containerType
               || object1.type != object2.type
               || object1.name != object2.name
               || object1.path != object2.path
               || object1.track != object2.track;
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
        return dataStream;
    }

    friend QDataStream & operator>>(QDataStream &dataStream, ScriptInfo &object)
    {
        dataStream >> object.name;
        dataStream >> object.filename;
        dataStream >> object.path;
        dataStream >> object.track;
        dataStream >> (qint8&)object.type;
        dataStream >> (qint8&)object.containerType;
        dataStream >> object.containerTypeName;
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
        return newItem;
    }
};

Q_DECLARE_METATYPE(ScriptInfo);
#endif // SCRIPTINFO_H
