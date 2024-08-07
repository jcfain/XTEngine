#include "xmath.h"

#include <random>
#include <chrono>
#include <stdlib.h>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>

//#include "../handler/loghandler.h"

int XMath::mapRange(int value, int inStart, int inEnd, int outStart, int outEnd)
{
    double slope = (double)(outEnd - outStart) / (inEnd - inStart);
    return qRound(outStart + slope * (value - inStart));
}

qint64 XMath::mapRange(qint64 value, qint64 inStart, qint64 inEnd, qint64 outStart, qint64 outEnd)
{
    double slope = (double)(outEnd - outStart) / (inEnd - inStart);
    return qRound64(outStart + slope * (value - inStart));
}

qint64 XMath::mapRange(double value, double inStart, double inEnd, int outStart, int outEnd)
{
    double slope = (outEnd - outStart) / (inEnd - inStart);
    return qRound(outStart + slope * (value - inStart));
}

int  XMath::constrain(int value, int min, int max)
{
    if (value > max)
        return max;
    if (value < min)
        return min;
    return value;
}
qint64 XMath::random(qint64 min, qint64 max)
{
    // unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    // std::mt19937_64 mt(seed1);
    // std::uniform_int_distribution<qint64> dist(min, max);
    // return dist(mt);
    return rand() % max + min;
}

int XMath::random(int min, int max)
{
    // unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    // std::mt19937 mt(seed1);
    // std::uniform_int_distribution<int> dist(min, max);
    // return dist(mt);
    return rand() % max + min;
}

double XMath::random(double min, double max)
{
    // unsigned seed1 = std::chrono::system_clock::now().time_since_epoch().count();
    // std::mt19937_64 mt(seed1);
    // std::uniform_real_distribution<double> dist(min, max);
    // return dist(mt);
    std::uniform_real_distribution<double> unif(min,max);
    std::default_random_engine re;
    return unif(re);
    //return rand() % max + min;
}

int XMath::middle(int min, int max)
{
    return qRound((min + max)/2.0);
}

int XMath::min(int value1, int value2)
{
    return value1 < value2 ? value1 : value2;
}

int XMath::randSine(double angle)
{
    double amplitude = random(0.0, 100.0);
    if(amplitude < 50)
        angle = reverseNumber(angle, 0, 180);
    return randSine(angle, amplitude);
}

int XMath::randSine(double angle, double amplitude)
{
    //LogHandler::Debug("amplitude: "+ QString::number(amplitude));
    //LogHandler::Debug("angle before: "+ QString::number(angle));
    //LogHandler::Debug("angle after:  "+ QString::number(angle));
    int value = int(amplitude * sin(angle));
    //LogHandler::Debug("value: "+ QString::number(value));
    return value < 0 ? -value : value;
}
int XMath::reverseNumber(int num, int min, int max) {
    return (max + min) - num;
}
//https://stackoverflow.com/questions/4353525/floating-point-linear-interpolation
int XMath::lerp(int start, int end, float t) {
    return start + (int)(t * (float)(end-start));
}

float XMath::calculateSpeed(qint64 timeStart, int posStart, qint64 timeEnd, int posEnd) {
    qint64 timeDiff = timeEnd - timeStart;
    int posDiff = abs(posEnd - posStart);
    if(timeDiff <= 0 || posDiff <= 0)
        return 0;
    return ((float)posDiff / (float)timeDiff) * 100;
}

QString XMath::calculateMD5(QString path)
{
    QCryptographicHash hash(QCryptographicHash::Md5);
    QFile in(path);
    QFileInfo fileInfo(path);
    qint64 imageSize = fileInfo.size();

    const int bufferSize = 10000;
    if (in.open(QIODevice::ReadOnly)) {
        char buf[bufferSize];
        int bytesRead;

        int readSize = min(imageSize, bufferSize);
        while (readSize > 0 && (bytesRead = in.read(buf, readSize)) > 0) {
            imageSize -= bytesRead;
            hash.addData(buf, bytesRead);
            readSize = min(imageSize, bufferSize);
        }

        in.close();
        return QString(hash.result().toHex());
    }
    return nullptr;
}
