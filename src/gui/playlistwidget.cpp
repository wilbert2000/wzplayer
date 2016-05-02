#include "gui/playlistwidget.h"

#include <QDebug>
#include <QHeaderView>
#include <QTreeWidgetItemIterator>

#include "images.h"
#include "helper.h"

namespace Gui {

enum TColID {
    COL_PLAY = 0,
    COL_NAME = 0,
    COL_TIME = 1,
    COL_COUNT = 2
};

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


TPlaylistWidgetItem::TPlaylistWidgetItem(QTreeWidgetItem* parent,
                                         const QString& filename,
                                         const QString& name,
                                         double duration,
                                         bool isDir) :
    QTreeWidgetItem(parent),
    playlistItem(filename, name, duration, isDir) {

    if (isDir) {
        setIcon(COL_PLAY, Images::icon("folder"));
    } else {
        setIcon(COL_PLAY, Images::icon("not_played"));
    }

    setText(COL_NAME, playlistItem.name());
    setToolTip(COL_NAME, playlistItem.name());

    setDuration(duration);
    setToolTip(COL_TIME, filename);
}

TPlaylistWidgetItem::~TPlaylistWidgetItem() {

    TPlaylistWidget* w = static_cast<TPlaylistWidget*>(treeWidget());
    if (w && w->playing_item == this) {
        w->playing_item = 0;
    }
}

void TPlaylistWidgetItem::setState(TPlaylistItemState state) {
    qDebug() << "Gui::TPlaylistWidgetItem::setState:"
             << filename() << state;

    playlistItem.setState(state);

    switch (state) {
        case PSTATE_STOPPED:
            if (playlistItem.played()) {
                setIcon(COL_PLAY, Images::icon("ok"));
            } else {
                setIcon(COL_PLAY, Images::icon("not_played"));
            }
            break;
        case PSTATE_LOADING:
            setIcon(COL_PLAY, Images::icon("loading"));
            break;
        case PSTATE_PLAYING:
            setIcon(COL_PLAY, Images::icon("play"));
            break;
        case PSTATE_FAILED:
            setIcon(COL_PLAY, Images::icon("failed"));
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
            setIcon(COL_PLAY, Images::icon("ok"));
        } else {
            setIcon(COL_PLAY, Images::icon("not_played"));
        }
    }
}

TPlaylistWidget::TPlaylistWidget(QWidget* parent) :
    QTreeWidget(parent),
    playing_item(0) {

    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    //setRootIsDecorated(false);
    setColumnCount(COL_COUNT);
    setHeaderLabels(QStringList() << tr("Name") << tr("Length"));
    header()->setStretchLastSection(false);
    header()->setResizeMode(COL_PLAY, QHeaderView::ResizeToContents);
    header()->setResizeMode(COL_NAME, QHeaderView::Stretch);
    header()->setResizeMode(COL_TIME, QHeaderView::ResizeToContents);

    // TODO:
    //setSortingEnabled(true);
    //header()->setSortIndicator(COL_NAME, Qt::AscendingOrder);
    //header()->setSortIndicatorShown(true);

    setIconSize(QSize(22, 22));

    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDragDropOverwriteMode(false);
    setDropIndicatorShown(true);
    setDefaultDropAction(Qt::MoveAction);
}

void TPlaylistWidget::clr() {

    playing_item = 0;
    clear();
}

int TPlaylistWidget::countChildren(QTreeWidgetItem* w) const {

    int count = w->childCount();
    for(int c = 0; c < w->childCount(); c++) {
        count += countChildren(w->child(c));
    }
    return count;
}

int TPlaylistWidget::count() const {
    return countChildren(root());
}

TPlaylistWidgetItem* TPlaylistWidget::currentPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(currentItem());
}

QTreeWidgetItem* TPlaylistWidget::currentPlaylistWidgetFolder() const {

    TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(currentItem());
    if (i) {
        if (i->isFolder()) {
            return i;
        } else if (i->parent()) {
            return i->parent();
        }
    }
    return root();
}

