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
    setData(data, duration);
    if(m_funscriptActionsSorted.empty()) {
        LogHandler::Debug("Heatmap draw called without funscript!");
        return QPixmap();
    }
    if(duration <= 0) {
        LogHandler::Debug("Heatmap draw duration 0!");
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
            int y1 = XMath::mapRange(posList[i], 0, 100, height, 0);
            int x1 = XMath::mapRange(atList[i], (qint64)0, (qint64)maxD, (qint64)0, (qint64)width);
            int y2 = XMath::mapRange(posList[i + 1], 0, 100, height, 0);
            int x2 = XMath::mapRange(atList[i + 1], (qint64)0, (qint64)maxD, (qint64)0, (qint64)width);
            qint64 timeDiff = atList[i + 1] - atList[i];
            int posDiff = abs(posList[i + 1] - posList[i]);
            float velocity = ((float)posDiff / (float)timeDiff) * 100;
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
        m_funscriptActionsSorted.clear();
        return;
    }
    auto actions = funscriptHandler->currentFunscript()->actions;
    qint64 lastAction = 0;
    foreach (auto key, actions.keys()) {
        auto currentAction = actions.value(key);
        auto actionTimeDifference = currentAction - lastAction;
        if(actionTimeDifference > longestAction) {
            longestAction = actionTimeDifference;
        } else if (actionTimeDifference < shortestAction) {
            shortestAction = actionTimeDifference;
        }
        m_funscriptActionsSorted.insert(key, actions.value(key));
        lastAction = currentAction;
    }
    //m_funscriptSegments = GetSegments();
}

QList<QMap<qint64, int>> HeatMap::GetSegments()
{
    QList<QMap<qint64, int>> segments;

//    int previous = 0;
//    foreach (auto beat, m_funscriptActions.keys())
//    {
//        if (beat < 0 || beat > m_funscriptActions.end().key()) continue;

//        if (beat - previous >= 1000)
//        {
//            segments.append(QMap<qint64, int>());
//        }

//        if (segments.empty())
//            segments.append(QMap<qint64, int>());

//        segments.last().insert(beat, m_funscriptActions.value(beat));
//        previous = beat;
//    }

    return segments;
}
