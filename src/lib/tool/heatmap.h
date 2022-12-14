#ifndef HEATMAP_H
#define HEATMAP_H

#include <QObject>
#include <QPixmap>
#include <QPainter>
#include "lib/handler/funscripthandler.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT HeatMap : public QObject
{
    Q_OBJECT
signals:
    void imageGenerated(QPixmap image);
    void maxHeat(qint64 at);

public:
    explicit HeatMap(QObject *parent = nullptr);

    void drawPixmapAsync(int width, int height, FunscriptHandler* data, qint64 duration);
    QPixmap drawPixmap(int width, int height, FunscriptHandler* data, qint64 duration);
    void getMaxHeatAsync(QHash<qint64, int> actions);
    qint64 getMaxHeat(QHash<qint64, int> actions);
    void paint(QPainter *painter, int width, int height, qint64 duration, QMap<qint64, int> actions, int startPoint = 0);

private:
    struct ActionSegment {
        qint64 maxVelocity = 0;;
        //qint64 segmentIndex = -1;;
        QMap<qint64, int> actions;
    };
    struct ActionSegments {
//        qint64 maxCountAt = 0;
//        qint64 maxCountIndex = -1;
//        qint64 maxCount = 0;
//        qint64 maxVelocityAt = 0;
//        qint64 maxVelocityIndex = -1;
//        qint64 maxVelocity = 0;
        qint64 maxWeight = 0;
        qint64 maxWeightAt = 0;
//        qint64 highVelosityStart = 0;
//        qint64 highVelosityEnd = 0;
//        qint64 highVelositySpan = 0;
        QList<ActionSegment> segments;
    };
//    qint64 longestAction = 0;
//    qint64 shortestAction = 0;
    void getSegments(QMap<qint64, int> actions, ActionSegments* actionSegments);
    QPen m_darkRedPen = QPen(Qt::GlobalColor::darkRed, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_redPen = QPen(Qt::red, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_yellowPen = QPen(Qt::yellow, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_greenPen = QPen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_orangePen = QPen(QColor(255,165,0), 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);

    void sortHash(QHash<qint64, int> actions, QMap<qint64, int>* sortedMap);

    float calculateVelocity(qint64 timeStart, int posStart, qint64 timeEnd, int posEnd);
};

#endif // HEATMAP_H
