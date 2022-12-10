#include "heatmap.h"
#include <QtConcurrent/QtConcurrent>

HeatMap::HeatMap(QObject *parent)
    : QObject{parent}
{

}

QPixmap HeatMap::setData(int width, int height, FunscriptHandler* funscriptHandler, qint64 duration){

    minD = 0;
    maxD = duration;
    LogHandler::Debug("Set duration: "+QString::number(duration));
    m_funscriptHandler = funscriptHandler;
//    minD = m_funscriptHandler->getMin();
//    maxD = m_funscriptHandler->getMax();

    auto actions = m_funscriptHandler->currentFunscript()->actions;
    qint64 lastAction = 0;
    foreach (auto key, actions.keys()) {
        auto currentAction = actions.value(key);
        auto actionTimeDifference = currentAction - lastAction;
        if(actionTimeDifference > longestAction) {
            longestAction = actionTimeDifference;
        } else if (actionTimeDifference < shortestAction) {
            shortestAction = actionTimeDifference;
        }
        m_funscriptActions.insert(key, actions.value(key));
        lastAction = currentAction;
    }
    m_funscriptSegments = GetSegments();

    //resize(wdth, hght);

//    this->setPixmap(mainFrame);

//    this->show();
    return draw(width, height);
}

QPixmap HeatMap::draw2(int width, int height) {
    QPixmap pixmap;
    QPainter _painter(&pixmap);
    _painter.setWindow( 0, 0, width, height );

    _painter.setRenderHint( QPainter::Antialiasing, true);
    _painter.setPen(QPen(Qt::black, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));

    int nGridStep = 20;
    for( int x = 0; x <= width / nGridStep; x ++ )
    {
       _painter.drawLine( x * nGridStep, 0, x * nGridStep, height );
    }
    for( int y = 0; y <= height / nGridStep; y ++ )
    {
       _painter.drawLine( 0, y * nGridStep, width, y * nGridStep );
    }

    _painter.setPen(QPen(Qt::red, 3, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin));
    int nSpace = 0;
    int nSpace2 = 0;
    auto funscript = m_funscriptHandler->currentFunscript();
    auto atList = m_funscriptActions.keys();
    auto posList = m_funscriptActions.values();
    for(int i = 0; i < posList.count(); i ++ ) {
        if(i + 1 < posList.count()) {
            nSpace += nGridStep;
            _painter.drawLine( nSpace2, height - posList[i] , nSpace, height - posList[i + 1]);
            nSpace2 += nGridStep;
        }
    }
    return pixmap;
}

QPixmap HeatMap::draw(int width, int height) {
    if(!m_funscriptHandler) {
        LogHandler::Error("Heatmap draw called without funscript!");
        return QPixmap();
    }
    QImage imageD = QImage(width, height, QImage::Format_RGB32);
    //QtConcurrent::run([this, imageD, width, height]() {
    //qint64 lastPos = 0;
    qint64 timeSinceLastAt = 0;
    qint64 averageLength = (m_funscriptHandler->getMax() - m_funscriptHandler->getMin()) / m_funscriptActions.count();
    int Rv = 0;
        for(int x = 0; x < width; x++) {
            int i = (int) ((float) x / width * m_funscriptSegments.size());
            auto segment = m_funscriptSegments[i];
            int j = (int) ((float) x / width * segment.size());
            qint64 at =  segment.keys()[j];
            int pos = segment[at];
            qint64 newTime = at - timeSinceLastAt;
            if (newTime > 0) {
                timeSinceLastAt = newTime;
                Rv = qRound(255.0f * ((double)timeSinceLastAt / (maxD - minD)));
                LogHandler::Debug("Time change: 255.0f * "+QString::number(timeSinceLastAt)+" / "+ QString::number(maxD) + " - "+QString::number(minD)+ " = "+QString::number(Rv));
            }

            LogHandler::Debug("Segment: at:"+QString::number(at)+" pos:"+QString::number(pos));
            LogHandler::Debug("      Size: segement total:"+QString::number(m_funscriptSegments.size())+" segment:"+QString::number(i)+" size:"+QString::number(segment.size()));
            LogHandler::Debug("      index: action:"+QString::number(j));
            LogHandler::Debug("      "+QString::number(x)+" / "+QString::number(width)+" * "+QString::number(m_funscriptSegments.size()) + " = "+QString::number(i));
            LogHandler::Debug("      "+QString::number(x)+" / "+QString::number(width)+" * "+QString::number(segment.size()) + " = "+QString::number(j));
            for(int y = 0; y < height; y++) {
    //            if (Rv > 0){
    //                Rv = Rv;
    //            }
                QRgb value = qRgb(Rv, 0, 0);
                LogHandler::Debug("      Image: x:"+QString::number(x)+" y:"+QString::number(y)+" rv:"+QString::number(Rv));
                imageD.setPixel(x, y, value);
            }
        };
    //});

//    double span = (double)(m_funscriptSegments.last().last() - m_funscriptSegments.first().first());
//    int segmentCount = (int)std::max((double)1, std::min((m_funscriptSegments.size() - 1) / 12.0, span / m_funscriptSegments.last().last()/200));
//    for (int i = 0; i < m_funscriptSegments.size(); i++)
//    {
//        int startIndex = std::min(m_funscriptSegments.size() - 1, (int)((i * (m_funscriptSegments.size() - 1)) / (double)segmentCount));
//        int endIndex = std::min(m_funscriptSegments.size() - 1, (int)(((i + 1) * (m_funscriptSegments.size() - 1)) / (double)segmentCount));
//        int beatCount = endIndex - startIndex - 1;

//        qint64 firstBeat = m_funscriptSegments[startIndex].first();
//        qint64 lastBeat = m_funscriptSegments[endIndex].last();

//        qint64 averageLength = (lastBeat - firstBeat) / beatCount;
//        double value = 200 / averageLength * 1.0;

//        value = std::min(1.0, std::max(0.0, value));
//        Color color = GetColorAtPosition(HeatMap, value);

//        double positionStart = firstBeat / duration;
//        double positionEnd = lastBeat / duration;

//        if (i == 0)
//            stops.Add(new HeatMapEntry(color, positionStart));

//        stops.Add(new HeatMapEntry(color, (positionEnd + positionStart) / 2.0));

//        if (i == segmentCount - 1)
//            stops.Add(new HeatMapEntry(color, positionEnd));
//    }
    return QPixmap::fromImage(imageD);
}

QList<QMap<qint64, int>> HeatMap::GetSegments()
{
    QList<QMap<qint64, int>> segments;

    //TimeSpan previous = timeFrom;
    int previous = 0;
    foreach (auto beat, m_funscriptActions.keys())
    {
        if (beat < 0 || beat > m_funscriptActions.end().key()) continue;

        if (beat - previous >= 1000)
        {
            segments.append(QMap<qint64, int>());
        }

        if (segments.empty())
            segments.append(QMap<qint64, int>());

        segments.last().insert(beat, m_funscriptActions.value(beat));
        previous = beat;
    }

    return segments;
}
