#pragma once

#include <QMutex>
#include <QString>
#include "MotionGenerator.hpp"
#include "motionChannel.h"
#include "../loghandler.h"
#include "../tcodehandler.h"

class MotionHandler {
public:
    void setup(TCodeVersion version, TCodeHandler* tcodeHandler)
    {
        m_tcodeVersion = version;
        m_tcodeHandler = tcodeHandler;
        //setMotionChannels(SettingsHandler::getMotionChannels());
    }

    QString getMovement() {
        QMutexLocker locker(&m_mutex);
        QString buf;
        LogHandler::Debug("getMovement Enter");
        if(!enabled || !m_motionGenerators.size()) {
            return buf;
        }
        for (int i = 0; i < m_motionGenerators.size(); i++) {
            QString temp = m_motionGenerators[i].getMovement();
            if(temp.isEmpty())
                continue;
            buf += temp;
            buf += " ";
        }
        buf += '\n';
        LogHandler::Debug("Exit " + buf);
        return buf;
    }
    
    void setMotionChannels(const QList<MotionChannel> &motionChannels) {
        QMutexLocker locker(&m_mutex);
        bool enabledBak = enabled;
        enabled = false;
        m_motionGenerators.clear();
        for (int i = 0; i < motionChannels.count(); i++)
        {
            LogHandler::Debug("Setup motion channel: " + motionChannels[i].name);
            MotionGenerator motionGenerator;
            motionGenerator.setup(m_tcodeVersion, motionChannels[i]);
            m_motionGenerators.push_back(motionGenerator);
        }
        enabled = enabledBak;
    }

    void setEnabled(bool enable, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
            m_motionGenerators[index].setEnabled(enable);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setEnabled(enable);
            }
        }
        LogHandler::Debug("setEnabled: " + QString::number(enable));
    }

    void setUpdate(int value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setUpdate(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setUpdate(value);
            }
        }
        LogHandler::Debug("setUpdate: " + QString::number(value));
    };

    // In miliseconds this is the duty cycle (lower is faster default 2000)
    void setPeriod(int value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPeriod(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPeriod(value);
            }
        }
        LogHandler::Debug("setPeriod: " + QString::number(value));
    };

    void setPeriodRandom(bool value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPeriodRandom(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPeriodRandom(value);
            }
        }
        LogHandler::Debug("setPeriodRandom: enabled " + QString::number(value));
    }

    void setPeriodRandomMin(int min, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPeriodRandomMin(min);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPeriodRandomMin(min);
            }
        }
        LogHandler::Debug("setPeriodRandomMin: " + QString::number(min));
    }

    void setPeriodRandomMax(int max, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPeriodRandomMax(max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPeriodRandomMax(max);
            }
        }
        LogHandler::Debug("setPeriodRandomMax: " + QString::number(max));
    }

    // Offset from center 0
    void setOffset(int value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setOffset(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setOffset(value);
            }
        }
        LogHandler::Debug("setOffset: " + QString::number(value));
    };

    void setOffsetRandom(bool value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setOffsetRandom(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setOffsetRandom(value);
            }
        }
        LogHandler::Debug("setOffsetRandom: enabled " + QString::number(value));
    }

    void setOffsetRandomMin(int min, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setOffsetRandomMin(min);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setOffsetRandomMin(min);
            }
        }
        LogHandler::Debug("setOffsetRandomMin: " + QString::number(min));
    }

    void setOffsetRandomMax(int max, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setOffsetRandomMax(max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setOffsetRandomMax(max);
            }
        }
        LogHandler::Debug("setOffsetRandomMax: " + QString::number(max));
    }

    // The amplitude of the motion
    void setAmplitude(int value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setAmplitude(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setAmplitude(value);
            }
        }
        LogHandler::Debug("setAmplitude: " + QString::number(value));
    };

    void setAmplitudeRandom(bool value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setAmplitudeRandom(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setAmplitudeRandom(value);
            }
        }
        LogHandler::Debug("setAmplitudeRandom: enabled " + QString::number(value));
    }

    void setAmplitudeRandomMin(int min, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setAmplitudeRandomMin(min);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setAmplitudeRandomMin(min);
            }
        }
        LogHandler::Debug("setAmplitudeRandomMin: " + QString::number(min));
    }

    void setAmplitudeRandomMax(int max, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setAmplitudeRandomMax(max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setAmplitudeRandomMax(max);
            }
        }
        LogHandler::Debug("setAmplitudeRandomMax: " + QString::number(max));
    }

    void setMotionRandomChangeMin(int min, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setMotionRandomChangeMin(min);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setMotionRandomChangeMin(min);
            }
        }
        LogHandler::Debug("setMotionRandomChangeMin: " + QString::number(min));
    }

    void setMotionRandomChangeMax(int max, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setMotionRandomChangeMax(max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setMotionRandomChangeMax(max);
            }
        }
        LogHandler::Debug("setMotionRandomChangeMax: " + QString::number(max));
    }

    // Initial phase in degrees. The phase should ideally be between (offset-amplitude/2) and (offset+amplitude/2)
    void setPhase(int value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPhase(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPhase(value);
            }
        }
        LogHandler::Debug("setPhase: " + QString::number(value));
    };

    void setPhaseRandom(bool value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPhaseRandom(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPhaseRandom(value);
            }
        }
        LogHandler::Debug("setOffsetRandom: enabled " + QString::number(value));
    }

    void setPhaseRandomMin(int min, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPhaseRandomMin(min);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPhaseRandomMin(min);
            }
        }
        LogHandler::Debug("setOffsetRandomMin: " + QString::number(min));
    }

    void setPhaseRandomMax(int max, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setPhaseRandomMax(max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setPhaseRandomMax(max);
            }
        }
        LogHandler::Debug("setOffsetRandomMax: " + QString::number(max));
    }

    // reverse cycle direction 
    void setReverse(bool value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setReverse(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].setReverse(value);
            }
        }
        LogHandler::Debug("setReverse: " + QString::number(value));
    };

    void stopAtCycle(float value, const QString name = nullptr) {
        QMutexLocker locker(&m_mutex);
        if(!name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].stopAtCycle(value);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].stopAtCycle(value);
            }
        }
        LogHandler::Debug("stopAtCycle: " + QString::number(value));
    };

    void updateChannelRanges(const QString name, int min = -1, int max = -1) {
        QMutexLocker locker(&m_mutex);
        if(min > -1 && max > -1 && !name.isEmpty()) {
            auto index = getMotionGeneratorIndex(name);
            if(index > -1)
                m_motionGenerators[index].setRange(min, max);
        } else {
            for (int i = 0; i < m_motionGenerators.size(); i++) {
                m_motionGenerators[i].updateRange();
            }
        }
    }

private:
    QList<MotionGenerator> m_motionGenerators;
    TCodeVersion m_tcodeVersion;
    TCodeHandler* m_tcodeHandler;
    bool enabled = false;
    static QMutex m_mutex;

    int getMotionGeneratorIndex(QString name) {
        for (int i = 0; i < m_motionGenerators.count(); i++)
        {
            if(m_motionGenerators[i].getName() == name) {
                return i;
            }
        }
        return -1;
    }
};

