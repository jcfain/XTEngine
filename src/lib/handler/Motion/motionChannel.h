/* MIT License

Copyright (c) 2024 Jason C. Fain

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE. */

#pragma once

#include <QJsonObject>

#define motionUpdateGlobalDefault 100
#define motionPeriodGlobalDefault 2000
#define motionAmplitudeGlobalDefault 60
#define motionOffsetGlobalDefault 5000
#define motionPeriodGlobalRandomDefault false
#define motionPeriodGlobalRandomMinDefault 500
#define motionPeriodGlobalRandomMaxDefault 2000
#define motionAmplitudeGlobalRandomDefault false
#define motionAmplitudeGlobalRandomMinDefault 20
#define motionAmplitudeGlobalRandomMaxDefault 60
#define motionOffsetGlobalRandomDefault false
#define motionOffsetGlobalRandomMinDefault 3000
#define motionOffsetGlobalRandomMaxDefault 7000
#define motionPhaseGlobalDefault 0.0f
#define motionPhaseRandomDefault false
#define motionPhaseRandomMinDefault 0.0f
#define motionPhaseRandomMaxDefault 180.0f
#define motionReversedGlobalDefault false
#define motionRandomChangeMinDefault 3000
#define motionRandomChangeMaxDefault 30000

struct MotionChannel {
    MotionChannel() { }
    MotionChannel(const QString& nameIn) {
        name = nameIn;
    }
    QString name;
    bool edited = false;
    // const int motionUpdateGlobalDefault = 100;
    // const int motionPeriodGlobalDefault = 2000;
    // const int motionAmplitudeGlobalDefault = 60;
    // const int motionOffsetGlobalDefault = 5000;
    // const bool motionPeriodGlobalRandomDefault = false;
    // const int motionPeriodGlobalRandomMinDefault = 500;
    // const int motionPeriodGlobalRandomMaxDefault = 2000;
    // const bool motionAmplitudeGlobalRandomDefault = false;
    // const int motionAmplitudeGlobalRandomMinDefault = 20;
    // const int motionAmplitudeGlobalRandomMaxDefault = 60;
    // const bool motionOffsetGlobalRandomDefault = false;
    // const int motionOffsetGlobalRandomMinDefault = 3000;
    // const int motionOffsetGlobalRandomMaxDefault = 7000;
    // const float motionPhaseGlobalDefault = 0;
    // const bool motionReversedGlobalDefault = false;
    // const int motionRandomChangeMinDefault = 3000;
    // const int motionRandomChangeMaxDefault = 30000;

    int motionUpdateGlobal = motionUpdateGlobalDefault;
    int motionPeriodGlobal = motionPeriodGlobalDefault;
    int motionAmplitudeGlobal = motionAmplitudeGlobalDefault;
    int motionOffsetGlobal = motionOffsetGlobalDefault;
    bool motionPeriodGlobalRandom = motionPeriodGlobalRandomDefault;
    int motionPeriodGlobalRandomMin = motionPeriodGlobalRandomMinDefault;
    int motionPeriodGlobalRandomMax = motionPeriodGlobalRandomMaxDefault;
    bool motionAmplitudeGlobalRandom = motionAmplitudeGlobalRandomDefault;
    int motionAmplitudeGlobalRandomMin = motionAmplitudeGlobalRandomMinDefault;
    int motionAmplitudeGlobalRandomMax = motionAmplitudeGlobalRandomMaxDefault;
    bool motionOffsetGlobalRandom = motionOffsetGlobalRandomDefault;
    int motionOffsetGlobalRandomMin = motionOffsetGlobalRandomMinDefault;
    int motionOffsetGlobalRandomMax = motionOffsetGlobalRandomMaxDefault;
    float motionPhaseGlobal = motionPhaseGlobalDefault;
    bool motionPhaseRandom = motionPhaseRandomDefault;
    float motionPhaseRandomMin = motionPhaseRandomMinDefault;
    float motionPhaseRandomMax = motionPhaseRandomMaxDefault;
    bool motionReversedGlobal = motionReversedGlobalDefault;
    int motionRandomChangeMin = motionRandomChangeMinDefault;
    int motionRandomChangeMax = motionRandomChangeMaxDefault;

