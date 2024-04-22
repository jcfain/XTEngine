#include "funscripthandler.h"

FunscriptHandler::FunscriptHandler(QString channel, QObject* parent) : QObject(parent)
{
    _channel = channel;
}

FunscriptHandler::~FunscriptHandler()
{
    if(funscript)
        delete funscript;
    _loaded = false;
    SettingsHandler::setFunscriptLoaded(_channel, _loaded);
}

QString FunscriptHandler::channel()
{
    return _channel;
}

bool FunscriptHandler::load(QString funscriptString)
{
    QMutexLocker locker(&mutex);
    if (funscriptString == nullptr)
    {
        _loaded = false;
        LogHandler::Error("Loading funscript failed: Empty string" );
        SettingsHandler::setFunscriptLoaded(_channel, _loaded);
        return false;
    }
    LogHandler::Debug("Funscript load: "+funscriptString);
    QFile loadFile(funscriptString);
    lastActionIndex = -1;
    nextActionIndex = 1;
    if (!loadFile.open(QIODevice::ReadOnly)) {
        LogHandler::Warn("Loading funscript failed: Couldn't open funscript file.");
        _loaded = false;
        SettingsHandler::setFunscriptLoaded(_channel, _loaded);
        return false;
    }

    //LogHandler::Debug("funscriptHandler->load "+QString::number((round(timer.nsecsElapsed()) / 1000000)));
    QByteArray funData = loadFile.readAll();
    locker.unlock();
    return load(funData);
}

bool FunscriptHandler::load(QByteArray byteArray)
{
    QMutexLocker locker(&mutex);
    resetModifier();
    _firstActionExecuted = false;
    _funscriptMax = -1;
    QJsonParseError* error = new QJsonParseError();
    QJsonDocument doc = QJsonDocument::fromJson(byteArray, error);

    if(doc.isNull()) {
        LogHandler::Error("Loading funscript JSON failed: " + error->errorString());
        delete error;
        _loaded = false;
        SettingsHandler::setFunscriptLoaded(_channel, _loaded);
        return false;
    }
    delete error;
    JSonToFunscript(doc.object());
    _funscriptMax = atList.length() > 0 ? atList.last() : -1;
    _funscriptMin = 0;
    foreach(qint64 value, atList)
    {
        if(value > 0)
        {
            _funscriptMin = value;
            break;
        }
    }

    _loaded = true;
    SettingsHandler::setFunscriptLoaded(_channel, _loaded);
    return true;
}

Funscript* FunscriptHandler::currentFunscript()
{
    QMutexLocker locker(&mutex);
    return funscript;
}

bool FunscriptHandler::isLoaded()
{
    QMutexLocker locker(&mutex);
    return _loaded;
}

void FunscriptHandler::setLoaded(bool value)
{
    QMutexLocker locker(&mutex);
    _loaded = value;
    if(!value && funscript) {
        delete funscript;
        funscript = 0;
    }
    SettingsHandler::setFunscriptLoaded(_channel, _loaded);
}

qint64 FunscriptHandler::getMin()
{
    return _funscriptMin;
}

qint64 FunscriptHandler::getMax()
{
    return _funscriptMax;
}

