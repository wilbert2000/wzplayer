#ifndef GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
#define GUI_PLAYLIST_PLAYLISTWIDGETITEM_H

#include <QString>
#include <QTreeWidgetItem>
#include <QTime>

namespace Gui {
namespace Playlist {

enum TPlaylistItemState {
    PSTATE_STOPPED,
    PSTATE_LOADING,
    PSTATE_PLAYING,
    PSTATE_FAILED
};

extern QIcon folderIcon;
extern QIcon notPlayedIcon;
extern QIcon okIcon;
extern QIcon loadingIcon;
extern QIcon playIcon;
extern QIcon failedIcon;

class TTimeStamp : public QTime {
public:
    TTimeStamp();
    virtual ~TTimeStamp();

    int getStamp();
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

    double duration() const { return _duration; }
    void setDuration(double duration) { _duration = duration; }

    TPlaylistItemState state() const { return _state; }
    void setState(TPlaylistItemState state);

    bool played() const { return _played; }
    void setPlayed(bool b) { _played = b; }

    bool edited() const { return _edited; }
    void setEdited(bool b) { _edited = b; }

    bool folder() const { return _folder; }

    int playedTime() const { return _playedTime; }

    bool operator == (const TPlaylistItem& item);

private:
    QString _filename, _name;
    double _duration;
    TPlaylistItemState _state;
    bool _played, _edited, _folder;
    int _playedTime;
};

class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    enum TColID {
        COL_PLAY = 0,
        COL_NAME = 0,
        COL_TIME = 1,
        COL_COUNT = 2
    };

    TPlaylistWidgetItem(QTreeWidgetItem* parent,
                        QTreeWidgetItem* after,
                        const QString& filename,
                        const QString& name,
                        double duration,
                        bool isDir);
    virtual ~TPlaylistWidgetItem();

    QString filename() const { return playlistItem.filename(); }

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

private:
    TPlaylistItem playlistItem;
};

} // namespace Playlist
} // namespace Gui

#endif // GUI_PLAYLIST_PLAYLISTWIDGETITEM_H
