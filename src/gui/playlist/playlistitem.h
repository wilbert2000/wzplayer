#ifndef GUI_PLAYLIST_PLAYLISTITEM_H
#define GUI_PLAYLIST_PLAYLISTITEM_H

#include <QString>
#include <QStringList>


namespace Gui {
namespace Playlist {

enum TPlaylistItemState {
    PSTATE_STOPPED,
    PSTATE_LOADING,
    PSTATE_PLAYING,
    PSTATE_FAILED
};

extern Qt::CaseSensitivity caseSensitiveFileNames;
extern int itemOrder;

class TPlaylistItem {
public:
    TPlaylistItem();
    TPlaylistItem(const TPlaylistItem& item);
    TPlaylistItem(const QString& filename,
                  const QString& name,
                  double duration,
                  bool protectName);
    virtual ~TPlaylistItem();

    static QString playlistItemState(TPlaylistItemState state);

    int order() const { return mOrder; }
    void setOrder(int order) { mOrder = order; }

    QString filename() const { return mFilename; }
    void setFilename(const QString& fileName, const QString& baseName);

    QString baseName() const { return mBaseName; }
    void setBaseName(const QString &baseName, bool protectName = false);

    QString extension() const { return mExt; }
    void setExtension(const QString& ext) { mExt = ext; }

    double duration() const { return mDuration; }
    void setDuration(double duration) { mDuration = duration; }

    TPlaylistItemState state() const { return mState; }
    void setState(TPlaylistItemState state);

    bool played() const { return mPlayed; }
    void setPlayed(bool b) { mPlayed = b; }

    bool edited() const { return mEdited; }
    void setEdited(bool b) { mEdited = b; }

    bool folder() const { return mFolder; }
    void setFolder(bool b) { mFolder = b; }

    bool playlist() const { return mPlaylist; }
    bool wzPlaylist() const { return mWZPlaylist; }
    bool symLink() const { return mSymLink; }
    QString target() const { return mTarget; }

    int playedTime() const { return mPlayedTime; }

    void blacklist(const QString& filename) {
        mBlacklist.append(filename);
    }
    bool blacklisted(const QString& filename) const;
    QStringList getBlacklist() const { return mBlacklist; }
    bool whitelist(const QString& filename);

    bool operator == (const TPlaylistItem& item);

private:
    int mOrder;
    QString mFilename;
    QString mBaseName;
    QString mExt;
    double mDuration;
    TPlaylistItemState mState;
    bool mPlayed;
    bool mEdited;
    bool mFolder;
    bool mPlaylist;
    bool mWZPlaylist;
    bool mSymLink;
    QString mTarget;
    int mPlayedTime;
    QStringList mBlacklist;

    void setFileInfo();
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTITEM_H
