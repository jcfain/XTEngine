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
