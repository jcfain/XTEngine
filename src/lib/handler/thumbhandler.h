#ifndef THUMBHANDLER_H
#define THUMBHANDLER_H

#include <QThread>

class thumbhandler : public QThread
{
    Q_OBJECT
public:
    explicit thumbhandler(QObject *parent = nullptr);
    void run() override;
};

#endif // THUMBHANDLER_H
