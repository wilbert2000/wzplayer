#ifndef GUI_PLAYLISTWIDGET_H
#define GUI_PLAYLISTWIDGET_H

#include <QTreeWidget>


namespace Gui {

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

    double duration() const { return _duration; }
    void setDuration(double duration) { _duration = duration; }

    TPlaylistItemState state() const { return _state; }
    void setState(TPlaylistItemState state);

    bool played() const { return _played; }
    void setPlayed(bool b) { _played = b; }

    bool edited() const { return _edited; }
    void setEdited(bool b) { _edited = b; }

    bool folder() const { return _folder; }

    bool operator == (const TPlaylistItem& item);

private:
    QString _filename, _name;
    double _duration;
    TPlaylistItemState _state;
    bool _played, _edited, _folder;
};


class TPlaylistWidgetItem : public QTreeWidgetItem {
public:
    TPlaylistWidgetItem(QTreeWidgetItem* parent,
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

private:
    TPlaylistItem playlistItem;
};


class TPlaylistWidget : public QTreeWidget {
public:
    explicit TPlaylistWidget(QWidget* parent);


    TPlaylistWidgetItem* playing_item;

    int count() const;
    TPlaylistWidgetItem* currentPlaylistWidgetItem() const;
    QTreeWidgetItem* currentPlaylistWidgetFolder() const;
    TPlaylistWidgetItem* firstPlaylistWidgetItem() const;
    QString playingFile() const;
    TPlaylistWidgetItem* findFilename(const QString& filename);

    TPlaylistWidgetItem* getNextPlaylistWidgetItem(TPlaylistWidgetItem* i) const;
    TPlaylistWidgetItem* getNextPlaylistWidgetItem() const;

    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem(TPlaylistWidgetItem* w) const;
    TPlaylistWidgetItem* getPreviousPlaylistWidgetItem() const;

    void setPlayingItem(TPlaylistWidgetItem* item);
    void clearPlayed();
    void clr();
    QTreeWidgetItem* root() const { return invisibleRootItem(); }

private:
    int countChildren(QTreeWidgetItem* w) const;

    TPlaylistWidgetItem* getNextItem(TPlaylistWidgetItem* w,
                                     bool allowChild = true) const;

    TPlaylistWidgetItem* getPreviousItem(TPlaylistWidgetItem* w,
                                         bool allowChild = true) const;
};

} // namespace Gui

#endif // GUI_PLAYLISTWIDGET_H
