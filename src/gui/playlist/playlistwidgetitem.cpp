#include "gui/playlist/playlistwidgetitem.h"

#include <QDebug>
#include <QString>

#include "images.h"
#include "helper.h"

namespace Gui {
namespace Playlist {


TPlaylistItem::TPlaylistItem() :
    _duration(0),
    _state(PSTATE_STOPPED),
    _played(false),
    _edited(false),
    _folder(false) {
}

TPlaylistItem::TPlaylistItem(const QString &filename,
                             const QString &name,
                             double duration,
                             bool isFolder) :
    _filename(filename),
    _name(name),
    _duration(duration),
    _state(PSTATE_STOPPED),
    _played(false),
    _edited(false),
    _folder(isFolder) {

    if (_name.isEmpty()) {
        _name = _filename;
    }
}

void TPlaylistItem::setState(TPlaylistItemState state) {

    if (state == PSTATE_PLAYING) {
        _played = true;
    }
    _state = state;
}

bool TPlaylistItem::operator == (const TPlaylistItem& item) {

    if (&item != 0) {
        return item.filename() == _filename;
    }
    return false;
}

QIcon folderIcon;
QIcon notPlayedIcon;
QIcon okIcon;
QIcon loadingIcon;
QIcon playIcon;
QIcon failedIcon;

TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         QTreeWidgetItem* after,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool isDir) :
    QTreeWidgetItem(parent, after),
    playlistItem(filename, name, duration, isDir) {

    Qt::ItemFlags flags = Qt::ItemIsSelectable
                          | Qt::ItemIsDragEnabled
                          | Qt::ItemIsEnabled;
    if (isDir) {
        setFlags(flags | Qt::ItemIsDropEnabled);
        setIcon(COL_PLAY, folderIcon);
    } else {
        // TODO:
        //setFlags(flags | Qt::ItemIsEditable);
        setFlags(flags);
        setIcon(COL_PLAY, notPlayedIcon);
    }

    setText(COL_NAME, playlistItem.name());
    setToolTip(COL_NAME, playlistItem.name());

    setDuration(duration);
    setToolTip(COL_TIME, filename);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {
}

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    qDebug() << "Gui::TPlaylistWidgetItem::setState:"
             << filename() << state;

    playlistItem.setState(state);

    switch (state) {
        case PSTATE_STOPPED:
            if (playlistItem.played()) {
                setIcon(COL_PLAY, okIcon);
            } else {
                setIcon(COL_PLAY, notPlayedIcon);
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_PLAY, loadingIcon);
            break;
        case PSTATE_PLAYING:
            setIcon(COL_PLAY, playIcon);
            break;
        case PSTATE_FAILED:
            setIcon(COL_PLAY, failedIcon);
            break;
    }
}

void TPlaylistWidgetItem::setName(const QString& name) {

    playlistItem.setName(name);
    setText(COL_NAME, name);
}

void TPlaylistWidgetItem::setDuration(double d) {

    playlistItem.setDuration(d);
    if (d > 0) {
        setText(COL_TIME, Helper::formatTime(qRound(d)));
    } else {
        setText(COL_TIME, "");
    }
}

void TPlaylistWidgetItem::setPlayed(bool played) {

    playlistItem.setPlayed(played);
    if (playlistItem.state() == PSTATE_STOPPED) {
        if (played) {
            setIcon(COL_PLAY, okIcon);
        } else {
            setIcon(COL_PLAY, notPlayedIcon);
        }
    }
}

} // namespace Playlist
} // namespace Gui
