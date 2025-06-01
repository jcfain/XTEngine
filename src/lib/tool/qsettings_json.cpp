#include "qsettings_json.h"

const QSettings::Format JSONSettingsFormatter::JsonFormat = QSettings::registerFormat("json", &readSettingsJson, &writeSettingsJson);

// -------------------- Writing -------------------------
bool JSONSettingsFormatter::writeSettingsJson(QIODevice &device, const QSettings::SettingsMap &map)
{
    QVariantMap vmap;
    foreach (auto item, map.keys())
    {
        vmap.insert(item, map[item]);
    }

    QJsonDocument json = QJsonDocument::fromVariant(vmap);
    device.write(json.toJson());
    return true;
}

// -------------------- Reading -------------------------
bool JSONSettingsFormatter::readSettingsJson(QIODevice &device, QSettings::SettingsMap &map)
{

    QJsonParseError jsonError;
    QByteArray jsonString = device.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonString, &jsonError);

    if(jsonError.error != QJsonParseError::NoError)
        return false;


    if (jsonDoc.isObject())
    {
        QJsonObject jsonObject = jsonDoc.object();
        parseJsonObject(jsonObject, map);
    }
    else
    {
        QJsonArray jsonArray = jsonDoc.array();
        parseJsonArray(jsonArray, map);
    }
    return true;
}

bool JSONSettingsFormatter::parseJsonObject(QJsonObject &jsonObject, QSettings::SettingsMap &map)
{
    QStringList keys = jsonObject.keys();
    if (! keys.isEmpty())
    {
        for(int i=0; i<keys.size(); i++)
        {
            QString currentKey = keys[i];
            QJsonValue currentValue = jsonObject.value(currentKey);
            parseJsonValue(currentKey, currentValue, map);
        }
        return true;
    } else
        // empty object
        return false;
}

bool JSONSettingsFormatter::parseJsonArray(const QJsonArray &jsonArray, QSettings::SettingsMap &map){
    int arr_ctr=0;
    if (! jsonArray.isEmpty())
    {

        for (const QJsonValue &arrayItem: jsonArray)
        {
            QString arrItemkey = QString("#%1").arg(arr_ctr, /*fieldWidth=*/ 5, /*base=*/ 10, /*fillChar=*/ QChar('0'));
            if (parseJsonValue(arrItemkey, arrayItem, map))
                arr_ctr++;
        }
        return true;
    } else
        // empty array
        return false;
};

bool JSONSettingsFormatter::parseJsonValue(const QString &jsonKey, const QJsonValue &jsonValue, QSettings::SettingsMap &map)
{
    if(jsonValue.isObject())
    {
        QJsonObject jsonObj = jsonValue.toObject();
        map.insert(jsonKey, jsonObj.toVariantMap());
    }
    else if(jsonValue.isArray())
    {
        QJsonArray jsonArray = jsonValue.toArray();
        map.insert(jsonKey, jsonArray.toVariantList());
    }
    else if(jsonValue.isUndefined())
    {
        map.insert(jsonKey, QVariant(QVariant::Invalid));
    }
    else
    {
        map.insert(jsonKey, jsonValue.toVariant());
    }
    return true;
}
