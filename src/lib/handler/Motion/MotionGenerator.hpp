#pragma once
#include <cmath>
#include <QString>
#include"QElapsedTimer"
#include "motionChannel.h"
#include "../settingshandler.h"
#include "../loghandler.h"
#include "../../tool/xmath.h"
#include "../tcodehandler.h"

class MotionGenerator {
public:
    void setup(TCodeVersion version, TCodeHandler* tcodeHandler, const MotionChannel& channel)
    {
        m_channel = channel.name;
        tcodeVersion = version;
        m_tcodeHandler = tcodeHandler;
        //setEnabled(SettingsHandler::getMotionEnabled());
        updateProfile(channel);
    }

    void updateProfile(const MotionChannel& channel) {
        //setRange(SettingsHandler::getChannelMin(channel.name), SettingsHandler::getChannelMax(channel.name));
        updateRange();
        setAmplitude(channel.motionAmplitudeGlobal);
        setOffset(channel.motionOffsetGlobal);
        setPeriod(channel.motionPeriodGlobal);
        setUpdate(channel.motionUpdateGlobal);
        setPhase(channel.motionPhaseGlobal);
        setReverse(channel.motionReversedGlobal);
        setAmplitudeRandom(channel.motionAmplitudeGlobalRandom);
        setAmplitudeRandomMin(channel.motionAmplitudeGlobalRandomMin);
        setAmplitudeRandomMax(channel.motionAmplitudeGlobalRandomMax);
        setPeriodRandom(channel.motionPeriodGlobalRandom);
        setPeriodRandomMin(channel.motionPeriodGlobalRandomMin);
        setPeriodRandomMax(channel.motionPeriodGlobalRandomMax);
        setOffsetRandom(channel.motionOffsetGlobalRandom);
        setOffsetRandomMin(channel.motionOffsetGlobalRandomMin);
        setOffsetRandomMax(channel.motionOffsetGlobalRandomMax);
        setPhaseRandom(channel.motionPhaseRandom);
        setPhaseRandomMin(channel.motionPhaseRandomMin);
        setPhaseRandomMax(channel.motionPhaseRandomMax);
    }

    void updateRange() {
        setRange(SettingsHandler::getChannelUserMin(m_channel), SettingsHandler::getChannelUserMax(m_channel));
    }
    QString getMovement() {
        return calculateNext();
    }

    QString getName() {
        return m_channel;
    }

    int getPeriod() {
        if(periodRandomMode) {
            if(millis() > lastRandomPeriodExecutionChange + randomPeriodExecutionPeriod) {
                lastRandomPeriodExecutionChange = millis();
                updatePeriodRandom();
            }
            return periodRandom;
        }
        return period;
    };

    // Offset from center 0
    int getOffset() {
        if(offsetRandomMode) {
            if(millis() > lastRandomOffsetExecutionChange + randomOffsetExecutionPeriod) {
                lastRandomOffsetExecutionChange = millis();
                updateOffsetRandom();
            }
            return offsetRandom;
        }
        return offset;
    };

    // The amplitude of the motion
    int getAmplitude() {
        if(amplitudeRandomMode) {
            if(millis() > lastRandomAmplitudeExecutionChange + randomAmplitudeExecutionPeriod) {
                lastRandomAmplitudeExecutionChange = millis();
                updateAmplitudeRandom();
            }
            return amplitudeRandom;
        }
        return amplitude;
    };

    // Initial phase in degrees. The phase should ideally be between (offset-amplitude/2) and (offset+amplitude/2)
    int getPhase() {
        if(phaseRandomMode) {
            if(millis() > lastRandomPhaseExecutionChange + randomPhaseExecutionPeriod) {
                lastRandomPhaseExecutionChange = millis();
                updatePhaseRandom();
            }
            return phaseRandom;
        }
        return phase;
    };

    // reverse cycle direction 
    bool getReverse() {
        return reversed;
    };

    float getStopAtCycle() {
        return stopAt;
    };

    void setEnabled(bool enable) {
        enabled = enable;
        if(enabled)
            updatePhaseIncrement();
        LogHandler::Debug(m_channel+" setEnabled: " + QString::number(enable));
    }

    void setRange(uint16_t min, uint16_t max) {
        m_min = min;
        m_max = max;
    }

    void setUpdate(int value) {
        updateRate = value;
        updatePhaseIncrement();
        LogHandler::Debug(m_channel + "setUpdate: " + QString::number(updateRate));
    };

    // In miliseconds this is the duty cycle (lower is faster default 2000)
    void setPeriod(int value) {
        period = value; 
        updatePhaseIncrement();
        LogHandler::Debug(m_channel + "setPeriod: " + QString::number(value));
    };

