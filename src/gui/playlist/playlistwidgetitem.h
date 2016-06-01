#ifndef GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
#define GUI_PLAYLIST_PLAYLISTWIDGETITEM_H

#include <QString>
#include <QTreeWidgetItem>
#include <QIcon>


namespace Gui {
namespace Playlist {


enum TPlaylistItemState {
    PSTATE_STOPPED,
    PSTATE_LOADING,
    PSTATE_PLAYING,
    PSTATE_FAILED
};


class TPlaylistItem {

public:
    TPlaylistItem();
    TPlaylistItem(const QString &filename,
                  const QString &name,
                  double duration,
                  bool isFolder,
                  bool protectName);
    virtual ~TPlaylistItem() {}

    QString filename() const { return mFilename; }
    void setFilename(const QString &filename);

    QString name() const { return mName; }
    void setName(const QString &name, bool protectName = false);

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

    int playedTime() const { return mPlayedTime; }

    void blacklist(const QString& filename) {
        mBlacklist.append(filename);
    }
    bool blacklisted(const QString& filename) const;
    QStringList getBlacklist() const { return mBlacklist; }
    void whitelist(const QString& filename);

    bool operator == (const TPlaylistItem& item);

private:
    QString mFilename, mName;
    double mDuration;
    TPlaylistItemState mState;
    bool mPlayed, mEdited, mFolder, mPlaylist;
    int mPlayedTime;
    QStringList mBlacklist;
};


extern int gRootNodeLevel;
extern int gNameColumnWidth;
extern QFontMetrics gNameFontMetrics;
extern QSize gIconSize;

extern QIcon okIcon;
extern QIcon loadingIcon;
extern QIcon playIcon;
extern QIcon failedIcon;

// TODO: root selectable or not...
const Qt::ItemFlags ROOT_FLAGS = Qt::ItemIsSelectable
                                 | Qt::ItemIsEnabled
                                 | Qt::ItemIsDropEnabled;

class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_NAME = 0,
        COL_TIME = 1,
        COL_COUNT = 2
    };

    TPlaylistWidgetItem();
    TPlaylistWidgetItem(QTreeWidgetItem* parent,
                        const QString& filename,
                        const QString& name,
                        double duration,
                        bool isDir,
                        const QIcon& icon,
                        bool protectName = false);
    virtual ~TPlaylistWidgetItem();

    QString filename() const { return playlistItem.filename(); }
    void setFilename(const QString& filename);

    QString path() const;
    QString fname() const;

    QString name() const { return playlistItem.name(); }
    void setName(const QString& name, bool protectName = false);

    double duration() const { return playlistItem.duration(); }
    void setDuration(double d);

    TPlaylistItemState state() const { return playlistItem.state(); }
    void setState(TPlaylistItemState state);

    bool played() const { return playlistItem.played(); }
    void setPlayed(bool played);

    bool edited() const { return playlistItem.edited(); }
    void setEdited(bool edited) { playlistItem.setEdited(edited); }

    bool modified() const { return mModified; }
    void setModified(bool modified = true,
                     bool recurse = false,
                     bool markParents = true);

    bool isRoot() const;
    bool isFolder() const { return playlistItem.folder(); }
    bool isPlaylist() const { return playlistItem.playlist(); }
    bool isWZPlaylist() const;

    int playedTime() const { return playlistItem.playedTime(); }

    void blacklist(const QString& filename) {
        playlistItem.blacklist(filename);
    }
    bool blacklisted(const QString& filename) const {
        return playlistItem.blacklisted(filename);
    }
    QStringList getBlacklist() const { return playlistItem.getBlacklist(); }
    void whitelist(const QString& filename) {
        playlistItem.whitelist(filename);
    }

    static QSize itemSize(const QString& text,
                          int width,
                          const QFontMetrics& fm,
                          const QSize& iconSize,
                          int level);
    void setSzHint(int level);
    int getLevel() const;

    TPlaylistWidgetItem* plParent() const {
        return static_cast<TPlaylistWidgetItem*>(parent());
    }
    TPlaylistWidgetItem* plChild(int idx) const {
        return static_cast<TPlaylistWidgetItem*>(child(idx));
    }

    virtual bool operator<(const QTreeWidgetItem& other) const;
    // TODO: override clone?

private:
    TPlaylistItem playlistItem;
    QIcon itemIcon;
    bool mModified;
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
