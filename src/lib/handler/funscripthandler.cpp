#include "funscripthandler.h"

#include <qmath.h>
#include "xmediastatehandler.h"
#include "../tool/file-util.h"

FunscriptHandler::FunscriptHandler(QObject* parent) : QObject(parent) { }

FunscriptHandler::~FunscriptHandler()
{
    m_loaded = false;
}

bool FunscriptHandler::load(const QString& funscriptString)
{
    unload();
    //LogHandler::Debug("funscriptHandler->load "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
    QByteArray funData = readFile(funscriptString);
    if(funData.isEmpty())
        return false;
    return load(funData);
}

bool FunscriptHandler::load(const QByteArray& byteArray)
{
    QMutexLocker locker(&mutex);
    unload();
    QJsonObject obj = readJson(byteArray);
    if(obj.isEmpty())
        return false;

    jsonToFunscript(obj);

    m_loaded = true;
    return true;
}

bool FunscriptHandler::load(const Track &channelName, const QString &funscriptPath)
{
    QByteArray funData = readFile(funscriptPath);
    if(funData.isEmpty())
        return false;

    QJsonObject obj = readJson(funData);
    if(obj.isEmpty())
        return false;

    Funscript funscript;
    jsonToFunscript(obj, funscript);
    setFunscriptSettings(channelName, funscript);
    SettingsHandler::setFunscriptLoaded(TCodeChannelLookup::ToString(channelName), true);
    m_funscripts[channelName] = funscript;
    return true;
}

void FunscriptHandler::unload()
{
    m_loaded = false;
    SettingsHandler::clearFunscriptLoaded();
    m_funscripts.clear();
    _firstActionExecuted = false;
}

///
/// \brief FunscriptHandler::getFunscript
/// \return Stroke funscript if exists. If not it returns the first loaded funscript or null
///
const Funscript *FunscriptHandler::getFunscript()
{
    if(!isLoaded())
        return 0;
    QMutexLocker locker(&mutex);
    if(m_funscripts.contains(Track::Stroke))
        return &m_funscripts[Track::Stroke];
    auto loaded = getLoaded();
    return &m_funscripts[loaded[0]];
}

///
/// \brief FunscriptHandler::getFunscript
/// \param channelName
/// \return null if channel not found
///
const Funscript* FunscriptHandler::getFunscript(const Track& channelName)
{
    QMutexLocker locker(&mutex);
    if(!m_funscripts.contains(channelName))
        return 0;
    return &m_funscripts[channelName];
}

bool FunscriptHandler::isLoaded()
{
    QMutexLocker locker(&mutex);
    return m_loaded;
}

bool FunscriptHandler::isLoaded(const Track &channelName)
{
    return m_funscripts.contains(channelName);
}

// void FunscriptHandler::setLoaded(const Track& channelName, const bool& value)
// {
//     QMutexLocker locker(&mutex);
//     m_loaded = value;
//     if(!value && m_funscript) {
//         delete m_funscript;
//         m_funscript = 0;
//     }
//     SettingsHandler::setFunscriptLoaded(_channel, m_loaded);
// }

qint64 FunscriptHandler::getMin(const Track& channelName) const
{
    if(!m_funscripts.contains(channelName))
        return 0;
    return m_funscripts.value(channelName).settings.min;
}

qint64 FunscriptHandler::getMax(const Track& channelName) const
{
    if(!m_funscripts.contains(channelName))
        return 0;
    return m_funscripts.value(channelName).settings.max;
}

qint64 FunscriptHandler::getNext(const Track& channelName) const
{
    if(!m_funscripts.contains(channelName))
        return 0;
    const Funscript* funscript = &m_funscripts[channelName];
    auto atList = funscript->settings.atList;
    if(funscript->settings.lastActionIndex == funscript->settings.nextActionIndex) {
        int nextAction = funscript->settings.lastActionIndex + 1;
        if(nextAction >= atList.length())
            return getMin(channelName);
        return atList[nextAction];
    }
    if(funscript->settings.nextActionIndex > -1) {
        return atList[funscript->settings.nextActionIndex];
    }
    return getMin(channelName);
}