TPlaylistWidgetItem* TPlaylistWidget::firstPlaylistWidgetItem() const {
    return static_cast<TPlaylistWidgetItem*>(topLevelItem(0));
}

QString TPlaylistWidget::playingFile() const {
    return playing_item ? playing_item->filename() : "";
}

TPlaylistWidgetItem* TPlaylistWidget::findFilename(const QString &filename) {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (i->filename() == filename) {
            return i;
        }
        ++it;
    }

    return 0;
}

void TPlaylistWidget::setPlayingItem(TPlaylistWidgetItem* item) {
    qDebug() << "Gui::TPlaylist::setPlayingItem";

    bool setCurrent;
    if (playing_item) {
        if (playing_item->state() == PSTATE_PLAYING
            || playing_item->state() == PSTATE_LOADING) {
            playing_item->setState(PSTATE_STOPPED);
        }
        setCurrent = playing_item == currentItem();
    } else {
        setCurrent = true;
    }

    playing_item = item;
    if (playing_item && setCurrent) {
        setCurrentItem(playing_item);
    }
}

void TPlaylistWidget::clearPlayed() {

    QTreeWidgetItemIterator it(this);
    while (*it) {
        TPlaylistWidgetItem* i = static_cast<TPlaylistWidgetItem*>(*it);
        if (!i->isFolder()) {
            i->setPlayed(false);
        }
        ++it;
    }
}

TPlaylistWidgetItem* TPlaylistWidget::getNextItem(TPlaylistWidgetItem* w,
                                                  bool allowChild) const {
    // See also itemBelow()

    if (w == 0) {
        w = firstPlaylistWidgetItem();
        if (w == 0) {
            return 0;
        }
    }

    if (allowChild && w->isFolder() && w->childCount() > 0) {
        return static_cast<TPlaylistWidgetItem*>(w->child(0));
    }

    QTreeWidgetItem* parent = w->parent();
    if(parent) {
       int idx = parent->indexOfChild(w) + 1;
       if (idx < parent->childCount()) {
           return static_cast<TPlaylistWidgetItem*>(parent->child(idx));
       }
       return getNextItem(static_cast<TPlaylistWidgetItem*>(parent), false);
    }

    return static_cast<TPlaylistWidgetItem*>(
                topLevelItem(indexOfTopLevelItem(w) + 1));
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem(TPlaylistWidgetItem* i) const {

    do {
        i = getNextItem(i);
    } while (i && i->isFolder());

    return i;
}

TPlaylistWidgetItem* TPlaylistWidget::getNextPlaylistWidgetItem() const {
    return getNextPlaylistWidgetItem(playing_item);
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousItem(TPlaylistWidgetItem* w,
                                                      bool allowChild) const {
    // See also itemAbove()

    if (w == 0) {
        // TODO:
        //w = lastPlaylistWidgetItem();
        //if (w == 0) {
            return 0;
        //}
    }

    int c = w->childCount();
    if (allowChild && w->isFolder() && c > 0) {
        return static_cast<TPlaylistWidgetItem*>(w->child(c - 1));
    }

    QTreeWidgetItem* parent = w->parent();
    if(parent) {
       int idx = parent->indexOfChild(w) - 1;
       if (idx >= 0) {
           return static_cast<TPlaylistWidgetItem*>(parent->child(idx));
       }
       return getPreviousItem(static_cast<TPlaylistWidgetItem*>(parent), false);
    }

    return static_cast<TPlaylistWidgetItem*>(
                topLevelItem(indexOfTopLevelItem(w) - 1));
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem(TPlaylistWidgetItem* i) const {

    do {
        i = getPreviousItem(i);
    } while (i && i->isFolder());

    return i;
}

TPlaylistWidgetItem* TPlaylistWidget::getPreviousPlaylistWidgetItem() const {
    return getPreviousPlaylistWidgetItem(playing_item);
}

} // namespace Gui