    void setPeriodRandom(bool value) {
        periodRandomMode = value;
        LogHandler::Debug(m_channel + "setPeriodRandom: enabled " + QString::number(value));
        if(value) {
            updatePeriodRandom();
        } else {
            updatePhaseIncrement();
        }
    }
    void setPeriodRandomMin(int min) {
        periodRandomMin = min;
        LogHandler::Debug(m_channel + " setPeriodRandomMin: " + QString::number(min));
        if(periodRandomMode) {
            updatePeriodRandom();
        }
    }
    void setPeriodRandomMax(int max) {
        periodRandomMax = max;
        LogHandler::Debug(m_channel + " setPeriodRandomMax: " + QString::number(max));
        if(periodRandomMode) {
            updatePeriodRandom();
        }
    }

    // Offset from center 0
    void setOffset(int value) {
        mapTCodeToDegrees(value, offset);
        LogHandler::Debug(m_channel + " setOffset: " + QString::number(value) + " calculated degree: " + QString::number(offset));
    };

    void setOffsetRandom(bool value) {
        offsetRandomMode = value;
        LogHandler::Debug(m_channel + " setOffsetRandom: enabled " + QString::number(value));
        if(value) {
            updateOffsetRandom();
        }
    }
    void setOffsetRandomMin(int min) {
        mapTCodeToDegrees(min, offsetRandomMin);
        LogHandler::Debug(m_channel + " setOffsetRandomMin: "+ QString::number(min) + " calculated degree: " + QString::number(offsetRandomMin));
        if(offsetRandomMode) {
            updateOffsetRandom();
        }
    }
    void setOffsetRandomMax(int max) {
        mapTCodeToDegrees(max, offsetRandomMax);
        LogHandler::Debug(m_channel + " setOffsetRandomMax: "+QString::number(max)+" calculated degree: " + QString::number(offsetRandomMax));
        if(offsetRandomMode) {
            updateOffsetRandom();
        }
    }

    // The amplitude of the motion
    void setAmplitude(int value) {
        amplitude = XMath::mapRange(value, 0, 100, 0, 90);
        LogHandler::Debug(m_channel + " setAmplitude: "+QString::number(value)+" calculated: " + QString::number(amplitude));
    };

    void setAmplitudeRandom(bool value) {
        amplitudeRandomMode = value;
        LogHandler::Debug(m_channel + "setAmplitudeRandom: enabled " + QString::number(value));
        if(value) {
            updateAmplitudeRandom();
        }
    }
    void setAmplitudeRandomMin(int min) {
        amplitudeRandomMin = XMath::mapRange(min, 0, 100, 0, 90);
        LogHandler::Debug(m_channel + " setAmplitudeRandomMin: "+QString::number(min)+" calculated: " + QString::number(amplitudeRandomMin));
        if(amplitudeRandomMode) {
            updateAmplitudeRandom();
        }
    }
    void setAmplitudeRandomMax(int max) {
        amplitudeRandomMax = XMath::mapRange(max, 0, 100, 0, 90);
        LogHandler::Debug(m_channel + " setAmplitudeRandomMax: "+QString::number(max)+" calculated: " + QString::number(amplitudeRandomMax));
        if(amplitudeRandomMode) {
            updateAmplitudeRandom();
        }
    }

    void setMotionRandomChangeMin(int min) {
        motionRandomChangeMin = min;
        LogHandler::Debug(m_channel + " setMotionRandomChangeMin: " + QString::number(min));
        if(amplitudeRandomMode || periodRandomMode || offsetRandomMode) {
            lastExecutionPeriodChange = 0;
            checkUpdateRandomExecutionPeriod();
        }
    }
    void setMotionRandomChangeMax(int max) {
        motionRandomChangeMax = max;
        LogHandler::Debug(m_channel + " setMotionRandomChangeMax: " + QString::number(max));
        if(amplitudeRandomMode) {
            lastExecutionPeriodChange = 0;
            checkUpdateRandomExecutionPeriod();
        }
    }

    // Initial phase in degrees. The phase should ideally be between (offset-amplitude/2) and (offset+amplitude/2)
    void setPhase(int value) {
        phase = degreesToRadian(value);
        LogHandler::Debug(m_channel + "setPhase: " + QString::number(value));
    };
    void setPhaseRandom(bool value) {
        phaseRandomMode = value;
        LogHandler::Debug(m_channel + "setPhaseRandom: enabled " + QString::number(value));
        if(value) {
            updatePhaseRandom();
        }
    }
    void setPhaseRandomMin(int min) {
        phaseRandomMin = min;
        LogHandler::Debug(m_channel + "setPhaseRandomMin: " + QString::number(min));
        if(offsetRandomMode) {
            updatePhaseRandom();
        }
    }
    void setPhaseRandomMax(int max) {
        phaseRandomMax = max;
        LogHandler::Debug(m_channel + "setPhaseRandomMax: " + QString::number(max));
        if(offsetRandomMode) {
            updatePhaseRandom();
        }
    }

