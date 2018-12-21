#ifndef GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
#define GUI_PLAYLIST_PLAYLISTWIDGETITEM_H

#include "gui/playlist/playlistitem.h"
#include <QString>
#include <QTreeWidgetItem>
#include <QIcon>


namespace Gui {
namespace Playlist {

class TPlaylistWidget;

extern const int ROOT_NODE_LEVEL;
extern int gNameColumnWidth;
extern QFontMetrics gNameFontMetrics;


const Qt::ItemFlags ROOT_FLAGS = Qt::ItemIsSelectable
                                 | Qt::ItemIsEnabled
                                 | Qt::ItemIsDropEnabled;

class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_NAME = 0,
        COL_TYPE = 1,
        COL_TIME = 2,
        COL_ORDER = 3,
        COL_COUNT = 4
    };

    // Create a root node
    TPlaylistWidgetItem();
    // Create a normal node
    TPlaylistWidgetItem(QTreeWidgetItem* parent,
                        const QString& filename,
                        const QString& name,
                        double duration,
                        bool protectName = false);
    virtual ~TPlaylistWidgetItem();

    virtual QVariant data(int column, int role) const override;
    virtual void setData(int column, int role, const QVariant &value) override;


    QString filename() const { return playlistItem.filename(); }
    void setFilename(const QString& fileName, const QString& baseName);

    QString path() const;
    QString pathPlusSep() const;
    QString fname() const;

    QString baseName() const { return playlistItem.baseName(); }
    void setName(const QString& baseName, const QString& ext, bool protectName);

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

    int order() const { return playlistItem.order(); }
    void setOrder(int order);

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

    static QSize sizeColumnName(const QString& text,
                          int width,
                          const QFontMetrics& fm,
                          const QSize& iconSize,
                          int level);
    void setSzHint(int level);
    int getLevel() const;

    TPlaylistWidget* plTreeWidget() const;
    TPlaylistWidgetItem* plParent() const {
        return static_cast<TPlaylistWidgetItem*>(parent());
    }
    TPlaylistWidgetItem* plChild(int idx) const {
        return static_cast<TPlaylistWidgetItem*>(child(idx));
    }

    void loadIcon();

    virtual bool operator<(const QTreeWidgetItem& other) const;
    // TODO: override clone?

private:
    TPlaylistItem playlistItem;
    QIcon itemIcon;
    bool mModified;

    QIcon getIcon();
    void setStateIcon();
    void refresh(const QString& dir, const QString& newDir);
    bool renameFile(const QString& newName);
    bool rename(const QString& newName);
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