void FunscriptHandler::jsonToFunscript(QJsonObject json)
{
    m_funscripts.clear();
    SettingsHandler::clearFunscriptLoaded();
    if (json.contains(m_sfmaJSONObjectName) && json[m_sfmaJSONObjectName].isObject())
    {
        auto jsonTracks = json[m_sfmaJSONObjectName].toObject();
        auto channels = TCodeChannelLookup::getChannels();
        foreach(QString channelName, channels)
        {
            ChannelModel33* channel = TCodeChannelLookup::getChannel(channelName);
            if(channel->Type == ChannelType::HalfOscillate)
                continue;
            QString trackName = channel->trackName;
            if(channel->track == Track::Stroke)// Modify for optional stroke in tracks object
                trackName = "stroke";
            if(jsonTracks.contains(trackName))
            {
                Funscript funscript;
                jsonToFunscript(jsonTracks[trackName].toObject(), funscript);
                setFunscriptSettings(channel->track, funscript);
                m_funscripts.insert(channel->track, funscript);
                SettingsHandler::setFunscriptLoaded(channelName, true);
            }
        }
    }
    if(!m_funscripts.contains(Track::Stroke))
    {
        Funscript funscript;
        jsonToFunscript(json, funscript);
        setFunscriptSettings(Track::Stroke, funscript);
        SettingsHandler::setFunscriptLoaded(TCodeChannelLookup::Stroke(), true);
        m_funscripts[Track::Stroke] = funscript;
    }
}

void FunscriptHandler::jsonToFunscript(const QJsonObject &json, Funscript &funscript)
{
    // if (json.contains("range"))
    //     funscript.range = json["range"].toInt();
    if (json.contains("version") && json["version"].isString())
        funscript.version = json["version"].toString();
    if (json.contains("inverted") && json["inverted"].isBool())
    {
        funscript.inverted = json["inverted"].toBool();
    }
    jsonToFunscript(json, funscript.metadata);
    jsonToFunscript(json, funscript.actions);
}

void FunscriptHandler::jsonToFunscript(const QJsonObject& json, QHash<qint64, int>& actions)
{
    actions.clear();
    if (json.contains("actions") && json["actions"].isArray())
    {
        QJsonArray actionArray = json["actions"].toArray();
        foreach(QJsonValue value, actionArray)
        {
            QJsonObject obj = value.toObject();
            if (obj.contains("at") && obj.contains("pos"))
            {
                actions[(qint64)obj["at"].toDouble()] = obj["pos"].toInt();
            }
        }
    }
}

