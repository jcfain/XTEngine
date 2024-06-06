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
    //Auto tags
    const QString PMV = "pmv";
    const QString MUSIC = "music";
    const QString POV = "pov";
    void addTag(const QString& tag) {
        m_userTags.append(tag);
    }
    bool hasTag(const QString& tag) {
        return m_userTags.contains(tag);
    }
    void removeTag(const QString& tag) {
        m_userTags.removeAll(tag);
    }
    QStringList getAutoTags() {
        return m_autoTags;
    }
    QStringList getUserTags() {
        return m_userTags;
    }
    void clearUserTags() {
        m_userTags.clear();
    }
    QStringList getTags() {
        QStringList allTags;
        foreach (QString tag, m_builtin) {
            allTags.append(tag);
        }
        foreach (QString tag, m_userTags) {
            allTags.append(tag);
        }
        foreach (QString tag, m_autoTags) {
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
        HAS_SCRIPT
    };
    const QStringList m_autoTags = {
        PMV,
        MUSIC,
        POV
    };
    QStringList m_userTags;
};

#endif // XTAGS_H
