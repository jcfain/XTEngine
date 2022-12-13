#include "heatmap.h"
#include <QtConcurrent/QtConcurrent>

HeatMap::HeatMap(QObject *parent)
    : QObject{parent}
{

}

void HeatMap::drawAsync(int width, int height, FunscriptHandler* data, qint64 duration) {
    QtConcurrent::run([this, width, height, data, duration]() {
        QPixmap pixmap = draw(width, height, data, duration);
        emit imageGenerated(pixmap);
    });
}

QPixmap HeatMap::draw(int width, int height, FunscriptHandler* data, qint64 duration) {
    if(duration <= 0) {
        LogHandler::Debug("Heatmap draw duration 0!");
        return QPixmap();
    }
    setData(data, duration);
    if(m_funscriptActionsSorted.empty()) {
        LogHandler::Debug("Heatmap draw called without funscript!");
        return QPixmap();
    }
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::GlobalColor::black);
    QPainter _painter;
    if(!_painter.begin(&pixmap)) {
        LogHandler::Error("Could not create painter for heat map");
        return QPixmap();
    }
    //_painter.setWindow( 0, 0, width, height);
    _painter.setRenderHint( QPainter::Antialiasing, true);

    auto atList = m_funscriptActionsSorted.keys();
    auto posList = m_funscriptActionsSorted.values();
    for(int i = 0; i < posList.count(); i ++ ) {
        if(i + 1 < posList.count()) {
            if(atList[i] > duration)
                 continue;
            int y1 = XMath::mapRange(posList[i], 0, 100, height, 0);
            int x1 = XMath::mapRange(atList[i], (qint64)0, (qint64)maxD, (qint64)0, (qint64)width);
            int y2 = XMath::mapRange(posList[i + 1], 0, 100, height, 0);
            int x2 = XMath::mapRange(atList[i + 1], (qint64)0, (qint64)maxD, (qint64)0, (qint64)width);
            float velocity = XMath::calculateSpeed(atList[i], posList[i], atList[i + 1], posList[i + 1]);;
            if(velocity > 50) {
                _painter.setPen(m_redPen);
            } else if(velocity > 25) {
                _painter.setPen(m_yellowPen);
            } else {
                _painter.setPen(m_greenPen);
            }
            _painter.drawLine(x1, y1, x2, y2);
        }
    }
    _painter.end();
    return pixmap;
//    QImage imageD = QImage(width, height, QImage::Format_RGB32);
    //QtConcurrent::run([this, imageD, width, height]() {
    //qint64 lastPos = 0;
//    qint64 timeSinceLastAt = 0;
//    qint64 averageLength = (m_funscriptHandler->getMax() - m_funscriptHandler->getMin()) / m_funscriptActions.count();
//    int Rv = 0;
//        for(int x = 0; x < width; x++) {
//            int i = (int) ((float) x / width * m_funscriptSegments.size());
//            auto segment = m_funscriptSegments[i];
//            int j = (int) ((float) x / width * segment.size());
//            qint64 at =  segment.keys()[j];
//            int pos = segment[at];
//            qint64 newTime = at - timeSinceLastAt;
//            if (newTime > 0) {
//                timeSinceLastAt = newTime;
//                Rv = qRound(255.0f * ((double)timeSinceLastAt / (maxD - minD)));
//                LogHandler::Debug("Time change: 255.0f * "+QString::number(timeSinceLastAt)+" / "+ QString::number(maxD) + " - "+QString::number(minD)+ " = "+QString::number(Rv));
//            }

//            LogHandler::Debug("Segment: at:"+QString::number(at)+" pos:"+QString::number(pos));
//            LogHandler::Debug("      Size: segement total:"+QString::number(m_funscriptSegments.size())+" segment:"+QString::number(i)+" size:"+QString::number(segment.size()));
//            LogHandler::Debug("      index: action:"+QString::number(j));
//            LogHandler::Debug("      "+QString::number(x)+" / "+QString::number(width)+" * "+QString::number(m_funscriptSegments.size()) + " = "+QString::number(i));
//            LogHandler::Debug("      "+QString::number(x)+" / "+QString::number(width)+" * "+QString::number(segment.size()) + " = "+QString::number(j));
//            for(int y = 0; y < height; y++) {
//    //            if (Rv > 0){
//    //                Rv = Rv;
//    //            }
//                QRgb value = qRgb(Rv, 0, 0);
//                LogHandler::Debug("      Image: x:"+QString::number(x)+" y:"+QString::number(y)+" rv:"+QString::number(Rv));
//                imageD.setPixel(x, y, value);
//            }
//        };
//    return QPixmap::fromImage(imageD);
}

