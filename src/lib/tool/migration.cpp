#include "migration.h"
#include "../handler/loghandler.h"
#include "../lookup/tcodechannellookup.h"

void  Migration::MigrateTo42(QSettings* settingsToLoadFrom) {
    QJsonObject availableChannelJson = settingsToLoadFrom->value("availableChannels").toJsonObject();
    TCodeChannelLookup::clearChannelProfiles();
    foreach(auto axis, availableChannelJson.keys())
    {
        TCodeChannelLookup::addChannel(axis, ChannelModel33::fromVariant(availableChannelJson.value(axis)), "Default");
    }
}

void Migration::MigrateTo46(QSettings *settingsToLoadFrom, QHash<QString, LibraryListItemMetaData258> &libraryListItemMetaDatas)
{
    auto metaDatas = libraryListItemMetaDatas;
    auto metadataKeys = metaDatas.keys();
    libraryListItemMetaDatas.clear();
    foreach (auto path, metadataKeys) {
        QFileInfo fileInfo(path);
        if(fileInfo.exists())
        {
            auto fileName = fileInfo.fileName();
            int index = fileName.lastIndexOf(".");
            if(index > -1)
                fileName.remove(index, fileName.length());
            libraryListItemMetaDatas.insert(fileName, metaDatas.value(path));
        }
    }
}

void Migration::MigrateTo52(QSettings *settingsToLoadFrom)
{
    auto profilekeys = TCodeChannelLookup::getChannelProfiles();
    foreach(auto profile, profilekeys)
    {
        auto channels = TCodeChannelLookup::getChannels(profile);
        foreach(auto channelName, channels)
        {
            auto channel = TCodeChannelLookup::getChannel(channelName, profile);
            LogHandler::Debug("Migrate channel from name: "+ QString::number((int)channel->track));
            channel->track = TCodeChannelLookup::FromString(channelName);
            LogHandler::Debug("to name: "+ QString::number((int)channel->track));
        }
    }
}

void Migration::MigrateTo592(QSettings *settingsToLoadFrom)
{
    float settingsVersionNum = settingsToLoadFrom->value("version").toFloat();

    QString roundedString = QString::number(settingsVersionNum);
    if(roundedString.length() > 5) {
        roundedString.remove(6, roundedString.length() - 6);
        if(roundedString.endsWith("0"))
            roundedString.remove(5, 1);

    }
    roundedString += "b";
    settingsToLoadFrom->setValue("versionString", roundedString);
    // Do not sync at this point as things have not loaded.
}

void Migration::MigrateTo595(QSettings *settingsToLoadFrom, QList<TCodeCommand>& list)
{
    list.clear();
    QStringList tcodeCommands = settingsToLoadFrom->value("customTCodeCommands").toStringList();
    foreach(QString command, tcodeCommands)
    {
        list.append({command, command});
    }
    QList<QVariant> tcodeCommandVarient;
    foreach(auto command, list)
    {
        tcodeCommandVarient.append(command.toVariant());
    }
    settingsToLoadFrom->setValue("customTCodeCommands", tcodeCommandVarient);
}

void Migration::MigrateTo61(QSettings *settingsToLoadFrom, int& major, int& minor, int& rev, QString& phase)
{
    float settingsVersion = settingsToLoadFrom->value("version").toFloat();
    if(settingsVersion == 0)
        return; // This is a true first load fresh install.
    QString versionString = settingsToLoadFrom->value("versionString", "").toString();
    major = 0;
    minor = settingsVersion * 10;
    rev = settingsVersion * 1000 - (minor * 100);
    phase = versionString.isEmpty() ? "b" :// Sorry...
                versionString.contains("b") ? "b" :
                versionString.contains("a") ? "a" : "";
}

void Migration::RenameChannelDamperToSpeed(QSettings *settingsToLoadFrom)
{
    QVariantMap availableChannelVariant = settingsToLoadFrom->value("availableChannels").toMap();
    foreach(auto profile, availableChannelVariant.keys())
    {
        QVariantMap profileChannelsVariant = availableChannelVariant.value(profile).toMap();
        foreach(auto tcodeChannelName, profileChannelsVariant.keys())
        {
            QVariant modelVariant = profileChannelsVariant.value(tcodeChannelName);
            QJsonObject obj = modelVariant.toJsonObject();
            if(tcodeChannelName == "R2")
            {
                LogHandler::Debug("break");
            }
            bool speedEnabled = obj["damperEnabled"].toBool();
            bool speedRandom = obj["damperRandom"].toBool();
            double speedValue = obj["damperValue"].toDouble();
            double offset = obj["delay"].toDouble();
            ChannelModel33 model = ChannelModel33::fromVariant(modelVariant);
            model.SpeedEnabled = speedEnabled;
            model.SpeedRandom = speedRandom;
            model.SpeedValue = speedValue;
            model.Offset = offset;
            profileChannelsVariant.insert(tcodeChannelName, ChannelModel33::toVariant(model));
        }
        availableChannelVariant.insert(profile, profileChannelsVariant);
    }
    settingsToLoadFrom->setValue("availableChannels", availableChannelVariant);
    // Do not sync at this point as things have not loaded.
}
