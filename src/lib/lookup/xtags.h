#ifndef XTAGS_H
#define XTAGS_H

#include <QString>
#include <QStringList>

class XTags {
public:
    const static inline QString MFS = "mfs";
    const static inline QString VR = "vr";
    const static inline QString VIDEO_2D = "2d";
    const static inline QString AUDIO = "audio";
    const static inline QString FUNSCRIPT = "funscript";
    //const static inline QString PLAYLIST_INTERNAL = "playlistInternal";
    const static inline QString SUBTITLE = "subtitle";
    const static inline QString MISSING_SCRIPT = "missingScript";
    const static inline QString HAS_SCRIPT = "hasScript";
    const static inline QString VIEWED = "viewed";
    const static inline QString UNVIEWED = "unviewed";
    //Smart tags
    const static inline QString PMV = "pmv";
    const static inline QString MUSIC = "music";
    const static inline QString POV = "pov";
    const static inline QString JOI = "joi";
    QStringList getBuiltInTags() {
        return m_builtin;
    }
    QStringList getBuiltInSmartTags() {
        return m_builtInSmartTags;
    }
    void addTag(const QString& tag) {
        if(m_userTags.contains(tag))
            return;
        m_userTags.append(tag);
    }
    bool hasTag(const QString& tag) {
        return m_userTags.contains(tag);
    }
    void removeTag(const QString& tag) {
        m_userTags.removeAll(tag);
    }
    void addSmartTag(const QString& tag) {
        if(m_userSmartTags.contains(tag))
            return;
        m_userSmartTags.append(tag);
    }
    bool hasSmartTag(const QString& tag) {
        return m_userSmartTags.contains(tag);
    }
    void removeSmartTag(const QString& tag) {
        m_userSmartTags.removeAll(tag);
    }
    QStringList getUserSmartags() {
        return m_userSmartTags;
    }
    QStringList getUserTags() {
        return m_userTags;
    }
    void clearUserTags() {
        m_userTags.clear();
    }
    void clearUserSmartTags() {
        m_userSmartTags.clear();
    }
    QStringList getTags() {
        QStringList allTags;
        foreach (QString tag, m_userTags) {
            allTags.append(tag);
        }
        foreach (QString tag, m_userSmartTags) {
            allTags.append(tag);
        }
        if(!allTags.contains(VIEWED))
            allTags.append(VIEWED);
        if(!allTags.contains(UNVIEWED))
            allTags.append(UNVIEWED);
        return allTags;
    }
private:
    const QStringList m_builtin = {
        MFS,
        VR,
        VIDEO_2D,
        AUDIO,
        FUNSCRIPT,
        //PLAYLIST_INTERNAL,
        SUBTITLE,
        MISSING_SCRIPT,
        HAS_SCRIPT,
        VIEWED,
        UNVIEWED
    };
    const QStringList m_builtInSmartTags = {
        PMV,
        MUSIC,
        POV,
        JOI
    };
    QStringList m_userTags;
    QStringList m_userSmartTags;
};

#endif // XTAGS_H