    // reverse cycle direction 
    void setReverse(bool value) {
        reversed = value;
        LogHandler::Debug(m_channel + "setReverse: " + QString::number(value));
    };

    void stopAtCycle(float value) {
        stopAt = currentPhase + 2*PI*value;
        LogHandler::Debug(m_channel + "stopAtCycle: "+QString::number(value)+", stopAt " + QString::number(stopAt));
    };

private:
    TCodeVersion tcodeVersion;
    TCodeHandler* m_tcodeHandler;
    bool enabled = false;
    uint16_t m_min;
    uint16_t m_max;
    QString m_channel;
    int updateRate = 100;
    int period = 2000;    
    int periodRandom = 2000;    
    bool periodRandomMode = false;
    int periodRandomMin = 500;
    int periodRandomMax = 6000;
    int amplitude = 60;  
    int amplitudeRandom = 60; 
    bool amplitudeRandomMode = false;
    int amplitudeRandomMin = 20;
    int amplitudeRandomMax = 60;
    int offset = 0;      
    int offsetRandom = 0; 
    bool offsetRandomMode = false;
    int offsetRandomMin = 20;
    int offsetRandomMax = 80;
    float phase = 0;  
    float phaseRandom = 0;
    bool phaseRandomMode = false;
    float phaseRandomMin = 0;
    float phaseRandomMax = 180;
    bool reversed = false;
    
    int motionRandomChangeMin = 3000;
    int motionRandomChangeMax = 10000;

//Internally updated
    long lastExecutionPeriodChangePeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
    int randomPeriodExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
    int randomAmplitudeExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
    int randomOffsetExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
    int randomPhaseExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
    long lastExecutionPeriodChange = millis();
    long lastRandomPeriodExecutionChange = millis();
    long lastRandomAmplitudeExecutionChange = millis();
    long lastRandomOffsetExecutionChange = millis();
    long lastRandomPhaseExecutionChange = millis();
    // The current phase angle (radians)
    float currentPhase = 0.0;    
    // By how much to increment phase on every position update
    float phaseIncrement = 1;    
    // Will be true if the oscillation is stopped 
    bool stopped = false;       
    // Set a cycle to stop the movement
    float stopAt = 0;  
    // Last time the servo position was updated   
    int lastUpdate = 0; 
    // The time in between the last update and the current update
    int interval = 0;

    QElapsedTimer m_timer;

    qint64 millis() {
        return round(m_timer.nsecsElapsed() / 1000000);
    }
    