qint64 FunscriptHandler::getNext() {
    if(lastActionIndex == nextActionIndex) {
        int nextAction = lastActionIndex + 1;
        if(nextAction >= atList.length())
            return getMin();
        return atList[nextAction];
    }
    if(nextActionIndex > -1) {
        return atList[nextActionIndex];
    }
    return getMin();
}
#include <QTime>
void FunscriptHandler::JSonToFunscript(QJsonObject json)
{
    if(!funscript)
        funscript = new Funscript();
    if (json.contains("range"))
        funscript->range = json["range"].toInt();
    if (json.contains("version") && json["version"].isString())
        funscript->version = json["version"].toString();
    if (json.contains("inverted") && json["inverted"].isBool())
    {
        _inverted = json["inverted"].toBool();
        funscript->inverted = json["inverted"].toBool();
    }
    if (json.contains("actions") && json["actions"].isArray())
    {
        funscript->actions.clear();
        QJsonArray actionArray = json["actions"].toArray();
        foreach(QJsonValue value, actionArray)
        {
            QJsonObject obj = value.toObject();
            if (obj.contains("at") && obj.contains("pos"))
            {
                funscript->actions[(qint64)obj["at"].toDouble()] = obj["pos"].toInt();
            }
        }
        atList = funscript->actions.keys();
        std::sort(atList.begin(), atList.end());
        n = atList.length() / sizeof(atList.first());
    }

    QJsonObject metaData = json;
    if(json.contains("metadata"))
        metaData = json["metadata"].toObject();

    if (metaData.contains("creator") && metaData["creator"].isString())
        funscript->metadata.creator = metaData["creator"].toString();
    if (metaData.contains("original_name") && metaData["original_name"].isString())
        funscript->metadata.original_name = metaData["original_name"].toString();
    if (metaData.contains("url") && metaData["url"].isString())
        funscript->metadata.url = metaData["url"].toString();
    if (metaData.contains("url_video") && metaData["url_video"].isString())
        funscript->metadata.url_video = metaData["url_video"].toString();
    if (metaData.contains("tags") && metaData["tags"].isArray())
    {
        QJsonArray tags = metaData["tags"].toArray();;
        foreach(QJsonValue tag, tags)
            funscript->metadata.tags.append(tag.toString());
    }
    if (metaData.contains("performers") && metaData["performers"].isArray())
    {
        QJsonArray performers = metaData["performers"].toArray();;
        foreach(QJsonValue performer, performers)
            funscript->metadata.performers.append(performer.toString());
    }
    if (metaData.contains("bookmarks") && metaData["bookmarks"].isArray())
    {
        QJsonArray bookmarks = metaData["bookmarks"].toArray();;
        foreach(QJsonValue bookmark, bookmarks) {
            qint64 milliseconds = QTime::fromString(bookmark.toObject()["time"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
            funscript->metadata.bookmarks.append({bookmark.toObject()["name"].toString(), milliseconds});
        }
    }
    if (metaData.contains("chapters") && metaData["chapters"].isArray())
    {
        QJsonArray chapters = metaData["chapters"].toArray();;
        foreach(QJsonValue chapter, chapters) {
            qint64 startTime = QTime::fromString(chapter.toObject()["startTime"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
            qint64 endTime = QTime::fromString(chapter.toObject()["endTime"].toString(), Qt::DateFormat::ISODateWithMs).msecsSinceStartOfDay();
            funscript->metadata.chapters.append({chapter.toObject()["name"].toString(), startTime, endTime});
        }
    }
    if (metaData.contains("paid") && metaData["paid"].isBool())
        funscript->metadata.paid = metaData["paid"].toBool();
    if (metaData.contains("comment") && metaData["comment"].isString())
        funscript->metadata.comment = metaData["comment"].toString();
    if (metaData.contains("original_total_duration_ms"))
        funscript->metadata.original_total_duration_ms = metaData["original_total_duration_ms"].toString().toLongLong();
}

bool FunscriptHandler::exists(QString path)
{
    QFile fpath(path);
    return fpath.exists();
}

std::shared_ptr<FunscriptAction> FunscriptHandler::getPosition(qint64 millis)
{
    QMutexLocker locker(&mutex);
    millis += SettingsHandler::getLiveOffSet();
    qint64 closestMillis = findClosest(millis, atList);
    if(closestMillis == -1)
        return nullptr;
    nextActionIndex = atList.indexOf(closestMillis) + 1;
    if(nextActionIndex >= atList.length())
        return nullptr;
    qint64 nextMillis = atList[nextActionIndex];
    //LogHandler::Debug("millis: "+ QString::number(millis));
    //LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
    //LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
    //LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
//    LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
    if ((lastActionIndex != nextActionIndex && millis >= closestMillis) || lastActionIndex == -1)
    {
        int interval = lastActionIndex == -1 ? closestMillis : nextMillis - closestMillis;
        if(!_firstActionExecuted)
        {
            _firstActionExecuted = true;
            if(interval < 500)
                interval = 500;
            else if(interval > 5000)
                interval = 5000;
        }
        //LogHandler::Debug("offSet: "+ QString::number(SettingsHandler::getoffSet()));
        //LogHandler::Debug("speed: "+ QString::number(speed));
        //LogHandler::Debug("millis: "+ QString::number(millis));
//        LogHandler::Debug("closestMillis: "+ QString::number(closestMillis));
//        LogHandler::Debug("nextMillis: "+ QString::number(nextMillis));
//        LogHandler::Debug("lastActionIndex: "+ QString::number(lastActionIndex));
//        LogHandler::Debug("nextActionIndex: "+ QString::number(nextActionIndex));
        //LogHandler::Debug("nextActionPos: "+ QString::number(funscript->actions.value(nextMillis)));
        qint64 executionMillis = lastActionIndex == -1 ? closestMillis : nextMillis;
        int pos = funscript->actions.value(executionMillis);
        if(_modifier != 1)
        {
            if(lastActionPos < pos) // up stroke
            {
                int distance = qRound((pos - lastActionPos) / 2.0);
                if(_modifier > 1)
                    pos = qRound(pos + (distance * (_modifier - 1)));
                else
                    pos = qRound(pos - (distance - (distance * _modifier)));
            }
            else if(lastActionPos > pos) // down stroke
            {
                int distance = qRound((lastActionPos - pos) / 2.0);
                if(_modifier > 1)
                    pos = qRound(pos - (distance * (_modifier - 1)));
                else
                    pos = qRound(pos + (distance - (distance * _modifier)));

            }
            pos = XMath::constrain(pos, 0, 100);
        }
        std::shared_ptr<FunscriptAction> nextAction(new FunscriptAction { _channel, executionMillis, pos, interval, lastActionPos, lastActionSpeed });
        //LogHandler::Debug("nextAction.speed: "+ QString::number(nextAction->speed));
        lastActionIndex = nextActionIndex;
        lastActionPos = funscript->actions.value(executionMillis);
        lastActionSpeed = interval;
        return nextAction;
    }
    return nullptr;
}

qint64 FunscriptHandler::findClosest(qint64 value, QList<qint64> a) {

    if(a.length() == 0)
        return -1;
      if(value < a[0]) {
          return a[0];
      }
      if(value > a[a.length()-1]) {
          return a[a.length()-1];
      }

      int lo = 0;
      int hi = a.length() - 1;

      while (lo <= hi) {
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


//static
bool FunscriptHandler::getInverted()
{
    QMutexLocker locker(&mutexStat);
    return _inverted;
}
void FunscriptHandler::setInverted(bool value)
{
    QMutexLocker locker(&mutexStat);
    _inverted = value;
}

void FunscriptHandler::setModifier(int percentage)
{
    _modifier = percentage / 100.0;
}

int FunscriptHandler::getModifier()
{
    return _modifier * 100.0;
}

void FunscriptHandler::resetModifier()
{
    _modifier = 1.0;
}

double FunscriptHandler::_modifier = 1;
bool FunscriptHandler::_inverted = false;
QMutex FunscriptHandler::mutexStat;
