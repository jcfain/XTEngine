#include "heatmap.h"
#include <QtConcurrent/QtConcurrent>
#include <QUrl>

HeatMap::HeatMap(QObject *parent)
    : QObject{parent}
{

}

void HeatMap::drawPixmapAsync(int width, int height, FunscriptHandler* data, qint64 duration) {
    QtConcurrent::run([this, width, height, data, duration]() {
        QPixmap pixmap = drawPixmap(width, height, data, duration);
        emit imageGenerated(pixmap);
    });
}

QPixmap HeatMap::drawPixmap(int width, int height, FunscriptHandler* data, qint64 duration) {
    if(duration <= 0) {
        LogHandler::Debug("Heatmap draw duration 0!");
        return QPixmap();
    }
    QMap<qint64, int> actions;

    auto funscript = data->getFunscript();
    if(!funscript) {
        return QPixmap();
    }

    auto actionsHash = funscript->actions;
    sortHash(actionsHash, &actions);

    if(actions.empty()) {
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

    paint(&_painter, width, height, duration, actions);

    _painter.end();
    return pixmap;
}

void HeatMap::drawSvgAsync(QHash<qint64, int> actions, qint64 duration) {
    QtConcurrent::run([this](QHash<qint64, int> actions, qint64 duration) {
        QString svg = drawSvg(actions, duration);
        if(svg.isEmpty()) {
            LogHandler::Debug(QString("Heatmap: svg generation produced no output (actions: %1, duration: %2)")
                .arg(actions.size()).arg(duration));
        } else {
            LogHandler::Debug(QString("Heatmap: svg generated (%1 actions, duration %2 ms, %3 chars)")
                .arg(actions.size()).arg(duration).arg(svg.size()));
            emit svgGenerated(svg);
        }
    }, actions, duration);
}

void HeatMap::requestSvg(QString mediaPath, QHash<qint64, int> actions, qint64 duration) {
    LogHandler::Debug(QString("Heatmap: requestSvg path: %1, actions: %2, duration: %3")
        .arg(mediaPath).arg(actions.size()).arg(duration));
    if(actions.isEmpty()) {
        LogHandler::Debug("Heatmap: requestSvg with no actions - nothing to draw");
        return;
    }
    if(duration > 0) {
        drawSvgAsync(actions, duration);
        return;
    }
    // The media duration has not been reported yet - hold the actions until
    // durationReported() is called with the real length. The funscript's own
    // length is NOT used as a fallback since it does not always match the
    // video duration.
    m_pendingMediaPath = mediaPath;
    m_pendingActions = actions;
}

// The client's posted path and the library item path can differ (percent
// encoding, prefix), so only compare the file name when deciding if a
// reported duration belongs to the pending media.
static QString heatMapBaseName(QString path) {
    QString decoded = QUrl::fromPercentEncoding(path.toUtf8());
    int idx = decoded.lastIndexOf('/');
    return idx >= 0 ? decoded.mid(idx + 1) : decoded;
}

void HeatMap::durationReported(QString mediaPath, qint64 duration) {
    if(m_pendingActions.isEmpty() || duration <= 0)
        return;
    if(!m_pendingMediaPath.isEmpty() && heatMapBaseName(m_pendingMediaPath) != heatMapBaseName(mediaPath)) {
        // The media changed while waiting - the pending actions are stale.
        LogHandler::Debug(QString("Heatmap: durationReported path mismatch - pending: %1, reported: %2 (discarding)")
            .arg(m_pendingMediaPath, mediaPath));
        m_pendingMediaPath.clear();
        m_pendingActions.clear();
        return;
    }
    LogHandler::Debug(QString("Heatmap: durationReported flushed pending svg (duration: %1)").arg(duration));
    drawSvgAsync(m_pendingActions, duration);
    m_pendingMediaPath.clear();
    m_pendingActions.clear();
}

// Generates a scalable, colored SVG of the funscript heat map. The line
// colors match the pixmap version (see paint()). The fixed viewBox keeps the
// SVG small while letting the client scale it to any size.
QString HeatMap::drawSvg(QHash<qint64, int> actions, qint64 duration) {
    if(duration <= 0) {
        LogHandler::Debug("Heatmap svg duration 0!");
        return QString();
    }

    QMap<qint64, int> sortedActions;
    sortHash(actions, &sortedActions);

    if(sortedActions.empty()) {
        LogHandler::Debug("Heatmap svg called without funscript!");
        return QString();
    }

    const int width = 1000;
    const int height = 40;
    // Inset the drawing area so lines at pos 0/100 (and the first/last
    // action) are not clipped at the SVG viewport edges.
    const int xPadding = 1;
    const int yPadding = 4;

    // One path per color keeps the SVG compact.
    QString darkRedPath;
    QString redPath;
    QString orangePath;
    QString yellowPath;
    QString greenPath;

    auto atList = sortedActions.keys();
    auto posList = sortedActions.values();
    for(int i = 0; i < posList.count(); i ++)
    {
        if(i + 1 < posList.count())
        {
            if(atList[i] > duration)
                continue;
            // Match the pixmap orientation (paint()): position 0 at the
            // bottom, position 100 at the top.
            double y1 = XMath::mapRange(posList[i], 0, 100, height - yPadding, yPadding);
            double x1 = XMath::mapRange(atList[i], (qint64)0, (qint64)duration, (qint64)xPadding, (qint64)(width - xPadding));
            double y2 = XMath::mapRange(posList[i + 1], 0, 100, height - yPadding, yPadding);
            double x2 = XMath::mapRange(atList[i + 1], (qint64)0, (qint64)duration, (qint64)xPadding, (qint64)(width - xPadding));
            float velocity = XMath::calculateSpeed(atList[i], posList[i], atList[i + 1], posList[i + 1]);

            QString segment = QString("M%1 %2 L%3 %4 ")
                .arg(x1, 0, 'f', 2).arg(y1, 0, 'f', 2)
                .arg(x2, 0, 'f', 2).arg(y2, 0, 'f', 2);

            if(velocity > 45) {
                darkRedPath += segment;
            } else if(velocity > 35) {
                redPath += segment;
            } else if(velocity > 25) {
                orangePath += segment;
            } else if(velocity > 15) {
                yellowPath += segment;
            } else {
                greenPath += segment;
            }
        }
    }

    QString svg = QString(
        "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"100%\" height=\"100%\" viewBox=\"0 0 %1 %2\" preserveAspectRatio=\"none\">"
        "<rect x=\"0\" y=\"0\" width=\"%1\" height=\"%2\" fill=\"black\"/>")
        .arg(width).arg(height);

    auto appendPath = [&svg](const QString& path, const QString& color) {
        if(!path.isEmpty()) {
            // vector-effect keeps the stroke at 1 screen pixel even though
            // preserveAspectRatio="none" stretches the viewBox non-uniformly
            // (without it the lines get crushed to sub-pixel and vanish).
            svg += QString("<path d=\"%1\" stroke=\"%2\" stroke-width=\"1\" fill=\"none\" stroke-linecap=\"round\" stroke-linejoin=\"miter\" vector-effect=\"non-scaling-stroke\"/>")
                .arg(path.trimmed()).arg(color);
        }
    };
    appendPath(greenPath, "#00FF00");
    appendPath(yellowPath, "#FFFF00");
    appendPath(orangePath, "#FFA500");
    appendPath(redPath, "#FF0000");
    appendPath(darkRedPath, "#8B0000");

    svg += "</svg>";
    return svg;
}

void HeatMap::paint(QPainter* painter, int width, int height, qint64 duration, QMap<qint64, int> actions, int startPoint) {
    auto atList = actions.keys();
    auto posList = actions.values();
    for(int i = 0; i < posList.count(); i ++ )
    {
        if(i + 1 < posList.count())
        {
            if(atList[i] > duration)
                 continue;
            qint64 y1 = XMath::mapRange(posList[i], 0, 100, height, startPoint);
            qint64 x1 = XMath::mapRange(atList[i], (qint64)0, (qint64)duration, (qint64)0, (qint64)width);
            qint64 y2 = XMath::mapRange(posList[i + 1], 0, 100, height, startPoint);
            qint64 x2 = XMath::mapRange(atList[i + 1], (qint64)0, (qint64)duration, (qint64)0, (qint64)width);
            float velocity = XMath::calculateSpeed(atList[i], posList[i], atList[i + 1], posList[i + 1]);;
            if(velocity > 45) {
                painter->setPen(m_darkRedPen);
            } else if(velocity > 35) {
                painter->setPen(m_redPen);
            } else if(velocity > 25) {
                painter->setPen(m_orangePen);
            } else if(velocity > 15) {
                painter->setPen(m_yellowPen);
            } else {
                painter->setPen(m_greenPen);
            }
            painter->drawLine(x1, y1, x2, y2);
        }
    }
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
    if(actions.isEmpty())
        return;
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
    qint64 maxCount = 0;
    qint64 maxSpeed = 0;
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

    //int index = 0;
    foreach (auto at, actions.keys())
    {
        if (at < 0)
            continue;

        int pos = actions.value(at);
        float speed = XMath::calculateSpeed(previousAt, previousPos, at, pos);
        if (!actionSegments->segments.empty())
        {
            ActionSegment lastActionSegment = actionSegments->segments.last();
            if (at - previousAt <= averageBeat && speed < averageSpeed)// I dont know why but speed < averageSpeed gives better results than >
            {
                if(maxCount <= segmentVelocity.count())
                {
                    maxCount = segmentVelocity.count();
                }
//                if(actionSegments->maxCount <= segmentVelocity.count())
//                {
//                    actionSegments->maxCount = segmentVelocity.count();
//                    actionSegments->maxCountIndex = index;
//                    actionSegments->maxCountAt = lastActionSegment.actions.firstKey();
//                }
                double previousAverageVelicity = std::accumulate(segmentVelocity.begin(), segmentVelocity.end(), 0.0) / segmentVelocity.size();
                if(maxSpeed <= previousAverageVelicity)
                {
                    maxSpeed = previousAverageVelicity;
                }
//                if(actionSegments->maxVelocity <= previousAverageVelicity)
//                {
//                    actionSegments->maxVelocity = previousAverageVelicity;
//                    actionSegments->maxVelocityIndex = index;
//                    actionSegments->maxVelocityAt = lastActionSegment.actions.firstKey();
//                }
                qint64 maxWeight = maxCount * maxSpeed;
                if(actionSegments->maxWeight < maxWeight)
                {
                    actionSegments->maxWeight = maxWeight;
                    actionSegments->maxWeightAt = lastActionSegment.actions.firstKey();
                }
                segmentVelocity.clear();
                actionSegments->segments.append(ActionSegment());
                //index++;
            }
        }
        else
            actionSegments->segments.append(ActionSegment());

        ActionSegment* lastActionSegment = &actionSegments->segments.last();
        if(speed > 0)
        {
            segmentVelocity.append(speed);
            if(lastActionSegment->maxVelocity < speed)
                lastActionSegment->maxVelocity = speed;
        }

        //lastActionSegment->segmentIndex = index;
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
    foreach (auto key, actions.keys()) {
        sortedMap->insert(key, actions.value(key));
    }
}
