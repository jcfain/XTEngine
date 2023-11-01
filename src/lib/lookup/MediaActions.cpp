#include "MediaActions.h"

MediaActions::MediaActions() { }


QMap<QString, QString> MediaActions::GetOtherActions(ActionType type) {
    switch(type) {
        case ActionType::NONE: {
            m_allOtherActions.clear();
            m_allOtherActions.insert(m_tCodeChannelProfileActions);
            m_allOtherActions.insert(m_tCodeActions);
            return m_allOtherActions;// Concat other maps in future
        }
        case ActionType::CHANNEL_PROFILE:
            return m_tCodeChannelProfileActions;
        case ActionType::TCODE:
            return m_tCodeActions;
    }
}
void MediaActions::AddOtherAction(QString action, QString friendlyName, ActionType type) {
    switch(type) {
        case ActionType::NONE:
            return;
        case ActionType::CHANNEL_PROFILE:
            m_tCodeChannelProfileActions.insert(action, friendlyName);
            break;
        case ActionType::TCODE:
            m_tCodeActions.insert(action, friendlyName);
            break;
    }
}

void MediaActions::RemoveOtherAction(QString action, ActionType type)
{
    switch(type) {
    case ActionType::NONE:{
        m_tCodeChannelProfileActions.remove(action);
        m_tCodeActions.remove(action);
        break;
    }
    case ActionType::CHANNEL_PROFILE:
        m_tCodeChannelProfileActions.remove(action);
        break;
    case ActionType::TCODE:
        m_tCodeActions.remove(action);
        break;
    }
}

void MediaActions::EditOtherAction(QString action, QString newAction, QString newFriendlyName, ActionType type)
{
    switch(type) {
    case ActionType::NONE:{
        if(m_tCodeChannelProfileActions.contains(action)) {
            m_tCodeChannelProfileActions.remove(action);
            m_tCodeChannelProfileActions.insert(newAction, newFriendlyName);
        }
        if(m_tCodeActions.contains(action)) {
            m_tCodeActions.remove(action);
            m_tCodeActions.insert(newAction, newFriendlyName);
        }
        break;
    }
    case ActionType::CHANNEL_PROFILE: {
        m_tCodeChannelProfileActions.remove(action);
        m_tCodeChannelProfileActions.insert(newAction, newFriendlyName);
        break;
    }
    case ActionType::TCODE: {
        m_tCodeActions.remove(action);
        m_tCodeActions.insert(newAction, newFriendlyName);
        break;
    }
    }
}

bool MediaActions::HasOtherAction(QString action, ActionType type) {
    switch(type) {
        case ActionType::NONE: {
            m_allOtherActions.clear();
            m_allOtherActions.insert(m_tCodeChannelProfileActions);
            m_allOtherActions.insert(m_tCodeActions);
            return m_allOtherActions.contains(action);// || otheractionMap etc
        }
        case ActionType::CHANNEL_PROFILE:
           return m_tCodeChannelProfileActions.contains(action);
        case ActionType::TCODE:
            return m_tCodeActions.contains(action);
    }
    return false;
}

QMap<QString, QString> MediaActions::m_allOtherActions;
QMap<QString, QString> MediaActions::m_tCodeChannelProfileActions;
QMap<QString, QString> MediaActions::m_tCodeActions;
