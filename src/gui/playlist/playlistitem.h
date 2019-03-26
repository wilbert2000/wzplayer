#ifndef GUI_PLAYLIST_PLAYLISTITEM_H
#define GUI_PLAYLIST_PLAYLISTITEM_H

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

class TPlaylistWidget;

extern Qt::CaseSensitivity caseSensitiveFileNames;

// Item flags to use for the root node
const Qt::ItemFlags ROOT_FLAGS = Qt::ItemIsSelectable
                                 | Qt::ItemIsEnabled
                                 | Qt::ItemIsDropEnabled;


class TPlaylistItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_NAME = 0,
        COL_EXT = 1,
        COL_LENGTH = 2,
        COL_ORDER = 3,
        COL_COUNT = 4
    };

    // Create a root node
    TPlaylistItem();
    // Copy constructor
    TPlaylistItem(const TPlaylistItem& item);
    // Create an item from arguments
    TPlaylistItem(QTreeWidgetItem* parent,
                  const QString& filename,
                  const QString& name,
                  double duration,
                  bool protectName = false);

    virtual QVariant data(int column, int role) const override;
    virtual void setData(int column, int role, const QVariant &value) override;

    QString filename() const { return mFilename; }
    void setFilename(const QString& fileName);
    void setFilename(const QString& fileName, const QString& baseName);
    void updateFilename(const QString& source, const QString& dest);

    QString fname() const;
    QString path() const;
    QString playlistPath() const;
    QString playlistPathPlusSep() const;
    int compareFilename(const TPlaylistItem& item) const;

    QString baseName() const { return mBaseName; }
    QString editName() const;
    void setName(const QString& baseName,
                 const QString& ext,
                 bool protectName);

    QString extension() const { return mExt; }

    double duration() const { return mDuration; }
    void setDuration(double d) { mDuration = d; }

    TPlaylistItemState state() const { return mState; }
    QString stateString() const;
    void setState(TPlaylistItemState state);

    QIcon getItemIcon() const { return itemIcon; }

    bool played() const { return mPlayed; }
    void setPlayed(bool played);

    bool edited() const { return mEdited; }
    void setEdited(bool edited) { mEdited = edited; }

    bool modified() const { return mModified; }
    void setModified(bool modified = true,
                     bool recurse = false,
                     bool markParents = true);

    int order() const { return mOrder; }
    void setOrder(int order) { mOrder = order; }

    bool isFolder() const { return mFolder; }
    void setFolder(bool folder) { mFolder = folder; }
    bool isPlaylist() const { return mPlaylist; }
    bool isWZPlaylist() const { return mWZPlaylist; }
    bool isSymLink() const { return mSymLink; }
    QString target() const { return mTarget; }
    bool isUrl() const { return mURL; }

    bool editURL() const { return mEditURL; }
    void setEditName() { mEditURL = false; }
    void setEditURL() { mEditURL = true; }

    bool isLink() const;

    int playedTime() const { return mPlayedTime; }

    void blacklist(const QString& filename) {
        mBlacklist.append(filename);
    }
    bool blacklisted(const QString& filename) const;
    QStringList getBlacklist() const { return mBlacklist; }
    int getBlacklistCount() const { return mBlacklist.count(); }
    bool whitelist(const QString& filename);

    int getLevel() const;
    void setSizeHintName(int level);
    void setSizeHintName();

    TPlaylistItem* plParent() const {
        return static_cast<TPlaylistItem*>(parent());
    }
    TPlaylistItem* plChild(int idx) const {
        return static_cast<TPlaylistItem*>(child(idx));
    }
    TPlaylistWidget* plTreeWidget() const;
    TPlaylistItem* plTakeChild(int idx) {
        return static_cast<TPlaylistItem*>(takeChild(idx));
    }

    virtual TPlaylistItem* clone() const override;
    virtual bool operator<(const QTreeWidgetItem& other) const override;

    void setSpacing();

private:
    static int hSpacing;
    static int vSpacing;
    static int bounding;

    static QSize getSizeColumnName(int width,
                                   const QString& text,
                                   const QFontMetrics& fm);
    static QString stateString(TPlaylistItemState state);
    static QString tr(const char* s);

    QString mFilename;
    QString mBaseName;
    QString mExt;
    double mDuration;
    int mOrder;

    // Anything that can contain something else, like a directory, a playlist,
    // a disc, the root node.
    bool mFolder;

    // A local file or link to a local file with the extensio m3u8 or m3u
    bool mPlaylist;

    // A playlist with the name wzplayer.m3u8
    bool mWZPlaylist;

    bool mSymLink;
    QString mTarget;

    bool mURL;
    bool mEditURL;

    TPlaylistItemState mState;
    bool mPlayed;
    bool mEdited;
    bool mModified;
    int mPlayedTime;
    QStringList mBlacklist;

    QIcon itemIcon;
    void setStateIcon();

    QSize getSizeHintName(int level) const;

    void renameDir(const QString& dir, const QString& newDir);
    bool renameFile(const QString& newName);
    bool rename(const QString& newName);

    void setFileInfo();
};

} // namespace Playlist
} // namespace Gui

Q_DECLARE_METATYPE(Gui::Playlist::TPlaylistItem*)

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
