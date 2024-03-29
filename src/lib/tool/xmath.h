#ifndef XMATH_H
#define XMATH_H
#include <QtGlobal>
#include <random>
#include <chrono>
#include "../handler/loghandler.h"
#include "XTEngine_global.h"

#define PI 3.141592653589793
class XTENGINE_EXPORT XMath
{
public:
    static int mapRange(int value, int inStart, int inEnd, int outStart, int outEnd);
    static qint64 mapRange(qint64 value, qint64 inStart, qint64 inEnd, qint64 outStart, qint64 outEnd);
    static qint64 mapRange(double value, double inStart, double inEnd, int outStart, int outEnd);
    static int constrain(int value, int min, int max);
    static int rand(int min, int max);
    static int middle(int min, int max);
    static int min(int value1, int value2);
    static qint64 rand(qint64 min, qint64 max);
    static double rand(double min, double max);
    static int randSine(double base);
    static int randSine(double base, double amplitude);
    static int reverseNumber(int num, int min, int max);
    static int lerp(int a, int b, float f);
    static float calculateSpeed(qint64 timeStart, int posStart, qint64 timeEnd, int posEnd);
};

#endif // XMATH_H
