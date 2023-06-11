
#ifndef SCRIPTINFO_H
#define SCRIPTINFO_H

#include <QString>
#include <QMetaType>

enum class ScriptType {
    MAIN,
    ALTERNATE
};

enum class ScriptContainerType {
    BASE,
    MFS,
    ZIP
};

struct ScriptInfo {
    QString name;
    QString filename;
    QString path;
    ScriptType type;
    ScriptContainerType containerType;
};

Q_DECLARE_METATYPE(ScriptInfo);
#endif // SCRIPTINFO_H
