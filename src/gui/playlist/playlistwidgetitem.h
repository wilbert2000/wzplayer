#ifndef GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
#define GUI_PLAYLIST_PLAYLISTWIDGETITEM_H

#include <QString>
#include <QTreeWidgetItem>
#include <QIcon>
#include "gui/playlist/playlistitem.h"


namespace Gui {
namespace Playlist {

extern int gRootNodeLevel;
extern int gNameColumnWidth;
extern QFontMetrics gNameFontMetrics;


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
                        bool protectName = false);
    TPlaylistWidgetItem(TPlaylistWidgetItem* parent,
                        const TPlaylistWidgetItem& item,
                        const QString& from,
                        QString to);
    virtual ~TPlaylistWidgetItem();

    QString filename() const { return playlistItem.filename(); }
    void setFilename(const QString& filename);

    QString path() const;
    QString pathPlusSep() const;
    QString fname() const;

    QString name() const { return playlistItem.name(); }
    void setName(const QString& name,
                 bool protectName = false,
                 bool setSizeHint = true);

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
    bool isWZPlaylist() const { return playlistItem.wzPlaylist(); }
    bool isSymLink() const { return playlistItem.symLink(); }
    QString target() const { return playlistItem.target(); }
    QString extension() const { return playlistItem.extension(); }

    int playedTime() const { return playlistItem.playedTime(); }

    void blacklist(const QString& filename) {
        playlistItem.blacklist(filename);
    }
    bool blacklisted(const QString& filename) const {
        return playlistItem.blacklisted(filename);
    }
    QStringList getBlacklist() const { return playlistItem.getBlacklist(); }
    bool whitelist(const QString& filename);

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

    void loadIcon();

    TPlaylistWidgetItem* ff(const QString& fname);

    virtual bool operator<(const QTreeWidgetItem& other) const;
    // TODO: override clone?

private:
    TPlaylistItem playlistItem;
    QIcon itemIcon;
    bool mModified;

    void init();
    QIcon getIcon();
    void setStateIcon();
    void setNameText(bool setSizeHint);
    void setDurationText();
    TPlaylistWidgetItem* f(const QString& fname);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
