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

public:
    explicit HeatMap(QObject *parent = nullptr);

    void drawAsync(int width, int height, FunscriptHandler* data, qint64 duration);
    QPixmap draw(int width, int height, FunscriptHandler* data, qint64 duration);

private:
    FunscriptHandler* m_funscriptHandler = 0;
    QMap<qint64, int> m_funscriptActionsSorted;
    //QList<QMap<qint64, int>> m_funscriptSegments;
    qint64 minD;
    qint64 maxD;
    qint64 longestAction = 0;
    qint64 shortestAction = 0;
    QList<QMap<qint64, int>> GetSegments();
    QPen m_redPen = QPen(Qt::red, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_greenPen = QPen(Qt::green, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);
    QPen m_yellowPen = QPen(Qt::yellow, 1, Qt::SolidLine, Qt::RoundCap, Qt::MiterJoin);

    void setData(FunscriptHandler* data, qint64 duration);
};

#endif // HEATMAP_H