    void updateOffsetRandom() {
        offsetRandom = XMath::random(offsetRandomMin, offsetRandomMax);
        LogHandler::Debug(m_channel + " New random offset " + QString::number(offsetRandom));
    }
    void updateAmplitudeRandom() {
        amplitudeRandom = XMath::random(amplitudeRandomMin, amplitudeRandomMax);
        LogHandler::Debug(m_channel + " New random amplitude " + QString::number(amplitudeRandom));
    }
    void updatePeriodRandom() {
        periodRandom = XMath::random(periodRandomMin, periodRandomMax);
        LogHandler::Debug(m_channel + " New random period " + QString::number(periodRandom));
        updatePhaseIncrement();
    }
    void updatePhaseRandom() {
        phaseRandom = XMath::random(phaseRandomMin, phaseRandomMax);
        LogHandler::Debug(m_channel + " New random phase " + QString::number(periodRandom));
    }
    /** Gate the random change period between all attributes in between two values. 
     * If this random value time outs and any attribute hasnt timed out yet,
     * it will wait till the next timeout. */
    void checkUpdateRandomExecutionPeriod() {
        if((offsetRandomMode || amplitudeRandomMode || periodRandomMode)  
            && millis() > lastExecutionPeriodChange + lastExecutionPeriodChangePeriod) {

            if(offsetRandomMode && millis() > lastRandomOffsetExecutionChange + randomOffsetExecutionPeriod)
                randomPeriodExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
            if(amplitudeRandomMode && millis() > lastRandomAmplitudeExecutionChange + randomAmplitudeExecutionPeriod)
                randomAmplitudeExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
            if(periodRandomMode && millis() > lastRandomPeriodExecutionChange + randomPeriodExecutionPeriod) 
                randomOffsetExecutionPeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);

            lastExecutionPeriodChangePeriod = XMath::random(motionRandomChangeMin, motionRandomChangeMax);
        }
    }

    QString calculateNext() {
        if(!canUpdate()) {
            return nullptr;
        }
        QString buf;

        checkUpdateRandomExecutionPeriod();
        if(periodRandomMode)
            getPeriod(); // Just call this to check for random updates.

        if (!stopped) {
            int pos = XMath::constrain((int)(getAmplitude() * sin(currentPhase + getPhase()) + getOffset()), -90, 90);
            if (getReverse())
                pos = -pos;
            int tcodeValue = XMath::constrain((uint16_t)XMath::mapRange(pos, -90, 90, 0, TCodeChannelLookup::getTCodeMaxValue()), m_min, m_max);
            QString buf  = m_tcodeHandler->formatTCode(m_channel, tcodeValue, "I", interval);

            LogHandler::Debug(m_channel + "pos: " + QString::number(pos));
            LogHandler::Debug(m_channel + "buf: " + buf);
        }
        currentPhase = currentPhase + phaseIncrement;

        if (stopAt && currentPhase > stopAt) {
            stopped = true;
            stopAt = 0;
        }
        return buf;
    }

    bool canUpdate() {
        if(enabled) {
            uint32_t now = millis();

            //LogHandler::Debug(m_channel + "enabled interval %ld lastUpdate  %ld now  %ld" , m_channel, interval, lastUpdate, now);
            interval = now - lastUpdate;
            if(interval > updateRate) {
                lastUpdate = now;
                return true;
            }
        }
        //LogHandler::Debug(m_channel + "cant update %ld" , m_channel, enabled);
        return false;
    }

    /** If the user has changed the global settings update accordingly */
    // void checkLiveUpdate() {
    //     if(SettingsHandler::motionPeriodGlobalRandom) {
    //         if(millis() > lastRandomPeriodExecutionChange + randomPeriodExecutionPeriod) {
    //             lastRandomPeriodExecutionChange = millis();
    //             updatePeriodRandom();
    //             updatePhaseIncrement();
    //         }
    //     } else {
    //         if(period != SettingsHandler::motionPeriodGlobal || updateRate != SettingsHandler::motionUpdateGlobal) {
    //             if(updateRate != SettingsHandler::motionUpdateGlobal) {
    //                 updateRate = SettingsHandler::motionUpdateGlobal;
    //             }
    //             if(period != SettingsHandler::motionPeriodGlobal) {
    //                 period = SettingsHandler::motionPeriodGlobal;
    //             }
    //             updatePhaseIncrement();
    //         }
    //     }
    //     if(phase != SettingsHandler::motionPhaseGlobal)
    //         setPhase(SettingsHandler::motionPhaseGlobal);
    // }

    void updatePhaseIncrement() {
        LogHandler::Debug(m_channel+" Update phase increment: period '"+QString::number(getPeriod())+"' updateRate " + QString::number(updateRate));
        phaseIncrement = 2.0*M_PI / ((float)getPeriod() / updateRate);
        LogHandler::Debug(m_channel+" New phase increment: " + QString::number(phaseIncrement));
    }

    int degreesToRadian(int degrees) {
        return ((degrees)*M_PI)/180;
    }

void mapTCodeToDegrees(int tcode, int &degreeVariable) {
    if(tcodeVersion == TCodeVersion::v2) {
        if(tcode > 999) {//Lazy Hack
            tcode = XMath::mapRange(tcode, 0, 9999, 0, 999);
        }
        degreeVariable = XMath::mapRange(tcode, 0, 999, -90, 90);
    } else {
        degreeVariable = XMath::mapRange(tcode, 0, 9999, -90, 90);
    }
}
    // float w_sweep(float t, float f0, float t0, float f1, float t1) {
    //     float const freq = Lerp(f0, f1, (t - t0)/(t1 - t0));
    //     return std::sin(PI * (f0 + freq) * (t-t0));
    // }

    // /// <summary>
    // /// Linearly interpolates a value between two floats
    // /// </summary>
    // /// <param name="start_value">Start value</param>
    // /// <param name="end_value">End value</param>
    // /// <param name="pct">Our progress or percentage. [0,1]</param>
    // /// <returns>Interpolated value between two floats</returns>
    // float Lerp(float start_value, float end_value, float pct) {
    //     return (start_value + (end_value - start_value) * pct);
    // }

    // float EaseIn(float t) {
    //     return t * t;
    // }
    // float EaseOut(float t) {
    //     return Flip(Square(Flip(t)));
    // }
    // // float EaseInOut(float t)
    // // {
    // //     return Lerp(EaseIn(t), EaseOut(t), t);
    // // }
    // float Flip(float x) {
    //     return 1 - x;
    // }
    // float Square(float x) {
    //     return pow(x, 2);
    // }
};

