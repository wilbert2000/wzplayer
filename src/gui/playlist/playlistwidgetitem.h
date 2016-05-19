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
                  bool isFolder);
    virtual ~TPlaylistItem() {}

    QString filename() const { return _filename; }
    void setFilename(const QString &filename) { _filename = filename; }

    QString name() const { return _name; }
    void setName(const QString &name) { _name = name; }
    static QString cleanName(const QString& name);

    double duration() const { return _duration; }
    void setDuration(double duration) { _duration = duration; }

    TPlaylistItemState state() const { return _state; }
    void setState(TPlaylistItemState state);

    bool played() const { return _played; }
    void setPlayed(bool b) { _played = b; }

    bool edited() const { return _edited; }
    void setEdited(bool b) { _edited = b; }

    bool folder() const { return _folder; }
    void setFolder(bool b) { _folder = b; }

    bool playlist() const { return _playlist; }

    int playedTime() const { return _playedTime; }

    bool operator == (const TPlaylistItem& item);

private:
    QString _filename, _name;
    double _duration;
    TPlaylistItemState _state;
    bool _played, _edited, _folder, _playlist;
    int _playedTime;
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
                        const QIcon& icon);
    virtual ~TPlaylistWidgetItem();

    QString filename() const { return playlistItem.filename(); }
    void setFilename(const QString& filename);

    QString path() const;

    QString name() const { return playlistItem.name(); }
    void setName(const QString& name);

    double duration() const { return playlistItem.duration(); }
    void setDuration(double d);

    TPlaylistItemState state() const { return playlistItem.state(); }
    void setState(TPlaylistItemState state);

    bool played() const { return playlistItem.played(); }
    void setPlayed(bool played);

    bool edited() const { return playlistItem.edited(); }
    void setEdited(bool edited) { playlistItem.setEdited(edited); }

    bool modified() const { return _modified; }
    void setModified(bool modified = true) { _modified = modified; }

    bool isRoot() const;
    bool isFolder() const { return playlistItem.folder(); }
    bool isPlaylist() const { return playlistItem.playlist(); }

    int playedTime() const { return playlistItem.playedTime(); }

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

private:
    TPlaylistItem playlistItem;
    QIcon itemIcon;
    bool _modified;
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
