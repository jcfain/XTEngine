#ifndef CRYPTHANDLER_H
#define CRYPTHANDLER_H

#include <QObject>
#include <QCryptographicHash>
#include "XTEngine_global.h"
//#include "lib/tool/simplecrypt.h"

enum PasswordResponse {
    CORRECT,
    INCORRECT,
    CANCEL
};

class XTENGINE_EXPORT CryptHandler : public QObject
{
    Q_OBJECT
public:
    explicit CryptHandler(QObject *parent = nullptr);

    static QString encryptPass(QString pass);
    //static QString decryptPass(QString pass);

    static PasswordResponse checkPass(QString pass, QString hashed);

signals:


private:
    //static SimpleCrypt m_crypto;
};

#endif // CRYPTHANDLER_H
