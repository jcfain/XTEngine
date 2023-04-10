#include "jsontools.h"

JsonTools::JsonTools()
{

}


QStringList JsonTools::toStringList(QJsonArray array) {
    QStringList stringList;
    foreach(auto stringValue, array) {
        stringList << stringValue.toString();
    }
    return stringList;
}
