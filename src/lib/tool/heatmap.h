#ifndef HEATMAP_H
#define HEATMAP_H

#include <QObject>
#include <QPixmap>
#include "lib/handler/funscripthandler.h"

#include "XTEngine_global.h"

class XTENGINE_EXPORT HeatMap : public QObject
{
    Q_OBJECT
public:
    explicit HeatMap(QObject *parent = nullptr);
    QPixmap setData(int width, int height, FunscriptHandler* data, qint64 duration);
    QPixmap draw(int width, int height);

private:
    FunscriptHandler* m_funscriptHandler = 0;
    QMap<qint64, int> m_funscriptActions;
    QList<QMap<qint64, int>> m_funscriptSegments;
    qint64 minD;
    qint64 maxD;
    qint64 longestAction = 0;
    qint64 shortestAction = 0;
    QList<QMap<qint64, int>> GetSegments();
signals:

};

#endif // HEATMAP_H
