#ifndef JSONTOOLS_H
#define JSONTOOLS_H

#include <QStringList>
#include <QJsonArray>

class JsonTools
{
public:
    JsonTools();

    static QStringList toStringList(QJsonArray array);
};

#endif // JSONTOOLS_H
