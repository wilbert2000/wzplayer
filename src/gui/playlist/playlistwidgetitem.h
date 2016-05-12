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

//extern QString playlistItemState(TPlaylistItemState state);

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

    int playedTime() const { return _playedTime; }

    bool operator == (const TPlaylistItem& item);

private:
    QString _filename, _name;
    double _duration;
    TPlaylistItemState _state;
    bool _played, _edited, _folder;
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


class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_NAME = 0,
        COL_TIME = 1,
        COL_COUNT = 2
    };

    TPlaylistWidgetItem(const QIcon& icon);
    TPlaylistWidgetItem(QTreeWidgetItem* parent,
                        QTreeWidgetItem* after,
                        const QString& filename,
                        const QString& name,
                        double duration,
                        bool isDir,
                        const QIcon& icon);
    virtual ~TPlaylistWidgetItem();

    QString filename() const { return playlistItem.filename(); }
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

    bool isFolder() const { return playlistItem.folder(); }

    int playedTime() const { return playlistItem.playedTime(); }

    static QSize itemSize(const QString& text,
                          int width,
                          const QFontMetrics& fm,
                          const QSize& iconSize,
                          int level);
    void setSzHint(int level);
    int getLevel() const;

private:
    TPlaylistItem playlistItem;
    QIcon itemIcon;
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
