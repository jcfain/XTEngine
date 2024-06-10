#ifndef XTAGS_H
#define XTAGS_H

#include <QString>
#include <QStringList>

class XTags {
public:
    const QString MFS = "mfs";
    const QString VR = "vr";
    const QString VIDEO_2D = "2d";
    const QString AUDIO = "audio";
    const QString FUNSCRIPT = "funscript";
    const QString PLAYLIST_INTERNAL = "playlistInternal";
    const QString SUBTITLE = "subtitle";
    const QString MISSING_SCRIPT = "missingScript";
    const QString HAS_SCRIPT = "hasScript";
    const QString VIEWED = "viewed";
    const QString UNVIEWED = "unviewed";
    //Auto tags
    const QString PMV = "pmv";
    const QString MUSIC = "music";
    const QString POV = "pov";
    const QString JOI = "joi";
    QStringList getBuiltInTags() {
        return m_builtin;
    }
    QStringList getBuiltInSmartTags() {
        return m_builtInSmartTags;
    }
    void addTag(const QString& tag) {
        m_userTags.append(tag);
    }
    bool hasTag(const QString& tag) {
        return m_userTags.contains(tag);
    }
    void removeTag(const QString& tag) {
        m_userTags.removeAll(tag);
    }
    void addSmartTag(const QString& tag) {
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
        return allTags;
    }
private:
    const QStringList m_builtin = {
        MFS,
        VR,
        VIDEO_2D,
        AUDIO,
        FUNSCRIPT,
        PLAYLIST_INTERNAL,
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