void FunscriptHandler::jsonToFunscript(const QJsonObject& json, FunscriptMetadata& metadata)
{
    if(json.contains("metadata")) {
        QJsonObject metaDataJson = json["metadata"].toObject();

        if (metaDataJson.contains("creator") && metaDataJson["creator"].isString())
            metadata.creator = metaDataJson["creator"].toString();
        if (metaDataJson.contains("original_name") && metaDataJson["original_name"].isString())
            metadata.original_name = metaDataJson["original_name"].toString();
        if (metaDataJson.contains("url") && metaDataJson["url"].isString())
            metadata.url = metaDataJson["url"].toString();
        if (metaDataJson.contains("url_video") && metaDataJson["url_video"].isString())
            metadata.url_video = metaDataJson["url_video"].toString();
        if (metaDataJson.contains("tags") && metaDataJson["tags"].isArray())
        {
            QJsonArray tags = metaDataJson["tags"].toArray();;
            foreach(QJsonValue tag, tags)
                metadata.tags.append(tag.toString());
        }
        if (metaDataJson.contains("performers") && metaDataJson["performers"].isArray())
        {
            QJsonArray performers = metaDataJson["performers"].toArray();;
            foreach(QJsonValue performer, performers)
                metadata.performers.append(performer.toString());
        }
        if (metaDataJson.contains("bookmarks") && metaDataJson["bookmarks"].isArray())
        {
            QJsonArray bookmarks = metaDataJson["bookmarks"].toArray();;
            foreach(QJsonValue bookmark, bookmarks) {
                qint64 milliseconds = QTime::fromString(bookmark.toObject()["time"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
                metadata.bookmarks.append({bookmark.toObject()["name"].toString(), milliseconds});
            }
        }
        if (metaDataJson.contains("chapters") && metaDataJson["chapters"].isArray())
        {
            QJsonArray chapters = metaDataJson["chapters"].toArray();;
            foreach(QJsonValue chapter, chapters) {
                qint64 startTime = QTime::fromString(chapter.toObject()["startTime"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
                qint64 endTime = QTime::fromString(chapter.toObject()["endTime"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
                metadata.chapters.append({chapter.toObject()["name"].toString(), startTime, endTime});
            }
        }
        if (metaDataJson.contains("paid") && metaDataJson["paid"].isBool())
            metadata.paid = metaDataJson["paid"].toBool();
        if (metaDataJson.contains("comment") && metaDataJson["comment"].isString())
            metadata.comment = metaDataJson["comment"].toString();
        if (metaDataJson.contains("original_total_duration_ms"))
            metadata.original_total_duration_ms = metaDataJson["original_total_duration_ms"].toString().toLongLong();
    }
}

void FunscriptHandler::setFunscriptSettings(const Track& channelName, Funscript &funscript)
{
    funscript.settings.channel = channelName;
    funscript.settings.trackName = TCodeChannelLookup::ToString(channelName);
    funscript.settings.lastActionIndex = -1;
    funscript.settings.nextActionIndex = 1;
    funscript.settings.atList = funscript.actions.keys();
    std::sort(funscript.settings.atList.begin(), funscript.settings.atList.end());
    funscript.settings.max = funscript.settings.atList.length() > 0 ? funscript.settings.atList.last() : -1;
    funscript.settings.min = 0;
    foreach(qint64 value, funscript.settings.atList)
    {
        if(value > 0)
        {
            funscript.settings.min = value;
            return;
        }
    }
}

bool FunscriptHandler::exists(const QString& path)
{
    return QFileInfo::exists(path);
}

const QList<Track> FunscriptHandler::getLoaded()
{
    return m_funscripts.keys();
}

int lastOffset = -1;
///
/// \brief FunscriptHandler::getPosition
/// This method finds the action to execute to move to the next point in the script.
/// This point is in the future so wehn the previous action has finished executing
/// over the interval of time, the next action will start
/// \param channelName the track you need to fine the position via the media time
/// \param at The current media time
/// \param offset Shift the time before or after the actual time.
/// negative values is before so the current time is 100 and you shift -10 then the time will be 90
/// \return nullptr if no action is ready to be executed
///
std::shared_ptr<FunscriptAction> FunscriptHandler::getPosition(const Track& channelName, const qint64& at, const int offset)
{
    QMutexLocker locker(&mutex);
    int offsetLocal =  offset ?: getOffSet();
    if(lastOffset != offsetLocal) {
        lastOffset = offsetLocal;
        LogHandler::Debug("FunscriptHandler::getPosition offset changed: "+ QString::number(offsetLocal));
    }
    qint64 millis = at + offsetLocal;
    if(!m_funscripts.contains(channelName))
        return nullptr;
    Funscript* funscript = &m_funscripts[channelName];
    auto atList = funscript->settings.atList;
    qint64 closestMillis = findClosest(millis, atList);
    if(closestMillis == -1)
        return nullptr;
    qint64 closestIndex = atList.indexOf(closestMillis);
    qint64 nextActionIndex = closestIndex + 1;
    // We are still moving to the next funcript point.
    if(funscript->settings.lastActionIndex == nextActionIndex)
        return nullptr;
    // We are at the end of the funscript points.
    if(destinationEnd(funscript, nextActionIndex))
        return nullptr;
    //LogHandler::Debug("millis: "+ QString::number(millis));
    //LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
    //LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
    //LogHandler::Debug("nextAction: "+ QString::number(nextAction));
    //LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
//    LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
    if (millis >= closestMillis || funscript->settings.lastActionIndex == -1)
    {
        // if(nextAction != funscript->settings.nextActionIndex)
        // {
        //     LogHandler::Warn("Potential action skip nextActionIndex: "+ QString::number(funscript->settings.nextActionIndex) + ", closestIndex: "+ QString::number(closestIndex));
        //     LogHandler::Warn("at: " + QString::number(closestMillis) + ", pos: "+QString::number(funscript->actions.value(closestMillis)));
        // }
        qint64 currentDestinationIndex = nextActionIndex;
        qint64 nextDestinationIndex = destinationEnd(funscript, currentDestinationIndex + 1) ? -1 : currentDestinationIndex + 1;
        // qint64 nextMillis = nextIndex > -1 ? atList[nextIndex] : -1;
        qint64 currentDestinationMillis = funscript->settings.lastActionIndex == -1 ? closestMillis : atList[currentDestinationIndex];
        int currentDestinationInterval = currentDestinationMillis == closestMillis ? closestMillis : currentDestinationMillis - closestMillis;
        int currentDestinationPos = funscript->actions.value(currentDestinationMillis);
        if(!_firstActionExecuted)
        {
            _firstActionExecuted = true;
            if(currentDestinationInterval < 500)
                currentDestinationInterval = 500;
            else if(currentDestinationInterval > 5000)
                currentDestinationInterval = 5000;
        }
        //LogHandler::Debug("offSet: "+ QString::number(SettingsHandler::getoffSet()));
        //LogHandler::Debug("speed: "+ QString::number(speed));
        //LogHandler::Debug("millis: "+ QString::number(millis));
//        LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
//        LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
//        LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
//        LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
        //LogHandler::Debug("nextActionPos: "+ QString::number(funscript->actions.value(nextMillis)));
        if(funscript->settings.lastActionIndex > -1 && funscript->settings.modifier != 1)
        {
            if(currentDestinationPos == funscript->settings.lastActionPos && funscript->settings.lastActionPosModified)
            {
                currentDestinationPos = funscript->settings.lastActionPosModified;
                if(funscript->settings.channel == Track::Stroke)
                    LogHandler::Debug(funscript->settings.trackName + " Modified pos unchanged: "+QString::number(currentDestinationPos));
            }
            else
            {
                int ogPos = currentDestinationPos;
                double distance = currentDestinationPos - funscript->settings.lastActionPos;
                double amplitude = distance / 2.0;
                if(funscript->settings.modifier > 1)
                {
                    currentDestinationPos = qRound(currentDestinationPos + (amplitude * (funscript->settings.modifier - 1)));
                }
                else if(funscript->settings.modifier > 0)
                {
                    currentDestinationPos = qRound(currentDestinationPos - (amplitude * (1 - funscript->settings.modifier)));
                }
                // double modifier = funscript->settings.modifier;
                // if(modifier > 1 )
                // {
                //     modifier = funscript->settings.modifier - 1;
                // }
                // bool downstroke = distance < 0;
                // if(downstroke)
                // {
                //     pos = qRound(pos + (modifier * 100));// Add % to the bottom end (current position)
                // }
                // else
                // {
                //     pos = qRound(pos - (modifier * 100));// Subtract % from the top end (current position)
                // }
                currentDestinationPos = XMath::constrain(currentDestinationPos, 0, 100);
                funscript->settings.lastActionPosModified = currentDestinationPos;
                if(funscript->settings.channel == Track::Stroke)
                {
                    LogHandler::Debug(funscript->settings.trackName + " Modifier: "+QString::number(funscript->settings.modifier));
                    LogHandler::Debug(funscript->settings.trackName + " last pos: "+QString::number(funscript->settings.lastActionPos));
                    LogHandler::Debug(funscript->settings.trackName + " pos: "+QString::number(ogPos));
                    LogHandler::Debug(funscript->settings.trackName + " distance: "+QString::number(distance));
                    // LogHandler::Debug(funscript->settings.trackName + " amplitude: "+QString::number(amplitude));
                    LogHandler::Debug(funscript->settings.trackName + " Modified pos: "+QString::number(currentDestinationPos));
                }
            }
        }
        calculateSpeedModifier(currentDestinationInterval);

        if(nextDestinationIndex > -1)
        {
            qint64 nextActionMillis = atList[nextDestinationIndex];
            funscript->settings.nextActionPos = funscript->actions.value(nextActionMillis);
            funscript->settings.nextActionInterval = nextActionMillis - currentDestinationMillis;
            calculateSpeedModifier(funscript->settings.nextActionInterval);

            FunscriptActionSequence seq;
            actionSequence(funscript, nextDestinationIndex, seq);
            funscript->settings.nextActionGradient =
                calculateGradient(seq.posA, seq.posB, seq.posC, seq.atA, seq.atC);
        }
        else
        {
            funscript->settings.nextActionPos = -1;
            funscript->settings.nextActionInterval = -1;
            funscript->settings.nextActionGradient = 0;
        }

        // Calculate the gradient
        bool isTCode4 = true;
        int currentDestinationGradient = 0;
        if(isTCode4)// Not used as im not sure its nessesary.
        {
            FunscriptActionSequence seq;
            actionSequence(funscript, currentDestinationIndex, seq);
            currentDestinationGradient = calculateGradient(seq.posA, seq.posB, seq.posC, seq.atA, seq.atC);
        }


        std::shared_ptr<FunscriptAction> nextAction(new FunscriptAction {
            funscript->settings.trackName, // channel
            currentDestinationMillis, // at
            currentDestinationPos, // pos
            currentDestinationInterval, // speed
            currentDestinationGradient, // gradient
            funscript->settings.lastActionPos, // lastPos
            funscript->settings.lastActionInterval, // lastSpeed
            funscript->settings.nextActionPos, // nextPos
            funscript->settings.nextActionInterval, // nextSpeed
            funscript->settings.lastActionGradient, // lastGradient
            funscript->settings.nextActionGradient, // nextGradient
            currentDestinationIndex // index
        });
        //LogHandler::Debug("nextAction.speed: "+ QString::number(nextAction->speed));
        funscript->settings.lastActionIndex = currentDestinationIndex;
        funscript->settings.lastActionPos = funscript->actions.value(currentDestinationMillis);
        funscript->settings.lastActionInterval = currentDestinationInterval;
        funscript->settings.lastActionGradient = currentDestinationGradient;
        funscript->settings.nextActionIndex = nextDestinationIndex;

        return nextAction;
    }
    return nullptr;
}


QByteArray FunscriptHandler::readFile(QString file)
{
    if (file.isEmpty())
    {
        LogHandler::Error("Loading funscript failed: Empty string" );
        return QByteArray();
    }
    LogHandler::Debug("Funscript load: "+file);
    QFile loadFile(file);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        LogHandler::Warn("Loading funscript failed: Couldn't open funscript file.");
        return QByteArray();
    }

    //LogHandler::Debug("funscriptHandler->load "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
    return loadFile.readAll();
}

QJsonObject FunscriptHandler::readJson(QByteArray data)
{
    if (data.isEmpty())
    {
        LogHandler::Error("Loading funscript failed: Empty byte array" );
        return QJsonObject();
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if(doc.isNull()) {
        LogHandler::Error("Loading funscript JSON failed: " + error.errorString());
        return QJsonObject();
    }
    return doc.object();
}

qint64 FunscriptHandler::findClosest(const qint64& value, const QList<qint64>& a)
{

    if(a.length() == 0)
        return -1;
    if(value < a[0])
    {
        return a[0];
    }
    if(value > a[a.length()-1])
    {
        return a[a.length()-1];
    }

    int lo = 0;
    int hi = a.length() - 1;

    while (lo <= hi)
    {
        int mid = (hi + lo) / 2;

        if (value < a[mid]) {
            hi = mid - 1;
        } else if (value > a[mid]) {
            lo = mid + 1;
        } else {
            return a[mid];
        }
    }
    // lo == hi + 1
    return (a[lo] - value) < (value - a[hi]) ? a[lo] : a[hi];
}

void FunscriptHandler::calculateSpeedModifier(int& interval)
{
    qreal speedmodifier = XMediaStateHandler::getPlaybackSpeed();
    if(speedmodifier > 0 && speedmodifier != 1.0 && speedmodifier < 2.0)
    {
        LogHandler::Debug("interval before: "+ QString::number(interval));
        interval = round(speedmodifier < 1.0 ? interval / speedmodifier :
                             interval - ((interval * speedmodifier) - interval));
        LogHandler::Debug("interval after: "+ QString::number(interval));
    }
}

///
/// \brief FunscriptHandler::actionSequence Gets the previous, current and next pos and at
/// \param funscript
/// \param destinationIndex
/// \param future
///
void FunscriptHandler::actionSequence(const Funscript* funscript, const int &destinationIndex, FunscriptActionSequence& future)
{
    future.posA = -1;
    future.atA = -1;
    future.posB = -1;
    future.atB = -1;
    future.posC = -1;
    auto atList = funscript->settings.atList;
    auto actionsList = funscript->actions;
    int atAIndex = destinationIndex - 1;
    if(atAIndex > -1)
    {
        if(destinationEnd(funscript, atAIndex))
            return;
        future.atA = atList.value(atAIndex);
        future.posA = actionsList.value(future.atA);
    }
    int atBIndex = atAIndex + 1;
    if(atBIndex > -1) // destinationIndex could be less than -1? Just in case...
    {
        if(destinationEnd(funscript, atBIndex))
            return;
        future.atB = atList.value(atBIndex);
        future.posB = actionsList.value(future.atB);
    }

    int atCIndex = atBIndex + 1;
    if(atCIndex > -1)
    {
        if(destinationEnd(funscript, atCIndex))
            return;
        future.atC = atList.value(atCIndex);
        future.posC = actionsList.value(future.atC);
    }
}

///
/// \brief FunscriptHandler::destinationEnd Checks if the index extends beyong the action list.
/// \param index
/// \return
///
bool FunscriptHandler::destinationEnd(const Funscript* funscript, const int& destinationIndex)
{
    return destinationIndex >= funscript->settings.atList.length();
}

int FunscriptHandler::calculateGradient(int posA, int posB, int posC, qint64 atA, qint64 atC)
{
    if((posB-posA)*(posC-posB) < 0)
        return 0;
    // Guard aginst dividing by 0
    double differencePos = (double)posC - (double)posA;
    if(!differencePos)
        return 0;
    if(atC == -1 || atA == -1)// -1 means we are at the end or begining
        return 0;
    double differenceAt = ((double)atC - (double)atA)/1000;
    if(!differenceAt)
        return 0;
    return (10*differencePos)/differenceAt;
}

//static

void FunscriptHandler::setInverted(const bool& value)
{
    QMutexLocker locker(&mutex);
    auto loaded = getLoaded();
    foreach (auto name, loaded) {
        m_funscripts[name].inverted = value;
    }
}

bool FunscriptHandler::getInverted(const Track& channelName)
{
    QMutexLocker locker(&mutex);
    return m_funscripts.contains(channelName) ? m_funscripts.value(channelName).inverted : false;
}

void FunscriptHandler::setInverted(const Track& channelName, const bool& value)
{
    QMutexLocker locker(&mutex);
    if(m_funscripts.contains(channelName)) {
        m_funscripts[channelName].inverted = value;
    }
}

void FunscriptHandler::setModifier(double percentage)
{
    QMutexLocker locker(&mutex);
    auto loaded = getLoaded();
    foreach (auto name, loaded) {
        m_funscripts[name].settings.modifier = percentage / 100.0;
    }
}

void FunscriptHandler::resetModifier()
{
    setModifier(1.0);
}


void FunscriptHandler::setModifier(const Track& channelName, double percentage)
{
    QMutexLocker locker(&mutex);
    if(m_funscripts.contains(channelName)) {
        m_funscripts[channelName].settings.modifier = percentage / 100.0;
    }
}

double FunscriptHandler::getModifier(const Track& channelName)
{
    QMutexLocker locker(&mutex);
    return m_funscripts.contains(channelName) ? m_funscripts.value(channelName).settings.modifier * 100.0 : 1;
}

void FunscriptHandler::updateMetadata(LibraryListItemMetaData258 value)
{
    setModifier(value.funscriptModifier);
    setScriptOffset(value.offset);
}

void FunscriptHandler::resetModifier(const Track& channelName)
{
    setModifier(channelName, 1.0);
}

int FunscriptHandler::getOffSet()
{
    return m_offset ?: m_globalOffset;
}

int FunscriptHandler::getScriptOffSet()
{
    return m_offset;
}

void FunscriptHandler::setScriptOffset(int value)
{
    m_offset = value;
}

void FunscriptHandler::setGlobalOffset(int value)
{
    m_globalOffset = value;
}

void FunscriptHandler::setGlobalOffsetWeb(int value)
{
    m_globalOffsetWeb = value;
}

int FunscriptHandler::getGlobalOffsetWeb()
{
    return m_globalOffsetWeb;
}

QList<ScriptInfo> FunscriptHandler::getSFMATracks(QString libraryItemMediaPath)
{
    QList<ScriptInfo> scriptInfos;
    QString scriptPath = XFileUtil::getPathNoExtension(libraryItemMediaPath) + ".funscript";
    QByteArray bytes = readFile(scriptPath);
    if(bytes.isEmpty())
        return scriptInfos;
    QJsonObject json = readJson(bytes);
    if(json.isEmpty())
        return scriptInfos;

    // scriptInfos.append({"Default", libraryItemMediaPathNoExt, scriptPath, TCodeChannelLookup::ToString(Track::Stroke), ScriptType::MAIN, ScriptContainerType::BASE, "" });
    if(!json[m_sfmaJSONObjectName].isNull())
    {
        QString libraryItemMediaNameNoExt = XFileUtil::getNameNoExtension(libraryItemMediaPath);
        auto jsonTracks = json[m_sfmaJSONObjectName].toObject();
        auto channels = TCodeChannelLookup::getChannels();
        foreach(QString channelName, channels)
        {
            ChannelModel33* channel = TCodeChannelLookup::getChannel(channelName);
            if(channel->Type == ChannelType::HalfOscillate || channel->track == Track::Stroke)
                continue;
            if(jsonTracks.contains(channel->trackName)) {
                scriptInfos.append({libraryItemMediaNameNoExt, libraryItemMediaNameNoExt, scriptPath, channel->trackName, ScriptType::MAIN, ScriptContainerType::SFMA, "" });
            }
        }
    }
    return scriptInfos;
}

bool FunscriptHandler::isSFMA(QString libraryItemMediaPath)
{
    QString scriptPath = XFileUtil::getPathNoExtension(libraryItemMediaPath) + ".funscript";
    QByteArray bytes = readFile(scriptPath);
    if(bytes.isEmpty())
        return false;
    QJsonObject json = readJson(bytes);
    if(json.isEmpty() || json[m_sfmaJSONObjectName].isNull())
        return false;

    auto jsonTracks = json[m_sfmaJSONObjectName].toObject();
    auto channels = TCodeChannelLookup::getChannels();
    foreach(QString channelName, channels)
    {
        ChannelModel33* channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type == ChannelType::HalfOscillate || channel->track == Track::Stroke)
            continue;
        if(jsonTracks.contains(channel->trackName))
            return true;
    }
    return false;
}

bool FunscriptHandler::isMFS(QString libraryItemMediaPath)
{
    QString libraryItemMediaPathNoExt = XFileUtil::getPathNoExtension(libraryItemMediaPath);
    auto channels = TCodeChannelLookup::getChannels();
    foreach(QString channelName, channels)
    {
        ChannelModel33* channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type == ChannelType::HalfOscillate || channel->track == Track::Stroke || channel->trackName.isEmpty() || libraryItemMediaPath.endsWith("."+channel->trackName+".funscript"))
            continue;
        if(QFileInfo::exists(libraryItemMediaPathNoExt+"."+channel->trackName+".funscript")) {
            return true;
        }
    }
    return false;
}

///
/// \brief FunscriptHandler::getMFSTracks Searches all funscripts in the libraryItemMediaPath directore for scripts matching MFS standard.
/// \param libraryItemMediaPath
/// \param scriptPath
/// \return A list of script info objects to the found scripts
///
QList<ScriptInfo> FunscriptHandler::getMFSTracks(QString libraryItemMediaPath)
{
    QList<ScriptInfo> scriptInfos;
    QString libraryItemMediaPathNoExt = XFileUtil::getPathNoExtension(libraryItemMediaPath);
    auto channels = TCodeChannelLookup::getChannels();
    foreach(QString channelName, channels)
    {
        ChannelModel33* channel = TCodeChannelLookup::getChannel(channelName);
        if(channel->Type == ChannelType::HalfOscillate || channel->track == Track::Stroke || channel->trackName.isEmpty() || libraryItemMediaPath.endsWith("."+channel->trackName+".funscript"))
            continue;
        auto scriptPath = libraryItemMediaPathNoExt+"."+channel->trackName+".funscript";
        if(QFileInfo::exists(scriptPath)) {
            scriptInfos.append({libraryItemMediaPathNoExt, libraryItemMediaPathNoExt, scriptPath, channel->trackName, ScriptType::MAIN, ScriptContainerType::MFS, "" });
        }
    }
    return scriptInfos;
}
