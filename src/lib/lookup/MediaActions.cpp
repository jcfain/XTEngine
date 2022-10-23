#include "MediaActions.h"

MediaActions::MediaActions() { }


QMap<QString, QString> MediaActions::GetOtherActions(ActionType type) {
    switch(type) {
    case ActionType::NONE:
        return m_tCodeChannelProfileActions;// Concat other maps in future
    case ActionType::CHANNEL_PROFILE:
        return m_tCodeChannelProfileActions;
    }
}
void MediaActions::AddOtherAction(QString action, QString friendlyName, ActionType type) {
    switch(type) {
    case ActionType::NONE:
        return;
    case ActionType::CHANNEL_PROFILE:
        m_tCodeChannelProfileActions.insert(action, friendlyName);
    }
}

bool MediaActions::HasOtherAction(QString action, ActionType type) {
    switch(type) {
    case ActionType::NONE:
        return m_tCodeChannelProfileActions.contains(action);// || otheractionMap etc
    case ActionType::CHANNEL_PROFILE:
       return m_tCodeChannelProfileActions.contains(action);
    }
    return false;
}

QMap<QString, QString> MediaActions::m_tCodeChannelProfileActions;
