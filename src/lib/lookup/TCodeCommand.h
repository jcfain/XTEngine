// #ifndef TCODECOMMAND_H
// #define TCODECOMMAND_H

// #include <QString>
// #include <QList>
// #include <QVariant>
// #include <QJsonObject>

// enum class TCodeCommandType {
//     BUTTON
// };

// struct TCodeCommand {
//     int id;
//     TCodeCommandType type;
//     QString command;
//     double value;

//     static TCodeCommand fromVariant(QVariant item)
//     {
//         return fromJson(item.toJsonObject());
//     }

//     static TCodeCommand fromJson(QJsonObject obj)
//     {
//         TCodeCommand newItem;
//         newItem.id = obj["id"].toInt();
//         newItem.type = (TCodeCommandType)obj["type"].toInt();
//         newItem.command = obj["command"].toString();
//         newItem.value = obj["value"].toDouble();
//         return newItem;
//     }

//     static QVariant toVariant(TCodeCommand item)
//     {
//         return QVariant::fromValue(toJson(item));
//     }

//     static QJsonObject toJson(TCodeCommand item)
//     {
//         QJsonObject obj;
//         obj["id"] = item.id;
//         obj["type"] = (int)item.type;
//         obj["command"] = item.command;
//         obj["value"] = item.value;
//         return obj;
//     }
// };

// #endif // TCODECOMMAND_H