    void toJson(QJsonObject &obj) {
        obj["name"] = name;
        obj["edited"] = edited;
        obj["update"] = motionUpdateGlobal;
        obj["period"] = motionPeriodGlobal;
        obj["amp"] = motionAmplitudeGlobal;
        obj["offset"] = motionOffsetGlobal;
        obj["phase"] = motionPhaseGlobal;
        obj["phaseRan"] = motionPhaseRandom;
        obj["phaseMin"] = motionPhaseRandomMin;
        obj["phaseMax"] = motionPhaseRandomMax;
        obj["reverse"] = motionReversedGlobal;
        obj["periodRan"] = motionPeriodGlobalRandom;
        obj["periodMin"] = motionPeriodGlobalRandomMin;
        obj["periodMax"] = motionPeriodGlobalRandomMax;
        obj["ampRan"] = motionAmplitudeGlobalRandom;
        obj["ampMin"] = motionAmplitudeGlobalRandomMin;
        obj["ampMax"] = motionAmplitudeGlobalRandomMax;
        obj["offsetRan"] = motionOffsetGlobalRandom;
        obj["offsetMin"] = motionOffsetGlobalRandomMin;
        obj["offsetMax"] = motionOffsetGlobalRandomMax;
        obj["ranMin"] = motionRandomChangeMin;
        obj["ranMax"] = motionRandomChangeMax;
    }

    void fromJson(QJsonObject obj) {
        name = obj["name"].toString("X0");
        edited = obj["edited"].toBool(false);
        motionUpdateGlobal = obj["update"].toInt(motionUpdateGlobalDefault);
        motionPeriodGlobal = obj["period"].toInt(motionPeriodGlobalDefault);
        motionAmplitudeGlobal = obj["amp"].toInt(motionAmplitudeGlobalDefault);
        motionOffsetGlobal = obj["offset"].toInt(motionOffsetGlobalDefault);
        motionPhaseGlobal = obj["phase"].toDouble(motionPhaseGlobalDefault);
        motionPhaseRandom = obj["phaseRan"].toBool(motionPhaseRandomDefault);
        motionPhaseRandomMin = obj["phaseMin"].toDouble(motionPhaseRandomMinDefault);
        motionPhaseRandomMax = obj["phaseMax"].toDouble(motionPhaseRandomMaxDefault);
        motionReversedGlobal = obj["reverse"].toBool(motionReversedGlobalDefault);
        motionPeriodGlobalRandom = obj["periodRan"].toBool(motionPeriodGlobalRandomDefault);
        motionPeriodGlobalRandomMin = obj["periodMin"].toInt(motionPeriodGlobalRandomMinDefault);
        motionPeriodGlobalRandomMax = obj["periodMax"].toInt(motionPeriodGlobalRandomMaxDefault);
        motionAmplitudeGlobalRandom = obj["ampRan"].toBool(motionAmplitudeGlobalRandomDefault);
        motionAmplitudeGlobalRandomMin = obj["ampMin"].toInt(motionAmplitudeGlobalRandomMinDefault);
        motionAmplitudeGlobalRandomMax = obj["ampMax"].toInt(motionAmplitudeGlobalRandomMaxDefault);
        motionOffsetGlobalRandom = obj["offsetRan"].toBool(motionOffsetGlobalRandomDefault);
        motionOffsetGlobalRandomMin = obj["offsetMin"].toInt(motionOffsetGlobalRandomMinDefault);
        motionOffsetGlobalRandomMax = obj["offsetMax"].toInt(motionOffsetGlobalRandomMaxDefault);
        motionRandomChangeMin = obj["ranMin"].toInt(motionRandomChangeMinDefault);
        motionRandomChangeMax = obj["ranMax"].toInt(motionRandomChangeMaxDefault);
    }
};