void HeatMap::setData(FunscriptHandler* funscriptHandler, qint64 duration){
    m_funscriptActionsSorted.clear();
    minD = 0;
    maxD = duration;
    LogHandler::Debug("Set duration: "+QString::number(duration));
//    minD = m_funscriptHandler->getMin();
//    maxD = m_funscriptHandler->getMax();

    auto funscript = funscriptHandler->currentFunscript();
    if(!funscript) {
        return;
    }
    auto actions = funscriptHandler->currentFunscript()->actions;
//    qint64 lastAction = 0;
//    foreach (auto key, actions.keys()) {
//        auto currentAction = actions.value(key);
//        auto actionTimeDifference = currentAction - lastAction;
//        if(actionTimeDifference > longestAction) {
//            longestAction = actionTimeDifference;
//        } else if (actionTimeDifference < shortestAction) {
//            shortestAction = actionTimeDifference;
//        }
//        m_funscriptActionsSorted.insert(key, actions.value(key));
//        lastAction = currentAction;
//    }

    sortHash(actions, &m_funscriptActionsSorted);
    //m_funscriptSegments = GetSegments();
}

void HeatMap::getMaxHeatAsync(QHash<qint64, int> actions) {
    QtConcurrent::run([this, actions]() {
        emit maxHeat(getMaxHeat(actions));
    });
}

qint64 HeatMap::getMaxHeat(QHash<qint64, int> actions) {
    QMap<qint64, int> sortedHash;
    sortHash(actions, &sortedHash);

//    double maxValue = 0;
//    qint64 at = 0;
//    qint64 windowSize = sortedHash.lastKey();        // Total number of created samples in the test signal
//    int numberOfPeriods = sortedHash.count();    // Total number of sinoid periods in the test signal
//    QList<double> values;
//    for (qint64 n = 0 ; n < windowSize; ++n ) {
//        auto value = sin( (2 * PI * numberOfPeriods * n) / windowSize );
//        if(maxValue < value) {
//            values.append(value);
//            maxValue = value;
//            at = n;
//        }
//    }
//    return at;

//    qint64 speedStart = 0;
//    float maxVelocity = 0;
//    float maxSpeedAt = 0;
//    int previousAt = 0;
//    int previousPos = 0;

//    const int velocityConst = 25;
////    const int minBeatsAfterHighSpeedFound = 100;
////    int beatsAfterHighSpeedFound = 0;
//    foreach (auto at, sortedHash.keys()) {
//        if (at < 0) {
//            continue;
//        }

//        int pos = actions.value(at);
//        float velocity = XMath::calculateVelocity(previousAt, previousPos, at, pos);
//        if(!speedStart && velocity > velocityConst) {
//            speedStart = at;
////            beatsAfterHighSpeedFound = 0;
//        }
//        else if(speedStart > 0 && velocity < 10) {
//            //speedEnd = at;
//            speedStart = 0;
//        }
//        if(speedStart && velocity > velocityConst && maxSpeedAt < speedStart) {
//            maxSpeedAt = speedStart;
//        }
////        if(speedStart)
////            beatsAfterHighSpeedFound++;
//        previousAt = at;
//        previousPos = pos;
//    }
//    return maxSpeedAt;


    ActionSegments actionSegments;
    getSegments(sortedHash, &actionSegments);
//    qint64 maxHeat = 0;
//    qint64 maxSize = 0;
//    foreach (auto beat, segment.segments) {
//        if(maxSize < beat.count()) {
//            maxHeat = beat.firstKey();
//            maxSize = beat.count();
//        }
//    }
//    if(actionSegments.maxCountIndex == actionSegments.maxVelocityIndex)
//        return actionSegments.maxCountAt;
//    else if(actionSegments.segments.at(actionSegments.maxCountIndex).actions.count() > actionSegments.segments.at(actionSegments.maxVelocityIndex).actions.count())
//        return actionSegments.maxCountAt;
//    else
//        return actionSegments.maxVelocityAt;
    return actionSegments.maxWeightAt;
}

