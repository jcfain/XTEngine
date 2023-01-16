#include "crypthandler.h"

CryptHandler::CryptHandler(QObject *parent)
    : QObject{parent}
{

}

QString CryptHandler::encryptPass(QString pass)
{
    return QString(QCryptographicHash::hash(pass.toUtf8(),QCryptographicHash::Keccak_512).toHex());
    //return m_crypto.encryptToString(pass);
}

//QString CryptHandler::decryptPass(QString pass)
//{
//    return m_crypto.decryptToString(pass);
//}

PasswordResponse CryptHandler::checkPass(QString pass, QString hashed)
{
    QString passHashed = encryptPass(pass);
    return passHashed == hashed ? PasswordResponse::CORRECT : PasswordResponse::INCORRECT;
}

//SimpleCrypt CryptHandler::m_crypto(Q_UINT64_C(0xcafbb6143ff01257));//some random number