void HeatMap::getSegments(QMap<qint64, int> actions, ActionSegments* actionSegments) {
    //QList<QMap<qint64, int>> segments;
    int previousAt = 0;
    int previousPos = 0;
    //qint64 maxSize = 0;
    //qint64 maxSegmentAt = 0;
    QList<int> segmentVelocity;
    //float maxAverageVelocity = 0;
    //qint64 maxAverageVelocityAt = 0;
    qint64 beatTotal = 0;
    qint64 previousBeat = 0;
    float speedTotal = 0;
    foreach (auto at, actions.keys()) {
        beatTotal += at - previousBeat;
        int pos = actions.value(at);
        float speed = XMath::calculateSpeed(previousAt, previousPos, at, pos);
        speedTotal += speed;
        previousAt = at;
        previousPos = pos;
    }
    qint64 averageBeat = beatTotal / actions.count();
    float averageSpeed = speedTotal / actions.count();

    previousAt = 0;
    previousPos = 0;

    int index = 0;
    foreach (auto at, actions.keys()) {
        if (at < 0) {
            continue;
        }

        int pos = actions.value(at);
        float speed = XMath::calculateSpeed(previousAt, previousPos, at, pos);
        if (!actionSegments->segments.empty()) {
            ActionSegment lastActionSegment = actionSegments->segments.last();
            if (at - previousAt <= averageBeat && speed < averageSpeed) {// I dont know why but speed < averageSpeed gives better results than >
                if(actionSegments->maxCount <= segmentVelocity.count()) {
                    actionSegments->maxCount = segmentVelocity.count();
                    actionSegments->maxCountIndex = index;
                    actionSegments->maxCountAt = lastActionSegment.actions.firstKey();
                }
                double previousAverageVelicity = std::accumulate(segmentVelocity.begin(), segmentVelocity.end(), 0.0) / segmentVelocity.size();
                if(actionSegments->maxVelocity <= previousAverageVelicity) {
                    actionSegments->maxVelocity = previousAverageVelicity;
                    actionSegments->maxVelocityIndex = index;
                    actionSegments->maxVelocityAt = lastActionSegment.actions.firstKey();
                }
                qint64 maxWeight = actionSegments->maxCount * actionSegments->maxVelocity;
                if(actionSegments->maxWeight < maxWeight) {
                    actionSegments->maxWeight = maxWeight;
                    actionSegments->maxWeightAt = lastActionSegment.actions.firstKey();
                }
                segmentVelocity.clear();
                actionSegments->segments.append(ActionSegment());
                index++;
            }
        } else
            actionSegments->segments.append(ActionSegment());

        ActionSegment* lastActionSegment = &actionSegments->segments.last();
        if(speed > 0) {
            segmentVelocity.append(speed);
            if(lastActionSegment->maxVelocity < speed)
                lastActionSegment->maxVelocity = speed;
        }
//        auto averageVelicity = std::accumulate(segmentVelocity.begin(), segmentVelocity.end(), 0.0) / segmentVelocity.size();
//        if(averageVelicity > 25)
//            actionSegments->highVelosityStart = at;
//        else if(actionSegments->highVelosityStart > 0) {
//            actionSegments->highVelosityEnd = at;
//            auto span = actionSegments->highVelosityEnd - actionSegments->highVelosityStart;
//            if(span > actionSegments->highVelositySpan)
//                actionSegments->highVelositySpan = span;
//        }

        lastActionSegment->segmentIndex = index;
        lastActionSegment->actions.insert(at, pos);

        previousAt = at;
        previousPos = actions.value(at);
    }




//    foreach (auto at, actions.keys())
//    {
//        if (at < 0 || at > actions.end().key()) continue;
//        if (at - previousAt >= 500)
//        {
//            segment.segments.append(QMap<qint64, int>());
//        }

//        if (segment.segments.empty())
//            segment.segments.append(QMap<qint64, int>());

//        segment.segments.last().insert(at, actions.value(at));
//        //previousPos = actions.value(at);
//        previousAt = at;

//    }

//    previousAt = 0;
//    foreach (auto beat, segment.segments) {
//        foreach (auto at, beat.keys()) {
//           int pos = beat.value(at);
//           qint64 timeDiff = at - previousAt;
//           int posDiff = abs(pos - previousPos);
//           float velocity = ((float)posDiff / (float)timeDiff) * 100;
//           if(velocity > 0)
//               segmentVelocity.append(velocity);
//           previousAt = at;
//           previousPos = pos;
//        }
//        auto previousAverageVelicity = std::accumulate(segmentVelocity.begin(), segmentVelocity.end(), 0.0) / segmentVelocity.size();
//        if(maxSize < beat.count() && maxAverageVelocity <= previousAverageVelicity) {
//            segment.maxSegmentAt = beat.firstKey();
//            segment.maxAverageVelocityAt = beat.firstKey();
//        }
//        if(maxSize < beat.count()) {
//            maxSegmentAt = beat.firstKey();
//            maxSize = beat.count();
//        }
//        if(maxAverageVelocity <= previousAverageVelicity) {
//            maxAverageVelocity = previousAverageVelicity;
//            if(!beat.empty())
//                maxAverageVelocityAt = beat.firstKey();
//        }
//    }
}

void HeatMap::sortHash(QHash<qint64, int> actions, QMap<qint64, int>* sortedMap) {
    //qint64 lastAction = 0;
    foreach (auto key, actions.keys()) {
//        auto currentAction = actions.value(key);
//        auto actionTimeDifference = currentAction - lastAction;
//        if(actionTimeDifference > longestAction) {
//            longestAction = actionTimeDifference;
//        } else if (actionTimeDifference < shortestAction) {
//            shortestAction = actionTimeDifference;
//        }
        sortedMap->insert(key, actions.value(key));
//        lastAction = currentAction;
    }
}
